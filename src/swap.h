/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief host to network and via versa functions
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_SWAP_H_
#define __CSPTP_SWAP_H_

#include "src/common.h"
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#endif

#ifndef HAVE_DECL_HTONLL
#ifdef HAVE_ENDIAN_H
static inline uint64_t htonll(uint64_t value) {return htobe64(value);}
static inline uint64_t ntohll(uint64_t value) {return be64toh(value);}
#endif /* HAVE_ENDIAN_H */
#endif /* HAVE_DECL_HTONLL */

static inline uint16_t cpu_to_net16(uint16_t value) {return htons(value);}
static inline uint32_t cpu_to_net32(uint32_t value) {return htonl(value);}
static inline uint64_t cpu_to_net64(uint64_t value) {return htonll(value);}
static inline uint16_t net_to_cpu16(uint16_t value) {return ntohs(value);}
static inline uint32_t net_to_cpu32(uint32_t value) {return ntohl(value);}
static inline uint64_t net_to_cpu64(uint64_t value) {return ntohll(value);}

#endif /* __CSPTP_SWAP_H_ */
