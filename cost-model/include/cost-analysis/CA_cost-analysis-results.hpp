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

#ifndef CA_COST_ANALYSIS_OUTPUT_HPP_
#define CA_COST_ANALYSIS_OUTPUT_HPP_

#include "BASE_maestro-class.hpp"
#include "BASE_constants.hpp"

#include "DFA_layer.hpp"
#include "DFA_iteration-status.hpp"
#include "CA_analysis-types.hpp"

namespace maestro {
    namespace CA {

        enum class BufferType {Upstream, Downstream};
        enum class BufferAccessType {Read, Write};
        enum class DelayType {Ingress, Egress, Computation, NumDelayTypes};
        enum class ValueType {Min, Max, Avg, NumValTypes};

        class CostAnalysisResults : public MAESTROClass {
        public:
            CostAnalysisResults(LayerType layer_type, int cluster_level) :
                    layer_type_(layer_type),
                    cluster_level_(cluster_level),
                    num_computations_(0),
                    MAESTROClass ("CostAnalysisResults at cluster level" + std::to_string(cluster_level)) {
                iter_status_info_ = std::make_shared<std::vector<std::shared_ptr<DFA::IterationStatus>>>();

                ingress_delay_[static_cast<int>(ValueType::Max)] = 0;
                ingress_delay_[static_cast<int>(ValueType::Min)] = std::numeric_limits<int>::max();
                ingress_delay_[static_cast<int>(ValueType::Avg)] = 0;

                egress_delay_[static_cast<int>(ValueType::Max)] = 0;
                egress_delay_[static_cast<int>(ValueType::Min)] = std::numeric_limits<int>::max();
                egress_delay_[static_cast<int>(ValueType::Avg)] = 0;

                compute_delay_[static_cast<int>(ValueType::Max)] = 0;
                compute_delay_[static_cast<int>(ValueType::Min)] = std::numeric_limits<int>::max();
                compute_delay_[static_cast<int>(ValueType::Avg)] = 0;

                avg_num_active_unit_clusters_ = 0;
            }

            long GetRuntime(CA::EstimationType estimation_type = CA::EstimationType::Exact) {
                return runtime_[static_cast<int>(estimation_type)];
            }

            long GetNumComputations() {
                return num_computations_;
            }

            long GetTopNumComputations() {
                return top_level_num_computations_;
            }


            long GetBufferAccessCount(BufferType target_buffer, BufferAccessType access_type, DataClass data_class) {
                long ret;
                int data_class_id = static_cast<int>(data_class);
                if(target_buffer == BufferType::Upstream) {
                    ret = (access_type == BufferAccessType::Read)?
                          upstream_buffer_read_[data_class_id] : upstream_buffer_write_estimate_[data_class_id];
                }
                else {
                    ret = (access_type == BufferAccessType::Read)?
                          downstream_buffer_read_estimate[data_class_id] : downstream_buffer_write_[data_class_id];
                }
                return ret;
            }

            long GetDelay(DelayType delay_type, ValueType val_type) {
                if(delay_type == DelayType::Ingress) {
                    return ingress_delay_[static_cast<int>(val_type)];
                }
                else if(delay_type == DelayType::Egress) {
                    return egress_delay_[static_cast<int>(val_type)];
                }
                else {
                    return compute_delay_[static_cast<int>(val_type)];
                }
            }

            void UpdateBufferAccessCount(BufferType target_buffer, BufferAccessType access_type, long counts, DataClass data_class) {

                int data_class_id = static_cast<int>(data_class);

                if((target_buffer == BufferType::Upstream) && (access_type == BufferAccessType::Read)) {
                    upstream_buffer_read_[data_class_id] = counts;
                }
                else if((target_buffer == BufferType::Upstream) && (access_type == BufferAccessType::Write)) {
                    upstream_buffer_write_estimate_[data_class_id] = counts;
                }
                else if((target_buffer == BufferType::Downstream) && (access_type == BufferAccessType::Read)) {
                    downstream_buffer_read_estimate[data_class_id] = counts;
                }
                else if ((target_buffer == BufferType::Downstream) && (access_type == BufferAccessType::Write)) {
                    downstream_buffer_write_[data_class_id] = counts;
                }
            }

            void UpdateBufferSizeReq(BufferType target_buffer, long size_req, DataClass data_class) {

                int data_class_id = static_cast<int>(data_class);

                if(target_buffer == BufferType::Upstream) {
                    upstream_buffer_size_req_[data_class_id] = size_req;
                }
                else {
                    downstream_buffer_size_req_[data_class_id] = size_req;
                }
            }

