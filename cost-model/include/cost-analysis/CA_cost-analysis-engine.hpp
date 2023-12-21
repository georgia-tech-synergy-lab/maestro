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

#include <iostream>
#include <memory>
#include <cmath>

#include "BASE_constants.hpp"

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "DFA_cluster-unit.hpp"
#include "DFA_cluster-table.hpp"
#include "DFA_iteration-status.hpp"
#include "DFA_iteration-analysis.hpp"

#include "CA_analysis-types.hpp"
#include "CA_reuse-analysis.hpp"
#include "CA_cost-analysis-results.hpp"

namespace maestro {
    namespace CA {

        class CostAnalysisEngine : public MAESTROClass {
        public:

            CostAnalysisEngine(
                    std::shared_ptr<ConfigurationV2> configs,
                    std::shared_ptr<DFA::TensorTable> tensors,
                    std::shared_ptr<DFA::ClusterTable> clusters
            ) :
                    configs_(configs),
                    tensors_(tensors),
                    clusters_(clusters),
                    num_simd_lanes_(configs->target_accelerator_->GetVectorWidth()),
                    MAESTROClass("PerformanceAnalysis") {}

            std::shared_ptr<std::vector<std::shared_ptr<CostAnalysisResults>>>
            AnalyzeEntireCluster(bool write_log_file = false) {

                std::shared_ptr<std::vector<std::shared_ptr<CostAnalysisResults>>> ret = std::make_shared<std::vector<std::shared_ptr<CostAnalysisResults>>>();

                AnalyzeClusterLevel_V2(0, clusters_->size(), clusters_->GetCluster(0)->GetDimensions(), ret, 1, true,
                                       write_log_file);

                return ret;
            }

