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


#ifndef API_USER_INTERFACE_V2_HPP_
#define API_USER_INTERFACE_V2_HPP_

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "AHW_noc-model.hpp"
#include "option.hpp"

#include "BASE_maestro-class.hpp"
#include "BASE_base-objects.hpp"

#include "DFSL_parser.hpp"
#include "DFSL_hw-parser.hpp"

#include "DFA_tensor.hpp"

#include "CA_cost-analysis-engine.hpp"
#include "CA_cost-analysis-results.hpp"

#include "API_configuration.hpp"

#include "DSE_csv_writer.hpp"

namespace maestro {
    class APIV2 : public MAESTROClass {

    public:
        APIV2 (std::shared_ptr<ConfigurationV2> config):
                MAESTROClass("APIV2"),
                configuration_(config),
                num_macs_(0) {
            tensor_info_mapping_table_ = std::make_unique<std::map<LayerType, int>>();

            ParseDFSL();
            ParseHW();
            ConstructNoCs();
            AnalyzeClusters();
        }


        std::string GetNetworkName() {
            return configuration_->network_->GetName();
        }

        std::shared_ptr<std::vector<std::shared_ptr<std::vector<std::shared_ptr<CA::CostAnalysisResults>>>>>
        AnalyzeNeuralNetwork(
                bool print_results_to_screen = false,
                bool print_results_to_file = false,
                bool print_log_to_file = false) {
            auto ret = std::make_shared<std::vector<std::shared_ptr<std::vector<std::shared_ptr<CA::CostAnalysisResults>>>>>();

            int layer_id = 0;
            for(auto layer : *(configuration_->network_)) {
                auto layer_results = AnalyzeCostAllClusters(layer_id, print_results_to_screen, print_log_to_file);

                long num_macs = this->GetNumPartialSums(layer_id);
                layer_results->at(layer_results->size()-1)->UpdateTopNumComputations(num_macs);

                ret->push_back(layer_results); // Take the top level cluster results
                layer_id++;
            }

            long model_wise_total_l1_size = 0;
            long model_wise_total_l2_size = 0;
            long min_l1_size_req = 0;
            long min_l2_size_req = 0;

            if(print_results_to_screen) {
                for(auto& layer_res : *ret) {
                    auto upper_most_cluster_res = layer_res->at(layer_res->size()-1);
                    auto inner_most_cluster_res = layer_res->at(0);
                    PrintAnalysisResultsSingleCluster(upper_most_cluster_res, inner_most_cluster_res);
                    auto layer_wise_total_l2_size = (upper_most_cluster_res->GetBufferSizeReq(CA::BufferType::Upstream, DataClass::Input) +
                                                     upper_most_cluster_res->GetBufferSizeReq(CA::BufferType::Upstream, DataClass::Output) +
                                                     upper_most_cluster_res->GetBufferSizeReq(CA::BufferType::Upstream, DataClass::Weight));
                    auto layer_wise_total_l1_size = (inner_most_cluster_res->GetBufferSizeReq(CA::BufferType::Downstream, DataClass::Input) +
                                                     inner_most_cluster_res->GetBufferSizeReq(CA::BufferType::Downstream, DataClass::Output) +
                                                     inner_most_cluster_res->GetBufferSizeReq(CA::BufferType::Downstream, DataClass::Weight));
                    model_wise_total_l2_size += layer_wise_total_l2_size;
                    model_wise_total_l1_size += layer_wise_total_l1_size;
                    if(layer_wise_total_l1_size > min_l1_size_req) {
                        min_l1_size_req = layer_wise_total_l1_size;
                    }
                    if(layer_wise_total_l2_size > min_l2_size_req) {
                        min_l2_size_req = layer_wise_total_l2_size;
                    }
                }
                bool pass=true;
                std::cout << "Buffer Analysis:"<<std::endl;
                if(min_l1_size_req > configuration_->l1_size_){
                    std::cout << "[WARNING:Buffer] Per-layer L1 size requirement [" << min_l1_size_req << "] is larger than the given L1 size [" << configuration_->l1_size_ << "]"<< std::endl;
                    pass= false;
                }
                if(min_l2_size_req > configuration_->l2_size_){
                    std::cout << "[WARNING:Buffer] Per-layer L2 size requirement [" << min_l2_size_req << "] is larger than the given L2 size [" << configuration_->l2_size_ << "]"<< std::endl;
                    pass= false;
                }
                if(pass) {
                    std::cout << "[PASS]" << std::endl;
                }
                std::cout << "[Model-wise Buffer Summary]" << std::endl;
                std::cout << "Model-wise total L2 size usage: " << model_wise_total_l2_size << std::endl;
                std::cout << "Model-wise total L1 size usage: " << model_wise_total_l1_size << std::endl;
            }

            if(print_results_to_file) {
                OutputResults(ret);
            }

            return ret;
        }

        long GetTempIterOverInnermostCluster(int layer_id) {
            auto target_cluster_table = configuration_->cluster_analysis_->at(layer_id-1)->GetClusters();
            long ret = 1;
            for(int cluster_idx = 0; cluster_idx < target_cluster_table->size()-1; cluster_idx++) {
                ret *= target_cluster_table->GetCluster(cluster_idx)->GetNumTotalIterations();
            }
            return ret;
        }

