/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief checking function for options
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/opt.h"
#include "src/slist.h"

#include <ctype.h>
#include <getopt.h> /* GNU */

static inline bool isValid(int c)
{
    switch(c) {
        case '-':
        case ':': // used to mark argument and used as first character flag
        case ';':
        case '+': // Is used as first character flag
        case 'W': // reserved by POSIX.2 for implementation extensions
            return false;
        default:
            break;
    }
    return isgraph(c) != 0;
}
static int cmp(void *d, void *c)
{
    return strcmp(*(void **)d, c);
}
static int set(void *d, void *c)
{
    *(void **)d = c;
    return 0;
}
/* inline function from src/opt.c */
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
bool opt_duplicates(pcopt self, const char *ps, void *lo)
{
    void *n;
    pslistmgr ml;
    struct option *p;
    bool ret = false;
    uint8_t x[32] = { 0 };
    struct s_link_list_t l = { 0 };
    if(self == NULL || self->_rcs == NULL)
        return false;
    if(ps != NULL) {
        for(; *ps != 0; ps++) {
            if(isgraph(*ps) == 0) {
                printf("'%c'(%d) is invalid in pre additional options.\n", *ps, *ps);
                return false; /* Found a duplication */
            }
            if(isValid(*ps)) {
                lldiv_t d = lldiv(*ps, 8);
                uint8_t m = 1 << d.rem;
                if((x[d.quot] & m) > 0) {
                    printf("'%c' appears twice in pre additional options.\n", *ps);
                    return false; /* Found a duplication */
                }
                x[d.quot] |= m;
            }
        }
    }
    LOOP_REC_START;
    if(a->sKey > 0) {
        lldiv_t d;
        uint8_t m;
        if(!isValid(a->sKey)) {
            printf("'%c'(%d) is invalid.\n", a->sKey, a->sKey);
            return false; /* Found a duplication */
        }
        d = lldiv(a->sKey, 8);
        m = 1 << d.rem;
        if((x[d.quot] & m) > 0) {
            printf("'%c' appears twice.\n", a->sKey);
            return false; /* Found a duplication */
        }
        x[d.quot] |= m;
    }
    LOOP_REC_END;
    ml = pslistmgr_alloc(sizeof(char *), cmp, set, NULL, NULL);
    if(ml == NULL)
        return false;
    p = (struct option *)lo;
    if(p != NULL) {
        for(size_t j = 0; p[j].name != NULL; j++) {
            if(p[j].name[0] != 0) {
                n = ml->fetchNode(ml, &l, (void *)p[j].name);
                if(n != NULL) {
                    printf("'%s' appears twice in additional keys.\n", p[j].name);
                    goto err; /* Found a duplication */
                }
                ml->updateNode(ml, &l, (void *)p[j].name);
            }
        }
    }
    LOOP_REC_START;
    if(a->key[0] != 0) {
        n = ml->fetchNode(ml, &l, (void *)a->key);
        if(n != NULL) {
            printf("'%s' appears twice.\n", a->key);
            goto err; /* Found a duplication */
        }
        ml->updateNode(ml, &l, (void *)a->key);
    }
    LOOP_REC_END;
    ret = true;
err:
    ml->freeList(ml, &l, false);
    ml->free(ml);
    return ret; // No duplication or any issue in keys
}
