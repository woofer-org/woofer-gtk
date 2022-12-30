#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# gdb  This file is part of Woofer GTK
# Copyright (C) 2021, 2022  Quico Augustijn
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed "as is" in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  If your
# computer no longer boots, divides by 0 or explodes, you are the only
# one responsible.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# version 3 along with this program.  If not, see
# <https://www.gnu.org/licenses/gpl-3.0.html>.
#
#
# Run the compiled application using the GNU Debugger.

ARGS=""
PRG="bin/woofer-gtk"

export LD_LIBRARY_PATH=/usr/local:${LD_LIBRARY_PATH}

export G_ENABLE_DIAGNOSTIC=1
export G_MESSAGES_DEBUG=all
export G_DEBUG=fatal-criticals,fatal-warnings,gc-friendly,resident-modules
export G_SLICE=always-malloc,debug-blocks
export GOBJECT_DEBUG=instance-count

gdb --args ${PRG} ${ARGS} "${@}"

exit $?