        long GetTensorSize(int layer_num, DataClass data_class, int tensor_table_idx = 0) {
            for(auto& tensor : *configuration_->tensors_->at(tensor_table_idx)) {
                if(tensor->GetDataClass() == data_class) {
                    auto coupled_dims = tensor->GetCoupledVariables();
                    auto layer_dim = configuration_->network_->at(layer_num)->GetDimensions();
                    long ret = 1;

                    for(auto dim : *layer_dim) {
                        auto dim_name = dim->GetName();


                        for(auto tensor_dim : *coupled_dims) {
                            if(tensor_dim == dim_name) {
                                ret *= dim->GetSize();
                                break;
                            }
                        }
                    }

                    return ret;
                }
            }
            return 0;
        }


    protected:
        std::shared_ptr<ConfigurationV2> configuration_;
        std::unique_ptr<std::map<LayerType, int>> tensor_info_mapping_table_;
        long num_macs_;


    private:

        void ParseDFSL(){
            DFSL::DFSLParser dfsl_parser(configuration_->dfsl_file_name_);
            dfsl_parser.ParseDFSL(configuration_->network_);

            message_printer->PrintMsg(1, "Parsing finished");
            message_printer->PrintMsg(1, "Network name:" + configuration_->network_->GetName());

            for(auto& layer: *(configuration_->network_)) {
                message_printer->PrintMsg(1, layer->ToString());
            }
        }

        void ParseHW(){
            if(configuration_->hw_file_name_ != "") {
                DFSL::HWParser hw_parser(configuration_->hw_file_name_);
                auto ret = hw_parser.ParseHW();
                configuration_->num_pes_ = ret->num_pes_;
                configuration_->l1_size_ = ret->l1_size_;
                configuration_->l2_size_ = ret->l2_size_;
                configuration_->offchip_bw_= ret->off_chip_bw_;
                configuration_->noc_bw_->at(0) = ret->noc_bw_;
                configuration_->noc_bw_->at(1) = ret->noc_bw_;
                configuration_->noc_bw_->at(2) = ret->noc_bw_;
                configuration_->noc_bw_->at(3) = ret->noc_bw_;
                configuration_->noc_latency_->at(0) = ret->noc_hops_;
                configuration_->noc_latency_->at(1) = ret->noc_hops_;
                configuration_->noc_latency_->at(2) = ret->noc_hops_;
                configuration_->noc_latency_->at(3) = ret->noc_hops_;
            }
        }

        void Setup(std::shared_ptr<maestro::DFA::Layer> layer){
            configuration_->network_->SetName("Marvel-CONV");
            configuration_->network_->AddLayer(layer);

            message_printer->PrintMsg(1, "Adding layer is finished");
            message_printer->PrintMsg(1, "Network name:" + configuration_->network_->GetName());

            for(auto& layer: *(configuration_->network_)) {
                message_printer->PrintMsg(1, layer->ToString());
            }
        }

        long GetNumPartialSums(int layer_id) {
            std::shared_ptr<std::vector<std::shared_ptr<DFA::LayerDimension>>> full_dimension = configuration_->network_->at(layer_id)->GetDimensions();
            long ret = 1;
            bool need_conversion = false;
            for(auto dim : *full_dimension) {
                ret *= dim->GetSize();
            }
            return ret;
        }

        void ConfigConvOverlapDimensions(std::shared_ptr<DFA::DimensionTable> target_dim_table){
            std::shared_ptr<std::list<std::shared_ptr<std::pair<std::string, std::string>>>> overlap_dim_list = std::make_shared<std::list<std::shared_ptr<std::pair<std::string, std::string>>>>();
            auto output_column_overlap = std::make_shared<std::pair<std::string, std::string>>("X", "S");
            auto output_row_overlap = std::make_shared<std::pair<std::string, std::string>>("Y", "R");
            overlap_dim_list->push_back(output_column_overlap);
            overlap_dim_list->push_back(output_row_overlap);

            target_dim_table->AddOverlapDimensions(overlap_dim_list);
        }

        int ConfigConvTensors(LayerType layer_type, bool batch_processing = false){
            /* Construct the convolution problem */
            std::list<std::string> input_coupled_vars;
            std::list<std::string> weight_coupled_vars;
            std::list<std::string> output_coupled_vars;


            switch(layer_type) {
                case (LayerType::DSCONV): {
                    if(batch_processing) {
                        input_coupled_vars =  {"N", "C", "Y", "X"};
                        weight_coupled_vars = {"C", "R", "S"};
                        output_coupled_vars = {"N", "C", "Y", "X"};
                    }
                    else {
                        input_coupled_vars =  {"C", "Y", "X"};
                        weight_coupled_vars = {"C", "R", "S"};
                        output_coupled_vars = {"C", "Y", "X"};
                    }
                    break;
                }
                case (LayerType::NGCONV): {
                    if(batch_processing) {
                        input_coupled_vars =  {"N", "G", "C", "Y", "X"};
                        weight_coupled_vars = {"G", "K", "C", "R", "S"};
                        output_coupled_vars = {"N", "G", "K", "C", "Y", "X"};
                    }
                    else {
                        input_coupled_vars =  {"G", "C", "Y", "X"};
                        weight_coupled_vars = {"G", "K", "C", "R", "S"};
                        output_coupled_vars = {"G", "K", "C", "Y", "X"};
                    }
                    break;
                }
                case (LayerType::GEMM): {
                    input_coupled_vars =  {"M", "K"};
                    weight_coupled_vars = {"K", "N"};
                    output_coupled_vars = {"M", "N"};
                    break;
                }
                case (LayerType::CONV):
                default : {
                    if(batch_processing) {
                        input_coupled_vars =  {"N", "C", "Y", "X"};
                        weight_coupled_vars = {"K", "C", "R", "S"};
                        output_coupled_vars = {"N", "K", "Y", "X"};
                    }
                    else {
                        input_coupled_vars =  {"C", "Y", "X"};
                        weight_coupled_vars = {"K", "C", "R", "S"};
                        output_coupled_vars = {"K", "Y", "X"};
                    }
                }

            }


            auto conv_tensor_table = std::make_shared<DFA::TensorTable>();

            auto input_tensor = ConstructTensor("input", DFA::TensorClass::InputTensor, DataClass::Input, input_coupled_vars);
            auto filter_tensor = ConstructTensor("filter", DFA::TensorClass::InputTensor, DataClass::Weight, weight_coupled_vars);
            auto output_tensor = ConstructTensor("output", DFA::TensorClass::OutputTensor, DataClass::Output, output_coupled_vars);

            conv_tensor_table->AddTensor(input_tensor);
            conv_tensor_table->AddTensor(filter_tensor);
            conv_tensor_table->AddTensor(output_tensor);

            configuration_->tensors_->push_back(conv_tensor_table);

            return configuration_->tensors_->size()-1;
        }

