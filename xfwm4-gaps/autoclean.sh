#!/bin/sh
#
# vi:set et ai sw=2 sts=2 ts=2: */
#-
# Copyright (c) 2011 Olivier Fourdan
#
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public 
# License along with this program; if not, write to the Free 
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autoclean.sh: You don't seem to have the Xfce development tools installed on
              your system, which are required to build this software.
              Please install the xfce4-dev-tools package first, it is 
	      available from http://www.xfce.org/.
EOF
  exit 1
}

exec xdt-autogen clean
