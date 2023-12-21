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


#ifndef MAESTRO_DFSL_HW_PARSER_HPP_
#define MAESTRO_DFSL_HW_PARSER_HPP_

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <memory>
#include <map>
#include <vector>

#include<boost/tokenizer.hpp>
#include<boost/format.hpp>

#include "BASE_maestro-class.hpp"
#include "DFSL_syntax_tokens.hpp"
#include "DFSL_parser.hpp"


namespace maestro {
    namespace DFSL {

        //Indentation shows the structure of the state machine in HWParser.
        enum class HWParserState {Idle,
            NumPEsIdentifier,
            L1SizeIdentifier,
            L2SizeIdentifier,
            NoCBWIdentifier,
            NoCNumHopsIdentifier,
            OffChipBWIdentifier,
        };

        class HWConfig : public MAESTROClass {
        public:
            int num_pes_ = 1;
            int l1_size_ = INT_MAX;
            int l2_size_ = INT_MAX;
            int noc_bw_ = INT_MAX;
            int noc_hops_ = 1;
            int off_chip_bw_ = INT_MAX;
        };

        class HWParser : public InputParser {
        public:
            HWParser(std::string file_name) : InputParser(file_name) {
            }

            std::shared_ptr<DFSL::HWConfig> ParseHW() {
                std::string line;
                boost::char_separator<char> sep(" ,->():\t;");
                int line_number = 1;
                auto ret = std::make_shared<DFSL::HWConfig>();
                while(std::getline(in_file_, line)) {
                    boost::tokenizer<boost::char_separator<char>> tokn(line, sep);

                    for(auto &tkn : tokn) {

                        if(tkn.size() >= 2 && tkn.substr(0,2) == DFSL::comments_) {
                            line_number++;
                            break;
                        }

                        switch(state_) {
                            case HWParserState::Idle: {
                                if(tkn == DFSL::tmp_num_pes_decl_) {
                                    state_ = HWParserState::NumPEsIdentifier;
                                }
                                else if(tkn == DFSL::tmp_l1size_decl_) {
                                    state_ = HWParserState::L1SizeIdentifier;
                                }
                                else if(tkn == DFSL::tmp_l2size_decl_) {
                                    state_ = HWParserState::L2SizeIdentifier;
                                }
                                else if(tkn == DFSL::tmp_noc_bw_decl_) {
                                    state_ = HWParserState::NoCBWIdentifier;
                                }
                                else if(tkn == DFSL::tmp_noc_hops_decl_) {
                                    state_ = HWParserState::NoCNumHopsIdentifier;
                                }
                                    //felix
                                else if(tkn == DFSL::tmp_offchip_bw_decl_) {
                                    state_ = HWParserState::OffChipBWIdentifier;
                                }
                                else {
                                    ParseError(line_number);
                                }
                                break;
                            }

                            case HWParserState::NumPEsIdentifier: {
                                ret->num_pes_ = std::atoi(tkn.c_str());
                                state_ = HWParserState::Idle;
                                break;
                            }

                            case HWParserState::L1SizeIdentifier: {
                                std::cout << "" << std::endl;
                                ret->l1_size_ = std::atoi(tkn.c_str());
                                state_ = HWParserState::Idle;
                                break;
                            }

                            case HWParserState::L2SizeIdentifier: {
                                ret->l2_size_ = std::atoi(tkn.c_str());
                                state_ = HWParserState::Idle;
                                break;
                            }

                            case HWParserState::NoCBWIdentifier: {
                                ret->noc_bw_ = std::atoi(tkn.c_str());
                                state_ = HWParserState::Idle;
                                break;
                            }

                            case HWParserState::NoCNumHopsIdentifier: {
                                ret->noc_hops_ = std::atoi(tkn.c_str());
                                state_ = HWParserState::Idle;
                                break;
                            }

                            case HWParserState::OffChipBWIdentifier: {
                                ret->off_chip_bw_ = std::atoi(tkn.c_str());
                                state_ = HWParserState::Idle;
                                break;
                            }

                            default: {
                                ParseError(line_number);
                                break;
                            }
                        } // End of switch(state_)
                    } // End of for(tkn)

                    line_number++;

                } // End of while(getline(...))

                if(state_ != HWParserState::Idle) {
                    ParseError(line_number);
                }

                return ret;

            } // End of ParseHW

        protected:
            HWParserState state_ = HWParserState::Idle;

        };
    }; // End of namespace DFSL
}; // End of namespace maestro

#endif