        std::shared_ptr<DFA::Tensor> ConstructTensor(std::string tensor_name, DFA::TensorClass tensor_class, DataClass data_class,
                                                     std::list<std::string> coupled_var_list) {

            auto coupled_var_list_ptr = std::make_shared<std::list<std::string>>(coupled_var_list);
            auto ret = std::make_shared<DFA::Tensor>(tensor_name, tensor_class, data_class, coupled_var_list_ptr);
            return ret;
        }

        std::shared_ptr<DFA::DimensionTable> ConstructConvDimensionTable (
                std::shared_ptr<std::vector<std::shared_ptr<DFA::LayerDimension>>> dimensions,
                LayerType layer_type) {
            auto dimension_table = std::make_shared<DFA::DimensionTable>();

            switch(layer_type) {
                case (LayerType::CONV) :

                case (LayerType::DSCONV) :

                case (LayerType::NGCONV) : {
                    int IX_size, IY_size, R_size, S_size, OX_size, OY_size;
                    int x_inner_stride, y_inner_stride;
                    int x_outer_stride, y_outer_stride;
                    bool has_IY = false;
                    bool has_IX = false;
                    bool has_OY =false;
                    bool has_OX =false;

                    num_macs_ = 1;

                    for(auto dim : *dimensions) {
                        if(dim->GetName() == DFSL::layer_dim_input_height_) {
                            if(has_OY) {
                                error_handler_->PrintErrorMsg(TL::ErrorCode::DoubleDimDefinition, "");
                                error_handler_->TerminateProgram();
                            }
                            y_outer_stride = dim->GetOuterStride();
                            y_inner_stride = dim->GetInnerStride();
                            has_IY = true;
                            IY_size = dim->GetSize();
                        }
                        else if(dim->GetName() == DFSL::layer_dim_input_width_) {
                            if(has_OX) {
                                error_handler_->PrintErrorMsg(TL::ErrorCode::DoubleDimDefinition, "");
                                error_handler_->TerminateProgram();
                            }
                            x_outer_stride = dim->GetOuterStride();
                            x_inner_stride = dim->GetInnerStride();
                            has_IX = true;
                            IX_size = dim->GetSize();
                        }
                        else if(dim->GetName() == DFSL::layer_dim_output_width_){
                            if(has_IX) {
                                error_handler_->PrintErrorMsg(TL::ErrorCode::DoubleDimDefinition, "");
                                error_handler_->TerminateProgram();
                            }
                            y_outer_stride = dim->GetOuterStride();
                            y_inner_stride = dim->GetInnerStride();
                            has_OX = true;
                            OX_size = dim->GetSize();
                        }
                        else if(dim->GetName() == DFSL::layer_dim_output_height_){
                            if(has_IY) {
                                error_handler_->PrintErrorMsg(TL::ErrorCode::DoubleDimDefinition, "");
                                error_handler_->TerminateProgram();
                            }
                            x_outer_stride = dim->GetOuterStride();
                            x_inner_stride = dim->GetInnerStride();
                            has_OY = true;
                            OY_size = dim->GetSize();
                        }
                        else if(dim->GetName() == DFSL::layer_dim_weight_height_){
                            R_size = dim->GetSize();
                        }
                        else if(dim->GetName() == DFSL::layer_dim_weight_width_){
                            S_size = dim->GetSize();
                        }
                        num_macs_ *= dim->GetSize();
                        dimension_table->AddDimension(dim);
                    }

                    if(has_IX && !has_OX) {
                        auto ox_dim = std::make_shared<DFA::LayerDimension>(DFSL::layer_dim_output_width_, IX_size - S_size +1, x_outer_stride, x_inner_stride);
                        dimension_table->AddDimension(ox_dim);
                    }
                    if(has_IY && !has_OY) {
                        auto oy_dim = std::make_shared<DFA::LayerDimension>(DFSL::layer_dim_output_height_, IY_size - R_size +1, y_outer_stride, y_inner_stride);
                        dimension_table->AddDimension(oy_dim);
                    }
                    if(!has_IX && has_OX) {
                        auto ix_dim = std::make_shared<DFA::LayerDimension>(DFSL::layer_dim_input_width_, OX_size + S_size -1, x_outer_stride, x_inner_stride);
                        dimension_table->AddDimension(ix_dim);
                    }
                    if(!has_IY && has_OY) {
                        auto iy_dim = std::make_shared<DFA::LayerDimension>(DFSL::layer_dim_input_height_, OY_size + R_size -1, y_outer_stride, y_inner_stride);
                        dimension_table->AddDimension(iy_dim);
                    }
                    break;
                }
                case (LayerType::GEMM) : {
                    for(auto dim : *dimensions) {
                        auto dimtbl_entry = std::make_shared<DFA::LayerDimension>(dim->GetName(), dim->GetSize(), dim->GetOuterStride(), dim->GetInnerStride());
                        dimension_table->AddDimension(dimtbl_entry);
                    }
                    break;
                }
                default: {}
            }

            message_printer_->PrintMsg(1, "Dimensions");

            for(auto dim : *dimension_table) {
                std::string print_msg = "Dim " + dim->GetName() + ", size = " + std::to_string(dim->GetSize());
                message_printer_->PrintMsg(1, print_msg);
            }

            return dimension_table;
        } // End of function ConstructConvDimensionTable


