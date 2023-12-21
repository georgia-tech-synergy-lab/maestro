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


#ifndef CA_REUSE_ANALYSIS_HPP_
#define CA_REUSE_ANALYSIS_HPP_

//#define DEBUG_REUSE_ANALYSIS

#include <memory>
#include <map>
#include <cmath>

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "DFSL_syntax_tokens.hpp"

#include "DFA_directives.hpp"
#include "DFA_tensor.hpp"
#include "DFA_cluster-unit.hpp"
#include "DFA_iteration-status.hpp"

#include "CA_analysis-types.hpp"


namespace  maestro {
    namespace CA {

        class ReuseAnalysis : public MAESTROClass {
        public:
            ReuseAnalysis (std::shared_ptr<DFA::ClusterUnit> target_cluster, bool write_log_file = false) :
                    MAESTROClass("Reuse Analysis"),
                    target_cluster_(target_cluster),
                    write_log_file_(write_log_file) {
                num_mapped_elements_ = std::make_unique<std::map<std::string, int>>();
                num_mapped_elements_edge_ = std::make_unique<std::map<std::string, int>>();

                num_unique_elements_ = std::make_unique<std::map<std::string, int>>();
                num_unique_elements_edge_ = std::make_unique<std::map<std::string, int>>();

                num_reused_elements_ = std::make_unique<std::map<std::string, int>>();
                num_reused_elements_edge_ = std::make_unique<std::map<std::string, int>>();

                AnalyzeInputMappingSizes(target_cluster);
                AnalyzeOutputMappingSizes(target_cluster);

#ifdef DEBUG_REUSE_ANALYSIS
                for(auto it : *num_mapped_elements_ ){
            std::cout << "NumMapped_elements[" << it.first << "] = " <<  it.second << std::endl;
          }
          for(auto it : *num_mapped_elements_edge_ ){
            std::cout << "NumMapped_elements_edge[" << it.first << "] = " <<  it.second << std::endl;
          }

          for(auto it : *num_unique_elements_ ){
            std::cout << "num_unique_elements_[" << it.first << "] = " <<  it.second << std::endl;
          }

          for(auto it : *num_unique_elements_edge_){
            std::cout << "num_unique_elements_edge[" << it.first << "] = " <<  it.second << std::endl;
          }

          for(auto it : *num_reused_elements_ ){
            std::cout << "Num_reused_elements[" << it.first << "] = " <<  it.second << std::endl;
          }

          for(auto it : *num_reused_elements_edge_ ){
            std::cout << "Num_reused_elements_edge_[" << it.first << "] = " <<  it.second << std::endl;
          }

#endif
            }

            long GetMappedVolume(std::shared_ptr<DFA::Tensor> tensor) {
                long ret = 1;

                auto coupled_var_list = tensor->GetCoupledVariables();
                for(auto var: *coupled_var_list){
                    if(num_mapped_elements_->find(var) != num_mapped_elements_->end()) {
                        ret *= num_mapped_elements_->at(var);
                    }
                }

                return ret;
            }

            std::shared_ptr<DFA::DimensionTable> ConstructSubClusterDimension(
                    std::shared_ptr<DFA::IterationStatus> iter_status,
                    bool is_sp_edge_edge = false) {
                std::shared_ptr<DFA::DimensionTable> ret = std::make_shared<DFA::DimensionTable>();

                auto dataflow = target_cluster_->GetDataflow();
                auto curr_dimension = target_cluster_->GetDimensions();

                for(auto& directive : * dataflow) {
                    auto dim = directive->GetVariable();
                    auto directive_class = directive->GetClass();
                    auto iter_state = iter_status->GetIterState(dim);
                    auto iter_pos = iter_state->GetIterPosition();

                    int dim_sz = 0;

                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                        if(iter_state->IsEdge()) {
                            dim_sz = (*num_mapped_elements_edge_)[dim];
                        }
                        else {
                            dim_sz = (*num_mapped_elements_)[dim];
                        }
                    } // End of if(directive_class == TemporalMap)
                    else if (directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        int num_sub_clusters = target_cluster_->GetNumClusters(false);
                        int num_edge_sub_clusters = target_cluster_->GetNumClusters(true);

                        switch(iter_pos) {
                            case DFA::IterationPosition::Init: {
                                if(iter_state->IsEdge()) {
                                    if(num_edge_sub_clusters == 1) {
                                        dim_sz = (*num_mapped_elements_edge_)[dim];
                                    }
                                    else if(is_sp_edge_edge) {
                                        dim_sz = (*num_mapped_elements_edge_)[dim];
                                    }
                                    else {
                                        dim_sz = (*num_mapped_elements_)[dim];
                                    }
                                }
                                else {
                                    dim_sz = (*num_mapped_elements_)[dim];
                                }
                                break;
                            }
                            case DFA::IterationPosition::Steady: {
                                dim_sz = (*num_mapped_elements_)[dim];
                                break;
                            }
                            case DFA::IterationPosition::Edge: {
                                if(num_edge_sub_clusters == 1) {
                                    dim_sz = (*num_mapped_elements_edge_)[dim];
                                }
                                else if(is_sp_edge_edge) {
                                    dim_sz = (*num_mapped_elements_edge_)[dim];
                                }
                                else {
                                    dim_sz = (*num_mapped_elements_)[dim];
                                }
                                break;
                            }
                            default:{
                                error_handler_->TerminateProgram();
                            }
                        } // End of switch (iter_pos)
                    } // End of else if(directive_class == SpatialMap)

                    auto outer_stride = curr_dimension->GetOuterStride(dim);
                    auto inner_stride = curr_dimension->GetOuterStride(dim);
                    auto dim_sub_cluster = std::make_shared<DFA::LayerDimension>(dim, dim_sz,outer_stride, inner_stride);

                    ret->AddDimension(dim_sub_cluster);
                } // End of for_each (directive) in (dataflow)

