/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief configuration file parser
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_CFG_H_
#define __CSPTP_CFG_H_

#include "src/common.h"

typedef struct cfg_t *pcfg;
typedef const struct cfg_t *pccfg;
/**
 * Set current section
 * @param[in] name of section
 * @param[in, out] cookie
 * @return reference to section
 * @note the name string is temporarly allocated by the cfg object.
 */
typedef void *(*setSection_t)(const char *name, void *cookie);
/**
 * Set a key value
 * @param[in] section reference of current section or null for general
 * @param[in] key name
 * @param[in] value
 * @param[in, out] cookie
 * @return true if key is parsed properly
 * @note the key and values strings are temporarly allocated by the cfg object.
 */
typedef bool (*setKey_f)(const void *section, const char *key,
    const char *value, void *cookie);

struct cfg_t {
    setSection_t _setSection; /* Set new section callback */
    setKey_f _setKey; /* Set net key value callback */
    void *_curSec; /* Current used section */
    void *_cookie; /* User cookie */

    /**
     * Free this cfg object
     * @param[in, out] self cfg object
     */
    void (*free)(pcfg self);

    /**
     * Parse line in a configuration file
     * @param[in, out] self cfg object
     * @param[in] line string to parse
     * @param[in] length of string or zero for null terminated string line
     */
    bool (*parseLine)(pcfg self, const char *line, size_t length);

    /**
     * Parse buffer represent a configuration file
     * @param[in, out] self cfg object
     * @param[in] buffer containing a configuration file content with null terminated
     */
    bool (*parseBuf)(pcfg self, const char *buffer);

    /**
     * Parse a configuration file
     * @param[in, out] self cfg object
     * @param[in] file name of a configuration file
     */
    bool (*parseFile)(pcfg self, const char *name);
};

/**
 * Allocate a cfg object
 * @param[in, out] self cfg object
 * @param[in, out] cookie caller cookie to pass to callback
 * @return pointer to a new cfg object or null
 */
pcfg cfg_alloc(setSection_t setSection, setKey_f setKey, void *cookie);

/**
 * Strip quotation marks from string
 * @param[in] value string
 * @return strip string
 * @note the function is a service function for the 'setKey_f' callback
 *       So the callback can handle strings differently then other value types
 * @note the reply is a subset of the same string, the function do not allocate!
 */
const char *cfg_rmStrQuote(const char *value);

#endif /* __CSPTP_CFG_H_ */
