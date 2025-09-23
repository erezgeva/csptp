#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com>
#
# @author Erez Geva <ErezGeva2@@gmail.com>
# @copyright © 2025 Erez Geva
#
# This script is for testing!
# User can execute 'csptp_client' directly.
#
###############################################################################
case `hostname` in
    erezlap)
        SRV_ADDR=bbb
        ;;
    BeagleBoneBlack)
        SRV_ADDR=erezlap
        ;;
esac
./csptp_client -ye -l log_info -d "$SRV_ADDR"
