/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief configuration file parser
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/cfg.h"
#include "src/log.h"
#include <ctype.h>

#define SEC_KEY_SIZE (64)
#define VAL_SIZE (1024)

static inline bool _parseSection(pcfg self, const char *sec, size_t l)
{
    void *ret;
    char b[SEC_KEY_SIZE];
    /* strip leading spaces of section name */
    while(*sec != 0 && l > 0 && isspace(*sec)) {
        sec++;
        l--;
    }
    if(*sec == 0 || l == 0) /* Nothing after the section open */
        return false;
    l--; /* Convert length to last character location */
    /* strip following spaces of line */
    while(l > 0 && isspace(sec[l]))
        l--;
    if(sec[l] != ']') /* We must be at the section end character */
        return false;
    l--; /* Skip the section end character */
    /* strip following spaces of section name */
    while(l > 0 && isspace(sec[l]))
        l--;
    /* strip section name quotation marks */
    if(l > 1 && ((sec[0] == '"' && sec[l] == '"') ||
            (sec[0] == '\'' && sec[l] == '\''))) {
        sec++;
        l -= 2;
    }
    l++; /* convert back to string size */
    if(l >= SEC_KEY_SIZE) {
        log_warning("section name exceed limit %zu", l);
        return false;
    }
    memcpy(b, sec, l);
    b[l] = 0;
    ret = self->_setSection(b, self->_cookie);
    if(ret == NULL)
        return false;
    self->_curSec = ret;
    return true;
}
static inline bool _parseKeyVal(pccfg self, const char *line, size_t len)
{
    size_t l;
    char key[SEC_KEY_SIZE];
    char val[VAL_SIZE];
    const char *v = strchr(line, '=');
    if(v == NULL)
        return false;
    l = v - line; /* length of key */
    len -= l; /* length of value */
    l--; /* Convert key length to last character of key location */
    /* strip following spaces of key */
    while(l > 0 && isspace(line[l]))
        l--;
    /* strip key quotation marks */
    if(l > 1 && ((line[0] == '"' && line[l] == '"') ||
            (line[0] == '\'' && line[l] == '\''))) {
        line++;
        l -= 2;
    }
    l++; /* convert back to key string size */
    if(l >= SEC_KEY_SIZE) {
        log_warning("key exceed limit %zu", l);
        return false;
    }
    memcpy(key, line, l);
    key[l] = 0;
    v++; /* Skip the equal sign */
    len--;
    /* strip leading spaces of value */
    while(len > 0 && isspace(*v)) {
        v++;
        len--;
    }
    if(*v == 0 || len == 0) /* Empty value string */
        return false;
    l = len - 1; /* last character of value location */
    /* strip following spaces of value */
    while(l > 0 && isspace(v[l]))
        l--;
    l++; /* convert to value string size */
    if(l >= VAL_SIZE) {
        log_warning("value exceed limit %zu", l);
        return false;
    }
    memcpy(val, v, l);
    val[l] = 0;
    return self->_setKey(self->_curSec, key, val, self->_cookie);
}
static void _free(pcfg self)
{
    if(LIKELY_COND(self != NULL))
        free(self);
}
static bool _parseLine(pcfg self, const char *line, size_t len)
{
    if(LIKELY_COND(self != NULL) && line != NULL) {
        if(len == 0)
            len = strlen(line);
        /* strip leading spaces */
        while(*line != 0 && len > 0 && isspace(*line)) {
            line++;
            len--;
        }
        if(len == 0) /* Empty line */
            return true;
        switch(*line) {
            case 0:
            case '#':
                /* Empty lines and comment lines are ignored */
                return true;
            case '[':
                return self->_setSection != NULL && _parseSection(self, ++line, --len);
            default:
                return _parseKeyVal(self, line, len);
        }
    }
    return false;
}
static bool _parseBuf(pcfg self, const char *buf)
{
    if(UNLIKELY_COND(self == NULL) || buf == NULL)
        return false;
    const char *last = buf;
    while(*last > 0) {
        const char *end = strpbrk(last, "\r\n");
        size_t l = end - last;
        if(l > 0) {
            if(!_parseLine(self, last, l))
                return false;
        }
        last = end + 1;
    }
    return true;
}
bool _parseFile(pcfg self, const char *name)
{
    FILE *in;
    bool ret = false;
    #ifdef HAVE_DECL_GETLINE
    ssize_t r;
    size_t size;
    char *line = NULL;
    #else
    char line[2000];
    #endif
    if(UNLIKELY_COND(self == NULL) || name == NULL)
        return false;
    in = fopen(name, "r");
    if(in == NULL) {
        logp_err("Fail open %s", name);
        return false;
    }
    #ifdef HAVE_DECL_GETLINE
    while((r = getline(&line, &size, in)) > 0) {
        if(!_parseLine(self, line, r))
    #else
    while(fgets(line, sizeof(line), in) != NULL) {
        if(!_parseLine(self, line, 0))
    #endif
            goto err;
    }
    ret = true;
err:
    #ifdef HAVE_DECL_GETLINE
    free(line);
    #endif
    fclose(in);
    return ret;
}
pcfg cfg_alloc(setSection_t setSection, setKey_f setKey, void *cookie)
{
    if(setKey == NULL)
        return NULL;
    pcfg ret = malloc(sizeof(struct cfg_t));
    if(ret != NULL) {
        ret->_curSec = NULL;
#define asgV(a) ret->_##a = a
        asgV(setSection);
        asgV(setKey);
        asgV(cookie);
#define asg(a) ret->a = _##a
        asg(free);
        asg(parseLine);
        asg(parseBuf);
        asg(parseFile);
    }
    return ret;
}
const char *cfg_rmStrQuote(const char *v)
{
    size_t l;
    char *r;
    if(v == NULL)
        return NULL;
    l = strnlen(v, VAL_SIZE);
    /* Value maximum is VAL_SIZE - 1 */
    if(l == VAL_SIZE) {
        log_warning("value exceed limit");
        return NULL;
    }
    l--; /* Convert length to last character location */
    r = (char *)v;
    /* strip value string quotation marks */
    if(l > 1 && ((r[0] == '"' && r[l] == '"') ||
            (r[0] == '\'' && r[l] == '\''))) {
        r[l] = 0;
        r++;
    }
    return r;
}
