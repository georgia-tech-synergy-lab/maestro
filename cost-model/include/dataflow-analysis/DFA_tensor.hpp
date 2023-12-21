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


#ifndef MAESTRO_DFA_TENSOR_HPP_
#define MAESTRO_DFA_TENSOR_HPP_

#include <string>
#include <list>

#include "BASE_constants.hpp"

namespace maestro {
    namespace DFA {

        enum class TensorClass {InputTensor, OutputTensor};

        class Tensor {
        public:
            Tensor(std::string name,
                   TensorClass tensor_class,
                   DataClass data_class,
                   std::shared_ptr<std::list<std::string>> correlated_variables) :
                    tensor_name_(name),
                    tensor_class_(tensor_class),
                    data_class_(data_class),
                    coupled_variables_(correlated_variables) {
            }

            std::string GetTensorName() {
                return tensor_name_;
            }

            TensorClass GetTensorClass() {
                return tensor_class_;
            }

            DataClass GetDataClass() {
                return data_class_;
            }

            std::shared_ptr<std::list<std::string>> GetCoupledVariables() {
                return coupled_variables_;
            }

            bool HasVariable(std::string search_var) {
                bool ret =false;
                for(auto& var : *coupled_variables_) {
                    if(var == search_var) {
                        ret = true;
                        break;
                    }
                }
                return ret;
            }

        protected:
            TensorClass tensor_class_;
            DataClass data_class_;
            std::string tensor_name_;
            std::shared_ptr<std::list<std::string>> coupled_variables_;
        }; // End of class Tensor

    }; // End of namespace DFA
}; // End of namespace maestro

#endif
