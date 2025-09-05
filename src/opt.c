/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief mapping options for configuration file and command line
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/opt.h"

#include <strings.h>
#include <math.h>
#include <errno.h>
#include <getopt.h> /* GNU */

static inline bool toInt64(const char *s, int64_t *i)
{
    char *end;
    long long r;
    errno = 0;
    r = strtoll(s, &end, 0);
    if(*end != 0 || errno == ERANGE)
        return false;
    *i = (int64_t)r;
    return true;
}
static inline bool toFloat(const char *s, long double *f)
{
    char *end;
    long double r = strtold(s, &end);
    if(*end != 0 || r == HUGE_VALL)
        return false;
    *f = r;
    return true;
}
#define LOOP_REC_START do{size_t i = 0;poptrl c = self->_rcs;\
        do { pcrec a = c->rcs + i
#define LOOP_REC_END } while(nextRec(&c, &i));}while(false)
static inline bool nextRec(poptrl *cur, size_t *idx)
{
    size_t i = *idx + 1;
    poptrl c = *cur;
    if(c->rcs[i].key == NULL) {
        c = c->nxt;
        if(c == NULL)
            return false;
        i = 0;
        *cur = c;
    }
    *idx = i;
    return true;
}
static inline bool parse_val(pcrec r, poptv v, const char *s, uint8_t fl)
{
    int64_t i;
    long double f;
    int (*scmp)(const char *, const char *);
    /* in case we do not replace old value return true */
    if((fl & OPT_REPLACE) == 0 && v->isSet)
        return true;
    if(r->type == CFG_BOOL && (fl & OPT_BOOL_DEF) > 0) {
        /* Use inverse of default value for boolean value */
        v->val.i = r->def.i == 0;
        v->isSet = true;
        return true;
    }
    /* We muss have somthing to parse */
    if(s == NULL || *s == 0)
        return false;
    if((fl & OPT_CASE_CMP) > 0)
        scmp = strcmp;
    else
        scmp = strcasecmp;
    switch(r->type) {
        case CFG_STR:
            if(v->isAlloc)
                free(v->val.s);
            switch(fl & OPT_STR_MASK) {
                case OPT_STR_ST:
                    /* Use staticly allocated string */
                    v->val.s = (char *)s;
                    v->isAlloc = false;
                    break;
                case OPT_STR_DUP:
                    /* Duplicate string, string is probably temporarly allicated */
                    v->val.s = strdup(s);
                    v->isAlloc = v->val.s != NULL;
                    break;
                case OPT_STR_PASS:
                    /* Pass dynamic allocated string */
                    v->val.s = (char *)s;
                    v->isAlloc = true;
                    break;
            }
            break;
        case CFG_INT:
            if(!toInt64(s, &i) || i < r->min.i || i > r->max.i)
                return false;
            v->val.i = i;
            break;
        case CFG_FLOAT:
            if(!toFloat(s, &f) || f < r->min.f || f > r->max.f)
                return false;
            v->val.f = f;
            break;
        case CFG_ENUM:
            if(toInt64(s, &i)) {
                if(i < r->min.i || i > r->max.i)
                    return false;
                v->val.i = i;
            } else {
                if(r->func == NULL)
                    return false;
                for(i = r->min.i; i <= r->max.i; i++) {
                    const char *a = r->func(i);
                    if(a != NULL && scmp(s, a) == 0) {
                        v->val.i = i;
                        v->isSet = true;
                        return true;
                    }
                }
                return false;
            }
            break;
        case CFG_BOOL:
            if(toInt64(s, &i))
                v->val.i = i != 0;
            else if(strcasecmp(s, "flase") == 0 || strcasecmp(s, "no") == 0 ||
                strcasecmp(s, "off") == 0 || strcasecmp(s, "none") == 0 ||
                strcasecmp(s, "null") == 0)
                v->val.i = false;
            else if(strcasecmp(s, "true") == 0 || strcasecmp(s, "on") == 0 ||
                strcasecmp(s, "yes") == 0)
                v->val.i = true;
            else
                return false;
            break;
        default:
            return false;
    }
    v->isSet = true;
    return true;
}

static void _free(popt self)
{
    if(self != NULL) {
        poptrl n, c = self->_rcs;
        free(self->_getoptBuff);
        free(self->_longOptions);
        while(c != NULL) {
            for(size_t i = 0; c->rcs[i].key != NULL; i++) {
                poptv v = c->vals + i;
                if(v->isAlloc)
                    free(v->val.s);
            }
            n = c->nxt;
            free(c);
            c = n;
        }
        free(self);
    }
}
static bool _addRecords(popt self, pcrec r)
{
    if(LIKELY_COND(self != NULL) && r != NULL) {
        poptrl n;
        size_t i = 0, sz;
        /* Count records */
        for(i = 0; r[i].key != NULL; i++);
        sz = i * sizeof(struct opt_rec_set_t);
        n = malloc(sizeof(struct opt_rec_list_t) + sz);
        if(n == NULL)
            return false;
        n->nxt = NULL;
        n->rcs = r;
        n->vals = (poptv)(n + 1);
        memset(n->vals, 0, sz);
        if(self->_rcs != NULL) {
            poptrl c;
            /* Find last records array  */
            for(c = self->_rcs; c->nxt != NULL; c = c->nxt);
            c->nxt = n;
        } else
            self->_rcs = n; /* First records array */
        self->_numRecords += i;
        return true;
    }
    return false;
}
static const char *_getopt(popt self, const char *p)
{
    char *r;
    size_t s;
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL))
        return NULL;
    if(UNLIKELY_COND(self->_getoptBuff != NULL))
        return self->_getoptBuff;
    s = (p == NULL || *p == 0) ? 0 : strlen(p);
    /* reserve enough space for the ':' */
    r = malloc(self->_numRecords * 2 + 1 + s);
    if(r == NULL)
        return NULL;
    if(p != NULL && *p != 0)
        strcpy(r, p);
    self->_getoptBuff = r;
    if(self->_rcs == NULL)
        return r;
    LOOP_REC_START;
    if(a->sKey > 0) {
        if(a->type != CFG_BOOL) {
            r[s++] = a->sKey;
            r[s++] = ':';
        } else
            r[s++] = a->sKey;
    }
    LOOP_REC_END;
    r[s] = 0; /* Null termination */
    return r;
}
static void *_getoptLong(popt self, void *ad)
{
    size_t l = 0;
    struct option *r;
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL))
        return NULL;
    if(UNLIKELY_COND(self->_longOptions != NULL))
        return self->_longOptions;
    if(ad != NULL) {
        r = (struct option *)ad;
        /* count additional options */
        for(; r[l].name != NULL && r[l].val > 0; l++);
    }
    r = malloc((self->_numRecords + l + 1) * sizeof(struct option));
    if(r != NULL) {
        self->_longOptions = r;
        if(l > 0)
            memcpy(r, ad, l * sizeof(struct option));
        LOOP_REC_START;
        if(a->sKey > 0) {
            r[l].name = a->key;
            r[l].has_arg = a->type != CFG_BOOL;
            r[l].flag = NULL;
            r[l].val = a->sKey;
            l++;
        }
        LOOP_REC_END;
        /* Add termination */
        r[l].name = NULL;
        r[l].has_arg = false;
        r[l].flag = NULL;
        r[l].val = 0;
    }
    return r;
}
static bool _printHelp(pcopt self, FILE *o, int in)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL))
        return false;
    LOOP_REC_START;
    if(a->sKey > 0) {
        const char *m = a->helpMsg;
        const char *e = strpbrk(m, "\r\n");
        if(e != NULL) {
            int l = e - m;
            fprintf(o, "%*s-%c  %.*s\n", in, " ", a->sKey, l, m);
            for(;;) {
                m = e + 1;
                e = strpbrk(m, "\r\n");
                if(e == NULL)
                    break;
                l = e - m;
                if(l > 0)
                    fprintf(o, "%*s    %.*s\n", in, " ", l, m);
            };
            /* Last line can be without new line*/
            if(*m != 0)
                fprintf(o, "%*s    %s\n", in, " ", m);
        } else
            fprintf(o, "%*s-%c  %s\n", in, " ", a->sKey, m);
    }
    LOOP_REC_END;
    return true;
}
static bool _parseSkey(popt self, int o, const char *s, uint8_t fl)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || o == 0))
        return false;
    LOOP_REC_START;
    if(a->sKey == o)
        return parse_val(a, c->vals + i, s, fl);
    LOOP_REC_END;
    return false;
}
static bool _parseKey(popt self, const char *k, const char *s, uint8_t fl)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || k == NULL || *k == 0))
        return false;
    LOOP_REC_START;
    if(a->key[0] > 0 && strcmp(k, a->key) == 0)
        return parse_val(a, c->vals + i, s, fl);
    LOOP_REC_END;
    return false;
}
static bool _getValSkey(pcopt self, int o, precval vl)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || o == 0 || vl == NULL))
        return false;
    LOOP_REC_START;
    if(a->sKey == o) {
        poptv v = c->vals + i;
        if(v->isSet)
            *vl = v->val;
        else
            *vl = a->def;
        return true;
    }
    LOOP_REC_END;
    return false;
}
static bool _getValKey(pcopt self, const char *k, precval vl)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || k == NULL || *k == 0 ||
            vl == NULL))
        return false;
    LOOP_REC_START;
    if(a->key[0] > 0 && strcmp(k, a->key) == 0) {
        poptv v = c->vals + i;
        if(v->isSet)
            *vl = v->val;
        else
            *vl = a->def;
        return true;
    }
    LOOP_REC_END;
    return false;
}
static const char *_getKey(pcopt self, int o)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || o == 0))
        return NULL;
    LOOP_REC_START;
    if(a->sKey == o)
        return a->key;
    LOOP_REC_END;
    return NULL;
}
static bool _isSkey(pcopt self, int o)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || o == 0))
        return false;
    LOOP_REC_START;
    if(a->sKey == o)
        return true;
    LOOP_REC_END;
    return false;
}
static bool _isKey(pcopt self, const char *k)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || k == NULL || *k == 0))
        return false;
    LOOP_REC_START;
    if(a->key[0] > 0 && strcmp(k, a->key) == 0)
        return true;
    LOOP_REC_END;
    return false;
}
static optType _skeyType(pcopt self, int o)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || o == 0))
        return CFG_INV;
    LOOP_REC_START;
    if(a->sKey == o)
        return a->type;
    LOOP_REC_END;
    return CFG_INV;
}
static optType _keyType(pcopt self, const char *k)
{
    if(UNLIKELY_COND(self == NULL || self->_rcs == NULL || k == NULL || *k == 0))
        return CFG_INV;
    LOOP_REC_START;
    if(a->key[0] > 0 && strcmp(k, a->key) == 0)
        return a->type;
    LOOP_REC_END;
    return CFG_INV;
}
popt opt_alloc()
{
    popt ret = malloc(sizeof(struct opt_t));
    if(ret != NULL) {
        ret->_numRecords = 0;
#define asgN(a) ret->_##a = NULL
        asgN(getoptBuff);
        asgN(longOptions);
        asgN(rcs);
#define asg(a) ret->a = _##a
        asg(free);
        asg(addRecords);
        asg(getopt);
        asg(getoptLong);
        asg(printHelp);
        asg(parseSkey);
        asg(parseKey);
        asg(getValSkey);
        asg(getValKey);
        asg(getKey);
        asg(isSkey);
        asg(isKey);
        asg(skeyType);
        asg(keyType);
    }
    return ret;
}
