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


#ifndef MAESTRO_DFA_DIMENSION_TABLE_HPP_
#define MAESTRO_DFA_DIMENSION_TABLE_HPP_

#include <vector>
#include <memory>
#include <string>

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

#include "DFA_layer.hpp"
#include "DFA_dimension-overlap-info-table.hpp"

namespace maestro {
    namespace DFA {

        class DimensionTable : public MAESTROClass {
        public:
            class iterator {
            private:
                std::map<std::string, std::shared_ptr<LayerDimension>>* map_ptr_;
                std::map<std::string, std::shared_ptr<LayerDimension>>::iterator map_iterator_;
            public:

                iterator(std::map<std::string, std::shared_ptr<LayerDimension>>* ptr_layerdim_map) :
                        map_ptr_(ptr_layerdim_map) {
                    map_iterator_ = ptr_layerdim_map->begin();
                }

                iterator operator++() {
                    map_iterator_++;
                    return *this;
                }

                std::shared_ptr<LayerDimension>& operator*() {
                    return map_iterator_->second;
                }

                bool operator==(const iterator& rhs) {
                    return (this->map_iterator_ == rhs.map_iterator_);
                }

                bool operator!=(const iterator& rhs) {
                    return (this->map_iterator_ != rhs.map_iterator_);
                }

                void set_end () {
                    map_iterator_ = map_ptr_->end();
                }
            }; // End of class iterator for class Directive_table

            iterator begin() {
                iterator iter(&dim_table_);
                return iter;
            }

            iterator end() {
                iterator iter(&dim_table_);
                iter.set_end();
                return iter;
            }


            DimensionTable() :
                    MAESTROClass("Dimension Table") {
                dim_overlap_table_ = std::make_shared<DimensionOverlapInfoTable>();
            }

            // Not a good way to use std::map; will remove after update deprecated code
            std::shared_ptr<LayerDimension> at (int idx) {
                int count = 0;

                for(auto iter = dim_table_.begin(); iter != dim_table_.end(); iter++) {
                    if(count == idx) {
                        return iter->second;
                    }
                    count ++;
                }
                return nullptr;
            }

            std::shared_ptr<LayerDimension> operator[] (int idx) {
                return this->at(idx);
            }

            bool HasVar(std::string targ) {
                return (dim_table_.find(targ) != dim_table_.end());
            }

            int GetSize(std::string targ) {

                if(!this->HasVar(targ)) {
                    error_handler_->PrintErrorMsg(TL::ErrorCode::MissingDimension, targ);
                    error_handler_->TerminateProgram();
                }

                return dim_table_[targ]->GetSize();
            }

            int GetOuterStride(std::string targ) {

                if(!this->HasVar(targ)) {
                    error_handler_->PrintErrorMsg(TL::ErrorCode::MissingDimension, targ);
                    error_handler_->TerminateProgram();
                }

                return dim_table_[targ]->GetOuterStride();

            }

            int GetInnerStride(std::string targ) {

                if(!this->HasVar(targ)) {
                    error_handler_->PrintErrorMsg(TL::ErrorCode::MissingDimension, targ);
                    error_handler_->TerminateProgram();
                }

                return dim_table_[targ]->GetInnerStride();

            }


            void AddDimension(std::shared_ptr<LayerDimension> new_dimension) {
                dim_table_.insert(std::make_pair(new_dimension->GetName(), new_dimension));
            }

            void AddOverlapDimension(std::string reference_dim, std::string sliding_dim) {
                dim_overlap_table_->AddOverlapDimension(reference_dim, sliding_dim);
            }

            void AddOverlapDimensions(std::shared_ptr<std::list<std::shared_ptr<std::pair<std::string, std::string>>>> overlap_dim_list) {
                dim_overlap_table_->AddOverlapDimensions(overlap_dim_list);
            }

            void SetOverlapTable (std::shared_ptr<DimensionOverlapInfoTable> new_table) {
                dim_overlap_table_ = new_table;
            }

            bool IsOverlapped(std::string dim) {
                return dim_overlap_table_->IsOverlapped(dim);
            }

            bool IsSlidingDim(std::string dim) {
                return dim_overlap_table_->IsSlidingDim(dim);
            }

            std::string GetOverlappingDim(std::string dim) {
                std::string ret;

                ret = dim_overlap_table_->GetCounterPart(dim);

                return ret;
            }

            std::shared_ptr<DimensionOverlapInfoTable> GetOverlapTable() {
                return dim_overlap_table_;
            }

            std::string ToString() {
                std::string ret = "Dimension Table contents\n";
                for(auto& it : dim_table_) {
                    ret += it.second->ToString();
                    ret += "\n";
                }
                return ret;
            }

        protected:
            std::map<std::string, std::shared_ptr<LayerDimension>> dim_table_;
            std::shared_ptr<DimensionOverlapInfoTable> dim_overlap_table_;
        };
    }
}
#endif
