#/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com>
#
# Continuous integration
#
# @author Erez Geva <ErezGeva2@@gmail.com>
# @copyright © 2025 Erez Geva
#
###############################################################################
out()
{
 local -r h="`printf '#%.0s' {1..62}`"
 local -r t="$@"
 local -i l="${#t}"
 local pd
 if (( l % 2 == 1 )); then
   (( l++ ))
   pd=' '
 fi
 local -ri p="(54 - l) / 2"
 printf "%s\n####%*s%s%s%*s####\n%s\n" "$h" $p ' ' "$t" "$pd" $p ' ' "$h"
}
run_ci()
{
 out "Install Google Test"
 DEBIAN_FRONTEND=noninteractive sudo apt-get install -y --no-install-recommends libgtest-dev
 set -e # Exit with error
 out "Build with GCC"
 make clean
 make CFLAGS=-Werror utest
 make CFLAGS=-Werror
 out "Build with Clang"
 make clean
 make CFLAGS=-Werror CC=clang CXX=clang++ utest
 make CFLAGS=-Werror CC=clang CXX=clang++
}
main()
{
 local -r base_dir="$(realpath "$(dirname "$0")/..")"
 cd "$base_dir"
 run_ci
}
main "$@"
