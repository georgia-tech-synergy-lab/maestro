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


#ifndef MAESTRO_DFA_ITERATION_STATUS_HPP_
#define MAESTRO_DFA_ITERATION_STATUS_HPP_

#include <memory>

#include "BASE_constants.hpp"

#include "BASE_maestro-class.hpp"
#include "TL_error-handler.hpp"

namespace maestro {
    namespace DFA {
        enum class IterationPosition {Init, Steady, Edge};

        // Over one dimension
        class IterationState : public MAESTROClass {
        public:
            IterationState(
                    std::string dimension_variable,
                    IterationPosition iter_position,
                    int num_occurrence,
                    bool is_unrolled,
                    bool is_edge = false,
                    bool has_sp_edge_edge = false
            ) :
                    MAESTROClass("IterationState"),
                    dimension_variable_(dimension_variable),
                    iter_position_(iter_position),
                    num_occurrence_(num_occurrence),
                    is_unrolled_(is_unrolled),
                    is_init_edge_(is_edge),
                    has_sp_edge_edge_(has_sp_edge_edge) {
            }

            std::string ToString() {
                std::string ret = "\n";
                ret += "<Iteration state>\n";

                std::string state_name="";

                switch(iter_position_) {
                    case IterationPosition::Init: {
                        state_name = "Init";
                        break;
                    }
                    case IterationPosition::Steady: {
                        state_name = "Steady";
                        break;
                    }
                    case IterationPosition::Edge: {
                        state_name = "Edge";
                        break;
                    }
                    default: {

                    }
                }

                ret += "Dimension " + dimension_variable_ + "\n";
                ret += "Iteration Position: " + state_name + "\n";

                ret += "is unrolled? ";
                if(is_unrolled_) {
                    ret += "Yes";
                }
                else {
                    ret += "No";
                }
                ret += "\n";

                ret += "is at Init position and edge? ";
                if(is_init_edge_) {
                    ret += "Yes";
                }
                else {
                    ret += "No";
                }
                ret += "\n";

                ret += "Does it have an edge PE at spatial edge case? ";
                if(has_sp_edge_edge_) {
                    ret += "Yes";
                }
                else {
                    ret += "No";
                }
                ret += "\n";

                return ret;
            }

            std::string GetDimVariable() {
                return dimension_variable_;
            }

            IterationPosition GetIterPosition() {
                return iter_position_;
            }

            int GetNumOccurrence() {
                return num_occurrence_;
            }

            bool IsUnrolled() {
                return is_unrolled_;
            }

            bool IsEdge() {
                return (iter_position_ == IterationPosition::Edge) || is_init_edge_;
            }

            bool HasSpEdgeEdge() {
                return has_sp_edge_edge_;
            }

        protected:
            std::string dimension_variable_;
            IterationPosition iter_position_;
            int num_occurrence_;
            bool is_unrolled_;
            bool is_init_edge_;
            bool has_sp_edge_edge_; // In spatial edge case, if the last active PE has edge mapping
        };

        // Over all dimensions
        class IterationStatus : public MAESTROClass {
        public:
            IterationStatus() :
                    MAESTROClass("IterationStatus"),
                    num_occurrences_(1) {
                iter_states_ = std::make_shared<std::map<std::string, std::shared_ptr<IterationState>>>();
            }

            IterationStatus(int num_occurrences) :
                    num_occurrences_(num_occurrences) {
                iter_states_ = std::make_shared<std::map<std::string, std::shared_ptr<IterationState>>>();
            }

            class iterator {
            private:
                std::shared_ptr<std::map<std::string, std::shared_ptr<IterationState>>> map_ptr_;
                std::map<std::string, std::shared_ptr<IterationState>>::iterator map_iterator_;
            public:

                iterator(std::shared_ptr<std::map<std::string, std::shared_ptr<IterationState>>> ptr_iter_states_) :
                        map_ptr_(ptr_iter_states_) {
                    map_iterator_ = ptr_iter_states_->begin();
                }

                iterator operator++() {
                    map_iterator_++;
                    return *this;
                }

                std::shared_ptr<IterationState>& operator*() {
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
                iterator iter(iter_states_);
                return iter;
            }

            iterator end() {
                iterator iter(iter_states_);
                iter.set_end();
                return iter;
            }

            std::string ToString() {
                std::string ret = "----------------------------\n";
                ret += "<<Iteration status>>\n";
                ret += "Num status occurrences: " + std::to_string(num_occurrences_) + " \n";
                ret += "Iteration states: \n";

                for(auto& iter_state: *iter_states_) {
                    ret += iter_state.second->ToString();
                }

                ret += "----------------------------\n";

                return ret;
            }

            void SetNumOccurrences(int num_occ) {
                num_occurrences_ = num_occ;
            }

            int GetNumOccurrences() {
                return num_occurrences_;
            }

            void AddIterState(std::shared_ptr<IterationState> iter_state) {
                (*iter_states_)[iter_state->GetDimVariable()] = iter_state;
            }

            std::shared_ptr<IterationState> GetIterState(std::string dim_var) {
                return (*iter_states_)[dim_var];
            }

            bool isAllInit() {
                bool ret = true;

                for(auto& iter_state : *iter_states_ ) {
                    if(iter_state.second->GetIterPosition() != DFA::IterationPosition::Init) {
                        ret = false;
                    }
                }

                return ret;
            }

        protected:
            int num_occurrences_ = 1;
            std::shared_ptr<std::map<std::string, std::shared_ptr<IterationState>>> iter_states_;

        }; // End of class IterationStatus
    }
}

#endif
