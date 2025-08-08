/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief buffer object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/buf.h"
#include "src/log.h"

static void _free(pbuffer self)
{
    if(UNLIKELY_COND(self != NULL)) {
        free(self->_buffer);
        free(self);
    }
}
static uint8_t *_getBuf(pcbuffer self)
{
    return UNLIKELY_COND(self == NULL) ? NULL : self->_buffer;
}
static bool _setLen(pbuffer self, size_t len)
{
    if(UNLIKELY_COND(self == NULL)) {
        log_err("buffer does not exist");
        return false;
    }
    if(len > self->_size) {
        log_warning("length exceed buffer size");
        return false;
    }
    self->_len = len;
    return true;
}
static size_t _getLen(pcbuffer self)
{
    return UNLIKELY_COND(self == NULL) ? 0 : self->_len;
}
static size_t _getSize(pcbuffer self)
{
    return UNLIKELY_COND(self == NULL) ? 0 : self->_size;
}
static bool _copy(pbuffer self, pcbuffer other)
{
    size_t len;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(other == NULL) {
        log_err("other buffer does not exist");
        return false;
    }
    if(self->_size < other->_len) {
        log_warning("buffer to small to copy");
        return false;
    }
    len = other->_len;
    self->_len = len;
    if(len > 0)
        memcpy(self->_buffer, other->_buffer, len);
    return true;
}
static void _asg(pbuffer self, void *m, size_t size, size_t len);
static pbuffer _spawn(pcbuffer self)
{
    size_t size, len;
    if(UNLIKELY_COND(self == NULL))
        return NULL;
    size = self->_size;
    len = self->_len;
    if(UNLIKELY_COND(size == 0))
        return NULL;
    if(len > size) {
        log_err("buffer length exceed its size");
        return NULL;
    }
    void *m = malloc(size);
    if(m == NULL) {
        log_err("memory allocation failed");
        return NULL;
    }
    pbuffer ret = malloc(sizeof(struct buffer_t));
    if(ret == NULL) {
        log_err("memory allocation failed");
        free(m);
    } else {
        _asg(ret, m, size, len);
        if(len > 0)
            memcpy(m, self->_buffer, len);
    }
    return ret;
}
bool _resize(pbuffer self, size_t newSize)
{
    void *m;
    if(UNLIKELY_COND(self == NULL))
        return NULL;
    if(newSize < self->_size) {
        log_warning("buffer shrink ignored");
        return false;
    }
    if(newSize == self->_size) {
        log_debug("Resize to same size");
        return true;
    }
    m = realloc(self->_buffer, newSize);
    if(m == NULL) {
        log_err("memory reallocation failed");
        return false;
    }
    self->_buffer = m;
    self->_size = newSize;
    return true;
}
static void _asg(pbuffer ret, void *m, size_t size, size_t len)
{
#define asg(a) ret->a = _##a
    asg(free);
    asg(getBuf);
    asg(setLen);
    asg(getLen);
    asg(getSize);
    asg(copy);
    asg(spawn);
    asg(resize);
    ret->_buffer = m;
    ret->_size = size;
    ret->_len = len;
}
pbuffer buffer_alloc(size_t size)
{
    if(size == 0) {
        log_warning("size is zero");
        return NULL;
    }
    void *m = malloc(size);
    if(m == NULL) {
        log_err("memory allocation failed");
        return NULL;
    }
    pbuffer ret = malloc(sizeof(struct buffer_t));
    if(ret == NULL) {
        log_err("memory allocation failed");
        free(m);
    } else
        _asg(ret, m, size, 0);
    return ret;
}
