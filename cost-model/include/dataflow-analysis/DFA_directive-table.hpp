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


#ifndef MAESTRO_DFA_DIRECTIVE_TABLE_HPP_
#define MAESTRO_DFA_DIRECTIVE_TABLE_HPP_

#include <memory>
#include <string>
#include <vector>
#include <list>


#include "DFA_directives.hpp"
#include "DFSL_syntax_tokens.hpp"

namespace maestro {
    namespace DFA {

        class DirectiveTable {
        public:

            class iterator {
            private:
                std::shared_ptr<std::vector<std::shared_ptr<directive::Directive>>> iter_directives_;
            public:

                int curr_idx_;

                iterator(std::shared_ptr<std::vector<std::shared_ptr<directive::Directive>>> ptr, int idx) :
                        iter_directives_(ptr),
                        curr_idx_(idx) {

                }

                iterator operator++() {
                    this->curr_idx_++;
                    iterator iter = *this;
                    return iter;
                }

                std::shared_ptr<directive::Directive>& operator*() {
                    return iter_directives_->at(curr_idx_);
                }

                bool operator==(const iterator& rhs) {
                    return (this->curr_idx_ == rhs.curr_idx_);
                }

                bool operator!=(const iterator& rhs) {
                    return (this->curr_idx_ != rhs.curr_idx_);
                }

            }; // End of class iterator for class Directive_table

            iterator begin() {
                iterator iter(directives_, 0);
                return iter;
            }

            iterator end() {
                iterator iter(directives_, directives_->size());
                return iter;
            }

            DirectiveTable() {
                directives_ = std::make_shared<std::vector<std::shared_ptr<directive::Directive>>>();
            }

            std::shared_ptr<directive::Directive> at (int idx) {
                if(idx < directives_->size()) {
                    return directives_->at(idx);
                }
                else {
                    return nullptr;
                }
            }

            std::shared_ptr<directive::Directive> operator[] (int idx) {
                return this->at(idx);
            }

            void ConvertToInputCentric() {

                int size_S, size_R = 0;

                for(auto& directive : *directives_)  {
                    if(directive->GetVariable() == DFSL::layer_dim_weight_width_) {
                        size_S = directive->GetSize();
                    }
                    else if(directive->GetVariable() == DFSL::layer_dim_weight_height_) {
                        size_R = directive->GetSize();
                    }
                }

                for(auto& directive : *directives_)  {
                    if(directive->GetVariable() == DFSL::layer_dim_output_width_) {
                        directive->SetVariable(DFSL::layer_dim_input_width_);
                        directive->SetSize(directive->GetSize() + size_S -1);
                    }
                    else if(directive->GetVariable() == DFSL::layer_dim_output_height_) {
                        directive->SetVariable(DFSL::layer_dim_input_height_);
                        directive->SetSize(directive->GetSize() + size_R -1);
                    }
                }
            }

            std::shared_ptr<directive::Directive> FindDirective (std::string var) {
                std::shared_ptr<directive::Directive> ret = nullptr;

                for(auto directive : *directives_) {
                    if(directive->GetVariable() == var) {
                        return directive;
                    }
                }

                return ret;
            }

            int GetDirectiveIdx (std::string var) {
                int idx = 0;
                for(auto directive : *directives_) {
                    if(directive->GetVariable() == var) {
                        return idx;
                    }
                    idx++;
                }
                return idx;
            }

            int GetTemporalMapIdx (std::string var) {
                int idx = 0;
                for(auto directive : *directives_) {
                    if(directive->GetVariable() == var) {
                        return idx;
                    }
                    if(directive->GetClass() == DFA::directive::DirectiveClass::TemporalMap) {
                        idx++;
                    }
                }
                return idx;
            }

            void AddDirectiveFront(std::shared_ptr<directive::Directive> new_directive) {
                directives_->insert(directives_->begin(), new_directive);
            }


            void AddDirective(std::shared_ptr<directive::Directive> new_directive) {
                directives_->push_back(new_directive);
            }

            void DeleteDirectives(){
                directives_->erase(directives_->begin(),
                                   directives_->begin() + directives_->size());
            }

            void ReverseDirectives() {
                std::reverse(std::begin(*directives_), std::end(*directives_));
            }

            int size() {
                return directives_->size();
            }

            std::string ToString() {
                std::string ret = "";
                for(auto& it : *directives_) {
                    ret = ret + it->ToString();
                    ret = ret + "\n";
                }

                return ret;
            }

        protected:
            std::shared_ptr<std::vector<std::shared_ptr<directive::Directive>>> directives_;

        }; // End of class DirectiveTable

    }; // End of namespace DFA
}; // End of namesapce maestro

#endif
