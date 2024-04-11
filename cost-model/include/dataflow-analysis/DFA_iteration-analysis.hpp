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


#ifndef MAESTRO_DFA_ITERATION_ANALYSIS_HPP_
#define MAESTRO_DFA_ITERATION_ANALYSIS_HPP_

//#define DEBUG_ITERATION_ANALYSIS

#include <vector>
#include <map>
#include <memory>
#include <string>

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "DFA_iteration-status.hpp"

namespace maestro {
    namespace DFA {

        class IterationAnalysis : public MAESTROClass {
        public:
            IterationAnalysis(
                    std::shared_ptr<DFA::DimensionTable> dimensions,
                    std::shared_ptr<DFA::ClusterUnit> cluster
            ) : dimensions_(dimensions), cluster_(cluster) {

                valid_iteration_states_ = std::make_shared<std::vector<std::shared_ptr<std::vector<std::shared_ptr<DFA::IterationState>>>>>();
                iteration_status_table_ = std::make_shared<std::vector<std::shared_ptr<IterationStatus>>>();
                AnalyzeIterationStates();
                ConstructIterationStatusTable();
            }

            std::string ToString() {
                std::string ret = "";

                for(auto& iter_status : *iteration_status_table_) {
                    ret += iter_status->ToString();
                }

                return ret;
            }

            std::shared_ptr<std::vector<std::shared_ptr<IterationStatus>>> GetAllIterationsStatus() {
                return iteration_status_table_;
            }

        protected:
            std::shared_ptr<DFA::DimensionTable> dimensions_;
            std::shared_ptr<DFA::ClusterUnit> cluster_;

            std::shared_ptr<std::vector<std::shared_ptr<std::vector<std::shared_ptr<DFA::IterationState>>>>> valid_iteration_states_;
            std::shared_ptr<std::vector<std::shared_ptr<IterationStatus>>> iteration_status_table_;

        private:

            void AnalyzeIterationStates() {
                auto dataflow = cluster_->GetDataflow();
                for(auto& directive : *dataflow) {
#ifdef DEBUG_ITERATION_ANALYSIS
                    std::cout << "Directive: " << directive->ToString() << std::endl;
#endif
                    auto directive_var = directive->GetVariable();
                    auto directive_class = directive->GetClass();

                    int dim_size = dimensions_->GetSize(directive_var);
                    int map_size = directive->GetSize();
                    int map_ofs = directive->GetOfs();

                    std::shared_ptr<std::vector<std::shared_ptr<DFA::IterationState>>> iter_state_list = std::make_shared<std::vector<std::shared_ptr<DFA::IterationState>>>();

                    if(directive_class == DFA::directive::DirectiveClass::TemporalMap) {
                        // 1. Init case
                        bool is_init_unroll = dim_size <= map_size;
                        bool is_init_edge = dim_size < map_size;
                        auto init_state = std::make_shared<DFA::IterationState>(directive_var, IterationPosition::Init, 1, is_init_unroll, is_init_edge);
                        iter_state_list->push_back(init_state);

                        // 2. Steady case
                        int num_tp_steady_iters = (dim_size - map_size) / map_ofs;
                        bool has_tp_steady_state = map_size < dim_size;
                        bool has_tp_edge_state = num_tp_steady_iters * map_ofs + map_size < dim_size ;

#ifdef DEBUG_ITERATION_ANALYSIS
                        std::cout << "Num tp_steady states = " << dim_size << " - " << map_size << " / " << map_ofs << std::endl;
#endif

                        if(has_tp_steady_state && num_tp_steady_iters > 0) {
                            auto steady_state = std::make_shared<DFA::IterationState>(directive_var, IterationPosition::Steady, num_tp_steady_iters, false, false);

                            iter_state_list->push_back(steady_state);
                        }

                        // 3. Edge case
                        if(!is_init_edge && has_tp_edge_state) {
                            auto edge_state = std::make_shared<DFA::IterationState>(directive_var, IterationPosition::Edge, 1, false, false);
                            iter_state_list->push_back(edge_state);
                        }
                    }  // End of if (directive_class == TemporalMap)
                    else if(directive_class == DFA::directive::DirectiveClass::SpatialMap) {
                        int num_sub_clusters = cluster_->GetNumClusters(false);

                        // 1. Init case
                        int spatial_coverage = map_size + map_ofs * (num_sub_clusters -1);
                        bool is_init_unroll = dim_size <= spatial_coverage;
                        bool is_init_edge = dim_size < spatial_coverage;

                        int init_edge_normal_sp_iters = (dim_size - map_size) / map_ofs + 1;

                        bool has_init_sp_edge_edge = is_init_edge;
//              bool has_init_sp_edge_edge = is_init_edge && (init_edge_normal_sp_iters * map_ofs + map_size > dim_size);

                        auto init_state = std::make_shared<DFA::IterationState>(directive_var, IterationPosition::Init, 1, is_init_unroll, is_init_edge, has_init_sp_edge_edge);
                        iter_state_list->push_back(init_state);

                        // 2. Steady case
                        bool has_sp_steady_state = spatial_coverage < dim_size;
                        int num_sp_steady_iters = ((dim_size - map_size) / map_ofs + 1) / num_sub_clusters -1; // Cluster array granularity
                        bool has_sp_edge_state =
                                !is_init_unroll
                                && (((num_sp_steady_iters + 1) * (map_ofs * num_sub_clusters) + spatial_coverage > dim_size)
                                    &&  ((num_sp_steady_iters) * (map_ofs * num_sub_clusters) + spatial_coverage != dim_size)); // + 1 : Init case



#ifdef DEBUG_ITERATION_ANALYSIS
                        std::cout << "init_edge_normal_sp_iters: " << init_edge_normal_sp_iters << std::endl;
              std::cout << "coverage: " << init_edge_normal_sp_iters * map_ofs + map_size << std::endl;
              std::cout << "spatial_coverage: " << spatial_coverage <<std::endl;

              std::cout << "Sp coverage (if full util): " << num_sp_steady_iters + 1 << " * " << map_ofs << " * " << num_sub_clusters << " + " << spatial_coverage << std::endl;
              std::cout << "dim size: " << dim_size << std::endl;

              if(has_sp_edge_state) {
                std::cout << "Has Edge!" << std::endl;
              }
#endif

                        if(has_sp_steady_state && num_sp_steady_iters > 0) {
                            auto steady_state = std::make_shared<DFA::IterationState>(directive_var, IterationPosition::Steady, num_sp_steady_iters, false, false);
                            iter_state_list->push_back(steady_state);
                        }

                        //3 . Edge case
                        bool has_sp_edge_edge = false;

                        int remaining_items = dim_size - (num_sp_steady_iters + 1) * map_ofs * num_sub_clusters;
                        int num_active_sub_clusters_at_edge;

#ifdef DEBUG_ITERATION_ANALYSIS
                        std::cout << "remaining_items: " << remaining_items <<std::endl;
              std::cout << "dim_size: " << dim_size <<std::endl;
              std::cout << "num_sp_steady_iters: " << num_sp_steady_iters <<std::endl;
              std::cout << "map_ofs: " << map_ofs <<std::endl;
              std::cout << "num_sub_clusters: " << num_sub_clusters <<std::endl;
#endif

                        if(remaining_items < map_size) {
                            num_active_sub_clusters_at_edge = 1;
                        }
                        else {
                            num_active_sub_clusters_at_edge = (remaining_items - map_size) / map_ofs + 1;
                        }

                        if((num_active_sub_clusters_at_edge-1) * map_ofs + map_size < remaining_items) {
                            num_active_sub_clusters_at_edge++;
                            has_sp_edge_edge = true;
                        }

                        if(!is_init_edge && has_sp_edge_state) {
                            auto edge_state = std::make_shared<DFA::IterationState>(directive_var, IterationPosition::Edge, 1, false, false, has_sp_edge_edge);
                            iter_state_list->push_back(edge_state);
                        }
                    } // End of else if (directive_class == SpatialMap)

                    valid_iteration_states_->push_back(iter_state_list);
                } // End of for_each (directive in dataflow)
            } // End of void AnalyzeIterationStates

