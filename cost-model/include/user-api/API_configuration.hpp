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


#ifndef API_CONFIGURATION_HPP_
#define API_CONFIGURATION_HPP_

#include <memory>
#include <vector>
#include "BASE_base-objects.hpp"

#include "DFA_tensor.hpp"
#include "DFA_neural-network.hpp"
#include "DFA_cluster-analysis.hpp"

#include "DSE_hardware_modules.hpp"

#include "AHW_noc-model.hpp"



namespace maestro {
    class ConfigurationV2 {

    public:
        ConfigurationV2(
                std::string dfsl_file_name,
                std::string hw_file_name,
                std::shared_ptr<std::vector<int>> noc_bw,
                std::shared_ptr<std::vector<int>> noc_latency,
                std::shared_ptr<std::vector<bool>> noc_multcast,
                int num_pes,
                int simd_width,
                int top_noc_bw,
                int l1_sram_byte_size,
                int l2_sram_byte_size,
                int offchip_bw) :
                dfsl_file_name_(dfsl_file_name),
                hw_file_name_(hw_file_name),
                noc_multcast_(noc_multcast),
                noc_latency_(noc_latency),
                noc_bw_(noc_bw),
                offchip_bw_(offchip_bw),
                num_pes_(num_pes),
                simd_width_(simd_width),
                l1_size_(l1_sram_byte_size),
                l2_size_(l2_sram_byte_size) {
            network_= std::make_shared<DFA::NeuralNetwork>();
            tensors_ = std::make_shared<std::vector<std::shared_ptr<DFA::TensorTable>>>();
            nocs_ = std::make_shared<std::vector<std::shared_ptr<AHW::NetworkOnChipModel>>>();
            cluster_analysis_= std::make_shared<std::vector<std::shared_ptr<DFA::ClusterAnalysis>>>();
            //TODO: Update NoC setup processes
            target_accelerator_ = std::make_shared<DSE::Accelerator>
                    (num_pes, simd_width, top_noc_bw, l1_sram_byte_size, l2_sram_byte_size);
        }

        std::string dfsl_file_name_;
        std::string hw_file_name_;


        std::shared_ptr<DFA::NeuralNetwork> network_;

        std::shared_ptr<std::vector<std::shared_ptr<DFA::TensorTable>>> tensors_;
        std::shared_ptr<std::vector<std::shared_ptr<DFA::ClusterAnalysis>>> cluster_analysis_;

        //Hardware
        std::shared_ptr<DSE::Accelerator> target_accelerator_;
        std::shared_ptr<std::vector<std::shared_ptr<AHW::NetworkOnChipModel>>> nocs_;

        std::shared_ptr<std::vector<bool>> noc_multcast_;
        std::shared_ptr<std::vector<int>> noc_latency_;
        std::shared_ptr<std::vector<int>> noc_bw_;

        int num_pes_;
        int simd_width_;
        int l1_size_;
        int l2_size_;
        int offchip_bw_;
    }; // End of class Configuration
}; // End of namespace maestro


#endif
