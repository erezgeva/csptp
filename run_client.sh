# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com>
#
# @author Erez Geva <ErezGeva2@@gmail.com>
# @copyright © 2025 Erez Geva
#
###############################################################################

[[ -n "$1" ]] && SRV_ADDR="$1" || SRV_ADDR="1.1.1.1"
echo "connect to $SRV_ADDR"
./csptp_client -ye -l log_info -d "$SRV_ADDR"
