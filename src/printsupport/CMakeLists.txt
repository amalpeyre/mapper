#
#    Copyright 2015 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

find_package(Qt5Gui REQUIRED COMPONENTS Private)
find_package(Qt5PrintSupport REQUIRED COMPONENTS Private)
find_package(ZLIB REQUIRED)

if (${Qt5PrintSupport_VERSION} VERSION_LESS 5.3.0)
	set(FORK_VERSION qt-5.2.1)
else()
	set(FORK_VERSION qt-5.5.1)
endif()

set(PRINTSUPPORT_SRCS
  advanced_pdf_printer.cpp
  ${FORK_VERSION}/advanced_pdf.cpp
  ${FORK_VERSION}/printengine_advanced_pdf.cpp
  ${FORK_VERSION}/qfontsubset.cpp
)


add_library(printsupport STATIC ${PRINTSUPPORT_SRCS})

target_include_directories(printsupport
  BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${FORK_VERSION}"
  INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(printsupport
  Qt5::PrintSupport
  Qt5::Gui
  ${ZLIB_LIBRARY}
)


# Workaround Qt private include dir issue
# Cf. https://bugreports.qt.io/browse/QTBUG-37417

foreach(module Core Gui PrintSupport)
	set(qt_module Qt${module})
	set(qt5_module Qt5${module})
	if("${${qt5_module}_PRIVATE_INCLUDE_DIRS}" STREQUAL "")
		foreach(base_dir ${${qt5_module}_INCLUDE_DIRS})
			if("${base_dir}" MATCHES "/${qt_module}\$")
				list(APPEND ${qt5_module}_PRIVATE_INCLUDE_DIRS "${base_dir}/${${qt5_module}_VERSION}/${qt_module}")
			endif()
		endforeach()
	endif()
	target_include_directories(printsupport PRIVATE ${${qt5_module}_PRIVATE_INCLUDE_DIRS})
endforeach()