                for(auto& directive : *dataflow) {
                    auto dim = directive->GetVariable();

                    if(dim == DFSL::layer_dim_input_height_) {
                        int output_sz = ret->GetSize(dim) - ret->GetSize(DFSL::layer_dim_weight_height_) + 1;
                        auto output_dim_sub_cluster = std::make_shared<DFA::LayerDimension>(DFSL::layer_dim_output_height_, output_sz, 1, 1);

                        ret->AddDimension(output_dim_sub_cluster);
                    }
                    else if (dim == DFSL::layer_dim_input_width_) {
                        int output_sz = ret->GetSize(dim) - ret->GetSize(DFSL::layer_dim_weight_width_) + 1;
                        auto output_dim_sub_cluster = std::make_shared<DFA::LayerDimension>(DFSL::layer_dim_output_width_, output_sz, 1, 1);

                        ret->AddDimension(output_dim_sub_cluster);
                    }

                    if(curr_dimension->IsOverlapped(dim)) {
                        if(!curr_dimension->IsSlidingDim(dim)) {
                            auto sliding_dim = curr_dimension->GetOverlappingDim(dim);
                            ret->AddOverlapDimension(dim, sliding_dim);
                        }
                    }
                }

                return ret;
            }

            long GetPEMappedVolume(
                    std::shared_ptr<DFA::Tensor> input_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status,
                    bool is_first_pe = true,
                    bool is_sp_edge_edge_pe = false) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = input_tensor->GetCoupledVariables();

                long ret = 1;

                int directive_idx = 0;
                for(auto& directive : *dataflow) {
                    auto directive_class = directive->GetClass();
                    auto cur_dim = directive->GetVariable();

                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                        auto dim = directive->GetVariable();
                        auto iter_state = iter_status->GetIterState(dim);
                        auto iter_pos = iter_state->GetIterPosition();

                        bool is_coupled = std::find(coupled_dims->begin(), coupled_dims->end(), dim) != coupled_dims->end();

                        if(is_coupled) {
                            if(iter_state->IsEdge()) {
                                ret *= (*num_mapped_elements_edge_)[dim];
                            }
                            else {
                                ret *= (*num_mapped_elements_)[dim];
                            }
                        } // End of if(is_coupled)
                    } // End of if(directive_class == TemporalMap)
                    else if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        auto dim = directive->GetVariable();
                        auto iter_state = iter_status->GetIterState(dim);
                        auto iter_pos = iter_state->GetIterPosition();

                        bool is_coupled = std::find(coupled_dims->begin(), coupled_dims->end(), dim) != coupled_dims->end();

                        if(is_coupled) {
                            if(iter_state->IsEdge()) {
                                int num_active_clusters = target_cluster_->GetNumClusters(true);

                                if(num_active_clusters == 1 && (is_first_pe  || is_sp_edge_edge_pe)) {
                                    ret *= (*num_mapped_elements_edge_)[dim];
                                }
                                else if (num_active_clusters > 1) {
                                    if(is_sp_edge_edge_pe) {
                                        ret *= (*num_mapped_elements_edge_)[dim];
                                    }
                                    else {
                                        ret *= (*num_mapped_elements_)[dim];
                                    }
                                }
                            }
                            else {
                                ret *= (*num_mapped_elements_)[dim];
                            }
                        } // End of if(is_coupled)
                    } // End of else if(directive_class == SpatialMap)

