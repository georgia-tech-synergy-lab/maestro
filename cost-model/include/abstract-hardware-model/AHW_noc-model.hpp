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

#ifndef MAESTRO_AHW_NOC_MODEL_HPP_
#define MAESTRO_AHW_NOC_MODEL_HPP_

namespace maestro {

    namespace AHW {

        class NetworkOnChipModel {
        public:
            NetworkOnChipModel(int bw, int hops, int hop_latency ,bool mc) :
                    bandwidth_(bw), num_average_hops_(hops), latency_per_hops_(hop_latency), multicast_support_(mc) {
            }

            int GetBandwidth() {
                return bandwidth_;
            }

            bool IsMulticastSupported() {
                return multicast_support_;
            }

            void SetBandwidth(int bw) {
                bandwidth_ = bw;
            }

            long GetOutStandingDelay(long data_amount) {
                long delay;

                long num_sends;
                if(data_amount % bandwidth_ != 0) {
                    num_sends = data_amount / bandwidth_ + 1;
                }
                else {
                    num_sends = data_amount / bandwidth_;
                }

                long avg_zero_load_delay = num_average_hops_ * latency_per_hops_;

                delay = avg_zero_load_delay // Head delay
                        + (num_sends-1); // Pipeline delay

                return delay;
            } // End of GetOutStandingDelay

        protected:
            int bandwidth_;
            int num_average_hops_;
            int latency_per_hops_;
            bool multicast_support_;

        }; // End of class NetworkOnChipModel
    }; // End of namespace abstract_hw
}; // End of namespace maestro
#endif
