/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief store time stamps for follow-up messages
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_STORE_H_
#define __CSPTP_STORE_H_

#include "src/sock.h"
#include "src/slist.h"

typedef struct store_t *pstore;
typedef const struct store_t *pcstore;

struct store_t {
    pslist _data; /**> pointer to hash table of lists */
    uint32_t _hashMask; /**> mask for the hash table */
    uint32_t _hashSize; /**> mask for the hash table */
    pslistmgr _mgr; /**> pointer to list manager */
    /**
     * Calculate Hash
     * @param[in, out] self timestamp object
     * @param[in] address of client
     * @return hash value on success
     */
    size_t (*_hash)(pcstore self, pcipaddr address);

    /**
     * Free this timestamp object
     * @param[in, out] self timestamp object
     */
    void (*free)(pstore self);

    /**
     * Update record using address with timestamp
     * @param[in, out] self timestamp object
     * @param[in] address of client
     * @param[in] timestamp to store in record
     * @param[in] sequenceId of the messages used to store the timestamp
     * @param[in] domainNumber used by client
     * @return true on success
     */
    bool (*update)(pstore self, pcipaddr address, pcts timestamp,
        uint16_t sequenceId, uint8_t domainNumber);

    /**
     * Fetch record timestamp
     * @param[in, out] self timestamp object
     * @param[in] address of client
     * @param[out] timestamp to fetch
     * @param[in] clear timestamp
     * @param[in] sequenceId of the messages used to store the timestamp
     * @param[in] domainNumber used by client
     * @return true on success
     * @note return false if record do not exist
     *       clear timestamp means zero value timestamp
     */
    bool (*fetch)(pstore self, pcipaddr address, pts timestamp, uint16_t sequenceId,
        uint8_t domainNumber, bool clear);

    /**
     * cleanup of old records
     * @param[in, out] self timestamp object
     * @param[in] time gap, all nodes before it will be removed
     * @return number of nodes removed from all lists
     * @note Should be called from a different thread!
     */
    size_t (*cleanup)(pstore self, uint32_t time);

    /**
     * Get hash table size
     * @param[in] self timestamp object
     * @return hash table size
     */
    size_t (*getHashSize)(pcstore self);

    /**
     * Get number of recoeds stored
     * @param[in] self timestamp object
     * @return number of recoeds stored
     */
    size_t (*records)(pcstore self);
};

/**
 * Allocate a store object
 * @param[in] protocol to use to store recoreds
 * @param[in] hashSize number of bits of the hash table size (2 ^ hashSize)
 * @return pointer to a new store object or null
 * @note hashSize of zero will NOT use hash table.
 *       hashSize is limit upto 32 bits.
 */
pstore store_alloc(prot protocol, size_t hashSize);

#endif /* __CSPTP_STORE_H_ */
