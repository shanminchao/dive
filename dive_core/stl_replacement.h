/*
 Copyright 2024 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#pragma once

// Provides a replacement of some STL containers. The reason for this is that Windows DEBUG versions
// of STL libraries are notoriously slow (multiple orders-of-magnitude slower than RELEASE), so a
// replacement of simplified and less-safe version of these STL containers is warranted to make
// DEBUG useful. Only the simplest functions are provided here

namespace Dive
{

template<class Type> class Vector
{
public:
    Vector();
    //Vector(Vector<Type> &&a);
    Vector(const Vector<Type> &a);
    Vector(uint64_t size, const Type &a = Type());
    Vector(std::initializer_list<Type> a);
    ~Vector();
    Type    &operator[](uint64_t i) const;
    //Vector<Type> &operator=(const Vector<Type> &a);
    Type    *data() const;
    Type    &back() const;
    uint64_t size() const;
    bool     empty() const;
    void     push_back(const Type &a);
    void     pop_back();

    template<typename... Args> void emplace_back(Args &&...args);

    void resize(uint64_t size, const Type &a = Type());
    void reserve(uint64_t size);
    void clear();

private:
    Type    *m_buffer;
    uint32_t m_reserved;
    uint32_t m_size;
};

}  // namespace Dive

#include "stl_replacement.hpp"

template<typename T>
using DiveVector = std::vector<T>;
// using DiveVector = Dive::Vector<T>;