                    directive_idx++;
                } // End of for_each (directive) in (dataflow)

                return ret;
            }


            /* This is a critical function that actually estimates reuse based on various cases, which are intertwined in a complicated manner */
            /* And also, this is actually one of the most complicated functions in MAESTRO! (Although it seems pretty short)*/
            long GetPEIngressVolume(
                    std::shared_ptr<DFA::Tensor> input_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status,
                    bool is_first_pe = true,
                    bool is_sp_edge_edge_pe = false) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = input_tensor->GetCoupledVariables();

                long ret = 1;

                int changing_dim_directive_idx = GetInnermostUpdatedDimDirectiveID(input_tensor, iter_status);
                bool is_tensor_overall_inited = IsTensorInited(input_tensor, iter_status, changing_dim_directive_idx);
                bool is_all_reset = (changing_dim_directive_idx == -1);

                if(is_all_reset) {
                    return GetPEMappedVolume(input_tensor, iter_status, is_first_pe, is_sp_edge_edge_pe);
                }

                bool is_this_tensor_changing = false;
                int directive_idx = 0;
                for(auto& directive : *dataflow) {
                    auto dim = directive->GetVariable();
                    auto directive_class = directive->GetClass();
                    auto iter_state = iter_status->GetIterState(dim);
                    auto iter_pos = iter_state->GetIterPosition();

                    bool is_coupled_dim = ( std::find(coupled_dims->begin(), coupled_dims->end(), dim) != coupled_dims->end() );
                    bool is_changing_dim = directive_idx == changing_dim_directive_idx;
                    bool is_reset_dim = directive_idx > changing_dim_directive_idx;
                    bool is_unroll = iter_state->IsUnrolled();

                    if(is_coupled_dim && is_changing_dim) {
                        is_this_tensor_changing = true;
                    }

                    if(is_coupled_dim) {
                        if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                            switch(iter_pos) {
                                case DFA::IterationPosition::Init: {
                                    if(iter_state->IsEdge()) {
                                        ret *= (*num_mapped_elements_edge_)[dim];
                                    }
                                    else {
                                        ret *= (*num_mapped_elements_)[dim];
                                    }
                                    break;
                                }
                                case DFA::IterationPosition::Steady:
                                case DFA::IterationPosition::Edge: {
                                    if(is_changing_dim  && !is_tensor_overall_inited) {
                                        if((*num_unique_elements_)[dim] == 0) {
                                            error_handler_->TerminateProgram();
                                        }
                                        ret *= std::max((*num_unique_elements_)[dim], 1);
                                    }
                                    else {
                                        ret *= (*num_mapped_elements_)[dim];
                                    }
                                    break;
                                }
                                default: {
                                    error_handler_->TerminateProgram();
                                }
                            } // End of switch(iter_pos)
                        } // End of  if(directive_class == TemporalMap)
                        else if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                            int num_total_subclusters = target_cluster_->GetNumClusters(false);
                            int num_edge_subclusters = target_cluster_->GetNumClusters(true);

                            switch(iter_pos) {
                                case DFA::IterationPosition::Init: {
                                    if(is_first_pe) {
                                        long mult = is_sp_edge_edge_pe? (*num_mapped_elements_edge_)[dim] : (*num_mapped_elements_)[dim];
                                        ret *= mult;
                                    }
                                    else {
                                        if(!is_reset_dim && !is_all_reset) {
                                            long mult = is_sp_edge_edge_pe? (*num_mapped_elements_edge_)[dim] : (*num_mapped_elements_)[dim];
                                            ret *= mult;
                                        }
                                        else {
                                            long mult = is_sp_edge_edge_pe? (*num_unique_elements_edge_)[dim] : (*num_unique_elements_)[dim];
                                            ret *= mult;
                                        }
                                    }
                                    break;
                                }
                                case DFA::IterationPosition::Steady:
                                case DFA::IterationPosition::Edge: {
                                    if(is_first_pe) {
                                        long mult = is_sp_edge_edge_pe? (*num_mapped_elements_edge_)[dim] : (*num_mapped_elements_)[dim];
                                        ret *= mult;
                                    }
                                    else {
                                        if(!is_tensor_overall_inited && is_changing_dim) {
                                            long mult = is_sp_edge_edge_pe? (*num_unique_elements_edge_)[dim] : (*num_unique_elements_)[dim];
                                            ret *= mult;
                                        }
                                        else {
                                            long mult = is_sp_edge_edge_pe? (*num_mapped_elements_edge_)[dim] : (*num_mapped_elements_)[dim];
                                            ret *= mult;
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    error_handler_->TerminateProgram();
                                }
                            }
                        }
                        else {
                            error_handler_->TerminateProgram();
                        }
                    } // End of if(is_coupled_dim)

                    directive_idx++;
                } // End of for_each (directive) in (dataflow)

                if(!is_this_tensor_changing) ret = 0;

                return ret;
            }


            long GetSpatialIngressTraffic(
                    std::shared_ptr<DFA::Tensor> input_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status
            ) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = input_tensor->GetCoupledVariables();

                long ret = 1;

                std::shared_ptr<DFA::IterationState> sp_iter_state;
                bool is_sp_mapped = false;
                bool is_sp_edge = false;

                for(auto& dim : *coupled_dims) {
                    auto iter_state = iter_status->GetIterState(dim);
                    auto dataflow = target_cluster_->GetDataflow();
                    auto directive = dataflow->FindDirective(dim);
                    auto directive_class = directive->GetClass();

                    if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        is_sp_mapped = true;
                        sp_iter_state = iter_status->GetIterState(dim);
                        if(iter_state->IsEdge()) {
                            is_sp_edge = true;
                        }
                    }
                }

                std::ofstream log_file;

                if(write_log_file_) {
                    log_file.open("log.txt",std::fstream::in | std::fstream::out | std::fstream::app);
                }

                if(!is_sp_mapped) {
                    if(write_log_file_) {
                        log_file << "Input tensor <" << input_tensor->GetTensorName() << " is not sp-mapped " << std::endl;
                    }
                    ret = GetPEIngressVolume(input_tensor, iter_status);
                }
                else {
                    int num_clusters = target_cluster_->GetNumClusters(false);
                    int num_edge_clusters = target_cluster_->GetNumClusters(true);
                    if(write_log_file_) {
                        log_file << "Input tensor <" << input_tensor->GetTensorName() << " is sp-mapped. num clusters =  " << num_clusters << ", num edge clusters = " << num_edge_clusters << std::endl;
                    }

                    if(!is_sp_edge) {
                        ret = GetPEIngressVolume(input_tensor, iter_status, true, false);
                        ret += (num_clusters -1) * GetPEIngressVolume(input_tensor, iter_status, false, false);
                    }
                    else {
                        bool has_sp_edge_edge = sp_iter_state->HasSpEdgeEdge();
                        if(num_edge_clusters == 1 && has_sp_edge_edge) {
                            ret = GetPEIngressVolume(input_tensor, iter_status, true, true);
                        }
                        else
                            ret = GetPEIngressVolume(input_tensor, iter_status, true, false);

                        if(num_edge_clusters > 1) {
                            int num_sp_edge_edge_cluster = has_sp_edge_edge? 1 : 0;
                            int num_full_spmap_clusters = num_edge_clusters - num_sp_edge_edge_cluster -1 ; //-1: Init
                            num_full_spmap_clusters = std::max(num_full_spmap_clusters, 0);

                            if(write_log_file_) {
                                log_file << "num_sp_edge_edge_cluster: " << num_sp_edge_edge_cluster << std::endl;
                                log_file << "num_full_spmap_clusters: " << num_full_spmap_clusters << std::endl;
                            }

                            ret += num_full_spmap_clusters * GetPEIngressVolume(input_tensor, iter_status, false, false);
                            if(write_log_file_) {
                                log_file << "full sp map clusters, subcluster load volume : " << GetPEIngressVolume(input_tensor, iter_status, false, false) << std::endl;
                            }
                            ret += num_sp_edge_edge_cluster * GetPEIngressVolume(input_tensor, iter_status, false, true);
                            if(write_log_file_) {
                                log_file << "sp_edge_edge sp map clusters, subcluster load volume : " << GetPEIngressVolume(input_tensor, iter_status, false, true) << std::endl;
                            }
                        }
                    }
                }

                return ret;
            }

            long GetInputTensorSpatialMappingSize(
                    std::shared_ptr<DFA::Tensor> input_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status
            ) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = input_tensor->GetCoupledVariables();

                long ret = 1;

                std::shared_ptr<DFA::IterationState> sp_iter_state;
                bool is_sp_mapped = false;
                bool is_sp_edge = false;

                for(auto& dim : *coupled_dims) {
                    auto iter_state = iter_status->GetIterState(dim);
                    auto dataflow = target_cluster_->GetDataflow();
                    auto directive = dataflow->FindDirective(dim);
                    auto directive_class = directive->GetClass();

                    if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        is_sp_mapped = true;
                        sp_iter_state = iter_status->GetIterState(dim);
                        if(iter_state->IsEdge()) {
                            is_sp_edge = true;
                        }
                    }
                }
                int num_clusters = target_cluster_->GetNumClusters(false);

                if(!is_sp_mapped) {
                    ret = num_clusters * GetPEMappedVolume(input_tensor, iter_status);
                }
                else {
                    int num_edge_clusters = target_cluster_->GetNumClusters(true);
                    if(!is_sp_edge) {
                        ret = num_clusters * GetPEMappedVolume(input_tensor, iter_status, true, false);
                    }
                    else {
                        bool has_sp_edge_edge = sp_iter_state->HasSpEdgeEdge();
                        if(num_edge_clusters == 1 && has_sp_edge_edge)
                            ret = GetPEMappedVolume(input_tensor, iter_status, true, true);
                        else
                            ret = GetPEMappedVolume(input_tensor, iter_status, true, false);

                        if(num_edge_clusters > 1) {
                            int num_sp_edge_edge_cluster = has_sp_edge_edge? 1 : 0;
                            int num_full_spmap_clusters = num_edge_clusters - num_sp_edge_edge_cluster -1 ; //-1: Init
                            num_full_spmap_clusters = std::max(num_full_spmap_clusters, 0);

                            ret += num_full_spmap_clusters * GetPEMappedVolume(input_tensor, iter_status, true, false);
                            ret += num_sp_edge_edge_cluster * GetPEMappedVolume(input_tensor, iter_status, false, true);
                        }
                    }
                }

                return ret;
            }

            long GetPEEgressVolume(
                    std::shared_ptr<DFA::Tensor> output_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status,
                    bool get_num_partial_sums = false,
                    bool is_first_pe = true,
                    bool is_sp_edge_edge_pe = false,
                    bool consider_reuse_at_edge = true
            ) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto output_coupled_var_list = output_tensor->GetCoupledVariables();

                long ret = 1;

                if(get_num_partial_sums) {
                    for(auto& directive : *dataflow) {
                        auto dim = directive->GetVariable();
                        auto directive_class = directive->GetClass();

                        if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                            if(std::find(output_coupled_var_list->begin(), output_coupled_var_list->end(), dim) == output_coupled_var_list->end()) {

                                auto iter_state = iter_status->GetIterState(dim);
                                if(iter_state->IsEdge()) {
                                    ret *= (*num_mapped_elements_edge_)[dim];
                                }
                                else {
                                    ret *= (*num_mapped_elements_)[dim];
                                }
                            }
                        } // End of if(directive_class == TemporalMap)
                        else if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                            if(std::find(output_coupled_var_list->begin(), output_coupled_var_list->end(), dim) == output_coupled_var_list->end()) {

                                auto iter_state = iter_status->GetIterState(dim);
                                auto iter_position = iter_state->GetIterPosition();
                                switch(iter_position) {
                                    case DFA::IterationPosition::Init: {
                                        if(iter_state->IsEdge() && is_sp_edge_edge_pe) {
                                            int num_active_sub_clusters = target_cluster_->GetNumClusters(true);

                                            if(num_active_sub_clusters == 1 && (is_first_pe  || is_sp_edge_edge_pe)) {
                                                ret *= (*num_mapped_elements_edge_)[dim];
                                            }
                                            else if (num_active_sub_clusters > 1) {
                                                if(is_sp_edge_edge_pe) {
                                                    ret *= (*num_mapped_elements_edge_)[dim];
                                                }
                                                else if (is_first_pe) {
                                                    ret *= (*num_mapped_elements_)[dim];
                                                }
                                                else {
                                                    if((*num_unique_elements_)[dim] != 0)
                                                        ret *= (*num_unique_elements_)[dim];
                                                }
                                            }
                                        }
                                        else {
                                            if(is_first_pe) {
                                                ret *= (*num_mapped_elements_)[dim];
                                            }
                                            else {
                                                if((*num_unique_elements_)[dim] != 0)
                                                    ret *= (*num_unique_elements_)[dim];
                                            }
                                        }
                                        break;
                                    }
                                    case DFA::IterationPosition::Steady: {
                                        if(is_first_pe) {
                                            ret *= (*num_mapped_elements_)[dim];
                                        }
                                        else {
                                            if((*num_unique_elements_)[dim] != 0)
                                                ret *= (*num_unique_elements_)[dim];
                                        }

                                        break;
                                    }
                                    case DFA::IterationPosition::Edge: {
                                        int num_active_sub_clusters = target_cluster_->GetNumClusters(true);
                                        if(num_active_sub_clusters == 1 && (is_first_pe  || is_sp_edge_edge_pe)) {
                                            ret *= (*num_mapped_elements_edge_)[dim];
                                        }
                                        else if (num_active_sub_clusters > 1) {
                                            if(is_sp_edge_edge_pe) {
                                                ret *= (*num_unique_elements_edge_)[dim];
                                            }
                                            else if (is_first_pe) {
                                                ret *= (*num_mapped_elements_)[dim];
                                            }
                                            else {
                                                if((*num_unique_elements_)[dim] != 0)
                                                    ret *= (*num_unique_elements_)[dim];
                                            }
                                        }
                                        break;
                                    }
                                    default: {
                                        //TODO: handle this error
                                        error_handler_->TerminateProgram();
                                    }
                                }
                            } // End of if(directive_var is not in output coupled variables)
                        } // End of else if(directive_class == SpatialMap)
                    } // End of for each (directive) in (dataflow)
                } // End of if(get_partial_sums)

                for(auto& dim : *output_coupled_var_list) {
                    int directive_idx = dataflow->GetDirectiveIdx(dim);
                    auto directive = dataflow->at(directive_idx);
                    auto directive_class = directive->GetClass();
                    auto iter_state = iter_status->GetIterState(dim);
                    auto iter_position = iter_state->GetIterPosition();

                    bool is_ref_dim = dimensions->IsOverlapped(dim) && !dimensions->IsSlidingDim(dim);
                    std::string actual_dim = dim;

                    if(dim == DFSL::layer_dim_input_height_) {
                        actual_dim = DFSL::layer_dim_output_height_;
                    }
                    else if(dim == DFSL::layer_dim_input_width_) {
                        actual_dim = DFSL::layer_dim_output_width_;
                    }

                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                        switch(iter_position) {
                            case DFA::IterationPosition::Init: {
                                if(iter_state->IsEdge()) {
                                    ret *= (*num_mapped_elements_edge_)[actual_dim];
                                }
                                else {
                                    ret *= (*num_mapped_elements_)[actual_dim];
                                }
                                break;
                            }
                            case DFA::IterationPosition::Steady: {
                                ret *= (*num_mapped_elements_)[actual_dim];
                                break;
                            }
                            case DFA::IterationPosition::Edge: {
                                ret *= (*num_mapped_elements_edge_)[actual_dim];
                                break;
                            }
                            default: {
                                //TODO: Handler this error
                                error_handler_->TerminateProgram();
                            }
                        } // End of switch(iter_position)
                    }// End of if(directive_class == TemporalMap)
                    else if (directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        int num_sub_clusters = target_cluster_->GetNumClusters(false);

                        switch(iter_position) {
                            case DFA::IterationPosition::Init: {
                                if(iter_state->IsEdge() && is_sp_edge_edge_pe) {
                                    int num_active_sub_clusters = target_cluster_->GetNumClusters(true);

                                    if(num_active_sub_clusters == 1 && (is_first_pe  || is_sp_edge_edge_pe)) {
                                        ret *= (*num_mapped_elements_edge_)[actual_dim];
                                    }
                                    else if (num_active_sub_clusters > 1) {
                                        if(is_sp_edge_edge_pe) {
                                            if(consider_reuse_at_edge)
                                                ret *= (*num_unique_elements_edge_)[actual_dim];
                                            else
                                                ret *= (*num_mapped_elements_edge_)[actual_dim];
                                        }
                                        else if (is_first_pe) {
                                            ret *= (*num_mapped_elements_)[actual_dim];
                                        }
                                        else {
                                            if((*num_unique_elements_)[actual_dim] != 0)
                                                ret *= (*num_unique_elements_)[actual_dim];
                                        }
                                    }
                                }
                                else {
                                    if(is_first_pe) {
                                        ret *= (*num_mapped_elements_)[actual_dim];
                                    }
                                    else {
                                        if((*num_unique_elements_)[actual_dim] != 0)
                                            ret *= (*num_unique_elements_)[actual_dim];
                                    }
                                }
                                break;
                            }
                            case DFA::IterationPosition::Steady: {
                                if(is_first_pe) {
                                    ret *= (*num_mapped_elements_)[actual_dim];
                                }
                                else {
                                    if((*num_unique_elements_)[actual_dim] != 0)
                                        ret *= (*num_unique_elements_)[actual_dim];
                                }
                                break;
                            }
                            case DFA::IterationPosition::Edge: {
                                int num_active_sub_clusters = target_cluster_->GetNumClusters(true);
                                if(num_active_sub_clusters == 1 && (is_first_pe  || is_sp_edge_edge_pe)) {
                                    ret *= (*num_mapped_elements_edge_)[dim];
                                }
                                else if (num_active_sub_clusters > 1) {
                                    if(is_sp_edge_edge_pe) {
                                        ret *= (*num_unique_elements_edge_)[actual_dim];
                                    }
                                    else if (is_first_pe) {
                                        ret *= (*num_mapped_elements_)[actual_dim];
                                    }
                                    else {
                                        if((*num_unique_elements_)[actual_dim] != 0)
                                            ret *= (*num_unique_elements_)[actual_dim];
                                    }
                                }
                                break;
                            }
                            default: {
                                //TODO: Handler this error
                                error_handler_->TerminateProgram();
                            }
                        } // End of switch(iter_position)
                    } // End of else if (directive_class == SpatialMap)
                } // End of for_each (var) in (output_coupled_list)

                return ret;
            }

            long GetSpatialEgressTraffic(
                    std::shared_ptr<DFA::Tensor> output_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status
            ) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = output_tensor->GetCoupledVariables();

                long ret = 1;

                std::shared_ptr<DFA::IterationState> sp_iter_state;
                bool is_sp_mapped = false;
                bool is_sp_edge = false;

                for(auto& dim : *coupled_dims) {
                    auto iter_state = iter_status->GetIterState(dim);
                    auto dataflow = target_cluster_->GetDataflow();
                    auto directive = dataflow->FindDirective(dim);
                    auto directive_class = directive->GetClass();

                    if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        is_sp_mapped = true;
                        sp_iter_state = iter_status->GetIterState(dim);
                        if(iter_state->IsEdge()) {
                            is_sp_edge = true;
                        }
                    }
                }


                if(!is_sp_mapped) {
                    ret = GetPEEgressVolume(output_tensor, iter_status);
                }
                else {
                    int num_clusters = target_cluster_->GetNumClusters(false);
                    int num_edge_clusters = target_cluster_->GetNumClusters(true);
                    if(!is_sp_edge) {
                        ret = GetPEEgressVolume(output_tensor, iter_status, false, true, false);
                        ret += (num_clusters -1) * GetPEEgressVolume(output_tensor, iter_status, false, false, false);

                    }
                    else {
                        bool has_sp_edge_edge = sp_iter_state->HasSpEdgeEdge();
                        if(num_edge_clusters == 1 && has_sp_edge_edge)
                            ret = GetPEEgressVolume(output_tensor, iter_status, false, true, true);
                        else
                            ret = GetPEEgressVolume(output_tensor, iter_status, false, true, false);

                        if(num_edge_clusters > 1) {
                            int num_sp_edge_edge_cluster = has_sp_edge_edge? 1 : 0;
                            int num_full_spmap_clusters = num_edge_clusters - num_sp_edge_edge_cluster -1 ; //-1: Init
                            num_full_spmap_clusters = std::max(num_full_spmap_clusters, 0);

                            ret += num_full_spmap_clusters * GetPEEgressVolume(output_tensor, iter_status, false, false, false);
                            ret += num_sp_edge_edge_cluster * GetPEEgressVolume(output_tensor, iter_status, false, false, true);
                        }
                    }
                }

                return ret;
            }

            long GetNumCriticalPathPartialSums(
                    std::shared_ptr<DFA::Tensor> output_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status
            ) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = output_tensor->GetCoupledVariables();

                long ret = 1;

                std::shared_ptr<DFA::IterationState> sp_iter_state;
                bool is_sp_mapped = false;
                bool is_sp_edge = false;

                for(auto& dim : *coupled_dims) {
                    auto iter_state = iter_status->GetIterState(dim);
                    auto dataflow = target_cluster_->GetDataflow();
                    auto directive = dataflow->FindDirective(dim);
                    auto directive_class = directive->GetClass();

                    if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        is_sp_mapped = true;
                        sp_iter_state = iter_status->GetIterState(dim);
                        if(iter_state->IsEdge()) {
                            is_sp_edge = true;
                        }
                    }
                }


                if(!is_sp_mapped) {
                    ret = GetPEEgressVolume(output_tensor, iter_status, true);
                }
                else {
                    int num_clusters = target_cluster_->GetNumClusters(false);
                    int num_edge_clusters = target_cluster_->GetNumClusters(true);
                    if(!is_sp_edge) {
                        ret = GetPEEgressVolume(output_tensor, iter_status, true, true, false);
                    }
                    else {
                        bool has_sp_edge_edge = sp_iter_state->HasSpEdgeEdge();
                        if(num_edge_clusters == 1 && has_sp_edge_edge) {
                            ret = GetPEEgressVolume(output_tensor, iter_status, true, true, true);
                        }
                        else {
                            ret = GetPEEgressVolume(output_tensor, iter_status, true, true, false);
                        }
                    }
                }

                return ret;
            }


            long GetOutputTensorSpatialMappingSize(
                    std::shared_ptr<DFA::Tensor> output_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status,
                    bool for_partial_sum = false
            ) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = output_tensor->GetCoupledVariables();

                long ret = 1;

                std::shared_ptr<DFA::IterationState> sp_iter_state;
                bool is_sp_mapped = false;
                bool is_sp_edge = false;

                for(auto& dim : *coupled_dims) {
                    auto iter_state = iter_status->GetIterState(dim);
                    auto dataflow = target_cluster_->GetDataflow();
                    auto directive = dataflow->FindDirective(dim);
                    auto directive_class = directive->GetClass();

                    if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        is_sp_mapped = true;
                        sp_iter_state = iter_status->GetIterState(dim);
                        if(iter_state->IsEdge()) {
                            is_sp_edge = true;
                        }
                    }
                }

                if(for_partial_sum) {
                    for(auto& directive : *dataflow) {
                        auto directive_dim = directive->GetVariable();
                        auto directive_class = directive->GetClass();
                        if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                            auto iter_state = iter_status->GetIterState(directive_dim);
                            is_sp_mapped = true;
                            sp_iter_state = iter_status->GetIterState(directive_dim);

                            if(iter_state->IsEdge()) {
                                is_sp_edge = true;
                            }
                        }
                    }
                }

                int num_clusters = target_cluster_->GetNumClusters(false);
                if(!is_sp_mapped) {
                    ret = num_clusters * GetPEEgressVolume(output_tensor, iter_status, for_partial_sum);
                }
                else {
                    int num_edge_clusters = target_cluster_->GetNumClusters(true);
                    if(!is_sp_edge) {
                        ret = num_clusters * GetPEEgressVolume(output_tensor, iter_status, for_partial_sum, true, false);
                    }
                    else {
                        bool has_sp_edge_edge = sp_iter_state->HasSpEdgeEdge();

                        if(num_edge_clusters == 1 && has_sp_edge_edge)
                            ret = GetPEEgressVolume(output_tensor, iter_status, for_partial_sum, true, true);
                        else
                            ret = GetPEEgressVolume(output_tensor, iter_status, for_partial_sum, true, false);

                        if(num_edge_clusters > 1) {
                            int num_sp_edge_edge_cluster = has_sp_edge_edge? 1 : 0;
                            int num_full_spmap_clusters = num_edge_clusters - num_sp_edge_edge_cluster -1 ; //-1: Init
                            num_full_spmap_clusters = std::max(num_full_spmap_clusters, 0);

                            ret += num_full_spmap_clusters * GetPEEgressVolume(output_tensor, iter_status, for_partial_sum, true, false);
                            ret += num_sp_edge_edge_cluster * GetPEEgressVolume(output_tensor, iter_status, for_partial_sum, false, true, false);
                            long tmp = GetPEEgressVolume(output_tensor, iter_status, for_partial_sum, true, false);
                        }
                    }
                }

                return ret;
            }

        protected:
            bool write_log_file_ = false;
            std::shared_ptr<DFA::ClusterUnit> target_cluster_;

            std::unique_ptr<std::map<std::string, int>> num_mapped_elements_;
            std::unique_ptr<std::map<std::string, int>> num_mapped_elements_edge_;
            std::unique_ptr<std::map<std::string, int>> num_mapped_elements_sp_edge_;

            std::unique_ptr<std::map<std::string, int>> num_unique_elements_;
            std::unique_ptr<std::map<std::string, int>> num_unique_elements_edge_;
            std::unique_ptr<std::map<std::string, int>> num_unique_elements_sp_edge_;

            std::unique_ptr<std::map<std::string, int>> num_reused_elements_;
            std::unique_ptr<std::map<std::string, int>> num_reused_elements_edge_;
            std::unique_ptr<std::map<std::string, int>> num_reused_elements_sp_edge_;

        private:

            int GetInnermostUpdatedDimDirectiveID(
                    std::shared_ptr<DFA::Tensor> input_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = input_tensor->GetCoupledVariables();

                int prime_change_dim_directive_idx = -1;

                int idx = 0;
                for(auto& directive : *dataflow) {
                    auto directive_class = directive->GetClass();
                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap || directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        auto dim = directive->GetVariable();
                        auto iter_state = iter_status->GetIterState(dim);
                        auto iter_pos = iter_state->GetIterPosition();

                        if(iter_pos != DFA::IterationPosition::Init) {
                            prime_change_dim_directive_idx = idx;
                        }

                    }
                    idx++;
                }

                return prime_change_dim_directive_idx;
            }


            bool IsTensorInited(
                    std::shared_ptr<DFA::Tensor> input_tensor,
                    std::shared_ptr<DFA::IterationStatus> iter_status,
                    int changing_dim_idx) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = input_tensor->GetCoupledVariables();

                bool tensor_inited = false;

                int directive_idx = 0;
                for(auto& directive : *dataflow) {
                    auto directive_class = directive->GetClass();
                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap || directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        auto dim = directive->GetVariable();
                        auto iter_state = iter_status->GetIterState(dim);
                        auto iter_pos = iter_state->GetIterPosition();
                        bool is_coupled = std::find(coupled_dims->begin(), coupled_dims->end(), dim) != coupled_dims->end();

                        if(iter_pos == DFA::IterationPosition::Init
                           && is_coupled
                           && directive_idx > changing_dim_idx
                           && !iter_state->IsUnrolled()) {
                            tensor_inited = true;
                            break;
                        }
                    }

                    directive_idx++;
                }

                return tensor_inited;
            }


        public:
            /* This is a critical function that actually estimates reuse based on various cases */
            /* And also, this is actually the most complicated function! (Although it seems pretty short)*/
            long GetPELoadVolume(std::shared_ptr<DFA::Tensor> tensor,
                                 std::shared_ptr<std::vector<IterationStatus>> iter_status,
                                 bool is_first_pe) {
//          std::cout << "[ReuseAnalysis-GetPELoadVolume] Tensor is " << tensor->GetTensorName() << std::endl;


                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = tensor->GetCoupledVariables();

                /* Identify which tensor is changing at the given iter_status*/
                // Prime change id: The inner-most Steady or Edge dimension;
                //   when a variable is incremented, all the sub dimensions are init
                int prime_changing_dim_id = GetPrimeChangingDimDirectiveID(iter_status);

//          std::cout << "[ReuseAnalysis-GetPELoadVolume] Prime changing dim id is " << prime_changing_dim_id << std::endl;

                int tensor_prime_changing_dim_id = GetPrimeChangingDimDirectiveIDTensor(tensor, iter_status, prime_changing_dim_id);

                bool is_all_init_case = (prime_changing_dim_id == -1); // Maybe redundant now. Let's check it later
                long load_volume_with_no_tp_reuse = GetFullMappedVolume(tensor, iter_status, is_first_pe);
                if(is_all_init_case) {
                    return load_volume_with_no_tp_reuse;
                }

                bool is_full_unroll = IsTensorFullUnroll(tensor, iter_status);
                if(is_full_unroll) {
                    return 0;
                }

                //This must come after full unroll check because it contains all of the followings:
                // 1) full init
                // 2) full unroll
                // 3) init + unroll mixed
                // Except for 2, we need full load, which is manged below
                bool tensor_require_full_load = (tensor_prime_changing_dim_id == -1); // Full init
                if(tensor_require_full_load) {
                    return load_volume_with_no_tp_reuse;
                }


                // When we have something changing, check if it is also globally changing among tensors
                bool is_tensor_changing = tensor_prime_changing_dim_id == prime_changing_dim_id;
                // If this tensor is not actually globally changing, it is stationary; no load
                if(!is_tensor_changing) {
                    return 0;
                }

                //If this tensor is changing, compute the load volume considering the "partial" temporal reuse
                long load_voulme_with_tp_reuse = 1;

                for(auto& dim : *coupled_dims) {
                    auto directive = dataflow->FindDirective(dim);
                    auto directive_class = directive->GetClass();
                    int directive_idx = dataflow->GetDirectiveIdx(dim);

                    bool is_changing_dim = (directive_idx == prime_changing_dim_id);
/*
            if(is_changing_dim)
              std::cout << "[ReuseAnalysis-GetPELoadVolume] Dimension " << dim << " is changing" << std::endl;
            else
              std::cout << "[ReuseAnalysis-GetPELoadVolume] Dimension " << dim << " is not changing" << std::endl;
*/

                    if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        //std::cout << "[ReuseAnalysis-GetPELoadVolume] Dimension " << dim << " is spatially mapped" << std::endl;

                        //Spatial reuse check is always there
                        if(is_first_pe) {
                            load_voulme_with_tp_reuse *= (*num_mapped_elements_)[dim];
                        }
                        else {
                            load_voulme_with_tp_reuse *= (*num_unique_elements_)[dim];
                        }
                    }
                    else {
//              std::cout << "[ReuseAnalysis-GetPELoadVolume] Dimension " << dim << " is temporally mapped" << std::endl;
                        if(is_changing_dim) {
                            //std::cout << "[ReuseAnalysis-GetPELoadVolume] Dimension " << dim << ", multiply unique element counts: " << (*num_unique_elements_)[dim] << std::endl;
                            load_voulme_with_tp_reuse *= (*num_unique_elements_)[dim];
                        }
                        else {

                            if(iter_status->at(directive_idx) == IterationStatus::Unroll) {
                                load_voulme_with_tp_reuse *= dimensions->GetSize(dim);
                            }
                            else {
                                load_voulme_with_tp_reuse *= (*num_mapped_elements_)[dim];
                            }
                        }
                    }

                    //std::cout << "[ReuseAnalysis-GetPELoadVolume] load_voulme_with_tp_reuse = " << load_voulme_with_tp_reuse << std::endl;
                }

                return load_voulme_with_tp_reuse;
            }

        private:

            int GetPrimeChangingDimDirectiveID(std::shared_ptr<std::vector<IterationStatus>> iter_status) {
                int changing_dim_directive_id = -1;
                int directive_id = -1;

                for(auto& dim_iter_status : *iter_status) {
                    directive_id++;
                    if(dim_iter_status != CA::IterationStatus::Unroll && dim_iter_status != CA::IterationStatus::Init) {
                        if(directive_id > changing_dim_directive_id) {
                            changing_dim_directive_id = directive_id;
                        }
                    }
                }

                return changing_dim_directive_id;
            }


            int GetPrimeChangingDimDirectiveIDTensor(
                    std::shared_ptr<DFA::Tensor> tensor,
                    std::shared_ptr<std::vector<IterationStatus>> iter_status,
                    int prime_changing_dim_id)
            {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = tensor->GetCoupledVariables();

                int changing_dim_directive_id = -1;
                int directive_id = -1;

                for(auto& dim_iter_status : *iter_status) {
                    directive_id++;
                    auto directive = dataflow->at(directive_id);
                    auto directive_dim = directive->GetVariable();

                    bool found = false;
                    for(auto& dim: *coupled_dims) {
                        if(dim == directive_dim) {
                            found =true;
                            break;
                        }
                    }

                    if(!found) {
                        continue; //To next directive
                    }

                    if(dim_iter_status != CA::IterationStatus::Unroll) {
                        if(dim_iter_status != CA::IterationStatus::Init || (dim_iter_status == CA::IterationStatus::Init && prime_changing_dim_id > directive_id)) {
                            if(directive_id > changing_dim_directive_id) {
                                changing_dim_directive_id = directive_id;
                            }
                        }
                    }
                }

                return changing_dim_directive_id;
            }

            bool IsTensorFullUnroll(
                    std::shared_ptr<DFA::Tensor> tensor,
                    std::shared_ptr<std::vector<IterationStatus>> iter_status) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_dims = tensor->GetCoupledVariables();

                bool ret = true;

                int directive_id = -1;
                for(auto& dim_iter_status : *iter_status) {
                    directive_id++;
                    auto directive = dataflow->at(directive_id);
                    auto directive_dim = directive->GetVariable();

                    bool found = false;
                    for(auto& dim: *coupled_dims) {
                        if(dim == directive_dim) {
                            found =true;
                            break;
                        }
                    }

                    if(!found) {
                        continue; //To next directive
                    }

                    if(dim_iter_status != CA::IterationStatus::Unroll) {
                        ret = false;
                    }
                }

                return ret;
            }

            long GetFullMappedVolume(
                    std::shared_ptr<DFA::Tensor> tensor,
                    std::shared_ptr<std::vector<IterationStatus>> iter_status,
                    bool is_first_pe) {
                auto dataflow = target_cluster_->GetDataflow();
                auto dimensions = target_cluster_->GetDimensions();
                auto coupled_vars = tensor->GetCoupledVariables();

                bool has_sp_map = false;
                for(auto& var : * coupled_vars) {
                    auto directive = dataflow->FindDirective(var);
                    if(directive->GetClass() == DFA::directive::DirectiveClass::SpatialMap) {
                        has_sp_map = true;
                        break;
                    }
                }

                long load_volume_with_no_reuse = 1;

                for(auto& var : * coupled_vars) {
                    auto directive = dataflow->FindDirective(var);

                    if(directive->GetClass() == DFA::directive::DirectiveClass::SpatialMap) {
                        if(is_first_pe) {
                            load_volume_with_no_reuse *= (*num_mapped_elements_)[var];
                        }
                        else {
                            load_volume_with_no_reuse *= (*num_unique_elements_)[var]; //Spatial reuse
                        }
                    }
                    else { // Temporal Map
                        if(has_sp_map) {
                            if(target_cluster_->IsInitEdge(var)) {
                                load_volume_with_no_reuse *= (*num_mapped_elements_edge_)[var];
                            }
                            else {
                                load_volume_with_no_reuse *= (*num_mapped_elements_)[var];
                            }
                        }
                        else {
                            if(is_first_pe && target_cluster_->IsInitEdge(var)) {
                                load_volume_with_no_reuse *= (*num_mapped_elements_edge_)[var];
                            }
                            else if(is_first_pe && !target_cluster_->IsInitEdge(var)) {
                                load_volume_with_no_reuse *= (*num_mapped_elements_)[var];
                            }
                            else {
                                load_volume_with_no_reuse = 0;
                            }
                        }

                    }
                }

                return load_volume_with_no_reuse;

            }

            void AnalyzeInputMappingSizes(std::shared_ptr<DFA::ClusterUnit> target_cluster) {
                auto dataflow = target_cluster->GetDataflow();
                auto dimensions = target_cluster->GetDimensions();

                //For each data class (tensor)
                for(auto& directive : *dataflow) {
                    auto directive_class = directive->GetClass();
                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap || directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        auto loop_var = directive->GetVariable();
                        auto map_size = directive->GetSize();
                        auto ofs_size = directive->GetOfs();
                        auto dim_size = dimensions->GetSize(loop_var);

                        int num_steady_iterations = (dim_size - map_size) / ofs_size;
                        num_steady_iterations = (num_steady_iterations < 0)? 0 : num_steady_iterations;
                        bool has_edge = (num_steady_iterations * ofs_size + map_size < dim_size) || (map_size > dim_size); // the latter one: Init-unroll; reverse_edge

                        // 1. Steady cases
                        (*num_mapped_elements_)[loop_var] = map_size;
                        (*num_unique_elements_)[loop_var] = std::min(map_size, ofs_size);
                        (*num_reused_elements_)[loop_var] = std::max(map_size - ofs_size, 0);

                        // 2. Unroll case
                        if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                            bool is_fully_tp_unrolled = dim_size <= map_size;
                            if(is_fully_tp_unrolled) {
                                (*num_unique_elements_)[loop_var] = 0;
                                (*num_reused_elements_)[loop_var] = map_size;
                            }
                        }

                        // 2. Edge  cases; Either temporally or spatially in sub-cluster granularity
                        int edge_map_size = dim_size - ((num_steady_iterations + 1) * ofs_size);
                        edge_map_size = (edge_map_size < 0)? dim_size : edge_map_size;

                        int edge_out_of_bound_size = map_size - edge_map_size;
                        (*num_mapped_elements_edge_)[loop_var] = has_edge? edge_map_size : map_size; // map_size : deals with init-edge
                        (*num_unique_elements_edge_)[loop_var] = std::max(ofs_size - edge_out_of_bound_size, 0);
                        (*num_reused_elements_edge_)[loop_var] = has_edge? (*num_mapped_elements_edge_)[loop_var] - (*num_unique_elements_edge_)[loop_var] : 0;
                    } // End of if(directve_class == tMap or sMap)
                } // End of for(auto directive : dataflow)
            } // End of void AnalyzeMappingSizes


            void AnalyzeOutputMappingSizes(std::shared_ptr<DFA::ClusterUnit> target_cluster) {
                auto dataflow = target_cluster->GetDataflow();
                auto dimensions = target_cluster->GetDimensions();

                //For each data class (tensor)
                for(auto& directive : *dataflow) {
                    auto directive_class = directive->GetClass();
                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap || directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        auto directive_var = directive->GetVariable();
                        bool is_ref_dim = dimensions->IsOverlapped(directive_var) && !dimensions->IsSlidingDim(directive_var);
                        if(is_ref_dim) {
                            auto sliding_dim = dimensions->GetOverlappingDim(directive_var);

                            //TODO: This is only for DNN ops. Generalize this for arbitrary ops. (Mostly, it'll be fine though)
                            auto output_var = (directive_var == DFSL::layer_dim_input_height_)? DFSL::layer_dim_output_height_ : DFSL::layer_dim_output_width_;

                            (*num_mapped_elements_)[output_var] = (*num_mapped_elements_)[directive_var] - (*num_mapped_elements_)[sliding_dim] + 1;
                            (*num_unique_elements_)[output_var] = std::min(directive->GetOfs(), directive->GetSize());
                            //TODO: The following assumes legal dataflow; this will yield wield results for illegal dataflows
                            (*num_mapped_elements_edge_)[output_var] = (*num_mapped_elements_edge_)[directive_var] - (*num_mapped_elements_edge_)[sliding_dim] + 1;
                            (*num_unique_elements_edge_)[output_var] = std::min(directive->GetOfs(), (*num_mapped_elements_edge_)[output_var]);

                        }
                    } // End of if(directve_class == tMap or sMap)
                } // End of for(auto directive : dataflow)
            } // End of void AnalyzeOutputMappingSizes
        }; // End of class TemporalReuseAnalysis
    }; // End of namespace CA
}; // End of namespace maestro
#endif
