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

#ifndef MAESTRO_DFA_CLUSTER_UNIT_HPP_
#define MAESTRO_DFA_CLUSTER_UNIT_HPP_

//#define DEBUG_CLUSTER_UNIT

#include <memory>
#include <vector>
#include <list>
#include <map>
#include <string>

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "AHW_noc-model.hpp"
#include "DFA_dimension-table.hpp"
#include "DFA_directives.hpp"
#include "DFA_directive-table.hpp"

#include "DFA_tensor.hpp"
#include "DFA_tensor-table.hpp"


namespace maestro {
    namespace DFA {

        const int invalid_map_pos = -1;

        class ClusterUnit : public MAESTROClass {
        public:
            ClusterUnit(int cluster_level, int cluster_size,
                        std::shared_ptr<DFA::DirectiveTable> dataflow,
                        std::shared_ptr<DFA::DimensionTable> dimensions,
                        std::shared_ptr<DFA::TensorTable> tensors,
                        std::shared_ptr<AHW::NetworkOnChipModel> noc) :
                    cluster_level_(cluster_level),
                    cluster_size_(cluster_size),
                    noc_(noc),
                    dimensions_(dimensions),
                    dataflow_(dataflow),
                    tensors_(tensors),
                    MAESTROClass("ClusterUnitAnalysis_Lv"+ std::to_string(cluster_level))
            {

                num_mapped_elements_ = std::make_unique<std::map<std::string, int>>();
                sp_mapped_unique_elements_ = std::make_unique<std::map<std::string, int>>();
                tp_mapped_unique_elements_ = std::make_unique<std::map<std::string, int>>();
                sp_mapped_reused_elements_ = std::make_unique<std::map<std::string, int>>();
                tp_mapped_reused_elements_ = std::make_unique<std::map<std::string, int>>();
                dataflow->ConvertToInputCentric();
                Preprocess();
            }

            int GetClusterLevel() {
                return cluster_level_;
            }

            std::shared_ptr<AHW::NetworkOnChipModel> GetNoCModel() {
                return noc_;
            }

            std::shared_ptr<DFA::DimensionTable> GetDimensions() {
                return dimensions_;
            }

            std::shared_ptr<DFA::DirectiveTable> GetDataflow() {
                return dataflow_;
            }

            bool IsUnrolled(std::string dim) {
                bool ret = false;
                auto directive = dataflow_->FindDirective(dim);
                int dim_size =  dimensions_->GetSize(dim);
                int map_size = directive->GetSize();
                int ofs_size = directive->GetOfs();

                if(directive != nullptr) {
                    if(directive->GetClass() == DFA::directive::DirectiveClass::TemporalMap) {
                        ret = (map_size >= dim_size);
                    }
                    else { // Spatial Map case
                        //Spatial unroll: If one spatial iteration covers entire dimension, it is the spatial unroll
                        int sp_covered_size = (cluster_size_-1) * ofs_size + map_size; //TODO: double-check this
                        ret = (sp_covered_size >= dim_size);
                    }
                }
                else {
                    this->error_handler_->PrintErrorMsg(TL::ErrorCode::InvalidDirective, dim, this->GetName());
                    this->error_handler_->TerminateProgram();
                }

                return ret;
            }

            bool IsInitEdge(std::string var) {
                bool ret = false;
                auto directive = dataflow_->FindDirective(var);
                if(directive != nullptr) {
                    auto directive_class = directive->GetClass();
                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                        ret = (directive->GetSize() > dimensions_->GetSize(var));
                    }
                    else {
                        ret = ( (directive->GetOfs() * cluster_size_) > dimensions_->GetSize(var));
                    }
                }
                return ret;
            }

            bool IsInitUnroll(std::string var) {
                bool ret = false;
                auto directive = dataflow_->FindDirective(var);

                if(directive != nullptr) {
                    auto directive_class = directive->GetClass();
                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                        ret = (directive->GetSize() >= dimensions_->GetSize(var)); // Updated from ==  to >=
                    }
                    else {
                        ret = ( (directive->GetOfs() * cluster_size_) >= dimensions_->GetSize(var)); // Updated from ==  to >=
                    }
                }

                return ret;
            }

            bool HasSpatialEdgeCase() {
                return (num_edge_spatial_iterations_ != 0);
            }

