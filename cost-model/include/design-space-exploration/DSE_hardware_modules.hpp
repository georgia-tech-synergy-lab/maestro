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


#ifndef MAESTRO_DSE_BUILDLINGBLOCKSPEC_HPP_
#define MAESTRO_DSE_BUILDLINGBLOCKSPEC_HPP_

#include <cmath>
#include <vector>
#include <memory>

#include "DSE_scaling-coefficients.hpp"
#include "DSE_cost-database.hpp"

namespace maestro {

    namespace DSE {

        /* Generic hardware module */
        class HardwareModule {
        public:

            HardwareModule() :
                    area_(0),
                    power_(0) {
            }

            HardwareModule(double area, double power) :
                    area_(area),
                    power_(power) {
            }
            virtual ~HardwareModule() {}

            virtual double GetArea() {
                double total_area = 0;

                for(auto& submodule : submodules_) {
                    total_area += submodule->GetArea();
                }

                return total_area;
            }

            virtual double GetPower() {
                double total_power = 0;

                for(auto& submodule : submodules_) {
                    total_power += submodule->GetPower();
                }

                return total_power;
            }

            virtual double GetSize () {return 1;}

            void AddSubmodule(std::shared_ptr<HardwareModule> submodule) {
                submodules_.push_back(submodule);
            }

            void ClearSubmodules() {
                submodules_.clear();
            }

        protected:
            std::vector<std::shared_ptr<HardwareModule>> submodules_;
            double area_ = 0.0;
            double power_ = 0.0;
        }; // End of class HardwareModule

        /* Multiplier-Accumulator unit */
        class MAC : public HardwareModule {
        public:
            MAC(double area, double power, int vector_width) :
                    vector_width_(static_cast<double>(vector_width)),
                    HardwareModule(area, power) {
            }

            virtual double GetArea() { return area_ * vector_width_; }
            virtual double GetPower() { return power_ * vector_width_; }
            virtual double GetSize () {return vector_width_;}

        protected:
            double vector_width_ = 1;
        }; // End of class MAC

        /* SRAM unit */
        class SRAM : public HardwareModule {
        public:
            SRAM(double area, double power, int unit_size_, int total_size_) :
                    HardwareModule(area, power) {
                double num_sram_cells = ceil((static_cast<double>(total_size_)) / (static_cast<double>(unit_size_)));
                if (num_sram_cells == 0) num_sram_cells = 1;
                num_sram_cells_ = num_sram_cells;
            }

            virtual double GetArea() { return area_ * num_sram_cells_; }
            virtual double GetPower() { return power_ * num_sram_cells_; }
            virtual double GetSize () {return num_sram_cells_;}

        protected:
            double num_sram_cells_ = 1;
        }; // End of class SRAM

        /* Arbiter */
        class MatrixArbiter : public HardwareModule {
        public:
            MatrixArbiter(double area, double power, double num_connections) :
                    num_connections_(num_connections),
                    HardwareModule(area, power) {
            }

            virtual double GetArea() {
                double estimated_area = area_ * GetScaledNumber();
                if (estimated_area < area_) {return area_;}
                return estimated_area;
            }

            virtual double GetPower() {
                double estimated_power = power_ * GetScaledNumber()/1000; //TODO: Update power number
                if (estimated_power < power_) {return power_;}
                return estimated_power;
            }

            virtual double GetSize () {return num_connections_;}

        protected:
            double num_connections_ = 1;

        private:

            double GetScaledNumber() {
                double mult_ = scaling_model::matrix_arbiter_c2 * pow(num_connections_, 2) + scaling_model::matrix_arbirter_c1 * num_connections_ + scaling_model::matrix_arbiter_c0 ;
                return mult_;
            }
        };

        /* Network-on-Chip */
        class NOC : public HardwareModule {
        public:
            NOC(double area, double power, double num_connections ) :
                    HardwareModule(area, power),
                    num_connections_(num_connections) {
            }

        protected:
            double num_connections_ = 1;

