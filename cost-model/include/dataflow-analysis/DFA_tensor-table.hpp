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


#ifndef MAESTRO_DFA_TENSOR_TABLE_HPP_
#define MAESTRO_DFA_TENSOR_TABLE_HPP_

#include <string>
#include <vector>

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "DFA_tensor.hpp"

namespace maestro {
    namespace DFA {
        class TensorTable : public MAESTROClass {
        public:
            /*
             * Custom iterator to support "for-each" loop over this data structure
             */
            class iterator {
            private:
                std::shared_ptr<std::vector<std::shared_ptr<DFA::Tensor>>> iter_tensors_;
            public:

                int curr_idx_;

                iterator(std::shared_ptr<std::vector<std::shared_ptr<DFA::Tensor>>> ptr, int idx) :
                        iter_tensors_(ptr), curr_idx_(idx) {
                }

                iterator operator++() {
                    this->curr_idx_++;
                    iterator iter = *this;
                    return iter;
                }

                std::shared_ptr<DFA::Tensor>& operator*() {
                    return iter_tensors_->at(curr_idx_);
                }

                bool operator==(const iterator& rhs) {
                    return (this->curr_idx_ == rhs.curr_idx_);
                }

                bool operator!=(const iterator& rhs) {
                    return (this->curr_idx_ != rhs.curr_idx_);
                }

            }; // End of class iterator for class Directive_table

            iterator begin() {
                iterator iter(tensors_, 0);
                return iter;
            }

            iterator end() {
                iterator iter(tensors_, tensors_->size());
                return iter;
            }
            /***********************************************************************************************/


            TensorTable() {
                tensors_ = std::make_shared<std::vector<std::shared_ptr<DFA::Tensor>>>();
            }

            std::shared_ptr<DFA::Tensor> at (int idx) {
                if(idx < tensors_->size()) {
                    return tensors_->at(idx);
                }
                else {
                    return nullptr;
                }
            }

            std::shared_ptr<DFA::Tensor> operator[] (int idx) {
                return this->at(idx);
            }

            void AddTensor(std::shared_ptr<DFA::Tensor> new_tensor) {
                tensors_->push_back(new_tensor);
            }


            void AddTensor(std::string tensor_name, DFA::TensorClass tensor_class, DataClass data_class, std::vector<std::string> correlated_varaibles) {

                std::shared_ptr<std::list<std::string>> corr_vars_to_be_added = std::make_shared<std::list<std::string>>();

                for(auto var : correlated_varaibles) {
                    corr_vars_to_be_added->push_back(var);
                }

                auto new_tensor = std::make_shared<DFA::Tensor>(tensor_name, tensor_class, data_class, corr_vars_to_be_added);
                tensors_->push_back(new_tensor);
            }

            std::shared_ptr<DFA::Tensor> FindTensor(std::string tensor_name) {
                std::shared_ptr<DFA::Tensor> ret = nullptr;

                for(auto tensor : *tensors_) {
                    if(tensor->GetTensorName() == tensor_name) {
                        ret = tensor;
                    }
                }

                return ret;
            }

            std::shared_ptr<std::list<std::string>> GetTensorVarsInClass(DFA::TensorClass tensor_class) {
                std::shared_ptr<std::list<std::string>> ret = std::make_shared<std::list<std::string>>();

                for(auto& tensor : *tensors_) {
                    if(tensor->GetTensorClass() == tensor_class) {
                        auto corr_var_list = tensor->GetCoupledVariables();
                        for(auto var : *corr_var_list) {
                            ret->push_back(var);
                        }
                    }
                }

                ret->sort();
                ret->unique();

                return ret;
            }

            std::shared_ptr<std::list<std::shared_ptr<DFA::Tensor>>> GetTensorsInClass(DFA::TensorClass tensor_class) {
                std::shared_ptr<std::list<std::shared_ptr<DFA::Tensor>>> ret = std::make_shared<std::list<std::shared_ptr<DFA::Tensor>>>();

                for(auto& tensor : *tensors_) {
                    if(tensor->GetTensorClass() == tensor_class) {
                        ret->push_back(tensor);
                    }
                }

                return ret;
            }



        protected:
            std::shared_ptr<std::vector<std::shared_ptr<DFA::Tensor>>> tensors_;

        }; // End of class TensorTable
    }; // End of namespace DFA
}; // End of namespace maestro

#endif
