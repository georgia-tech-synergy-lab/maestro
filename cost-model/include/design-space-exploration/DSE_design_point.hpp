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


#ifndef MAESTRO_DSE_DESIGN_POINT_HPP_
#define MAESTRO_DSE_DESIGN_POINT_HPP_

#include <iostream>
#include <memory>
#include <list>

#include "DSE_config.hpp"

namespace maestro {
    namespace DSE {

        class DesignPoint {
        public:

            OptimizationTarget target_ = OptimizationTarget::Runtime;
            long runtime_ = 0;
            double energy_ = 0;
            long double performance_per_energy_ = 0;
            double area_ = 0;
            double power_ = 0;
            int num_pes_ = 1;
            int noc_bw_ = 16;
            int vector_width_ = 1;
            int l2_sram_sz = 1;
            int l1_sram_sz = 1;
            std::shared_ptr<std::list<std::pair<std::string, double>>> multicasting_factors_;

            DesignPoint(OptimizationTarget optimization_target, long runtime,
                        double energy, long double performance_per_energy, double area,
                        double power, int num_pes, int noc_bw, int vector_width,
                        int l2_size, int l1_size) :
                    target_(optimization_target), runtime_(runtime), energy_(energy), performance_per_energy_(
                    performance_per_energy), area_(area), power_(power), num_pes_(
                    num_pes), noc_bw_(noc_bw), vector_width_(vector_width), l2_sram_sz(
                    l2_size), l1_sram_sz(l1_size) {
                multicasting_factors_ = std::make_shared<
                        std::list<std::pair<std::string, double>>>();
            }

            bool operator<(std::shared_ptr<DesignPoint> dp) {
                bool ret = false;
                if (dp != nullptr) {
                    switch (target_) {
                        case OptimizationTarget::Runtime:
                            ret = runtime_ < dp->runtime_;
                            break;
                        case OptimizationTarget::Energy:
                            ret = energy_ < dp->energy_;
                            break;
                        case OptimizationTarget::PerformancePerWatt:
                            ret = performance_per_energy_ > dp->performance_per_energy_;
                            break;
                    }
                }
                return ret;
            }

            void PutMulticastingFactor(std::string dataclass, double factor) {
                multicasting_factors_->push_back(std::make_pair(dataclass, factor));
            }

            double GetMulticastingFactor(std::string dataclass) {
                double ret = -1;
                for(auto& it : *multicasting_factors_) {
                    if(it.first == dataclass) {
                        ret = it.second;
                        break;
                    }
                }
                return ret;
            }
        };
        // End of class DesignPoint
    }
// End of namespace DSE
}
// End of namespace maestro

#endif