        //From top cluster (global buffer side) to lower cluster (PE side)
        void ConstructNoCs() {
            int noc_levels = configuration_->noc_bw_->size();
            assert(noc_levels == configuration_->noc_latency_->size());
            assert(noc_levels == configuration_->noc_multcast_->size());

            for(int noc_lv = 0; noc_lv < noc_levels; noc_lv++) {
                auto noc = std::make_shared<maestro::AHW::NetworkOnChipModel>(
                        configuration_->noc_bw_->at(noc_lv),
                        1,
                        configuration_->noc_latency_->at(noc_lv),
                        configuration_->noc_multcast_->at(noc_lv));

                configuration_->nocs_->push_back(noc);
                message_printer_->PrintMsg(2, "[APIV2.ConstructNoCs] Added a NoC at cluster level " + std::to_string(configuration_->nocs_->size()) );
            }
        } // End of function void ConstructNoCs()

        void AnalyzeClusters() {
            int layer_id = -1;
            for(auto layer: *(configuration_->network_)) {
                layer_id++;
                auto dataflow = layer->GetDataflow();
                auto dimensions = layer->GetDimensions();
                auto layer_type = layer->GetLayerType();
                int tensor_info_idx = 0;

                std::shared_ptr<DFA::DimensionTable> dimension_table = ConstructConvDimensionTable(dimensions, layer_type);

                switch(layer_type) {
                    case (LayerType::CONV) :

                    case (LayerType::DSCONV) :

                    case (LayerType::NGCONV) : {
                        ConfigConvOverlapDimensions(dimension_table);

                        bool has_batch = false;

                        for(auto dim : *dimensions) {
                            if(dim->GetName() == DFSL::layer_dim_input_batch_) {
                                has_batch = true;
                                break;
                            }
                        }
                        if(tensor_info_mapping_table_->find(layer_type) == tensor_info_mapping_table_->end()){
                            tensor_info_idx = ConfigConvTensors(layer_type, has_batch);
                            (*tensor_info_mapping_table_)[layer_type] = tensor_info_idx;
                        }
                        else {
                            tensor_info_idx = (*tensor_info_mapping_table_)[layer_type];
                        }
                        break;
                    }
                    case (LayerType::GEMM):{
                        if(tensor_info_mapping_table_->find(layer_type) == tensor_info_mapping_table_->end()){
                            tensor_info_idx = ConfigConvTensors(layer_type, false);
                            (*tensor_info_mapping_table_)[layer_type] = tensor_info_idx;
                        }
                        else{
                            tensor_info_idx = (*tensor_info_mapping_table_)[layer_type];
                        }
                        break;
                    }
                    default: {
                        //TODO: Add generic/custom dimension table construction
                        //              dimension_table->AddOverlapDimensions(overlap_dim_list);
                        error_handler_->PrintErrorMsg(TL::ErrorCode::NotSupportedLayerType,"", this->GetName());
                    }
                }

                std::string print_msg_0 = "Layer " + layer->GetName();
                std::string print_msg_1 = "<Dataflow>\n" + dataflow->ToString();

                message_printer_->PrintMsg(1, print_msg_0);
                message_printer_->PrintMsg(1, print_msg_1);

                auto cluster_analysis = std::make_shared<DFA::ClusterAnalysis>(
                        layer_type, configuration_->num_pes_, configuration_->tensors_->at(tensor_info_idx),
                        dimension_table, dataflow, configuration_->nocs_);

                configuration_->cluster_analysis_->push_back(cluster_analysis);
            }

            message_printer_->PrintMsg(1, "Cluster construction and analysis is done");
        }

        std::shared_ptr<std::vector<std::shared_ptr<CA::CostAnalysisResults>>> AnalyzeCostAllClusters(int layer_id, bool print_results = false, bool write_log_file = false) {
            auto target_cluster_analysis = configuration_->cluster_analysis_->at(layer_id);
            auto clusters = target_cluster_analysis->GetClusters();
            auto layer_type = clusters->GetLayerType();
            int tensor_info_idx = (*tensor_info_mapping_table_)[layer_type];

            auto perf_analysis = std::make_unique<CA::CostAnalysisEngine>
                    (configuration_, configuration_->tensors_->at(tensor_info_idx), clusters);

            auto results = perf_analysis->AnalyzeEntireCluster(write_log_file);
            return results;
        }