            bool HasTemporalEdgeCase(std::string var) {
                auto directive = dataflow_->FindDirective(var);
                if(directive == nullptr) {
                    this->error_handler_->PrintErrorMsg(TL::ErrorCode::InvalidDimension, var);
                }

                if(directive->GetClass() ==  directive::DirectiveClass::SpatialMap) {
                    return false; // Treats entire spatial iterations as one temporal iteration
                }

                int dim_size = 0;
                int offset = directive->GetOfs();
                bool is_overlapping = dimensions_->IsOverlapped(var);

                if(is_overlapping) {
                    auto overlapping_var = dimensions_->GetOverlappingDim(var);
                    bool is_sliding_dim = dimensions_->IsSlidingDim(var);

                    if(is_sliding_dim) {
                        int sliding_dim_size = dimensions_->GetSize(var);
                        dim_size = sliding_dim_size;
                    }
                    else {
                        int reference_dim_size = dimensions_->GetSize(var);
                        int sliding_dim_size = dimensions_->GetSize(overlapping_var);

                        dim_size = reference_dim_size - sliding_dim_size + 1;
                    }
                }
                else {
                    dim_size = dimensions_->GetSize(var);
                }

                return (dim_size % offset != 0);
            }

            long GetNumTemporalIterations(std::string var) {
                long ret = 0;

                auto directive = dataflow_->FindDirective(var);
                if(directive == nullptr) {
                    this->error_handler_->PrintErrorMsg(TL::ErrorCode::InvalidDimension, var);
                }

                if(directive->GetClass() ==  directive::DirectiveClass::SpatialMap) {
                    return 1; // Treats entire spatial iterations as one temporal iteration
                }

                int dim_size = 0;
                int offset = directive->GetOfs();
                bool is_overlapping = dimensions_->IsOverlapped(var);

                if(is_overlapping) {
                    auto overlapping_var = dimensions_->GetOverlappingDim(var);
                    bool is_sliding_dim = dimensions_->IsSlidingDim(var);

                    if(is_sliding_dim) {
                        int sliding_dim_size = dimensions_->GetSize(var);
                        dim_size = sliding_dim_size;
                    }
                    else {
                        int reference_dim_size = dimensions_->GetSize(var);
                        int sliding_dim_size = dimensions_->GetSize(overlapping_var);

                        dim_size = reference_dim_size - sliding_dim_size + 1;
                    }
                }
                else {
                    dim_size = dimensions_->GetSize(var);
                }

                ret = dim_size / offset;
                if(dim_size % offset != 0) {
                    ret++;
                }

                return ret;
            }

            long GetNumSpatialIterations(std::string dim) {
                long ret = 0;

                auto directive = dataflow_->FindDirective(dim);
                if(directive == nullptr) {
                    this->error_handler_->PrintErrorMsg(TL::ErrorCode::InvalidDimension, dim);
                }

                if(directive->GetClass() ==  directive::DirectiveClass::TemporalMap) {
                    return 1; // Treats entire spatial iterations as one temporal iteration
                }

                int dim_size = 0;
                int offset = directive->GetOfs();
                bool is_overlapping = dimensions_->IsOverlapped(dim);

                if(is_overlapping) {
                    auto overlapping_var = dimensions_->GetOverlappingDim(dim);
                    bool is_sliding_dim = dimensions_->IsSlidingDim(dim);

                    if(is_sliding_dim) {
                        int sliding_dim_size = dimensions_->GetSize(dim);
                        dim_size = sliding_dim_size;
                    }
                    else {
                        int reference_dim_size = dimensions_->GetSize(dim);
                        int sliding_dim_size = dimensions_->GetSize(overlapping_var);

                        dim_size = reference_dim_size - sliding_dim_size + 1;
                    }
                }
                else {
                    dim_size = dimensions_->GetSize(dim);
                }

                int full_spatial_coverage = offset * cluster_size_;

                ret = dim_size / full_spatial_coverage;
                if(dim_size % full_spatial_coverage != 0) {
                    ret++;
                }

                return ret;
            }

