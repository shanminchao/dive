###############################################################################
# Copyright (c) 2018-2020 LunarG, Inc.
# Copyright (c) 2021-2023 Advanced Micro Devices, Inc.
# All rights reserved
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Author: LunarG Team
# Author: AMD Developer Tools Team
# Description: CMake script for layer gfxrecon_interceptor target
###############################################################################

add_library(gfxrecon_interceptor SHARED "")
target_sources(gfxrecon_interceptor PRIVATE
                ${CMAKE_CURRENT_LIST_DIR}/dll_main.cpp
                $<$<BOOL:WIN32>:${CMAKE_SOURCE_DIR}/version.rc>)
target_include_directories(gfxrecon_interceptor PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(gfxrecon_interceptor gfxrecon_util)