        std::string ConstructOutputFileName() {
            std::string output_file_name = configuration_->dfsl_file_name_;
            output_file_name = output_file_name.substr(output_file_name.find("/") +1);
            output_file_name = output_file_name.substr(output_file_name.find("/") +1);

            int pos_dot = output_file_name.find(".");

            output_file_name = output_file_name.substr(0, pos_dot);
            output_file_name = output_file_name  + ".csv";

            return output_file_name;
        }

        void OutputResults(std::shared_ptr<std::vector<std::shared_ptr<std::vector<std::shared_ptr<CA::CostAnalysisResults>>>>> analysis_result) {
            auto csv_writer = std::make_shared<maestro::DSE::CSVWriter>(configuration_, ConstructOutputFileName());

            long total_runtime = 0;

            long l2_rd_input_count = 0;
            long l2_rd_weight_count = 0;
            long l2_wr_input_count = 0;
            long l2_wr_weight_count = 0;
            long l2_wr_output_count = 0;

            long l2_to_l1_rd_input_count = 0;
            long l2_to_l1_rd_weight_count = 0;

            long l2_to_l1_wr_input_count = 0;
            long l2_to_l1_wr_weight_count = 0;

            long l1_to_l0_rd_input_count = 0;
            long l1_to_l0_rd_weight_count = 0;
            long l1_to_l0_rd_output_count = 0;

            long l1_rd_input_count = 0;
            long l1_rd_weight_count = 0;
            long l1_rd_output_count = 0;

            long l1_wr_input_count = 0;
            long l1_wr_weight_count = 0;
            long l1_wr_output_count = 0;

            int num_top_clusters = 0;
            int num_bottom_clusters = 0;

            int layer_id = 1;
            long max_noc_bw_req = 0;
            long max_offchip_bw_req = 0;
            for(auto& layer_res : *analysis_result) {
                LayerType layer_type;
                int cluster_lv = 0;
                long layer_runtime = 0;
                double layer_energy = 0;
                long double layer_perf_per_energy;
                double area;
                double power;
                int num_pes = configuration_->num_pes_;
                int noc_bw;
                int vector_width;
                int l2_size = 0;
                int l1_size = 0;
                long num_psums = 0;
                long num_macs_top = 0;

                long num_iters = GetTempIterOverInnermostCluster(layer_id);
                std::shared_ptr<maestro::CA::CostAnalysisResults> top_res;
                std::string layer_name = configuration_->network_->at(layer_id - 1)->GetName();

                for (auto &cluster_res: *layer_res) {
                    layer_type = cluster_res->GetLayerType();
                    top_res = cluster_res;

                    if (cluster_lv == layer_res->size() - 1) {
                        total_runtime += cluster_res->GetRuntime();
                        l2_rd_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Upstream,
                                                                              maestro::CA::BufferAccessType::Read,
                                                                              maestro::DataClass::Input);
                        l2_rd_weight_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Upstream,
                                                                               maestro::CA::BufferAccessType::Read,
                                                                               maestro::DataClass::Weight);
                        l2_wr_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Upstream,
                                                                              maestro::CA::BufferAccessType::Write,
                                                                              maestro::DataClass::Input);
                        l2_wr_weight_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Upstream,
                                                                               maestro::CA::BufferAccessType::Write,
                                                                               maestro::DataClass::Weight);
                        l2_wr_output_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Upstream,
                                                                               maestro::CA::BufferAccessType::Write,
                                                                               maestro::DataClass::Output);