            void ConstructIterationStatusTable() {
                int num_dimensions = valid_iteration_states_->size();

                int num_total_cases = 1;
                for(auto& dim_iter_states : *valid_iteration_states_) {
                    num_total_cases *=  dim_iter_states->size();
                }

                std::unique_ptr<std::vector<int>> recursive_occurrence_counters = std::make_unique<std::vector<int>>();

                std::reverse(valid_iteration_states_->begin(), valid_iteration_states_->end());
                int num_accumulative_occurrences = 1;
                for(auto& dim_iter_states : *valid_iteration_states_) {
                    recursive_occurrence_counters->push_back(num_accumulative_occurrences);
                    num_accumulative_occurrences *= dim_iter_states->size();
                }
                std::reverse(valid_iteration_states_->begin(), valid_iteration_states_->end());
                std::reverse(recursive_occurrence_counters->begin(), recursive_occurrence_counters->end());

                std::unique_ptr<std::vector<int>> iteration_counters = std::make_unique<std::vector<int>>();
                for(int i = 0; i < num_dimensions; i++) {
                    iteration_counters->push_back(0);
                }


                for(int case_id = 0; case_id < num_total_cases; case_id++) {
                    std::shared_ptr<IterationStatus> iter_status_this_case = std::make_shared<IterationStatus>();

                    int num_occurrence = 1;
                    int idx = 0;

                    for(auto& dim_iter_states : *valid_iteration_states_) {
                        int change_frequency = recursive_occurrence_counters->at(idx);
                        int state_idx = iteration_counters->at(idx);

                        auto this_state = dim_iter_states->at(state_idx);
                        iter_status_this_case->AddIterState(this_state);

                        num_occurrence *= this_state->GetNumOccurrence();

                        if(case_id % change_frequency == 0) {
                            if(state_idx + 1 == dim_iter_states->size()) {
                                iteration_counters->at(idx) = 0;
                            }
                            else {
                                iteration_counters->at(idx) = state_idx + 1;
                            }
                        }

                        idx++;
                    }

                    iter_status_this_case->SetNumOccurrences(num_occurrence);
                    iteration_status_table_->push_back(iter_status_this_case);
                }
            } // End of void ConstructIterationStatusTable
        }; // End of class IterationAnalysis
    }
}

#endif
