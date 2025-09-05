/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief mapping options for configuration file and command line
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_OPT_H_
#define __CSPTP_OPT_H_

#include "src/common.h"

typedef struct opt_t *popt;
typedef const struct opt_t *pcopt;
typedef struct opt_rec_list_t *poptrl;
typedef struct opt_rec_set_t *poptv;
typedef const struct opt_rec_t *pcrec;
typedef union opt_rec_val_t *precval;

/**
 * Function to convert value to enumerator string
 * @param[in] value to convert and store result
 * @return string or NULL if value is not in enumerator
 */
typedef const char *(*optEnum2Str)(int64_t value);

typedef enum opt_val_type_e {
    CFG_BOOL,  /**> boolean value */
    CFG_STR,   /**> String value */
    CFG_INT,   /**> Integer value */
    CFG_FLOAT, /**> Float value */
    CFG_ENUM,  /**> Enumerator value */
    CFG_INV,
} optType;

/**
 * Handle flags
 */
enum opt_flags_e {
    OPT_STR_ST   = 0,        /* Use the pass string as it staticly stored */
    OPT_STR_DUP  = (1 << 0), /* Duplicate string to a new allocation */
    OPT_STR_PASS = (1 << 1), /* Passed string was allicated, passed and need free */
    OPT_STR_MASK = (OPT_STR_ST | OPT_STR_DUP | OPT_STR_PASS),
    OPT_REPLACE  = (1 << 2), /* Replcae with old value with new one */
    OPT_BOOL_DEF = (1 << 3), /* Use inverse of default value for boolean value */
    OPT_CASE_CMP = (1 << 4), /* Use strict case compare for enumerators */
};

typedef union opt_rec_val_t {
    char *s; /**> string value */
    int64_t i; /**> integer, enumeratore or boolean value */
    long double f; /**> float value */
} optRecVal;

struct opt_rec_set_t {
    optRecVal val; /**> value */
    bool isAlloc; /**> flag indicate value need freeing */
    bool isSet; /**> Value set */
};

struct opt_rec_list_t {
    pcrec rcs; /* records array */
    poptv vals; /* store values */
    poptrl nxt;
};

struct opt_rec_t {
    const char *key; /**> Key name */
    const char sKey; /**> single character or 0 for none */
    const char *helpMsg; /**> help message */
    optType type; /**> Value type */
    optEnum2Str func; /**> callback to convert string to enumerator */
    /**
     * maximum string length of string and enumerators
     * Use zero for unbound strings
     */
    size_t maxLen;
    optRecVal def; /**> Default value */
    optRecVal min; /**> Minimum value of integer, enumerator or float value */
    optRecVal max; /**>* Maximum value of integer, enumerator or float value */
};

#define KEY_STR(_n, _s, _h, _d, _l)\
    { .key = _n, .sKey = _s, .helpMsg = _h, .type = CFG_STR, .maxLen = _l, .def = { .s = (char*)_d }, }
#define KEY_INT(_n, _s, _h, _d, _mi, _ma)\
    { .key = _n, .sKey = _s, .helpMsg = _h, .type = CFG_INT, .def = { .i = _d }, .min = { .i = _mi }, .max = { .i = _ma } }
#define KEY_FLT(_n, _s, _h, _d, _mi, _ma)\
    { .key = _n, .sKey = _s, .helpMsg = _h, .type = CFG_FLOAT, .def = { .f = _d }, .min = { .f = _mi }, .max = { .f = _ma } }
#define KEY_BOOL(_n, _s, _h, _d)\
    { .key = _n, .sKey = _s, .helpMsg = _h, .type = CFG_BOOL, .def = { .i = _d } }
#define KEY_ENUM(_n, _s, _h, _d, _f, _l, _mi, _ma)\
    { .key = _n, .sKey = _s, .helpMsg = _h, .type = CFG_ENUM, .func = _f, .maxLen = _l, .def = { .i = _d }, .min = { .i = _mi }, .max = { .i = _ma } }
#define KEY_LAST { 0 }

