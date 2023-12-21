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


#ifndef MAESTRO_DSE_SCALING_COEFFICIENTS_HPP_
#define MAESTRO_DSE_SCALING_COEFFICIENTS_HPP_

namespace maestro {
    namespace DSE {
        namespace scaling_model {
            /* Embedded scaling model */
            // Based on 28nm technology
            const double matrix_arbiter_c2 = 1.2886;
            const double matrix_arbirter_c1 = 5.5814;
            const double matrix_arbiter_c0 = -23.711;

            const double bus_c1 = 14.662;
            const double bus_c0 = 28.895;

        }; // End of namespace scaling_model
    }; // End of namespace DSE
}; // End of namespace maestro


#endif