            virtual double GetArea() { return area_ * num_connections_; }
            virtual double GetPower() { return power_ * num_connections_; }
            virtual double GetSize () {return num_connections_;}
        }; // End of class NoC

        class Bus: public NOC {
        public:
            Bus(double area, double power, double num_connections, int bandwidth) :
                    bandwidth_(bandwidth),
                    NOC(area, power, num_connections) {
            }

            virtual double GetArea() { return area_ * GetScaledNumber() * bandwidth_; }
            virtual double GetPower() { return power_ * GetScaledNumber()/1000 * bandwidth_; } //TODO: Update power number
            virtual double GetSize () {return num_connections_;}

        protected:
            int bandwidth_;

            double GetScaledNumber() {
                double res = scaling_model::bus_c1 * num_connections_ + scaling_model::bus_c0;
                return res;
            }
        }; // End of class Bus

        class Accelerator : public HardwareModule {
        public:

            Accelerator (int num_pes, int vector_width, int noc_bw, int l1_sram_byte_size, int l2_sram_byte_size)
                    : num_pes_(num_pes), vector_width_(vector_width), noc_bw_(noc_bw) {
                ReconstructAccelerator(num_pes, vector_width, noc_bw, l1_sram_byte_size, l2_sram_byte_size);
            }


            void ReconstructAccelerator (int num_pes, int vector_width, int noc_bw, int l1_sram_byte_size, int l2_sram_byte_size) {
                this->ClearSubmodules();

                num_pes_ = num_pes;
                vector_width_ = vector_width;
                noc_bw_ = noc_bw;

                auto pe_array = std::make_shared<DSE::HardwareModule>();

                for(int peID = 0; peID < num_pes; peID++) {
                    std::shared_ptr<DSE::HardwareModule> pe = std::make_shared<DSE::HardwareModule>();
                    std::shared_ptr<DSE::HardwareModule> mac = std::make_shared<DSE::MAC>(cost::mac_area, cost::mac_power, vector_width);
                    std::shared_ptr<DSE::HardwareModule> l1_sram = std::make_shared<DSE::SRAM>(cost::sram_area_64, cost::sram_power_64, cost::sram_unit_size_64, l1_sram_byte_size);

                    pe->AddSubmodule(mac);
                    pe->AddSubmodule(l1_sram);
                    pe_array->AddSubmodule(pe);

                    l1_sram_power_ += l1_sram->GetPower();
                    pe_power_ += mac->GetPower();
                }

                /* L2 Buffer */
                auto l2_sram = std::make_shared<DSE::SRAM>(cost::sram_area_32768, cost::sram_power_32768, cost::sram_unit_size_32768, l2_sram_byte_size);
                l2_sram_power_ = l2_sram->GetPower();

                /* NoC */
                auto noc = std::make_shared<DSE::HardwareModule>();
                std::shared_ptr<DSE::HardwareModule> bus = std::make_shared<DSE::Bus>(cost::bus_unit_area, cost::bus_unit_power, num_pes, noc_bw);
                std::shared_ptr<DSE::HardwareModule> arbiter = std::make_shared<DSE::MatrixArbiter>(cost::arbiter_unit_area, cost::arbiter_unit_power, num_pes);

                noc->AddSubmodule(bus);
                noc->AddSubmodule(arbiter);

                noc_power_ = noc->GetPower();

                this->AddSubmodule(pe_array);
                this->AddSubmodule(l2_sram);
                this->AddSubmodule(noc);
            }

            int GetVectorWidth() {
                return vector_width_;
            }

            double GetPEPower() {
                return pe_power_;
            }

            double GetNoCPower() {
                return noc_power_;
            }

            double GetL1Power() {
                return l1_sram_power_;
            }

            double GetL2Power() {
                return l2_sram_power_;
            }


        protected:
            int vector_width_;
            int num_pes_;
            int noc_bw_;
            double l2_sram_power_ = 0;
            double l1_sram_power_ = 0;
            double pe_power_ = 0;
            double noc_power_ = 0;
        }; // End of class Accelerator
    } // End of namespace DSE
} // End of namespace maestro

#endif
