/******************************************************************************
Copyright (c) 2019 Georgia Instititue of Technology
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Author : Hyoukjun Kwon (hyoukjun@gatech.edu)
*******************************************************************************/


#ifndef MAESTRO_DFA_CONV_CLUSTER_ANALYSIS_HPP_
#define MAESTRO_DFA_CONV_CLUSTER_ANALYSIS_HPP_

//#define DEBUG_CLUSTER_ANALYSIS

#include <vector>
#include <memory>

#include "BASE_constants.hpp"
#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "DFA_directives.hpp"
#include "DFA_directive-table.hpp"
#include "DFA_layer.hpp"
#include "DFA_tensor.hpp"

#include "DFA_cluster-unit.hpp"
#include "DFA_cluster-table.hpp"

#include "DFA_cluster-analysis.hpp"

#include "AHW_noc-model.hpp"


namespace maestro {
    namespace DFA {

        /* Class ClusterAnalysis
         * Receive full dataflow description and split it into clusters
         */

        class ClusterAnalysis : public MAESTROClass {
        public:
            ClusterAnalysis(LayerType layer_type,
                            int num_pes,
                            std::shared_ptr<DFA::TensorTable> tensors,
                            std::shared_ptr<DFA::DimensionTable> full_dimensions,
                            std::shared_ptr<DFA::DirectiveTable> full_dataflow,
                            std::shared_ptr<std::vector<std::shared_ptr<AHW::NetworkOnChipModel>>> nocs) :
                    MAESTROClass("ClusterAnalysis"),
                    layer_type_(layer_type),
                    num_unit_clusters_(num_pes),
                    tensors_(tensors),
                    full_dimensions_(full_dimensions),
                    full_dataflow_(full_dataflow),
                    nocs_(nocs) {
                clusters_ = std::make_shared<DFA::ClusterTable>(layer_type);
                AnalyzeClusters();
            }

            std::shared_ptr<DFA::ClusterTable> GetClusters() {
                return clusters_;
            }

            int GetNumActivePEs() {
                return num_active_unit_clusters_;
            }

            int GetUppermostClusterSize() {
                return uppermost_cluster_unit_size;
            }

            int GetInnermostClusterSize() {
                return innermost_cluster_unit_size;
            }

        protected:
            LayerType layer_type_;
            int num_unit_clusters_;
            int num_active_unit_clusters_ = 1;
            int num_cluster_lvs = 0;
            int uppermost_cluster_unit_size = 1;
            int innermost_cluster_unit_size = 1;

            // Dimension and dataflow information of entire accelerator(s)
            std::shared_ptr<DFA::TensorTable> tensors_;
            std::shared_ptr<DFA::DimensionTable> full_dimensions_;
            std::shared_ptr<DFA::DirectiveTable> full_dataflow_;
            std::shared_ptr<std::vector<std::shared_ptr<AHW::NetworkOnChipModel>>> nocs_;


            std::shared_ptr<DFA::ClusterTable> clusters_;

        private:

