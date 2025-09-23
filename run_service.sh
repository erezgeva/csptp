#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com>
#
# @author Erez Geva <ErezGeva2@@gmail.com>
# @copyright © 2025 Erez Geva
#
# This script is for testing!
# User can execute 'csptp_service' directly.
#
###############################################################################
case `hostname` in
    erezlap)
        BIND_IF="-i enp0s25"
        ;;
esac
sudo ./csptp_service -ye -l log_debug $BIND_IF