            long GetNumTotalIterations() {
                long ret = 1;

                for(int idx = 0; idx < dataflow_->size();idx ++) {

                    auto directive = dataflow_->at(idx);
                    auto directive_dim = directive->GetVariable();
                    auto dim_size = dimensions_->GetSize(directive_dim);
                    int num_iter_this_dim = 1;

                    if(dimensions_->IsOverlapped(directive_dim)) {
                        if(!dimensions_->IsSlidingDim(directive_dim)) {
                            auto sliding_dim = dimensions_->GetOverlappingDim(directive_dim);
                            auto sliding_dim_directive = dataflow_->FindDirective(directive_dim);
                            assert(sliding_dim_directive != nullptr);
                            auto sliding_dim_map_size = sliding_dim_directive->GetSize();
                            auto sliding_dim_size = dimensions_->GetSize(sliding_dim);

                            if(sliding_dim_map_size == sliding_dim_size) {
                                dim_size = dim_size - sliding_dim_size + 1;
                            }
                        }
                    }

                    if (directive->GetClass() == directive::DirectiveClass::TemporalMap) {
                        num_iter_this_dim = dim_size/directive->GetOfs();
                        if(dim_size % directive->GetOfs() != 0) {
                            num_iter_this_dim++;
                        }
                    }
                    else if(directive->GetClass() == directive::DirectiveClass::SpatialMap) {
                        num_iter_this_dim = dim_size/(directive->GetOfs() * cluster_size_);
                        if(dim_size % (directive->GetOfs() * cluster_size_) != 0) {
                            num_iter_this_dim++;
                        }
                    }
                    else {
                        //TODO: Handle this error
                    }

                    ret *= num_iter_this_dim;
                } //End of for(idx)
                return ret;
            }

            bool isInnerTemporalMapped(std::string var) {
                auto directive = dataflow_->FindDirective(var);
                auto inner_temporal_map_directive = dataflow_->at(inner_temporal_map_idx_);

                return (directive == inner_temporal_map_directive);
            }


            long GetNumClusters(bool is_spatial_edge = false) {
                if(!is_spatial_edge) {
                    return cluster_size_;
                }
                else {
                    return num_spatial_edge_clusters_;
                }
            }

            long GetNumPartialOutputs() {
                return num_pouts_;
            }

            void SetDataflow(std::shared_ptr<DFA::DirectiveTable> new_dataflow) {
                dataflow_ = new_dataflow;
            }

            void Reset() {
                num_mapped_elements_->clear();
                sp_mapped_unique_elements_->clear();
                tp_mapped_unique_elements_->clear();
                sp_mapped_reused_elements_->clear();
                tp_mapped_reused_elements_->clear();

                cluster_size_ = 1;

                upper_spatial_map_idx_ = invalid_map_pos;
                lower_spatial_map_idx_ = invalid_map_pos;

                outer_temporal_map_idx_ = invalid_map_pos;
                inner_temporal_map_idx_ = invalid_map_pos;

                num_spatial_iterations_ = 1;
                num_spatial_edge_clusters_ = 1;

                num_pouts_ = 0;
            }

            void Reanalyze() {
                Preprocess();
            }


        protected:

            int cluster_level_ = -1;
            int cluster_size_ = 1;

            int upper_spatial_map_idx_ = invalid_map_pos;
            int lower_spatial_map_idx_ = invalid_map_pos;
            int outer_temporal_map_idx_ = invalid_map_pos;
            int inner_temporal_map_idx_ = invalid_map_pos;

            int num_spatial_iterations_ = 1;

            int num_spatial_edge_clusters_ = 1;
            int num_steady_spatial_iterations_ = 1;
            int num_edge_spatial_iterations_ = 0;

            long num_pouts_ = 0;

            std::shared_ptr<DFA::DimensionTable> dimensions_;
            std::shared_ptr<DFA::DirectiveTable> dataflow_;
            std::shared_ptr<AHW::NetworkOnChipModel> noc_;

            std::unique_ptr<std::map<std::string, int>> num_mapped_elements_; //TSz

            std::unique_ptr<std::map<std::string, int>> sp_mapped_unique_elements_; //TUSz
            std::unique_ptr<std::map<std::string, int>> tp_mapped_unique_elements_; //TUSz

            std::unique_ptr<std::map<std::string, int>> sp_mapped_reused_elements_; //
            std::unique_ptr<std::map<std::string, int>> tp_mapped_reused_elements_; //

            std::shared_ptr<DFA::TensorTable> tensors_;

