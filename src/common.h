/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief common header
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_COMMON_H_
#define __CSPTP_COMMON_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <threads.h>

#if 0
References to C attributes:
https://en.cppreference.com/w/c/language/attributes.html
https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html#Attribute-Syntax
https://clang.llvm.org/docs/AttributeReference.html
https://learn.microsoft.com/en-us/cpp/cpp/attributes?view=msvc-170#microsoft-specific-attributes
#endif
#define _PACKED__ __attribute__((packed))

#define LIKELY_COND(_expr) (__builtin_expect((_expr), true))
#define UNLIKELY_COND(_expr) (__builtin_expect((_expr), false))

/* Need 2 levels to stringify macros value instead of macro name */
#define stringify(s) #s
#define stringifyVal(a) stringify(a)

/* stringify enumerator through switch with cases */
#define caseItem(a) a: return #a

#define IPV4_ADDR_LEN (4)  /* sizeof(struct in_addr) */
#define IPV6_ADDR_LEN (16) /* sizeof(struct in_addr6) */

/** ALTERNATE_TIME_OFFSET_INDICATOR displayName maximum length */
#define MAX_TZ_LEN (10)

typedef enum networkProtocol_e {
    Invalid_PROTO = 0,
    UDP_IPv4 = 1,
    UDP_IPv6 = 2,
} prot;

struct UInteger48_t { /* 48 unsigned number */
    /* Order by network */
    uint16_t _high;
    uint32_t _low;
} _PACKED__;

typedef struct UInteger48_t *puint48;
typedef const struct UInteger48_t *pcuint48;

#endif /* __CSPTP_COMMON_H_ */
