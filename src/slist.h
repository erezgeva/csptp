/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief single liked list
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_SLIST_H_
#define __CSPTP_SLIST_H_

#include "src/mutex.h"

typedef struct node_t *pnode;
typedef struct s_link_list_t *pslist;
typedef struct s_link_list_mgr_t *pslistmgr;
typedef int (*sfunc_f)(void *data, void *cookie);

struct node_t {
    pnode _nxt;
    uint8_t _data[];
};

struct s_link_list_t {
    pnode _head;
};

struct s_link_list_mgr_t {
    size_t _count; /**> Number of used nodes */
    size_t _dataSize; /**> data size of node */
    size_t _freeCount; /**> Number of free nodes */
    pmutex _mtx; /**> mutex for all lists */
    struct s_link_list_t _freeList; /**> List containing free nodes */
    /**
     * Compare callback.
     * compare cookie used to set a new node with current nodes in list
     * @param[in] data of node to compare to
     * @param[in] cookie passed through calling method
     * @return 1 if cookie is larger than node
     *         0 if cookie is equal to node
     *        -1 if cookie is smaller than node
     * @note caller can use reverse logic to sort list in descending order
     */
    int (*_cmp)(void *data, void *cookie);
    /**
     * Set callback
     * Set the node data based on cookie
     * @param[out] data of node
     * @param[in] cookie passed through calling method
     * @return the return value is ignored
     * @note the 'set' callback can be called several times for the same node.
     *       The node can be reused through the free nodes list.
     *       You can reuse allocated memories inside the node.
     *       You must manage it yourself!
     */
    int (*_set)(void *data, void *cookie);
    /**
     * CleanUp callback
     * Travers a list and remove nodes that pass an condition
     * @param[in] data of node
     * @param[in] cookie passed through calling method
     * @return 1 the node pass the condition and should be removed
     *         0 otherwise
     */
    int (*_cleanup)(void *data, void *cookie);
    /**
     * Release callback
     * When freeing a node called to ensure any allocation
     * done during set, be released
     * @param[in] data of node
     * @param[in] reuse pointer to boolean reuse flag
     *  true:  if node will be reused
     *  false: if node is free.
     * @return the return value is ignored
     * @note caller must use information stored in the node.
     * @note the node allocation and free is done by manager.
     *       Any additional allocations done during 'set' callback,
     *       Should be released here!
     * @note if node is reused, you may leave the allocations on the node,
     *       be ware you need to manage it properly when 'set' is called later!
     */
    int (*_release)(void *data, void *reuse);

    /**
     * Free this object
     * And free all nodes in the free list!
     * @param[in, out] self object
      */
    void (*free)(pslistmgr self);

    /**
     * Free a single link list from all nodes
     * @param[in, out] self object
     * @param[in, out] list to free
     * @param[in] useFreeList pass nodes to free list if true
     */
    void (*freeList)(pslistmgr self, pslist list, bool useFreeList);

    /**
     * Update a node in list and create it if needed.
     * @param[in, out] self object
     * @param[in, out] list to use the node
     * @param[in] cookie from caller, to pass to callbacks
     * @return true if node is updated or create
     */
    bool (*updateNode)(pslistmgr self, pslist list, void *cookie);

    /**
     * Fetch node from list.
     * @param[in, out] self object
     * @param[in, out] list to search
     * @param[in] cookie from caller, to pass to callbacks
     * @return pointer to node or null if node does not exist.
     * @note add nodes from free nodes list if available.
     */
    void *(*fetchNode)(pslistmgr self, pslist list, void *cookie);

    /**
     * CleanUp lookup for nodes that are not used for a long timn from list.
     * determined by the cleanup callback
     * @param[in, out] self object
     * @param[in, out] list to use the node
     * @param[in] cookie from caller, to pass to callbacks
     * @return number of nodes removed from the list
     */
    size_t (*cleanUpNodes)(pslistmgr self, pslist list, void *cookie);

    /**
     * Get nunber of nodes in the free nodes list.
     * @param[in] self object
     * @return nunber of node in the free nodes list.
     */
    size_t (*getFreeNodes)(pslistmgr self);

    /**
     * Get nunber of used node by all list using 'updateNode'.
     * @param[in] self object
     * @return nunber of used nodes.
     */
    size_t (*getUsedNodes)(pslistmgr self);
};

/**
 * Allocate a new single link lists manager object
 * @param[in] dataSize size of data to store in node
 * @param[in] cmp compare function
 * @param[in] set function that set node
 * @param[in] cleanup function that predict nodes for cleanup
 * @param[in] release function free memory of any allocation done during setting
 * @return pointer to a new single link lists manager or null
 * @note If set do not allocate, release callback can be NULL
 */
pslistmgr pslistmgr_alloc(size_t dataSize, sfunc_f cmp, sfunc_f set,
    sfunc_f cleanup, sfunc_f release);

#endif /* __CSPTP_SLIST_H_ */