            void AnalyzeClusterLevel_V2(
                    int cluster_idx,
                    int num_cluster_lvs,
                    std::shared_ptr<DFA::DimensionTable> dimensions,
                    std::shared_ptr<std::vector<std::shared_ptr<CostAnalysisResults>>> ret,
                    int print_cluster_lv = 0,
                    bool do_double_buffering = true,
                    bool write_log_file = false) {

                /* Base information */
                std::shared_ptr<DFA::ClusterUnit> target_cluster = clusters_->GetCluster(cluster_idx);
                int num_sub_clusters = target_cluster->GetNumClusters(false);
                int num_edge_clusters = target_cluster->GetNumClusters(true);
                int num_active_clusters = num_sub_clusters;
                auto dataflow = target_cluster->GetDataflow();
                auto noc = target_cluster->GetNoCModel();

                auto output_tensors = tensors_->GetTensorsInClass(DFA::TensorClass::OutputTensor);
                auto input_tensors = tensors_->GetTensorsInClass(DFA::TensorClass::InputTensor);

                std::ofstream log_file;

                if (write_log_file && cluster_idx <= print_cluster_lv) {
                    log_file.open("log.txt", std::fstream::in | std::fstream::out | std::fstream::app);
                    log_file << "=======================" << std::endl;
                    log_file << "Log for cluster level " << cluster_idx << std::endl;
                    log_file << "=======================" << std::endl;

                    log_file << "Num sub clusters: " << num_sub_clusters << std::endl;

                    log_file << "Cluster tile size" << std::endl;
                    log_file << dimensions->ToString() << std::endl;

                    log_file << "Cluster Dataflow" << std::endl;
                    log_file << dataflow->ToString() << std::endl;
                }

                /* Intermediate analysis */
                auto reuse_analysis = std::make_shared<CA::ReuseAnalysis>(target_cluster, write_log_file);
                auto results = std::make_shared<CostAnalysisResults>(clusters_->GetLayerType(), cluster_idx);
                results->UpdateNumSubClusters(target_cluster->GetNumClusters());

                /* Cost stats */
                long peak_noc_bw_req = 0;
                long double avg_noc_bw_req = 0;
                //felix
                long off_chip_bw_req = 0;
                //
                long double delays[static_cast<int>(DelayType::NumDelayTypes)][static_cast<int>(ValueType::NumValTypes)];
                for (int i = 0; i < static_cast<int>(DelayType::NumDelayTypes); i++) {
                    delays[i][static_cast<int>(ValueType::Min)] = std::numeric_limits<long double>::max();
                    delays[i][static_cast<int>(ValueType::Max)] = std::numeric_limits<long double>::min();
                    delays[i][static_cast<int>(ValueType::Avg)] = 0;
                }

                auto iteration_analysis = std::make_unique<DFA::IterationAnalysis>(dimensions, target_cluster);
                std::shared_ptr<std::vector<std::shared_ptr<DFA::IterationStatus>>> all_iteration_cases = iteration_analysis->GetAllIterationsStatus();

                //Set the buffer size based on the worst case
                // TODO: Apply case-based analysis
//          UpdateBufferSizeReq(results, dimensions, reuse_analysis, do_double_buffering);

                long num_total_cases = 0;
                int case_id = 0;
                for (auto &iteration_case: *all_iteration_cases) {
                    if (case_id == 0) {
                        UpdateBufferSizeReq(results, dimensions, reuse_analysis, iteration_case, cluster_idx,
                                            num_cluster_lvs, do_double_buffering);
                    }

                    long num_case_occurrences = iteration_case->GetNumOccurrences();
                    assert(num_case_occurrences > 0);

                    if (write_log_file && cluster_idx <= print_cluster_lv) {
                        log_file << "======================= CASE " << case_id << " ======================="
                                 << std::endl;
                        log_file << "@ cluster level " << cluster_idx << std::endl;
                        log_file << iteration_case->ToString() << std::endl;
                    }

                    long ingress_spatial_traffic = 0;
                    long egress_spatial_traffic = 0;

                    long num_partial_sums = 0;
                    long tensor_spatial_partial_sum_mapping_size = 0;
                    for (auto &tensor: *output_tensors) {
                        long tensor_egress_traffic = reuse_analysis->GetSpatialEgressTraffic(tensor, iteration_case);
                        long tensor_spatial_mapping_size = reuse_analysis->GetOutputTensorSpatialMappingSize(tensor,
                                                                                                             iteration_case);
                        tensor_spatial_partial_sum_mapping_size += reuse_analysis->GetOutputTensorSpatialMappingSize(
                                tensor, iteration_case, true);
                        auto data_class = tensor->GetDataClass();

                        if (write_log_file && cluster_idx <= print_cluster_lv) {
                            log_file << "Output Tensor " << tensor->GetTensorName() << std::endl;
                            log_file << "\tegress_traffic " << tensor_egress_traffic << std::endl;
                            log_file << "\tspatial_mapping_size" << tensor_spatial_mapping_size << std::endl;
                            log_file << "\ttensor_spatial_partial_sum_mapping_size"
                                     << tensor_spatial_partial_sum_mapping_size << std::endl;
                        }


                        egress_spatial_traffic += tensor_egress_traffic;

                        long upstream_write_this_tensor = tensor_egress_traffic;

                        long prev_upstream_wr_count = results->GetBufferAccessCount(BufferType::Upstream,
                                                                                    BufferAccessType::Write,
                                                                                    data_class);
                        results->UpdateBufferAccessCount(BufferType::Upstream, BufferAccessType::Write,
                                                         prev_upstream_wr_count +
                                                         num_case_occurrences * upstream_write_this_tensor, data_class);

                        long prev_downstream_wr_count = results->GetBufferAccessCount(BufferType::Downstream,
                                                                                      BufferAccessType::Write,
                                                                                      data_class);
                        results->UpdateBufferAccessCount(BufferType::Downstream, BufferAccessType::Write,
                                                         prev_downstream_wr_count +
                                                         num_case_occurrences * tensor_spatial_partial_sum_mapping_size,
                                                         data_class);

                        long prev_downstream_rd_count = results->GetBufferAccessCount(BufferType::Downstream,
                                                                                      BufferAccessType::Read,
                                                                                      data_class);
                        results->UpdateBufferAccessCount(BufferType::Downstream, BufferAccessType::Read,
                                                         prev_downstream_rd_count +
                                                         num_case_occurrences * tensor_spatial_partial_sum_mapping_size,
                                                         data_class);
                        num_partial_sums += reuse_analysis->GetNumCriticalPathPartialSums(tensor, iteration_case);
                    }


                    if (num_partial_sums <= 0) {
//              std::cout << "Num partial sums is less than 0!" << std::endl;
                        if (write_log_file && cluster_idx <= print_cluster_lv) {
                            log_file << "Skipping Invalid case" << std::endl;
                        }
                        continue;
                    }


                    for (auto &tensor: *input_tensors) {
                        long tensor_ingress_traffic = reuse_analysis->GetSpatialIngressTraffic(tensor, iteration_case);
                        long tensor_spatial_mapping_size = reuse_analysis->GetInputTensorSpatialMappingSize(tensor,
                                                                                                            iteration_case);
                        auto data_class = tensor->GetDataClass();

                        if (write_log_file && cluster_idx <= print_cluster_lv) {
                            log_file << "Input Tensor " << tensor->GetTensorName() << std::endl;
                            log_file << "\tingress_traffic " << tensor_ingress_traffic << std::endl;
                            log_file << "\tspatial_mapping_size " << tensor_spatial_mapping_size << std::endl;
                        }

                        ingress_spatial_traffic += tensor_ingress_traffic;

                        long upstream_read_this_tensor = tensor_ingress_traffic;

                        long prev_rd_count = results->GetBufferAccessCount(BufferType::Upstream, BufferAccessType::Read,
                                                                           data_class);
                        results->UpdateBufferAccessCount(BufferType::Upstream, BufferAccessType::Read, prev_rd_count +
                                                                                                       num_case_occurrences *
                                                                                                       upstream_read_this_tensor,
                                                         data_class);

                        long prev_downstream_wr_count = results->GetBufferAccessCount(BufferType::Downstream,
                                                                                      BufferAccessType::Write,
                                                                                      data_class);
                        results->UpdateBufferAccessCount(BufferType::Downstream, BufferAccessType::Write,
                                                         prev_rd_count +
                                                         num_case_occurrences * upstream_read_this_tensor, data_class);

                        long prev_downstream_rd_count = results->GetBufferAccessCount(BufferType::Downstream,
                                                                                      BufferAccessType::Read,
                                                                                      data_class);
                        results->UpdateBufferAccessCount(BufferType::Downstream, BufferAccessType::Read,
                                                         prev_downstream_rd_count +
                                                         num_case_occurrences * tensor_spatial_partial_sum_mapping_size,
                                                         data_class);
                    }

                    double arithmetic_intensity = static_cast<double>(tensor_spatial_partial_sum_mapping_size) /
                                                  static_cast<double>(ingress_spatial_traffic);

                    results->SetArithmeticIntensity(arithmetic_intensity);


                    //TODO: Exactly model cross-PE accumulation

                    if (write_log_file && cluster_idx <= print_cluster_lv) {
                        log_file << "Overall ingress_spatial_traffic: " << ingress_spatial_traffic << std::endl;
                        log_file << "Overall egress_spatial_traffic: " << egress_spatial_traffic << std::endl;
                        log_file << "Number of MACs over sub cluster array: " << tensor_spatial_partial_sum_mapping_size
                                 << std::endl;
                    }

                    ////////////////////////////

                    long computation_delay = 0;
                    std::shared_ptr<std::vector<std::shared_ptr<CA::CostAnalysisResults>>> sub_cluster_results = std::make_shared<std::vector<std::shared_ptr<CA::CostAnalysisResults>>>();

                    std::shared_ptr<DFA::directive::Directive> spmap_directive = nullptr;
                    for (auto &directive: *dataflow) {
                        if (directive->GetClass() == DFA::directive::DirectiveClass::SpatialMap) {
                            spmap_directive = directive;
                            break;
                        }
                    }

                    if (spmap_directive == nullptr) {
                        error_handler_->PrintErrorMsg(TL::ErrorCode::NoSpatialMap, std::to_string(cluster_idx),
                                                      this->GetName());
                        error_handler_->TerminateProgram();
                    }

                    auto spmap_dim_iter_state = iteration_case->GetIterState(spmap_directive->GetVariable());

                    if (spmap_dim_iter_state->IsEdge()) {
                        num_active_clusters = num_edge_clusters;
                    } else {
                        num_active_clusters = num_sub_clusters;
                    }

                    // Recursively process subclusters
                    if (cluster_idx < num_cluster_lvs - 1) {
                        if (spmap_dim_iter_state->IsEdge()) {
                            if (spmap_dim_iter_state->HasSpEdgeEdge()) {
                                auto subclsuter_dim_under_sp_edge_edge = reuse_analysis->ConstructSubClusterDimension(
                                        iteration_case, true);
                                AnalyzeClusterLevel_V2(cluster_idx + 1, num_cluster_lvs,
                                                       subclsuter_dim_under_sp_edge_edge, ret, print_cluster_lv,
                                                       do_double_buffering, write_log_file);
                                auto sp_edge_edge_subcluster_res = ret->at(ret->size() - 1);
                                sub_cluster_results->push_back(sp_edge_edge_subcluster_res);

                                int num_rem_clusters = num_edge_clusters - 1;
                                if (num_rem_clusters > 0) {
                                    auto this_subclsuter_dim = reuse_analysis->ConstructSubClusterDimension(
                                            iteration_case, false);
                                    AnalyzeClusterLevel_V2(cluster_idx + 1, num_cluster_lvs, this_subclsuter_dim, ret,
                                                           print_cluster_lv, do_double_buffering, write_log_file);
                                    auto this_subcluster_res = ret->at(ret->size() - 1);
                                    this_subcluster_res->SetNumSpatialOccurrences(num_rem_clusters);
                                    sub_cluster_results->push_back(this_subcluster_res);
                                }
                            } // End of if(spmap_dim_iter_state->HasSpEdgeEdge())
                            else {
                                auto this_subclsuter_dim = reuse_analysis->ConstructSubClusterDimension(iteration_case,
                                                                                                        false);
                                AnalyzeClusterLevel_V2(cluster_idx + 1, num_cluster_lvs, this_subclsuter_dim, ret,
                                                       print_cluster_lv, do_double_buffering, write_log_file);
                                auto this_subcluster_res = ret->at(ret->size() - 1);
                                this_subcluster_res->SetNumSpatialOccurrences(num_edge_clusters);
                                sub_cluster_results->push_back(this_subcluster_res);
                            } // End of else of if(spmap_dim_iter_state->HasSpEdgeEdge())
                        } // End of if(spmap_dim_iter_state->IsEdge())
                        else {
                            auto this_subclsuter_dim = reuse_analysis->ConstructSubClusterDimension(iteration_case,
                                                                                                    false);
                            AnalyzeClusterLevel_V2(cluster_idx + 1, num_cluster_lvs, this_subclsuter_dim, ret,
                                                   print_cluster_lv, do_double_buffering, write_log_file);
                            auto this_subcluster_res = ret->at(ret->size() - 1);
                            this_subcluster_res->SetNumSpatialOccurrences(num_sub_clusters);
                            sub_cluster_results->push_back(this_subcluster_res);
                        }

                        // Take the worst-case delay as the computation delay
                        for (auto &sub_res: *sub_cluster_results) {
                            computation_delay = std::max(computation_delay, sub_res->GetRuntime());
                        }

                    } // End of if(cluster_idx < num_cluster_lvs-1)
                    else { // Base cluster
                        computation_delay = static_cast<long>(
                                std::ceil(
                                        static_cast<double>(num_partial_sums) / static_cast<double>(num_simd_lanes_)));
                    }
                    ////////////////////////////

                    long ingress_comm_delay = noc->GetOutStandingDelay(ingress_spatial_traffic);
                    long egress_comm_delay = noc->GetOutStandingDelay(egress_spatial_traffic);

                    long outstanding_delay;
                    if (iteration_case->isAllInit()) {
                        outstanding_delay = (do_double_buffering) ? computation_delay + ingress_comm_delay :
                                            ingress_comm_delay + computation_delay + egress_comm_delay;
                    } else {
                        outstanding_delay = (do_double_buffering) ? std::max(egress_comm_delay,
                                                                             std::max(computation_delay,
                                                                                      ingress_comm_delay)) :
                                            ingress_comm_delay + computation_delay + egress_comm_delay;
                    }
                    //felix
                    if (cluster_idx == 0) {
                        auto out_buffer_delay =
                                (results->GetBufferSizeReq(CA::BufferType::Upstream, DataClass::Output)) /
                                configs_->offchip_bw_;
                        auto in_buffer_delay = (results->GetBufferSizeReq(CA::BufferType::Upstream, DataClass::Input) +
                                                results->GetBufferSizeReq(CA::BufferType::Upstream,
                                                                          DataClass::Weight)) / configs_->offchip_bw_;
                        auto ingress_offchip_delay = (do_double_buffering) ? in_buffer_delay / 2 : in_buffer_delay;
                        auto egress_offchip_delay = (do_double_buffering) ? out_buffer_delay / 2 : out_buffer_delay;
                        outstanding_delay = (do_double_buffering) ? std::max(ingress_offchip_delay,
                                                                             std::max(outstanding_delay,
                                                                                      egress_offchip_delay)) :
                                            outstanding_delay + ingress_offchip_delay + egress_offchip_delay;
                        //felix
                        off_chip_bw_req = std::max(off_chip_bw_req, results->GetBufferSizeReq(CA::BufferType::Upstream,
                                                                                              DataClass::Output) /
                                                                    computation_delay);
                        off_chip_bw_req = std::max(off_chip_bw_req, (results->GetBufferSizeReq(CA::BufferType::Upstream,
                                                                                               DataClass::Input) +
                                                                     results->GetBufferSizeReq(CA::BufferType::Upstream,
                                                                                               DataClass::Weight)) /
                                                                    computation_delay);
                        off_chip_bw_req = (do_double_buffering) ? off_chip_bw_req / 2 : off_chip_bw_req;
                        //
                        //felix
                        results->UpdateOffchipBWReq(off_chip_bw_req);
                    }
                    //
                    results->UpdateRuntime(
                            results->GetRuntime(CA::EstimationType::Exact) + num_case_occurrences * outstanding_delay,
                            CA::EstimationType::Exact);
                    results->UpdateNumComputations(results->GetNumComputations() +
                                                   num_case_occurrences * tensor_spatial_partial_sum_mapping_size);


                    long double num_active_unit_clusters = 0;
                    for (auto &sub_res: *sub_cluster_results) {
                        num_active_unit_clusters +=
                                sub_res->GetNumAvgActiveClusters() * sub_res->GetNumSpatialOccurrences();
                    }

                    num_active_unit_clusters = (num_active_unit_clusters == 0) ? num_active_clusters
                                                                               : num_active_unit_clusters;
                    results->SetNumAvgActiveClusters(
                            results->GetNumAvgActiveClusters() + num_active_unit_clusters * num_case_occurrences);

                    //TODO: Doble check
                    if (computation_delay == 0)
                        computation_delay = 1;

                    peak_noc_bw_req = std::max(peak_noc_bw_req,
                                               std::max(ingress_spatial_traffic, egress_spatial_traffic) /
                                               computation_delay);
                    avg_noc_bw_req +=
                            (num_case_occurrences * std::max(ingress_spatial_traffic, egress_spatial_traffic)) /
                            computation_delay;


                    delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Avg)] +=
                            num_case_occurrences * ingress_comm_delay;
                    delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Avg)] +=
                            num_case_occurrences * egress_comm_delay;
                    delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Avg)] +=
                            num_case_occurrences * computation_delay;

                    delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Min)]
                            = std::min(delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Min)],
                                       static_cast<long double>(ingress_comm_delay));
                    delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Min)]
                            = std::min(delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Min)],
                                       static_cast<long double>(egress_comm_delay));
                    delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Min)]
                            = std::min(
                            delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Min)],
                            static_cast<long double>(computation_delay));

                    delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Max)]
                            = std::max(delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Max)],
                                       static_cast<long double>(ingress_comm_delay));
                    delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Max)]
                            = std::max(delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Max)],
                                       static_cast<long double>(egress_comm_delay));
                    delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Max)]
                            = std::max(
                            delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Max)],
                            static_cast<long double>(computation_delay));

                    num_total_cases += num_case_occurrences;
                    case_id++;

                    if (write_log_file && cluster_idx <= print_cluster_lv) {
                        if (iteration_case->isAllInit()) {
                            log_file << "Note: Initialization case; cannot exploit latency hiding in this case"
                                     << std::endl;
                        }

                        log_file << "num computations (per iteration): " << tensor_spatial_partial_sum_mapping_size
                                 << std::endl;
                        log_file << "ingress_spatial_traffic (per iteration): " << ingress_spatial_traffic << std::endl;
                        log_file << "egress_spatial_traffic (per iteration): " << egress_spatial_traffic << std::endl;
                        log_file << std::endl;

                        log_file << "ingress_comm_delay (per iteration): " << ingress_comm_delay << std::endl;
                        log_file << "egress_comm_delay (per iteration): " << egress_comm_delay << std::endl;
                        log_file << "computation_delay (per iteration): " << computation_delay << std::endl;
                        log_file << std::endl;

                        if (do_double_buffering) {
                            if (iteration_case->isAllInit()) {
                                log_file
                                        << "This case is <<Ingress communication + computation>> bound (Initialization case)"
                                        << std::endl;
                            } else if (outstanding_delay == computation_delay) {
                                log_file << "This case is <<Computation>> bound" << std::endl;
                            } else if (outstanding_delay == ingress_comm_delay) {
                                log_file << "This case is <<Ingress communication>> bound" << std::endl;
                            } else if (outstanding_delay == egress_comm_delay) {
                                log_file << "This case is <<Egress communication>> bound" << std::endl;
                            }
                        }

                        log_file << "outstanding_delay (per iteration): " << outstanding_delay << std::endl;
                        log_file << "outstanding_delay (for all iterations in this case): "
                                 << num_case_occurrences * outstanding_delay << std::endl;
                        log_file << "======================= END CASE " << case_id << " =======================\n\n"
                                 << std::endl;
                    }
                } // End of for_each (iteration_case) in (all_iteration_cases)
                avg_noc_bw_req = avg_noc_bw_req / num_total_cases;

                if (num_total_cases != 0) {
                    results->SetNumAvgActiveClusters(results->GetNumAvgActiveClusters() / num_total_cases);
                }
                results->UpdatePeakBWReq(peak_noc_bw_req);
                results->UpdateAvgBWReq(avg_noc_bw_req);

                delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Avg)] /= num_total_cases;
                delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Avg)] /= num_total_cases;
                delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Avg)] /= num_total_cases;

                results->UpdateDelay(DelayType::Ingress, ValueType::Avg,
                                     delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Avg)]);
                results->UpdateDelay(DelayType::Egress, ValueType::Avg,
                                     delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Avg)]);
                results->UpdateDelay(DelayType::Computation, ValueType::Avg,
                                     delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Avg)]);

                results->UpdateDelay(DelayType::Ingress, ValueType::Min,
                                     delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Min)]);
                results->UpdateDelay(DelayType::Egress, ValueType::Min,
                                     delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Min)]);
                results->UpdateDelay(DelayType::Computation, ValueType::Min,
                                     delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Min)]);

                results->UpdateDelay(DelayType::Ingress, ValueType::Max,
                                     delays[static_cast<int>(DelayType::Ingress)][static_cast<int>(ValueType::Max)]);
                results->UpdateDelay(DelayType::Egress, ValueType::Max,
                                     delays[static_cast<int>(DelayType::Egress)][static_cast<int>(ValueType::Max)]);
                results->UpdateDelay(DelayType::Computation, ValueType::Max,
                                     delays[static_cast<int>(DelayType::Computation)][static_cast<int>(ValueType::Max)]);

                ret->push_back(results);
            }


        protected:
            std::shared_ptr<ConfigurationV2> configs_;
            std::shared_ptr<DFA::TensorTable> tensors_;
            std::shared_ptr<DFA::ClusterTable> clusters_;
            int num_simd_lanes_;

        private:

            void UpdateBufferSizeReq(
                    std::shared_ptr<CostAnalysisResults> results,
                    std::shared_ptr<DFA::DimensionTable> dimensions,
                    std::shared_ptr<CA::ReuseAnalysis> reuse_analysis,
                    std::shared_ptr<DFA::IterationStatus> iter_status,
                    int cluster_idx,
                    int num_cluster_lvs,
                    bool do_double_buffering) {
                auto output_tensors = tensors_->GetTensorsInClass(DFA::TensorClass::OutputTensor);
                auto input_tensors = tensors_->GetTensorsInClass(DFA::TensorClass::InputTensor);

                int buffer_size_mult = do_double_buffering ? 2 : 1;
                for (auto &tensor: *input_tensors) {
                    auto dataclass = tensor->GetDataClass();
                    auto coupled_vars = tensor->GetCoupledVariables();
                    long size = 1;

                    for (auto &dim: *coupled_vars) {
                        size *= dimensions->GetSize(dim);
                    }

                    auto upstream_buffer_req = reuse_analysis->GetSpatialIngressTraffic(tensor, iter_status);
                    auto prev_upstream_buffer_req = results->GetBufferSizeReq(BufferType::Upstream,
                                                                              tensor->GetDataClass());

                    results->UpdateBufferSizeReq(BufferType::Upstream,
                                                 prev_upstream_buffer_req + upstream_buffer_req * buffer_size_mult,
                                                 dataclass);

                    results->UpdateBufferAccessCount(BufferType::Upstream, BufferAccessType::Write, size, dataclass);

                    auto downstream_buffer_req = reuse_analysis->GetMappedVolume(tensor);
                    auto prev_downstream_buffer_req = results->GetBufferSizeReq(BufferType::Downstream,
                                                                                tensor->GetDataClass());
                    results->UpdateBufferSizeReq(BufferType::Downstream,
                                                 prev_downstream_buffer_req + buffer_size_mult * downstream_buffer_req,
                                                 dataclass);
                }

                for (auto &tensor: *output_tensors) {
                    auto dataclass = tensor->GetDataClass();
                    auto coupled_vars = tensor->GetCoupledVariables();
                    long size = 1;

                    for (auto &dim: *coupled_vars) {
                        if (dimensions->IsOverlapped(dim)) {
                            auto overlapping_dim = dimensions->GetOverlappingDim(dim);
                            int sliding_dim_size = dimensions->GetSize(overlapping_dim);
                            int reference_dim_size = dimensions->GetSize(dim);;
                            int actual_reference_dim_size = reference_dim_size;
                            size *= reference_dim_size - sliding_dim_size + 1;
                        } else {
                            size *= dimensions->GetSize(dim);
                        }
                    }

                    auto upstream_buffer_req = reuse_analysis->GetSpatialEgressTraffic(tensor, iter_status);
                    results->UpdateBufferSizeReq(BufferType::Upstream, buffer_size_mult * upstream_buffer_req,
                                                 dataclass);

                    results->UpdateBufferAccessCount(BufferType::Upstream, BufferAccessType::Read, size, dataclass);

                    auto downstream_buffer_req = reuse_analysis->GetMappedVolume(tensor);
                    results->UpdateBufferSizeReq(BufferType::Downstream, buffer_size_mult * downstream_buffer_req,
                                                 dataclass);
                }
            }
        };
    }
}