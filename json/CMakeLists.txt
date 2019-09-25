# Copyright (C) 2019 Veronika Sokova <isokova@fit.vutbr.cz>
#
# This file is part of code listener.
#
# code listener is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# code listener is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with predator.  If not, see <http://www.gnu.org/licenses/>.

# project metadata
cmake_minimum_required(VERSION 2.6)
project(json C CXX)
enable_testing()

include("../build-aux/common.cmake")

option(JSON_DEBUG "Set to ON to enable debugging support" OFF)
if(JSON_DEBUG)
    add_definitions("-O0 -g")
    message (STATUS "Debugging support enabled...")
else()
    add_definitions("-O3 -DNDEBUG")
endif()

# libjson_core.a
add_library(json_core STATIC
    cl_json.cc)

# build compiler plug-in (libjson.so/.dylib)
CL_BUILD_COMPILER_PLUGIN(json json_core ../cl_build)

# make install
install(TARGETS json DESTINATION lib)

