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

if (WIN32 AND NOT DEFINED FXC_EXECUTABLE)

    # locate Win 10 kits
    get_filename_component(kit10_dir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" REALPATH)
    file(GLOB W10SDK_VERSIONS RELATIVE ${kit10_dir}/Include ${kit10_dir}/Include/10.*)          # enumerate pre-release and not yet known release versions
    list(APPEND W10SDK_VERSIONS "10.0.15063.0" "10.0.16299.0" "10.0.17134.0")   # enumerate well known release versions
    list(REMOVE_DUPLICATES W10SDK_VERSIONS)
    list(SORT W10SDK_VERSIONS)  # sort from low to high
    list(REVERSE W10SDK_VERSIONS) # reverse to start from high

    foreach(W10SDK_VER ${W10SDK_VERSIONS})

        set(WINSDK_PATHS "${kit10_dir}/bin/${W10SDK_VER}/x64" "C:/Program Files (x86)/Windows Kits/10/bin/${W10SDK_VER}/x64" "C:/Program Files/Windows Kits/10/bin/${W10SDK_VER}/x64")

        find_program(FXC_EXECUTABLE fxc PATHS ${WINSDK_PATHS} NO_DEFAULT_PATH)

        if (EXISTS ${FXC_EXECUTABLE})

            set(FXC_VERSION "${W10SDK_VER}")
            break()
        endif()

    endforeach()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(FXC
    REQUIRED_VARS
        FXC_EXECUTABLE
    VERSION_VAR
        FXC_VERSION
)

