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


#ifndef MAESTRO_TL_ERROR_HANDLER_HPP_
#define MAESTRO_TL_ERROR_HANDLER_HPP_

#include <iostream>
#include <string>

namespace maestro {
    namespace TL {

        enum class ErrorCode {
            NoSpatialMap,
            MissingDimension,
            NotEnoughSpDim,
            DuplicatedDimDefinition,
            DoubleDimDefinition,
            InvalidCluster,
            InvalidClusterLevel,
            IllegalClusterConstruction,
            IllegalTemporalEdgeSp,
            InvalidTemporalEdgeSz,
            InvalidDirective,
            InvalidDimension,
            InvalidAnalysisCase,
            EdgeOnSpatialMap,
            NotEnoughL2Buffer,
            NotEnoughL1Buffer,
            MultiParallelismInSingleCluster,
            MissingNoCForCluster,
            NotSupportedLayerType
        };

        class ErrorHandler {
        public:
            void PrintErrorMsg(ErrorCode error_code, std::string opt, std::string instance_name = "") {
                switch(error_code) {
                    case ErrorCode::NoSpatialMap: {
                        std::cout << "(Error@ " << instance_name << ") Cluster level: " << opt << ", No spatial map in a cluster" << std::endl;
                        break;
                    }
                    case ErrorCode::MissingDimension: {
                        std::cout << "(Error@ " << instance_name << ") Dimension " << opt << " not found" << std::endl;
                        break;
                    }
                    case ErrorCode::NotEnoughSpDim: {
                        std::cout << "(Error@ " << instance_name << ") Dimension " << opt << " is not sufficient for conv windows" << std::endl;
                        break;
                    }
                    case ErrorCode::DuplicatedDimDefinition: {
                        std::cout << "(Error@ " << instance_name << ") Trying to re-define the operator dimension " << opt << std::endl;
                        break;
                    }
                    case ErrorCode::DoubleDimDefinition: {
                        std::cout << "(Error@ " << instance_name << ") Both input- and output-centric dimension definition is used. " << opt << std::endl;
                        break;
                    }
                    case ErrorCode::InvalidCluster: {
                        std::cout << "(Error@ " << instance_name << ") Cluster level " << opt << " contains directives other than temporal and spatial map" << std::endl;
                        break;
                    }
                    case ErrorCode::IllegalClusterConstruction: {
                        std::cout << "(Error@ " << instance_name << ") Specified cluster does not cover entire number of PEs" << std::endl;
                        break;
                    }
                    case ErrorCode::InvalidClusterLevel: {
                        std::cout << "(Error@ " << instance_name << ") Cluster level " << opt << " does not exist" << std::endl;
                        break;
                    }
                    case ErrorCode::IllegalTemporalEdgeSp: {
                        std::cout << "(Error@ " << instance_name << ") variable " << opt << " is spatially mapped but temporal edge is set" << std::endl;
                        break;
                    }
                    case ErrorCode::InvalidTemporalEdgeSz: {
                        std::cout << "(Error@ " << instance_name << ") variable " << opt << " does not have edge" << std::endl;
                        break;
                    }
                    case ErrorCode::InvalidDirective: {
                        std::cout << "(Error@ " << instance_name << ") found an invalid directive on variable " << opt << "." << std::endl;
                        break;
                    }

                    case ErrorCode::InvalidDimension: {
                        std::cout << "(Error@ " << instance_name << ") encountered an invalid dimension " << opt << "." <<  std::endl;
                        break;
                    }

                    case ErrorCode::InvalidAnalysisCase: {
                        std::cout << "(Error@ " << instance_name << ") encountered an invalid analysis case. " <<  std::endl;
                        break;
                    }

                    case ErrorCode::EdgeOnSpatialMap: {
                        std::cout << "(Error@ " << instance_name << ") Dataflow cannot have edge on spatial map. Please check the mapping size of spatial map at cluter level " << opt << "." <<  std::endl;
                        break;
                    }

                    case ErrorCode::NotEnoughL1Buffer: {
                        std::cout << "(Error@ " << instance_name << ") The required L1 buffer size " << opt << " is larger than your L1 size. Reduce the L1 tile size by reducing mapping sizes." << std::endl;
                        break;
                    }

                    case ErrorCode::NotEnoughL2Buffer: {
                        std::cout << "(Error@ " << instance_name << ") The required L2 buffer size " << opt << " is larger than your L2 size. Reduce the L2 tile size by reducing mapping sizes." << std::endl;
                        break;
                    }


                    case ErrorCode::MultiParallelismInSingleCluster: {
                        std::cout << "(Error@ " << instance_name << ") Found too many spatial maps within a single cluster. Cluster level: " << opt << "." <<  std::endl;
                        break;
                    }

                    case ErrorCode::MissingNoCForCluster: {
                        std::cout << "(Error@ " << instance_name << ") NoC is not defined at cluster level " << opt << "." << std::endl;
                        break;
                    }

                    case ErrorCode::NotSupportedLayerType: {
                        std::cout << "(Error@ " << instance_name << ") Not supported layer type. " << std::endl;
                        break;
                    }

                    default: {
                        std::cout << "(Error) Error in class " << opt << std::endl;
                    }
                }

                this->TerminateProgram();
            }

            void TerminateProgram() {
                exit(-1);
            }

        protected:

        }; // End of class ErrorHandler
    }; // End of namespace DFA
}; // End of namespace maestro

#endif
