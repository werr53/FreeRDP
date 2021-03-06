# FreeRDP: A Remote Desktop Protocol Implementation
# FreeRDP Client Common
#
# Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(MODULE_NAME "freerdp-client")
set(MODULE_PREFIX "FREERDP_CLIENT")

#  Policy CMP0022: INTERFACE_LINK_LIBRARIES defines the link
#  interface.  Run "cmake --help-policy CMP0022" for policy details.  Use the
#  cmake_policy command to set the policy and suppress this warning.
if(POLICY CMP0022)
  cmake_policy(SET CMP0022 NEW)
endif()

set(${MODULE_PREFIX}_SRCS
	client.c
	cmdline.c
	compatibility.c
	compatibility.h
	file.c)

foreach(FREERDP_CHANNELS_CLIENT_SRC ${FREERDP_CHANNELS_CLIENT_SRCS})
	get_filename_component(NINC ${FREERDP_CHANNELS_CLIENT_SRC} PATH)
	include_directories(${NINC})
	set(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_SRCS} "${FREERDP_CHANNELS_CLIENT_SRC}")
endforeach()

if(MSVC)
	set(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_SRCS} module.def)
endif()

add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})

if (WITH_LIBRARY_VERSIONING)
  set_target_properties(${MODULE_NAME} PROPERTIES VERSION ${FREERDP_VERSION} SOVERSION ${FREERDP_API_VERSION})
endif()
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "lib")

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS}
	${FREERDP_CHANNELS_CLIENT_LIBS})
	
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS}
	${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})
	
set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE freerdp
	MODULES freerdp-core)

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} winpr)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries EXPORT FreeRDPTargets)

set_property(TARGET ${MODULE_NAME} PROPERTY FOLDER "Client/Common")

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

export_complex_library(LIBNAME ${MODULE_NAME})
