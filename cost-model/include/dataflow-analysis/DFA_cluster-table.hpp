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


#ifndef MAESTRO_DFA_CLUSTER_TABLE_HPP_
#define MAESTRO_DFA_CLUSTER_TABLE_HPP_

#include <memory>
#include <vector>

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "DFA_layer.hpp"
#include "DFA_cluster-unit.hpp"

namespace maestro {
    namespace DFA {
        class ClusterTable: public MAESTROClass {
        public:

            /*
             * Custom iterator to support "for-each" loop over this data structure
             */
            class iterator {
            private:
                std::shared_ptr<std::vector<std::shared_ptr<DFA::ClusterUnit>>> iter_clusters_;
            public:

                int curr_idx_;

                iterator(std::shared_ptr<std::vector<std::shared_ptr<DFA::ClusterUnit>>> ptr, int idx) :
                        iter_clusters_(ptr), curr_idx_(idx) {
                }

                iterator operator++() {
                    this->curr_idx_++;
                    iterator iter = *this;
                    return iter;
                }

                iterator operator--() {
                    if(this->curr_idx_ != 0) {
                        this->curr_idx_--;
                    }
                    iterator iter = *this;
                    return iter;
                }

                std::shared_ptr<DFA::ClusterUnit>& operator*() {
                    return iter_clusters_->at(curr_idx_);
                }

                bool operator==(const iterator& rhs) {
                    return (this->curr_idx_ == rhs.curr_idx_);
                }

                bool operator!=(const iterator& rhs) {
                    return (this->curr_idx_ != rhs.curr_idx_);
                }

            }; // End of class iterator for class Directive_table

            iterator begin() {
                iterator iter(clusters_, 0);
                return iter;
            }

            iterator end() {
                iterator iter(clusters_, clusters_->size());
                return iter;
            }
            /***********************************************************************************************/

            ClusterTable(LayerType layer_type) :
                    layer_type_(layer_type),
                    MAESTROClass("ClusterTable") {
                clusters_ = std::make_shared<std::vector<std::shared_ptr<DFA::ClusterUnit>>>();
            }

            int size() {
                return clusters_->size();
            }

            void PutCluster(std::shared_ptr<DFA::ClusterUnit> new_cluster) {
                clusters_->push_back(new_cluster);
            }

            std::shared_ptr<DFA::ClusterUnit> GetCluster(int lv) {
                if(lv < clusters_->size()) {
                    return clusters_->at(lv);
                }
                else {
                    error_handler_->PrintErrorMsg(TL::ErrorCode::InvalidClusterLevel, std::to_string(lv), instance_name_);
                    return nullptr;
                }
            }

            LayerType GetLayerType() {
                return layer_type_;
            }

        protected:
            LayerType layer_type_;
            std::shared_ptr<std::vector<std::shared_ptr<DFA::ClusterUnit>>> clusters_;

        }; // End of class ClusterTable
    }; // End of namespace DFA
};// End of namespace maestro

#endif