#define GET_OPT_TRUE(_sk) (opt->getValSkey(opt, _sk, &v) ? !v.i : true)
#define GET_OPT_FALSE(_sk) (opt->getValSkey(opt, _sk, &v) ? v.i : false)
#define GET_OPT_INT(_sk, _d) (opt->getValSkey(opt, _sk, &v) ? v.i : _d)
#define GET_OPT_STR(_sk) (opt->getValSkey(opt, _sk, &v) ? v.s : NULL)

/**
 * Verify we do not have duplications of keys
 * @param[in] self opt object
 * @param[in] preOptions additional options
 * @param[in] additional_long_options
 * @return true if found any duplication
 * @note This function is used during development!
 */
bool opt_duplicates(pcopt self, const char *preOptions,
    void *additional_long_options);

struct opt_t {
    char *_getoptBuff; /**> Buffer to hold getopt string */
    void *_longOptions; /**> pointer to allocated struct option array */
    size_t _numRecords; /**> Number of records we have */
    poptrl _rcs; /**> List of records */

    /**
     * Free this opt object
     * @param[in, out] self opt object
     */
    void (*free)(popt self);

    /**
     * Free this opt object
     * @param[in, out] self opt object
     * @param[in] records to add
     * @return true on success
     */
    bool (*addRecords)(popt self, pcrec records);

    /**
     * Build option string for getopt
     * All argument but boolean will use argument, mark by adding ':'
     * @param[in, out] self opt object
     * @param[in] preOptions start and additional options
     * @return pointer to buffer or NULL on error
     */
    const char *(*getopt)(popt self, const char *preOptions);

    /**
     * Build long options for getopt_long
     * All argument but boolean will be use argument
     * @param[in, out] self opt object
     * @param[in] additional_long_options
     * @return pointer to array of 'struct option' to be used with 'getopt_long()'
     */
    void *(*getoptLong)(popt self, void *additional_long_options);

    /**
     * Print help with short options
     * @param[in, out] self opt object
     * @param[in] out file decription to print with
     * @param[in] ident to add to all lines
     * @return true for success
     */
    bool (*printHelp)(pcopt self, FILE *out, int ident);

    /**
     * Parse string value and set value
     * @param[in, out] self opt object
     * @param[in] short_key
     * @param[in] str string to parse
     * @param[in] flags
     * @return true for success
     */
    bool (*parseSkey)(popt self, int short_key, const char *str, uint8_t flags);

    /**
     * Parse string value and set value
     * @param[in, out] self opt object
     * @param[in] key name
     * @param[in] str string to parse
     * @param[in] flags
     * @return true for success
     */
    bool (*parseKey)(popt self, const char *key, const char *str, uint8_t flags);

    /**
     * Get value
     * @param[in] self opt object
     * @param[in] short_key
     * @param[in, out] value storage
     * @return true for success
     * @note if value is not set return default value
     */
    bool (*getValSkey)(pcopt self, int short_key, precval value);

    /**
     * Get value
     * @param[in] self opt object
     * @param[in] key name
     * @param[in, out] value storage
     * @return true for success
     * @note if value is not set return default value
     */
    bool (*getValKey)(pcopt self, const char *key, precval value);

    /**
     * Get key name from short key name
     * @param[in] self opt object
     * @param[in] short_key
     * @return key name or null if key does not exist
     */
    const char *(*getKey)(pcopt self, int short_key);

    /**
     * Is short key exist
     * @param[in] self opt object
     * @param[in] short_key
     * @return true if key exist
     */
    bool (*isSkey)(pcopt self, int short_key);

    /**
     * Is key exist
     * @param[in] self opt object
     * @param[in] key name
     * @return true if key exist
     */
    bool (*isKey)(pcopt self, const char *key);

    /**
     * return short key type
     * @param[in] self opt object
     * @param[in] short_key
     * @return key type or CFG_INV if key do not exist
     */
    optType(*skeyType)(pcopt self, int short_key);

    /**
     * return key type
     * @param[in] self opt object
     * @param[in] key name
     * @return key type or CFG_INV if key do not exist
     */
    optType(*keyType)(pcopt self, const char *key);
};

/**
 * Allocate a opt object
 * @return pointer to a new opt object or null
 */
popt opt_alloc();

#endif /* __CSPTP_OPT_H_ */
