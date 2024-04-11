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


#ifndef MAESTRO_BASE_MAESTRO_CLASS_HPP_
#define MAESTRO_BASE_MAESTRO_CLASS_HPP_

#include <memory>

#include "BASE_base-objects.hpp"
#include "TL_error-handler.hpp"

namespace maestro {
    class MAESTROClass {
    public:

        MAESTROClass() :
                instance_name_("class"),
                error_handler_(error_handler),
                message_printer_(message_printer) {
        }

        MAESTROClass(std::string instance_name) :
                instance_name_(instance_name),
                error_handler_(error_handler),
                message_printer_(message_printer) {
        }

        std::string GetName() {
            return instance_name_;
        }

        void ToString() {
            std::cout << "Base class" << std::endl;;
        }

    protected:
        std::string instance_name_;
        std::shared_ptr<TL::ErrorHandler> error_handler_;
        std::shared_ptr<TL::MessagePrinter> message_printer_;
    };
};
#endif