                        long num_sub_clusters = cluster_res->GetNumSubClusters();
                        num_top_clusters = num_sub_clusters;
                        /*
                        l2_to_l1_rd_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Read, maestro::DataClass::Input) * num_sub_clusters - l2_rd_input_count;
                        l2_to_l1_rd_weight_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Read, maestro::DataClass::Weight) * num_sub_clusters - l2_rd_weight_count;
                        */
                        l2_to_l1_wr_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                                    maestro::CA::BufferAccessType::Write,
                                                                                    maestro::DataClass::Input);
                        l2_to_l1_wr_weight_count = cluster_res->GetBufferAccessCount(
                                maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Write,
                                maestro::DataClass::Weight);

                        layer_runtime = cluster_res->GetRuntime();
                        layer_energy += l2_rd_input_count * maestro::l2_energy_multiplier;
                        layer_energy += l2_wr_input_count * maestro::l2_energy_multiplier;
                        layer_energy += l2_rd_weight_count * maestro::l2_energy_multiplier;
                        layer_energy += l2_wr_weight_count * maestro::l2_energy_multiplier;
                        layer_energy += l2_wr_output_count * maestro::l2_energy_multiplier;

                        l2_size += cluster_res->GetBufferSizeReq(maestro::CA::BufferType::Upstream,
                                                                 maestro::DataClass::Input);
                        l2_size += cluster_res->GetBufferSizeReq(maestro::CA::BufferType::Upstream,
                                                                 maestro::DataClass::Output);
                        l2_size += cluster_res->GetBufferSizeReq(maestro::CA::BufferType::Upstream,
                                                                 maestro::DataClass::Weight);
                        num_psums = cluster_res->GetNumComputations();

                        num_macs_top = cluster_res->GetTopNumComputations();

                        l1_rd_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                              maestro::CA::BufferAccessType::Read,
                                                                              maestro::DataClass::Input);
                        l1_rd_weight_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                               maestro::CA::BufferAccessType::Read,
                                                                               maestro::DataClass::Weight);
                        l1_rd_output_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                               maestro::CA::BufferAccessType::Read,
                                                                               maestro::DataClass::Output);
                        l1_wr_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                              maestro::CA::BufferAccessType::Write,
                                                                              maestro::DataClass::Input);
                        l1_wr_weight_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                               maestro::CA::BufferAccessType::Write,
                                                                               maestro::DataClass::Weight);
                        l1_wr_output_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                               maestro::CA::BufferAccessType::Write,
                                                                               maestro::DataClass::Output);
                    }
                    if (cluster_lv == 0) {
                        /*
                        l1_rd_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Read, maestro::DataClass::Input);
                        l1_rd_weight_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Read, maestro::DataClass::Weight);
                        l1_rd_output_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Read, maestro::DataClass::Output);
                        l1_wr_input_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Write, maestro::DataClass::Input);
                        l1_wr_weight_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Write, maestro::DataClass::Weight);
                        l1_wr_output_count = cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream, maestro::CA::BufferAccessType::Write, maestro::DataClass::Output);
                        */

                        long num_sub_clusters = cluster_res->GetNumSubClusters();
                        long num_cur_clusters = num_pes / num_sub_clusters;
                        num_bottom_clusters = num_cur_clusters;

                        l1_to_l0_rd_input_count =
                                (cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                   maestro::CA::BufferAccessType::Read,
                                                                   maestro::DataClass::Input) - l1_wr_input_count) *
                                num_pes * num_iters;
                        l1_to_l0_rd_weight_count =
                                (cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                   maestro::CA::BufferAccessType::Read,
                                                                   maestro::DataClass::Weight) - l1_wr_weight_count) *
                                num_pes * num_iters;
                        l1_to_l0_rd_output_count =
                                (cluster_res->GetBufferAccessCount(maestro::CA::BufferType::Downstream,
                                                                   maestro::CA::BufferAccessType::Read,
                                                                   maestro::DataClass::Output) - l1_wr_output_count) *
                                num_pes * num_iters;

                        layer_energy += l1_rd_input_count * maestro::l1_energy_multiplier;
                        layer_energy += l1_wr_input_count * maestro::l1_energy_multiplier;
                        layer_energy += l1_rd_weight_count * maestro::l1_energy_multiplier;
                        layer_energy += l1_wr_weight_count * maestro::l1_energy_multiplier;
                        layer_energy += l1_rd_output_count * maestro::l1_energy_multiplier;
                        layer_energy += l1_wr_output_count * maestro::l1_energy_multiplier;

                        l1_size += cluster_res->GetBufferSizeReq(maestro::CA::BufferType::Downstream,
                                                                 maestro::DataClass::Input);
                        l1_size += cluster_res->GetBufferSizeReq(maestro::CA::BufferType::Downstream,
                                                                 maestro::DataClass::Output);
                        l1_size += cluster_res->GetBufferSizeReq(maestro::CA::BufferType::Downstream,
                                                                 maestro::DataClass::Weight);
                    }
                    cluster_lv++;
                }

                layer_energy *= maestro::DSE::cost::mac_energy;

                layer_perf_per_energy = static_cast<long double>(num_psums) / static_cast<long double>(layer_runtime) /
                                        static_cast<long double>(layer_energy);

                layer_perf_per_energy *= 1000000000; //nW -> W

                num_pes = configuration_->num_pes_;
                noc_bw = configuration_->noc_bw_->at(0);
                vector_width = configuration_->simd_width_;

                int tensor_info_idx = (*tensor_info_mapping_table_)[layer_type];

                long input_tensor_size = GetTensorSize(layer_id - 1, maestro::DataClass::Input, tensor_info_idx);
                long weight_tensor_size = GetTensorSize(layer_id - 1, maestro::DataClass::Weight, tensor_info_idx);

                std::shared_ptr<maestro::DSE::Accelerator> accelerator = std::make_shared<maestro::DSE::Accelerator>(
                        num_pes, vector_width, noc_bw, l1_size, l2_size);
                area = accelerator->GetArea();
                power = accelerator->GetPower();

                long double ops_per_joule = num_psums / layer_energy * 1000000000; //nJ -> J


                auto layer_dp = std::make_shared<maestro::DSE::DesignPoint>(
                        maestro::DSE::OptimizationTarget::Runtime, layer_runtime, layer_energy,
                        layer_perf_per_energy, area, power, num_pes, noc_bw, vector_width, l2_size, l1_size);


                int num_active_pes = configuration_->cluster_analysis_->at(layer_id - 1)->GetNumActivePEs();
                int num_innermost_unit_clusters = configuration_->cluster_analysis_->at(
                        layer_id - 1)->GetInnermostClusterSize();

                int l1_mult = num_innermost_unit_clusters;
                assert(l1_mult != 0);

                layer_dp->PutMulticastingFactor("input",
                                                static_cast<double>(l2_to_l1_wr_input_count) / l2_rd_input_count);
                layer_dp->PutMulticastingFactor("weight",
                                                static_cast<double>(l2_to_l1_wr_weight_count) / l2_rd_weight_count);

                double pe_power = accelerator->GetPEPower();
                double l1_power = accelerator->GetL1Power();
                double l2_power = accelerator->GetL2Power();
                double noc_power = accelerator->GetNoCPower();
                /*
                ingress_delay_[static_cast<int>(CA::ValueType::Min)] = cluster_res->GetDelay(CA::DelayType::Ingress, CA::ValueType::Min);
                ingress_delay_[static_cast<int>(CA::ValueType::Max)] = cluster_res->GetDelay(CA::DelayType::Ingress, CA::ValueType::Max);

                egress_delay_[static_cast<int>(CA::ValueType::Min)] = cluster_res->GetDelay(CA::DelayType::Egress, CA::ValueType::Min);
                egress_delay_[static_cast<int>(CA::ValueType::Max)] = cluster_res->GetDelay(CA::DelayType::Egress, CA::ValueType::Max);

                compute_delay_[static_cast<int>(CA::ValueType::Min)] = cluster_res->GetDelay(CA::DelayType::Computation, CA::ValueType::Min);
                compute_delay_[static_cast<int>(CA::ValueType::Max)] = cluster_res->GetDelay(CA::DelayType::Computation, CA::ValueType::Max);
                */

                csv_writer->WriteDesignPoint(configuration_, tensor_info_idx, layer_dp, GetNetworkName(), layer_name,
                                             num_psums, input_tensor_size, weight_tensor_size, ops_per_joule, pe_power,
                                             l1_power, l2_power, noc_power, top_res);

                if (top_res->GetPeakBWReq() > max_noc_bw_req) {
                    max_noc_bw_req = top_res->GetPeakBWReq();
                }
                if (top_res->GetOffchipBWReq() > max_offchip_bw_req) {
                    max_offchip_bw_req = top_res->GetOffchipBWReq();
                }
                layer_id++;

            }

            //felix
            bool pass=true;
            std::cout << "BW Analysis:"<<std::endl;
            if( max_noc_bw_req > configuration_->noc_bw_->at(0)){
                std::cout << "[WARNING:BW] Per-layer NoC BW requirement [" << max_noc_bw_req << "] is larger than the given NoC BW [" <<  configuration_->noc_bw_->at(0)<< "]"<< std::endl;
                pass=false;
            }
            if( max_offchip_bw_req > configuration_->offchip_bw_){
                std::cout << "[WARNING:BW] Per-layer OffChip BW requirement [" << max_offchip_bw_req << "] is larger than the given OffChip BW [" << configuration_->offchip_bw_ << "]"<< std::endl;
                pass=false;
            }
            if(pass==true){
                std::cout << "[PASS]"<<std::endl;
            }
            //

        }



        void PrintAnalysisResultsSingleCluster(std::shared_ptr<CA::CostAnalysisResults> results, std::shared_ptr<CA::CostAnalysisResults> inner_results) {
            std::cout << std::endl;
            std::cout << std::endl;

            long num_computations = results->GetNumComputations();
            long num_abs_computations = results->GetTopNumComputations();

            long double throughput = static_cast<double>(num_computations) / results->GetRuntime(CA::EstimationType::Exact);
            long double throughput_min = static_cast<double>(num_computations) / results->GetRuntime(CA::EstimationType::Max);
            long double throughput_max = static_cast<double>(num_computations) / results->GetRuntime(CA::EstimationType::Min);

            long double abs_throughput = static_cast<double>(num_abs_computations) / results->GetRuntime(CA::EstimationType::Exact);
            long double abs_throughput_min = static_cast<double>(num_abs_computations) / results->GetRuntime(CA::EstimationType::Max);
            long double abs_throughput_max = static_cast<double>(num_abs_computations) / results->GetRuntime(CA::EstimationType::Min);


            std::cout << "Num MACs: " << num_computations << std::endl;


            std::cout << std::endl;
            std::cout << "[Performance Analysis]" << std::endl;
            std::cout << "Runtime: " << results->GetRuntime(CA::EstimationType::Exact) << " cycles" << std::endl;

            std::cout << "Throughput: " << throughput << " MACs/cycle" << std::endl;


            std::cout << "[Buffer Access Analysis]" << std::endl;


            int num_data_classes = static_cast<int>(DataClass::NumDataClasses);
            long num_l1_read[num_data_classes];

            long total_l1_write = 0;
            long total_l1_read = 0;

            //felix
            long total_l1_size = 0;
            long total_l2_size = 0;
            //==========

            auto layer_type = results->GetLayerType();
            int tensor_info_idx = (*tensor_info_mapping_table_)[layer_type];

            for(auto tensor : *(configuration_->tensors_->at(tensor_info_idx))) {
                auto dataclass = tensor->GetDataClass();

                std::cout << "Tensor " << tensor->GetTensorName() << std::endl;
                std::cout << "L2 size requirement: " << results->GetBufferSizeReq(CA::BufferType::Upstream, dataclass) << std::endl;
                std::cout << "L1 size requirement: " << inner_results->GetBufferSizeReq(CA::BufferType::Downstream, dataclass) << std::endl;

                std::cout << "L2 buffer write: "
                          << results->GetBufferAccessCount(CA::BufferType::Upstream, CA::BufferAccessType::Write, dataclass) << std::endl;
                std::cout << "L2 buffer read: "
                          << results->GetBufferAccessCount(CA::BufferType::Upstream, CA::BufferAccessType::Read, dataclass) << std::endl;
                std::cout << "L1 buffer write: "
                          << results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Write, dataclass) << std::endl;
                std::cout << "L1 buffer read: "
                          << results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Read, dataclass) << std::endl;
                std::cout << "Data reuse factor: "
                          << static_cast<double>(results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Read, dataclass))
                             / results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Write, dataclass) << std::endl;

                total_l1_write += results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Write, dataclass);
                total_l1_read += results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Read, dataclass);
                //felix
                total_l2_size += results->GetBufferSizeReq(CA::BufferType::Upstream, dataclass);
                total_l1_size += inner_results->GetBufferSizeReq(CA::BufferType::Downstream, dataclass);
                //====
            }


            std::cout << "Overall data reuse factor: " << static_cast<double>(total_l1_read) / static_cast<double>(total_l1_write);

            std::cout << std::endl;
            std::cout << "[Energy Analysis]" << std::endl;

            long double l2_write_energy = 0;
            long double l2_read_energy = 0;
            long double l1_write_energy = 0;
            long double l1_read_energy = 0;
            long double total_energy = 0;

            std::cout << "-For each data class" << std::endl;
            long double tmp;
            for(auto tensor : *(configuration_->tensors_->at(tensor_info_idx))) {
                auto dataclass = tensor->GetDataClass();
                std::cout << "Tensor " << tensor->GetTensorName() << std::endl;

                tmp = results->GetBufferAccessCount(CA::BufferType::Upstream, CA::BufferAccessType::Write, dataclass) * l2_energy_multiplier;
                std::cout << "L2 buffer write energy: " << tmp << " X MAC energy" << std::endl;
                l2_write_energy += tmp;

                tmp = results->GetBufferAccessCount(CA::BufferType::Upstream, CA::BufferAccessType::Read, dataclass)  * l2_energy_multiplier;
                std::cout << "L2 buffer read energy: "  << tmp << " X MAC energy" << std::endl;
                l2_read_energy += tmp;

                tmp = results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Write, dataclass) * l1_energy_multiplier;
                std::cout << "L1 buffer write energy: " << tmp << " X MAC energy" << std::endl;
                l1_write_energy += tmp;

                tmp = results->GetBufferAccessCount(CA::BufferType::Downstream, CA::BufferAccessType::Read, dataclass) * l1_energy_multiplier;
                std::cout << "L1 buffer read energy: " << tmp << " X MAC energy" << std::endl;
                l1_read_energy += tmp;
            }

            std::cout << std::endl;

            std::cout << "[Summary]" << std::endl;
            //felix
            std::cout << "Total L2 buffer requirement: " << total_l2_size << std::endl;
            std::cout << "Total L1 buffer requirement: " << total_l1_size << std::endl;
            //====
            std::cout << "Total L2 buffer write energy: " << l2_write_energy << " X MAC energy" << std::endl;
            std::cout << "Total L2 buffer read energy: " << l2_read_energy << " X MAC energy" << std::endl;
            std::cout << "Total L1 buffer write energy: " << l1_write_energy << " X MAC energy" << std::endl;
            std::cout << "Total L1 buffer read energy: " << l1_read_energy << " X MAC energy" << std::endl;
            std::cout << "Total MAC energy: " << num_computations << " X MAC energy" << std::endl;

            std::cout <<"Peak bandwidth requirement: " <<  results->GetPeakBWReq() << std::endl;
            std::cout <<"Avg bandwidth requirement: " <<  results->GetAvgBWReq() << std::endl;

            std::cout << std::endl;
            total_energy = l2_write_energy + l2_read_energy + l1_write_energy + l1_read_energy + num_computations;
            std::cout << "Total energy consumption: " << total_energy << " X MAC energy" << std::endl;

            std::cout << "Runtime: " << results->GetRuntime() << " cycles" << std::endl;
            std::cout << "Throughput: " << throughput << " MACs/cycle" << std::endl;
            long double performance_per_enrgy = throughput / total_energy;
            std::cout << "Performance per MAC energy: " << performance_per_enrgy << " MACs/cycle/(MAC_energy)" << std::endl;

            std::cout << "Ingress Delay" << std::endl;
            std::cout << "Min: " << results->GetDelay(CA::DelayType::Ingress, CA::ValueType::Min) << std::endl;
            std::cout << "Max: " << results->GetDelay(CA::DelayType::Ingress, CA::ValueType::Max) << std::endl;
            std::cout << "Avg: " << results->GetDelay(CA::DelayType::Ingress, CA::ValueType::Avg) << std::endl;

            std::cout << "Egress Delay" << std::endl;
            std::cout << "Min: " << results->GetDelay(CA::DelayType::Egress, CA::ValueType::Min) << std::endl;
            std::cout << "Max: " << results->GetDelay(CA::DelayType::Egress, CA::ValueType::Max) << std::endl;
            std::cout << "Avg: " << results->GetDelay(CA::DelayType::Egress, CA::ValueType::Avg) << std::endl;

            std::cout << "Computation Delay" << std::endl;
            std::cout << "Min: " << results->GetDelay(CA::DelayType::Computation, CA::ValueType::Min) << std::endl;
            std::cout << "Max: " << results->GetDelay(CA::DelayType::Computation, CA::ValueType::Max) << std::endl;
            std::cout << "Avg: " << results->GetDelay(CA::DelayType::Computation, CA::ValueType::Avg) << std::endl;

            std::cout << "Average number of utilized PEs: " << results->GetNumAvgActiveClusters() << std::endl;
            std::cout << "Arithmetic intensity: " << results->GetArithmeticIntensity() << std::endl;

        }
    }; // End of class API
}; // End of namespace maestro
#endif
