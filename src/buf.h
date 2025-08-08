/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief buffer object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_BUF_H_
#define __CSPTP_BUF_H_

#include "src/common.h"

typedef struct buffer_t *pbuffer;
typedef const struct buffer_t *pcbuffer;

struct buffer_t {
    void *_buffer; /**> The actual buffer */
    size_t _size; /**> Buffer size */
    size_t _len; /**> Length of data in buffer */

    /**
     * Free this buffer object
     * @param[in, out] self buffer object
     */
    void (*free)(pbuffer self);
    /**
     * Get pointer to actual buffer
     * @param[in] self buffer object
     * @return pointer to actual buffer
     */
    uint8_t *(*getBuf)(pcbuffer self);
    /**
     * Set buffer data length
     * @param[in, out] self buffer object
     * @param[in] len new legth
     * @return true if length is valid
     */
    bool (*setLen)(pbuffer self, size_t len);
    /**
     * Get buffer data length
     * @param[in] self buffer object
     * @return buffer data length
     */
    size_t (*getLen)(pcbuffer self);
    /**
     * Get buffer size available to use
     * @param[in] self buffer object
     * @return buffer size
     */
    size_t (*getSize)(pcbuffer self);
    /**
     * Copy from other buffer
     * @param[in, out] self buffer object
     * @param[in] other buffer object to copy from
     * @return true on success
     */
    bool (*copy)(pbuffer self, pcbuffer other);

    /**
     * Spawn a copy buffer object
     * @param[in] self buffer
     * @return pointer to a new duplicate of buffer object or null
     * @note we only copy the data
     */
    pbuffer(*spawn)(pcbuffer self);

    /**
     * Resize buffer
     * @param[in] self buffer
     * @param[in] newSize buffer new size
     * @return true on success
     * @note function do not shrink rather return false.
     */
    bool (*resize)(pbuffer self, size_t newSize);
};

/**
 * Allocate a buffer object
 * @param[in] size of buffer
 * @return pointer to a new buffer object or null
 */
pbuffer buffer_alloc(size_t size);

#endif /* __CSPTP_BUF_H_ */
