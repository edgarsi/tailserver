# Copyright (c) 2006, 2010, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

SET(prefix "${CMAKE_INSTALL_PREFIX}")
SET(libexecdir "${CMAKE_INSTALL_PREFIX}/${INSTALL_SBINDIR}")
SET(bindir "${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}" )
SET(CC ${CMAKE_C_COMPILER})
SET(CFLAGS ${CMAKE_C_FLAGS})

# XXX: shouldn't we just have variables for all this stuff and centralise
# XXX: their configuration in install_layout.cmake?
SET(inst_location ${INSTALL_SUPPORTFILESDIR})

# TODO: So, why do I have this file, really? Even the folder. Maybe move build-tags to cmake/
