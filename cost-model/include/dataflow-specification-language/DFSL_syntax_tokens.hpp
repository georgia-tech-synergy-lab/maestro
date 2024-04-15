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


#ifndef MAESTRO_DSL_SYNTAX_TOKENS_HPP_
#define MAESTRO_DSL_SYNTAX_TOKENS_HPP_

#include <string>

namespace maestro {


    namespace DFSL {

        /* Common tokens */
        const std::string brace_open_ = "{";
        const std::string brace_close_ = "}";
        const std::string bracket_open_ = "[";
        const std::string bracket_close_ = "]";

        const std::string separator_ = ";";
        const std::string comments_ = "//";

        /* Constant definition */
        const std::string constant_decl_ = "Constant";

        /* Neural network and layer Description */
        const std::string network_decl_ = "Network";
        const std::string layer_decl_ = "Layer";
        const std::string layer_type_decl_ = "Type";
        const std::string layer_type_conv_ = "CONV";
        const std::string layer_type_residual_ = "RESIDUAL_IDENTITY";
        const std::string layer_type_fc_ = "FC";
        const std::string layer_type_pool_ = "POOL";
        const std::string layer_type_dsconv_ = "DSCONV";
        const std::string layer_type_trconv_ = "TRCONV";
        const std::string layer_type_ngconv_ = "NGCONV"; // Nested grouped convolution (ResNeXt)
        const std::string layer_type_lstm_ = "LSTM";
        const std::string layer_type_gemm_ = "GEMM";
        const std::string layer_stride_decl_ = "Stride";

        const std::string layer_precision_decl_ = "Precision";
        const std::string layer_quant_fp32 = "FP32";
        const std::string layer_quant_fp16 = "FP16";
        const std::string layer_quant_fp8 = "FP8";
        const std::string layer_quant_int32 = "INT32";
        const std::string layer_quant_int16 = "INT16";
        const std::string layer_quant_int8 = "INT8";


        const std::string layer_dim_decl_ = "Dimensions";
        const std::string layer_dim_input_batch_    = "N";
        const std::string layer_dim_group_          = "G";
        const std::string layer_dim_output_channel_ = "K";
        const std::string layer_dim_input_channel_  = "C";
        const std::string layer_dim_weight_height_  = "R";
        const std::string layer_dim_weight_width_   = "S";
        const std::string layer_dim_input_height_   = "Y";
        const std::string layer_dim_input_width_    = "X";
        const std::string layer_dim_output_height_  = "Y'";
        const std::string layer_dim_output_width_   = "X'";


        const std::string layer_expansion_factor_decl_ = "ExpansionSize";
        const std::string layer_pooling_size_decl_ = "PoolSize";
        const std::string layer_stride_size_decl_ = "Stride";

        const std::string layer_dataflow_decl_ = "Dataflow";
        const std::string dataflow_temporal_map_ = "TemporalMap";
        const std::string dataflow_spatial_map_ = "SpatialMap";
        const std::string dataflow_cluster_ = "Cluster";

        const std::string dataflow_dim_size_indicatior_ = "Sz";

        const std::string dataflow_cluster_type_logical_ = "L";
        const std::string dataflow_cluster_type_physical_ = "P";

        /* Tmp HW parser */
        const std::string tmp_num_pes_decl_ = "num_pes";
        const std::string tmp_l1size_decl_ = "l1_size_cstr";
        const std::string tmp_l2size_decl_ = "l2_size_cstr";
        const std::string tmp_noc_bw_decl_ = "noc_bw_cstr";
        const std::string tmp_noc_hops_decl_ = "NoC_NumHops";
        //felix
        const std::string tmp_offchip_bw_decl_ = "offchip_bw_cstr";
        //====

        /* Hardware Resource Description */
        const std::string accelerator_decl_ = "Accelerator";
        const std::string compute_cluster_decl_ = "ComputeUnitCluster";
        const std::string compute_cluster_level_decl_ = "Level";
        const std::string compute_cluster_counts_ = "NumInstances";
        const std::string compute_cluster_size_decl_ = "NumSubClusters";

        // NoC Description
        const std::string noc_decl_ = "DistributionNoC";
        const std::string noc_bandwidth_decl_ = "Bandwidth";
        const std::string noc_latency_per_hop_decl_ = "LatencyPerHop";
        const std::string noc_tensor_decl_ = "Tensor";
        const std::string noc_avg_num_hops_decl_ = "AvgNumHops";
        const std::string noc_type_decl_ = "Type";
        const std::string dist_noc_type_bus_ = "Bus";
        const std::string dist_noc_type_tree_ = "Tree";
        const std::string dist_noc_type_crossbar_ = "Crossbar";
        const std::string dist_noc_type_systolic_ = "Systolic";
        const std::string dist_noc_type_mesh_ = "Mesh";

        const std::string redc_noc_decl_ = "ReductionNoC";
        const std::string redc_noc_type_adder_tree_ = "AdderTree";
        const std::string redc_noc_type_systolic_ = "Systolic";


        // PE Description
        const std::string pe_decl_ = "PE";
        const std::string num_pe_decl_ = "NumPEs";
        const std::string vector_width_decl_ = "VectorWidth";

        const std::string mult_precision_decl_ = "MultPrecision";
        const std::string add_precision_decl_ = "AddPrecision";
        const std::string bfloat_precision_type = "BFLOAT";
        const std::string float_precision_type = "FLOAT";
        const std::string fixed_precision_type = "INT";

        const std::string fp4_precision = "FP4";
        const std::string fp8_precision = "FP8";
        const std::string fp16_precision = "FP16";
        const std::string fp32_precision = "FP32";

        const std::string int4_precision = "INT4";
        const std::string int8_precision = "INT8";
        const std::string int16_precision = "INT16";
        const std::string int32_precision = "INT32";


        // Buffer Description
        const std::string buffer_decl_ = "Buffer";
        const std::string buffer_tensor_decl_ = "Tensor";
        const std::string buffer_size_decl_ = "Size";
        const std::string buffer_type_decl_ = "Type";
        const std::string buffer_type_fifo_ = "FIFO";
        const std::string buffer_type_circular_fifo_ = "CFIFO";
        const std::string buffer_type_scratchpad_ = "ScratchPad";
        const std::string buffer_type_buffet_ = "Buffet";

    };

}
;
// End of name space maestro

#endif
