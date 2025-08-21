/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test store object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/store.h"
}

// Test store with hash table
// void free(pstore self)
// bool update(pstore self, pcipaddr address, pcts timestamp, uint16_t sequenceId, uint8_t domainNumber)
// bool fetch(pstore self, pcipaddr address, pts timestamp, uint16_t sequenceId, uint8_t domainNumber, bool clear)
// size_t cleanup(pstore self, uint32_t time)
// size_t getHashSize(pcstore self)
// size_t records(pcstore self)
// pstore store_alloc(prot protocol, size_t hashSize)
TEST(storeTest, hash)
{
  pstore s = store_alloc(UDP_IPv4, 8);
  ASSERT_NE(s, nullptr);
  EXPECT_EQ(s->getHashSize(s), 256);
  EXPECT_EQ(s->records(s), 0);
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  a->setIPStr(a, "4.3.2.1");
  t->setTs(t, 12000000075);
  useTestMode(true);
  setMono(400);
  EXPECT_TRUE(s->update(s, a, t, 2, 2));
  EXPECT_EQ(s->records(s), 1);
  a->setIPStr(a, "4.3.2.2");
  t->setTs(t, 12000000076);
  setMono(200);
  EXPECT_TRUE(s->update(s, a, t, 2, 2));
  EXPECT_EQ(s->records(s), 2);
  a->setIPStr(a, "4.3.2.1");
  EXPECT_FALSE(s->fetch(s, a, t, 1, 1, false));
  EXPECT_TRUE(s->fetch(s, a, t, 2, 2, true));
  EXPECT_EQ(t->getTs(t), 12000000075);
  EXPECT_TRUE(s->fetch(s, a, t, 2, 2, false));
  EXPECT_EQ(t->getTs(t), 0);
  setMono(700); // 700 - 400 = 300
  EXPECT_EQ(s->cleanup(s, 400), 1);
  EXPECT_EQ(s->records(s), 1);
  useTestMode(false);
  t->free(t);
  a->free(a);
  s->free(s);
}

// Test store with single list
TEST(storeTest, single)
{
  pstore s = store_alloc(UDP_IPv4, 0);
  ASSERT_NE(s, nullptr);
  EXPECT_EQ(s->getHashSize(s), 1);
  EXPECT_EQ(s->records(s), 0);
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  a->setIPStr(a, "4.3.2.1");
  t->setTs(t, 12000000075);
  useTestMode(true);
  setMono(400);
  EXPECT_TRUE(s->update(s, a, t, 2, 2));
  EXPECT_EQ(s->records(s), 1);
  a->setIPStr(a, "4.3.2.2");
  t->setTs(t, 12000000076);
  setMono(200);
  EXPECT_TRUE(s->update(s, a, t, 2, 2));
  EXPECT_EQ(s->records(s), 2);
  a->setIPStr(a, "4.3.2.1");
  EXPECT_FALSE(s->fetch(s, a, t, 1, 1, false));
  EXPECT_TRUE(s->fetch(s, a, t, 2, 2, true));
  EXPECT_EQ(t->getTs(t), 12000000075);
  EXPECT_TRUE(s->fetch(s, a, t, 2, 2, false));
  EXPECT_EQ(t->getTs(t), 0);
  setMono(700); // 700 - 400 = 300
  EXPECT_EQ(s->cleanup(s, 400), 1);
  EXPECT_EQ(s->records(s), 1);
  useTestMode(false);
  t->free(t);
  a->free(a);
  s->free(s);
}

// Test store with hash table with IPv6
TEST(storeTest, hash6)
{
  pstore s = store_alloc(UDP_IPv6, 8);
  ASSERT_NE(s, nullptr);
  EXPECT_EQ(s->getHashSize(s), 256);
  EXPECT_EQ(s->records(s), 0);
  pipaddr a = addr_alloc(UDP_IPv6);
  ASSERT_NE(a, nullptr);
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  a->setIPStr(a, "100:90::1");
  t->setTs(t, 12000000075);
  useTestMode(true);
  setMono(400);
  EXPECT_TRUE(s->update(s, a, t, 2, 2));
  EXPECT_EQ(s->records(s), 1);
  a->setIPStr(a, "100:90::2");
  t->setTs(t, 12000000076);
  setMono(200);
  EXPECT_TRUE(s->update(s, a, t, 2, 2));
  EXPECT_EQ(s->records(s), 2);
  a->setIPStr(a, "100:90::1");
  EXPECT_FALSE(s->fetch(s, a, t, 1, 1, false));
  EXPECT_TRUE(s->fetch(s, a, t, 2, 2, true));
  EXPECT_EQ(t->getTs(t), 12000000075);
  EXPECT_TRUE(s->fetch(s, a, t, 2, 2, false));
  EXPECT_EQ(t->getTs(t), 0);
  setMono(700); // 700 - 400 = 300
  EXPECT_EQ(s->cleanup(s, 400), 1);
  EXPECT_EQ(s->records(s), 1);
  useTestMode(false);
  t->free(t);
  a->free(a);
  s->free(s);
}
