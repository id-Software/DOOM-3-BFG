# Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.


find_package(PackageHandleStandardArgs)

if (NOT DXC_SPIRV_EXECUTABLE)
    if (WIN32)
        find_program(DXC_SPIRV_EXECUTABLE dxc PATHS
            "$ENV{VULKAN_SDK}/Bin"
            NO_DEFAULT_PATH)
    else()
        find_program(DXC_SPIRV_EXECUTABLE dxc PATHS
            /usr/bin
            /usr/local/bin)
    endif()

    if (DXC_SPIRV_EXECUTABLE)
        message(STATUS "Found DXC for SPIR-V generation: ${DXC_SPIRV_EXECUTABLE}.")
        message(STATUS "Please note that older versions of the compiler may result in shader compilation errors.")
    endif()
endif()

find_package_handle_standard_args(DXCspirv
    REQUIRED_VARS DXC_SPIRV_EXECUTABLE
    VERSION_VAR DXC_SPIRV_VERSION
    FAIL_MESSAGE "Cannot find a SPIR-V capable DXC executable. Please provide a valid path through the DXC_SPIRV_EXECUTABLE variable."
)