            void AnalyzeClusters() {
                AnalyzeClusterStructure();

                int current_cluster_level = num_cluster_lvs;
                int num_unit_clusters = uppermost_cluster_unit_size * (num_unit_clusters_/uppermost_cluster_unit_size); //Normalizing the number
                num_active_unit_clusters_ = num_unit_clusters;
                int current_cluster_size = num_unit_clusters;
                int last_cluster_size = num_unit_clusters;

                auto cluster_dataflow = std::make_shared<DFA::DirectiveTable>();
                std::shared_ptr<DFA::DirectiveTable> prev_dataflow = nullptr;
                auto cluster_dimensions =full_dimensions_;
                std::shared_ptr<DFA::DimensionTable> prev_cluster_dimensions = nullptr;

                //Main loop
                for(auto directive: *full_dataflow_) {
                    switch(directive->GetClass()) {
                        case directive::DirectiveClass::SpatialMap: {
                            cluster_dataflow->AddDirective(directive);
                            break;
                        } // End of case SpatialMap

                        case directive::DirectiveClass::TemporalMap: {
                            cluster_dataflow->AddDirective(directive);
                            break;
                        } // End of case TemporalMap

                        case directive::DirectiveClass::Cluster: {
                            // Get NoC setting for this cluster
                            if(nocs_->size() < current_cluster_level) {
                                error_handler->PrintErrorMsg(TL::ErrorCode::MissingNoCForCluster, std::to_string(current_cluster_level) ,this->GetName());
                                error_handler->TerminateProgram();
                            }
                            std::shared_ptr<AHW::NetworkOnChipModel> noc = nocs_->at(current_cluster_level);

                            auto var_list_input_tensor = GetInputTensorVars();
                            ConvertToInputCentric(cluster_dataflow);

                            /* Complement dataflow; automotically guess missing directives */
                            // Inherit missing directives
                            InheritMissingDirectives(prev_dataflow, cluster_dataflow);
                            // Insert "Unroll" of missing dimensions
                            UnrollMissingDirectives(var_list_input_tensor, cluster_dimensions, cluster_dataflow);

                            bool is_top_cluster = (current_cluster_level == num_cluster_lvs);
                            ApplyOuterStride(is_top_cluster, cluster_dataflow, cluster_dimensions);


#ifdef DEBUG_CLUSTER_ANALYSIS
                            std::cout << "Cluster level: " << current_cluster_level << std::endl;
                std::cout << "num_cluster_lvs: " << num_cluster_lvs << std::endl;
                std::cout << "Cluster dimensions:" << std::endl;
                std::cout << cluster_dimensions->ToString() << std::endl;
                std::cout << cluster_dataflow->ToString() << std::endl;
                std::cout << "uppermost_cluster_unit_size: " << uppermost_cluster_unit_size << std::endl;
                std::cout << "NoC Bandwidth: " << noc->GetBandwidth() << std::endl;
#endif

                            //Note that the cluster level is 0 in the inner-most cluster
                            // We are filling the cluster from backward (from outer-most to inner-most)
                            if(is_top_cluster) {
                                current_cluster_size = num_unit_clusters / uppermost_cluster_unit_size;
                            }
                            else {
                                current_cluster_size = last_cluster_size;
                            }

                            auto new_cluster = std::make_shared<DFA::ClusterUnit>(current_cluster_level,
                                                                                  current_cluster_size,
                                                                                  cluster_dataflow,
                                                                                  cluster_dimensions,
                                                                                  tensors_,
                                                                                  noc);
                            last_cluster_size = directive->GetSize();

#ifdef DEBUG_CLUSTER_ANALYSIS
                            std::cout << "num_unit_clusters: " << num_unit_clusters << std::endl;
                std::cout << "Current cluster size: " << current_cluster_size << std::endl;
                std::cout << "last_cluster_size: " << last_cluster_size << std::endl;
#endif

                            clusters_->PutCluster(new_cluster);
                            prev_cluster_dimensions = cluster_dimensions;
                            cluster_dimensions = std::make_shared<DFA::DimensionTable>();
                            cluster_dimensions->SetOverlapTable(prev_cluster_dimensions->GetOverlapTable());

                            ConstructLowerClusterDimensionTable(var_list_input_tensor, prev_cluster_dimensions, cluster_dimensions, cluster_dataflow);
                            prev_dataflow = cluster_dataflow;
                            cluster_dataflow = std::make_shared<DFA::DirectiveTable>();

                            current_cluster_level--;
                        } // End of case Cluster
                            break;
                        default: {
                            error_handler->PrintErrorMsg(TL::ErrorCode::InvalidDirective,"", instance_name_);
                        }
                    }// End of switch(directive->GetClass)
                } // End of for-each directive in full_dataflow
            } // End of function void AnalyzeClusters


