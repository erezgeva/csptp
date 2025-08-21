/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test single liked list objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

extern "C" {
#include "src/slist.h"
}

struct data_t {
  int key;
  int val;
};

static int cmp(void *data, void *cookie)
{
  int ret;
  struct data_t *d = (struct data_t *)cookie;
  struct data_t *nd = (struct data_t *)data;
  ret = d->key > nd->key ? 1 : (d->key < nd->key ? -1 : 0);
  //printf("cmp (%p) %d = %d <=> %d(%d)\n", (uint8_t*)data - 8, ret, nd->key, d->key, d->val);
  return ret;
}
static int set(void *data, void *cookie)
{
  struct data_t *d = (struct data_t *)cookie;
  struct data_t *nd = (struct data_t *)data;
  //printf("set (%p) %d-%d\n", (uint8_t*)data - 8, d->key, d->val);
  nd->key = d->key;
  nd->val = d->val;
  return 0;
}
static int cleanup(void *data, void *cookie)
{
  int ret;
  struct data_t *d = (struct data_t *)cookie;
  struct data_t *nd = (struct data_t *)data;
  ret = nd->val > d->val ? 1 : 0;
  //printf("clr (%p) %d = %d-%d > %d\n", (uint8_t*)data - 8, ret, nd->key, nd->val, d->val);
  return ret;
}

// Debugging tool
static inline void printList(pslist list)
{
  for(pnode cur = list->_head; cur != nullptr; cur = cur->_nxt) {
      struct data_t *d = (struct data_t *)&cur->_data;
      printf("%p) %d-%d\n", cur, d->key, d->val);
  }
}

// Test slist
// void free(pslistmgr self)
// void freeList(pslistmgr self, pslist list, bool useFreeList)
// bool updateNode(pslistmgr self, pslist list, void *cookie)
// void *fetchNode(pcslistmgr self, pslist list, void *cookie)
// void cleanUpNodes(pslistmgr self, pslist list, void *cookie)
// size_t getFreeNodes(pcslistmgr self)
// size_t getUsedNodes(pcslistmgr self)
// pslistmgr pslistmgr_alloc(size_t dataSize, sfunc_f cmp, sfunc_f set, sfunc_f cleanup, sfunc_f release);
TEST(slistTest, slist)
{
  struct data_t d, *f;
  struct s_link_list_t myList = { 0 };
  pslistmgr m = pslistmgr_alloc(sizeof(struct data_t), cmp, set, cleanup, nullptr);
  ASSERT_NE(m, nullptr);
  d = { 2, 4 }; // First node
  EXPECT_TRUE(m->updateNode(m, &myList, &d));
  EXPECT_EQ(m->getFreeNodes(m), 0);
  EXPECT_EQ(m->getUsedNodes(m), 1);
  d = { 4, 2 }; // New tail
  EXPECT_TRUE(m->updateNode(m, &myList, &d));
  EXPECT_EQ(m->getFreeNodes(m), 0);
  EXPECT_EQ(m->getUsedNodes(m), 2);
  d = { 3, 3 }; // In middle
  EXPECT_TRUE(m->updateNode(m, &myList, &d));
  EXPECT_EQ(m->getFreeNodes(m), 0);
  EXPECT_EQ(m->getUsedNodes(m), 3);
  d = { 1, 3 }; // New head
  EXPECT_TRUE(m->updateNode(m, &myList, &d));
  EXPECT_EQ(m->getFreeNodes(m), 0);
  EXPECT_EQ(m->getUsedNodes(m), 4);
  d = { 2, 2 }; // Update node
  EXPECT_TRUE(m->updateNode(m, &myList, &d));
  EXPECT_EQ(m->getFreeNodes(m), 0);
  EXPECT_EQ(m->getUsedNodes(m), 4);
  d = { 6, 1 }; // Update node
  EXPECT_TRUE(m->updateNode(m, &myList, &d));
  EXPECT_EQ(m->getFreeNodes(m), 0);
  EXPECT_EQ(m->getUsedNodes(m), 5);
  d = { 0, 0 }; /* First node is bigger */
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_EQ(f, nullptr);
  d = { 1, 0 };
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->key, 1);
  EXPECT_EQ(f->val, 3);
  d = { 2, 0 };
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->key, 2);
  EXPECT_EQ(f->val, 2);
  d = { 3, 0 };
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->key, 3);
  EXPECT_EQ(f->val, 3);
  d = { 4, 0 };
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->key, 4);
  EXPECT_EQ(f->val, 2);
  d = { 5, 0 };
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_EQ(f, nullptr);
  d = { 6, 0 }; /* Find place in middle where node is missed */
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->key, 6);
  EXPECT_EQ(f->val, 1);
  d = { 7, 0 }; /* Loop all list */
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_EQ(f, nullptr);
  d = { 0, 2 };
  EXPECT_EQ(m->cleanUpNodes(m, &myList, &d), 2);
  EXPECT_EQ(m->getFreeNodes(m), 2);
  EXPECT_EQ(m->getUsedNodes(m), 3);
  d = { 7, 2 }; // Update node
  EXPECT_TRUE(m->updateNode(m, &myList, &d));
  EXPECT_EQ(m->getFreeNodes(m), 1);
  EXPECT_EQ(m->getUsedNodes(m), 4);
  d = { 7, 0 };
  f = (struct data_t *)m->fetchNode(m, &myList, &d);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->key, 7);
  EXPECT_EQ(f->val, 2);
  m->freeList(m, &myList, false);
  EXPECT_EQ(m->getFreeNodes(m), 1);
  EXPECT_EQ(m->getUsedNodes(m), 0);
  m->free(m);
}