            void UpdateDelay(DelayType delay_type, ValueType val_type, long delay) {
                if(delay_type == DelayType::Ingress) {
                    ingress_delay_[static_cast<int>(val_type)] = delay;
                }
                else if(delay_type == DelayType::Egress) {
                    egress_delay_[static_cast<int>(val_type)] = delay;
                }
                else {
                    compute_delay_[static_cast<int>(val_type)] = delay;
                }
            }

            long GetBufferSizeReq(BufferType target_buffer, DataClass data_class) {
                int data_class_id = static_cast<int>(data_class);

                if(target_buffer == BufferType::Upstream) {
                    return upstream_buffer_size_req_[data_class_id];
                }
                else {
                    return downstream_buffer_size_req_[data_class_id];
                }
            }

            long GetPeakBWReq() {
                return peak_bw_req_;
            }
            long GetOffchipBWReq() {
                return offchip_bw_req_;
            }

            double GetAvgBWReq() {
                return avg_bw_req_;
            }

            long GetNumSubClusters() {
                return num_sub_clusters_;
            }

            void UpdateRuntime(long new_runtime, CA::EstimationType estimation_type = CA::EstimationType::Exact) {
                runtime_[static_cast<int>(estimation_type)] = new_runtime;
            }

            void UpdateNumComputations(long num_computations) {
                num_computations_ = num_computations;
            }

            void UpdateTopNumComputations(long num_computations) {
                top_level_num_computations_ = num_computations;
            }

            void UpdatePeakBWReq(long bw_req) {
                peak_bw_req_ = bw_req;
            }
            void UpdateOffchipBWReq(long bw_req) {
                offchip_bw_req_ = bw_req;
            }

            void UpdateAvgBWReq(double avg_bw_req) {
                avg_bw_req_ = avg_bw_req;
            }

            void UpdateNumSubClusters(long num_clusters) {
                num_sub_clusters_ = num_clusters;
            }

            LayerType GetLayerType() {
                return layer_type_;
            }

            void SetNumSpatialOccurrences(long num_occ) {
                num_sp_occurrences_ = num_occ;
            }

            long GetNumSpatialOccurrences() {
                return num_sp_occurrences_;
            }

            void SetNumAvgActiveClusters(double active_clusters) {
                avg_num_active_unit_clusters_ = active_clusters;
            }

            double GetNumAvgActiveClusters() {
                return avg_num_active_unit_clusters_;
            }

            void SetArithmeticIntensity(double new_arin) {
                arithmetic_intensity_ = new_arin;
            }

            double GetArithmeticIntensity() {
                return arithmetic_intensity_;
            }

        protected:
            LayerType layer_type_;
            std::shared_ptr<std::vector<std::shared_ptr<DFA::IterationStatus>>> iter_status_info_;

            long num_sp_occurrences_ = 1;
            long cluster_level_ = 0;
            long num_sub_clusters_ = 0;
            double avg_num_active_unit_clusters_ = 0;
            double arithmetic_intensity_ = 0;

            long runtime_[static_cast<int>(CA::EstimationType::NumEstimationTypes)]= {0, };

            long upstream_buffer_write_estimate_[static_cast<int>(DataClass::NumDataClasses)] = {0};
            long upstream_buffer_read_[static_cast<int>(DataClass::NumDataClasses)] = {0};
            long downstream_buffer_write_[static_cast<int>(DataClass::NumDataClasses)] = {0};
            long downstream_buffer_read_estimate[static_cast<int>(DataClass::NumDataClasses)] = {0};

            long upstream_buffer_size_req_[static_cast<int>(DataClass::NumDataClasses)] = {0};
            long downstream_buffer_size_req_[static_cast<int>(DataClass::NumDataClasses)] = {0};

            long ingress_delay_[static_cast<int>(ValueType::NumValTypes)] = {0};
            long egress_delay_[static_cast<int>(ValueType::NumValTypes)] = {0};
            long compute_delay_[static_cast<int>(ValueType::NumValTypes)] = {0};

            long peak_bw_req_ = 0;
            double avg_bw_req_ = 0;
            long offchip_bw_req_ = 0;
            long num_computations_ = 0;
            long top_level_num_computations_ = 0;
        private:

        }; // End of class CostAnalysisResults
    } // End of namespace CA
} // End of namespace maestro

#endif