            void AnalyzeClusterStructure() {
                std::vector<int> cluster_sizes;
                int last_cluster_size = num_unit_clusters_;

                //Count the number of clusters
                for(auto directive: *full_dataflow_) {
                    if(directive->GetClass() == DFA::directive::DirectiveClass::Cluster) {
                        cluster_sizes.push_back(directive->GetSize());
                        num_cluster_lvs++;
                        uppermost_cluster_unit_size *= directive->GetSize();
                        last_cluster_size = directive->GetSize();
                    }
                }

                // Add the inner-most cluster directive
                std::shared_ptr<DFA::directive::Cluster> last_cluster
                        = std::make_shared<DFA::directive::Cluster>(last_cluster_size, DFA::directive::ClusterType::Physical);
                full_dataflow_->AddDirective(last_cluster);

                innermost_cluster_unit_size = last_cluster_size;

            }

            void ApplyOuterStride(
                    bool is_top_cluster,
                    std::shared_ptr<DFA::DirectiveTable> cluster_dataflow,
                    std::shared_ptr<DFA::DimensionTable> cluster_dimensions) {

                for(auto& directive: *cluster_dataflow) {
                    auto directive_var = directive->GetVariable();
                    int outer_stride = cluster_dimensions->GetOuterStride(directive_var);

                    if(cluster_dimensions->IsOverlapped(directive_var) && !cluster_dimensions->IsSlidingDim(directive_var) && outer_stride != 1) {
                        int map_size = directive->GetSize();
                        int ofs_size = directive->GetOfs();
                        int directive_idx = cluster_dataflow->GetDirectiveIdx(directive_var);

                        auto sliding_dim = cluster_dimensions->GetOverlappingDim(directive_var);

                        int sliding_dim_size = full_dimensions_->GetSize(sliding_dim);
                        int reference_dim_size = cluster_dimensions->GetSize(directive_var);

                        if(map_size >= full_dimensions_->GetSize(sliding_dim)) {
                            int output_map_size = (map_size + outer_stride -  sliding_dim_size) / outer_stride; // Implicit flooring
                            int adjusted_map_size = output_map_size * std::min(outer_stride, sliding_dim_size) + std::max(sliding_dim_size - outer_stride, 0);
                            int adjusted_ofs_size = output_map_size * outer_stride; //std::min(outer_stride, sliding_dim_size);

                            if(!is_top_cluster && sliding_dim_size < outer_stride) {
                                adjusted_ofs_size = output_map_size * sliding_dim_size;
                            }

                            cluster_dataflow->at(directive_idx)->SetSize(adjusted_map_size);
                            cluster_dataflow->at(directive_idx)->SetOfs(adjusted_ofs_size);

                            int adjusted_dim_size;
                        }
                    }
                }

            }

            void InheritMissingDirectives(
                    std::shared_ptr<DFA::DirectiveTable> prev_dataflow,
                    std::shared_ptr<DFA::DirectiveTable> cluster_dataflow) {

                if(prev_dataflow != nullptr) {
                    for(auto& upper_directive : *prev_dataflow) {
                        auto upper_directive_var = upper_directive->GetVariable();
                        if(cluster_dataflow->FindDirective(upper_directive_var) == nullptr) {
                            int upper_map_sz = upper_directive->GetSize();
                            int upper_ofs_sz = upper_directive->GetOfs();

                            auto inherited_dataflow = std::make_shared<DFA::directive::TemporalMap>(upper_map_sz, upper_ofs_sz, upper_directive_var);
                            cluster_dataflow->AddDirectiveFront(inherited_dataflow);
                        }
                    }
                }
            }

            void UnrollMissingDirectives(
                    std::list<std::string> var_list_input_tensor,
                    std::shared_ptr<DFA::DimensionTable> cluster_dimensions,
                    std::shared_ptr<DFA::DirectiveTable> cluster_dataflow) {
                for(auto& var : var_list_input_tensor) {
                    if(cluster_dataflow->FindDirective(var) == nullptr) {

                        if(cluster_dimensions->HasVar(var)) {
                            int dim_sz = cluster_dimensions->GetSize(var);
                            auto dummy_directive = std::make_shared<DFA::directive::TemporalMap>(dim_sz, dim_sz, var);
                            cluster_dataflow->AddDirective(dummy_directive);
                        }
                    }
                }
            }