        private:
            //Get the index of the inner-most spatial map
            void AnalyzeSpatialMapIdx() {
                int idx = 0;

                for(auto& it : *dataflow_) {
                    if(it->GetClass() == directive::DirectiveClass::SpatialMap) {
                        if(upper_spatial_map_idx_ == invalid_map_pos) { // First spatial map
                            upper_spatial_map_idx_ = idx;
                        }
                        else if(lower_spatial_map_idx_ == invalid_map_pos) { // Second spatial map
                            lower_spatial_map_idx_ = idx;
                        }
                        else { // Third spatial map == Error!
                            this->error_handler_->PrintErrorMsg(TL::ErrorCode::MultiParallelismInSingleCluster, std::to_string(cluster_level_), this->GetName());
                            this->error_handler_->TerminateProgram();
                        }
                    } // End of if(this directive == SpatialMap)
                    idx++;
                }

                if(upper_spatial_map_idx_ == invalid_map_pos) { // Error: No spatial map!
                    this->error_handler_->PrintErrorMsg(TL::ErrorCode::NoSpatialMap, std::to_string(cluster_level_), this->GetName());
                    this->error_handler_->TerminateProgram();
                }

                //TODO: Add another error check for invalid double spatial map

            }

            //Get the index of the inner-most temporal map under the inner-most spatial map.
            // If no temporal map is under the inner-most spatial map, it returns the index to the inner-most spatial map
            // TODO: FLAG! Needs to check the correctness
            void AnalyzeInnerTemporalMapIdx() {
                int inner_temporal_map_index = -1;
                int idx = 0;

                inner_temporal_map_index = upper_spatial_map_idx_;

                for(int idx = upper_spatial_map_idx_; idx < dataflow_->size() ; idx++) {
                    if(dataflow_->at(idx)->GetClass() == directive::DirectiveClass::TemporalMap) {
                        bool isUnrolled = dataflow_->at(idx)->GetSize() >= dimensions_->GetSize(dataflow_->at(idx)->GetVariable());
                        if(!isUnrolled) {
                            inner_temporal_map_index = idx;
                        }
                    }
                }

                inner_temporal_map_idx_ = inner_temporal_map_index;
            }

            void AnalyzeNumSpatialIterations() {
                auto upper_spatial_map_directive = dataflow_->at(upper_spatial_map_idx_);
                auto spatially_mapped_dimension = upper_spatial_map_directive->GetVariable();

                auto sp_dim_size = dimensions_->GetSize(spatially_mapped_dimension);
                auto sp_map_ofs = upper_spatial_map_directive->GetOfs();

                num_spatial_iterations_ = sp_dim_size / (sp_map_ofs * cluster_size_); // TODO: Double-check this

                //This covers edges (either init-edge or normal spatial edge)
                if(sp_dim_size % (sp_map_ofs * cluster_size_) != 0) {
                    num_spatial_iterations_++;
                }
            }

            void AnalyzeMappingSizes() {
                int idx = 0;

                for(auto& directive : *dataflow_) {
                    auto loop_var = directive->GetVariable();

                    (*num_mapped_elements_)[loop_var] = directive->GetSize();

                    if(directive->GetClass() == directive::DirectiveClass::SpatialMap) {
                        (*sp_mapped_unique_elements_)[loop_var] = directive->GetOfs();
                        (*tp_mapped_unique_elements_)[loop_var] = directive->GetSize();
                    }
                    else { // if the directive is TemporalMap
                        (*sp_mapped_unique_elements_)[loop_var] = 0;
                        (*tp_mapped_unique_elements_)[loop_var] = (idx == inner_temporal_map_idx_)? directive->GetOfs() : directive->GetSize();
                    }

                    (*sp_mapped_reused_elements_)[loop_var] = (*num_mapped_elements_)[loop_var] - (*sp_mapped_unique_elements_)[loop_var];
                    (*tp_mapped_reused_elements_)[loop_var] = (*num_mapped_elements_)[loop_var] - (*tp_mapped_unique_elements_)[loop_var];

                    idx++;
                } // End of for_each (directive)
            }

