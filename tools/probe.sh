#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com>
#
# @author Erez Geva <ErezGeva2@@gmail.com>
# @copyright © 2025 Erez Geva
#
###############################################################################
probe_func()
{
  local key="$1" inc="$2" body="$3" binc allIncs
  for n in $inc; do binc+="#include <$n.h>\n";done
  allIncs=$(printf "$binc")
  if false; then
    echo "==== $key ===="
    printf "%s\nint main(){%s;return 0;}" "$allIncs" "$body" |\
       $CC -pipe -S -x c - -Werror -o $temp_c
  fi
  if printf "%s\nint main(){%s;return 0;}" "$allIncs" "$body" |\
     $CC -pipe -S -x c - -Werror -o $temp_c &> /dev/null; then
      echo "#define HAVE_$key 1" >> $out_h
  else
      echo "/* #undef HAVE_$key */" >> $out_h
  fi
}
probe_headers()
{
  [[ -n "$CC" ]] || local -r CC=cc
  local -r temp_c=tmp.c out_h=config.h
  echo "#ifndef __CSPTP_CONFIG_H_" > $out_h
  echo "#define __CSPTP_CONFIG_H_" >> $out_h
  # Standard C headers
  local list='stdbool threads'
  # POSIX headers
  list+=' unistd pthread syslog strings fcntl poll
         netdb endian sys/stat sys/socket sys/types
         arpa/inet net/if netinet/in'
  # GNU headers
  list+=' ifaddrs getopt sys/ioctl'
  local n m u
  for n in $list; do
  # u=${n^^} BASH 4
  u="$(echo "$n" | tr '[:lower:]' '[:upper:]')"
  m=${u/\//_}
  if printf "#include <$n.h>" | $CC -pipe -E -x c - -o $temp_c &> /dev/null; then
      echo "#define HAVE_${m}_H 1" >> $out_h
  else
      echo "/* #undef HAVE_${m}_H */" >> $out_h
  fi
  done
  probe_func 'DECL_GETLINE' 'stdio' 'char *l;size_t s;ssize_t r=getline(&l,&s,stdout)'
  probe_func 'DECL_REALPATH' 'stdlib' 'char b[1],*a=realpath("X",b)'
  probe_func 'DECL_HTONLL' 'arpa/inet' 'uint64_t v,r=htonll(v)'
  probe_func 'TM_GMTOFF' 'time' 'struct tm l;long o=l.tm_gmtoff'
  echo "#endif" >> $out_h
  rm -f $temp_c
}
main()
{
 local -r base_dir="$(realpath "$(dirname "$0")/..")"
 cd "$base_dir"
 probe_headers
}
main "$@"