            std::list<std::string> GetInputTensorVars() {

                std::list<std::string> var_list_input_tensor;
                for(auto& tensor : *tensors_) {
                    if(tensor->GetTensorClass() == TensorClass::InputTensor) {
                        auto coupled_vars = tensor->GetCoupledVariables();

                        for(auto var : *coupled_vars) {
                            var_list_input_tensor.push_back(var);
                        }
                    }
                }
                var_list_input_tensor.sort();
                var_list_input_tensor.unique();

                return var_list_input_tensor;
            }

            void ConstructLowerClusterDimensionTable(
                    std::list<std::string> var_list_input_tensor,
                    std::shared_ptr<DFA::DimensionTable> prev_cluster_dimensions,
                    std::shared_ptr<DFA::DimensionTable> cluster_dimensions,
                    std::shared_ptr<DFA::DirectiveTable> cluster_dataflow) {

                assert(prev_cluster_dimensions != nullptr);

                // Update Dimension table; new dimension = mapping size at upper cluster
                for(auto var : var_list_input_tensor) {
                    auto directive = cluster_dataflow->FindDirective(var);

                    //Deal with edge cases
                    int final_dim_sz = std::min(directive->GetSize(), prev_cluster_dimensions->GetSize(var));
                    int inner_stride = prev_cluster_dimensions->GetInnerStride(var);
                    int outer_stride = prev_cluster_dimensions->GetOuterStride(var);
                    auto dim = std::make_shared<DFA::LayerDimension>(var, final_dim_sz, outer_stride, inner_stride);
                    cluster_dimensions->AddDimension(dim);
                }
            }

            void ConvertToInputCentric(
                    std::shared_ptr<DFA::DirectiveTable> dataflow) {

                int IX_size, IY_size, R_size, S_size, OX_size, OY_size;
                bool has_IY = false;
                bool has_IX = false;
                bool has_OY =false;
                bool has_OX =false;

                for(auto directive : *dataflow) {
                    if(directive->GetVariable() == DFSL::layer_dim_input_height_) {
                        has_IY = true;
                        IY_size = directive->GetSize();
                    }
                    else if(directive->GetVariable() == DFSL::layer_dim_input_width_) {
                        has_IX = true;
                        IX_size = directive->GetSize();
                    }
                    else if(directive->GetVariable() == DFSL::layer_dim_output_height_) {
                        has_OY = true;
                        OY_size = directive->GetSize();
                    }
                    else if(directive->GetVariable() == DFSL::layer_dim_output_width_) {
                        has_OX = true;
                        OX_size = directive->GetSize();
                    }
                    else if(directive->GetVariable() == DFSL::layer_dim_weight_height_) {
                        R_size = directive->GetSize();
                    }
                    else if(directive->GetVariable() == DFSL::layer_dim_weight_width_) {
                        S_size = directive->GetSize();
                    }
                }

                if(!has_IX && has_OX) {
                    int directive_idx = dataflow->GetDirectiveIdx(DFSL::layer_dim_output_width_);
                    dataflow->at(directive_idx)->SetVariable(DFSL::layer_dim_input_width_);
                    dataflow->at(directive_idx)->SetSize(OX_size + S_size - 1); // TODO: Fix this for special cases (non-unroll S)
                }

                if(!has_IY && has_OY) {
                    int directive_idx = dataflow->GetDirectiveIdx(DFSL::layer_dim_output_height_);
                    dataflow->at(directive_idx)->SetVariable(DFSL::layer_dim_input_height_);
                    dataflow->at(directive_idx)->SetSize(OY_size + R_size - 1); // TODO: Fix this for special cases (non-unroll R)
                }
            }

        }; // End of class ClusterMap
    }; // End of namespace DFA
}; // End of namespace maestro

#endif