            void AnalyzeSpatialEdgeCase() {
                auto sp_map_directive = dataflow_->at(upper_spatial_map_idx_);
                auto sp_var = sp_map_directive->GetVariable();
                auto sp_dim_sz = dimensions_->GetSize(sp_var);

                // Handle sliding window overlaps
                /*
                if(dimensions_->IsOverlapped(sp_var) && !dimensions_->IsSlidingDim(sp_var)) {
                  auto overlap_dim = dimensions_->GetOverlappingDim(sp_var);
                  sp_dim_sz = sp_dim_sz - dimensions_->GetSize(overlap_dim) + 1;
                }
                 */
                int map_size = sp_map_directive->GetSize();
                int map_ofs = sp_map_directive->GetOfs();

                // TODO: Double check this; Currently it ignores out-of-bound caused by mapping size
                // Note: It should be fine with overlapped dimensions (input column/row in conv) if the given dataflow is legal
                // Note: This is now handled below (beta version)
                int each_sp_iter_base_coverage = (sp_map_directive->GetOfs() * cluster_size_);
                int each_sp_iter_full_coverage = (sp_map_directive->GetOfs() * (cluster_size_-1)) + sp_map_directive->GetSize();

                if(sp_dim_sz > each_sp_iter_full_coverage) {
                    auto sp_dim_to_cover_after_first_sp_iter = (sp_dim_sz-each_sp_iter_base_coverage);

                    num_steady_spatial_iterations_ = ((sp_dim_sz - map_size) / map_ofs + 1) / cluster_size_ -1;
                    num_edge_spatial_iterations_ = ((num_steady_spatial_iterations_ + 1) * (map_ofs * cluster_size_) + each_sp_iter_full_coverage > sp_dim_sz )? 1 : 0;

                    int remaining_items = sp_dim_sz - (num_steady_spatial_iterations_ + 1) * map_ofs * cluster_size_;

                    if(remaining_items < map_size) {
                        num_spatial_edge_clusters_ = 1;
                    }
                    else {
                        num_spatial_edge_clusters_ = (remaining_items - map_size) / map_ofs + 1; // TODO: Fix
                    }
                }
                else {
                    num_steady_spatial_iterations_ = 0;
                    num_edge_spatial_iterations_ = 1;
                    if(sp_dim_sz > sp_map_directive->GetSize()) {
                        num_spatial_edge_clusters_ = (sp_dim_sz -  map_size) / map_ofs + 1; // TODO: Fix
                        int sp_edge_clsuter_coverage = (sp_map_directive->GetOfs() * (num_spatial_edge_clusters_-1)) + sp_map_directive->GetSize();
                        if(sp_edge_clsuter_coverage < sp_dim_sz) num_spatial_edge_clusters_++;
                    }
                    else {
                        num_spatial_edge_clusters_ = 1;
                    }
                }
                if(sp_dim_sz <= sp_map_directive->GetSize()) {
                    num_spatial_edge_clusters_ = 1;
                }
                // End of original version


#ifdef DEBUG_CLUSTER_UNIT
                std::cout << "Cluster lv: " << cluster_level_ << std::endl;
          std::cout << "Cluster size: " << cluster_size_ << std::endl;
          std::cout << "num_steady_spatial_iterations_ size: " << num_steady_spatial_iterations_ << std::endl;
          std::cout << "Cluster num_edge_spatial_iterations_: " << num_edge_spatial_iterations_ << std::endl;
#endif
            }


            void AnalyzeNumPartialOutputs() {
                long num_pouts = 1;

                for(auto& dim : *dimensions_) {
                    auto dim_name = dim->GetName();

                    if(dim_name == DFSL::layer_dim_output_width_ || dim_name == DFSL::layer_dim_output_height_) {
                        continue;
                    }

                    if(dimensions_->IsOverlapped(dim_name)) {
                        if(dimensions_->IsSlidingDim(dim_name)) {
                            num_pouts *= dim->GetSize();
                        }
                        else {
                            auto sliding_dim_name = dimensions_->GetOverlappingDim(dim_name);
                            int sliding_dim_size = dimensions_->GetSize(sliding_dim_name);
                            int adjusted_size =  dim->GetSize() - sliding_dim_size + 1;
                            num_pouts *= (adjusted_size > 0)? dim->GetSize() - sliding_dim_size + 1 : dim->GetSize();
                        }
                    }
                    else {
                        num_pouts *= dim->GetSize();
                    }

                }
            } // End of void AnalyzeNumPartialOutputs()

            void Preprocess() {
                //Functions regarding spatial mapping always need to be called first
                AnalyzeSpatialMapIdx();
                AnalyzeInnerTemporalMapIdx();
                AnalyzeNumSpatialIterations();
                AnalyzeSpatialEdgeCase();
                AnalyzeMappingSizes();
                AnalyzeNumPartialOutputs();
            }
        }; // End of class ClusterUnit
    } // End of namespace DFA
} // End of namespace maestro


#endif
