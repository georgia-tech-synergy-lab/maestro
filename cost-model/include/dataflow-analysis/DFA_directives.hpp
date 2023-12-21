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


#ifndef MAESTRO_DFA_DIRECTIVES_HPP_
#define MAESTRO_DFA_DIRECTIVES_HPP_

#include <string>

#include "DFSL_syntax_tokens.hpp"

namespace maestro {
    namespace DFA {

        namespace directive {

            enum class DirectiveClass{TemporalMap, SpatialMap, Cluster, Invalid};
            enum class ClusterType {Physical, Logical, Invalid};

            class Directive {
            public:
                virtual ~Directive() {}

                virtual std::string ToString() {
                    std::string ret = "";
                    return ret;
                }

                virtual void SetVariable(std::string new_var) {

                }

                virtual void SetSize(int new_size) {

                }

                virtual void SetOfs(int new_ofs) {

                }

                virtual DirectiveClass GetClass() {
                    return DirectiveClass::Invalid;
                }

                virtual int GetSize() {
                    return 0;
                }

                virtual int GetOfs() {
                    return 0;
                }

                virtual std::string GetVariable() {
                    return "";
                }

                virtual ClusterType GetAllocType () {
                    return ClusterType::Invalid;
                }

            };

            class Map : public Directive {
            public:
                Map(int size, int offset, std::string var) :
                        size_(size), offset_(offset), variable_(var) {
                }

                virtual DirectiveClass GetClass() {
                    return DirectiveClass::Invalid;
                }

                virtual int GetSize() {
                    return size_;
                }

                virtual int GetOfs() {
                    return offset_;
                }

                virtual std::string GetVariable() {
                    return variable_;
                }

                virtual ClusterType GetAllocType () {
                    return ClusterType::Invalid;
                }

                virtual std::string ToString() {
                    std::string ret = "Map " + variable_;
                    return ret;
                }

                virtual void SetVariable(std::string new_var) {
                    variable_ = new_var;
                }

                virtual void SetSize(int new_size) {
                    size_ = new_size;
                }

                virtual void SetOfs(int new_ofs) {
                    offset_ = new_ofs;
                }


            protected:
                std::string variable_;
                int size_;
                int offset_;
            }; // End of class Map

            class TemporalMap : public Map {
            public:

                TemporalMap(int size, int offset, std::string var) :
                        Map(size, offset, var) {
                }

                virtual DirectiveClass GetClass() {
                    return DirectiveClass::TemporalMap;
                }

                virtual int GetSize() {
                    return size_;
                }

                virtual int GetOfs() {
                    return offset_;
                }

                virtual std::string GetVariable() {
                    return variable_;
                }

                virtual std::string ToString() {
                    std::string ret = "TemporalMap(" + std::to_string(size_) + "," + std::to_string(offset_) + ") " + variable_;
                    return ret;
                }

                virtual void SetVariable(std::string new_var) {
                    variable_ = new_var;
                }

                virtual void SetSize(int new_size) {
                    size_ = new_size;
                }

                virtual void SetOfs(int new_ofs) {
                    offset_ = new_ofs;
                }

            protected:
            }; // End of class TemporalMap

            class SpatialMap : public Map {
            public:

                SpatialMap(int size, int offset, std::string var) :
                        Map(size, offset, var) {
                }

                virtual DirectiveClass GetClass() {
                    return DirectiveClass::SpatialMap;
                }

                virtual int GetSize() {
                    return size_;
                }

                virtual int GetOfs() {
                    return offset_;
                }

                virtual std::string GetVariable() {
                    return variable_;
                }

                virtual ClusterType GetAllocType () {
                    return ClusterType::Invalid;
                }

                virtual std::string ToString() {
                    std::string ret = "SpatialMap(" + std::to_string(size_) + "," + std::to_string(offset_) + ") " + variable_;
                    return ret;
                }

                virtual void SetVariable(std::string new_var) {
                    variable_ = new_var;
                }

                virtual void SetSize(int new_size) {
                    size_ = new_size;
                }

                virtual void SetOfs(int new_ofs) {
                    offset_ = new_ofs;
                }

            protected:
            }; // End of class SpatialMap

            class Cluster : public Directive {
            public:
                Cluster(int size, ClusterType type) : size_(size), type_(type) {
                }

                virtual DirectiveClass GetClass() {
                    return DirectiveClass::Cluster;
                }

                virtual int GetSize() {
                    return size_;
                }

                virtual int GetOfs() {
                    return 0;
                }

                virtual std::string GetVariable() {
                    return "";
                }

                virtual ClusterType GetAllocType () {
                    return type_;
                }

                virtual std::string ToString() {
                    std::string type_str;
                    if(type_ == ClusterType::Logical) {
                        type_str = DFSL::dataflow_cluster_type_logical_;
                    }
                    else {
                        type_str = DFSL::dataflow_cluster_type_physical_;
                    }
                    std::string ret = "Cluster(" + std::to_string(size_) + "," + type_str + ")";
                    return ret;
                }

                virtual void SetVariable(std::string new_var) {
                }

                virtual void SetSize(int new_size) {
                    size_ = new_size;
                }

                virtual void SetOfs(int new_ofs) {
                }

            protected:
                int size_;
                ClusterType type_ = ClusterType::Logical;
            };// End of class Cluster

        }; // End of namespace directive
    }; // End of namespace DFA
}; // End of namespace maestro

#endif
