/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test buffer object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/buf.h"
#include "src/log.h"
}

// Test memory buffer
// void free(pbuffer self)
// uint8_t *getBuf(pcbuffer self)
// bool setLen(pbuffer self, size_t len)
// size_t getLen(pcbuffer self)
// size_t getSize(pbuffer self)
// bool copy(pbuffer self, pcbuffer other)
// pbuffer spawn(pcbuffer self)
// bool resize(pbuffer self, size_t newSize)
// pbuffer buffer_alloc(size_t size)
TEST(bufferTest, bufferTest)
{
  pbuffer a = buffer_alloc(11);
  ASSERT_NE(a, nullptr);
  EXPECT_NE(a->getBuf(a), nullptr);
  EXPECT_EQ(a->getLen(a), 0);
  EXPECT_EQ(a->getSize(a), 11);
  EXPECT_TRUE(a->setLen(a, 7));
  EXPECT_EQ(a->getLen(a), 7);
  memcpy(a->getBuf(a), "test123", 7);
  pbuffer b = a->spawn(a);
  EXPECT_NE(b->getBuf(b), nullptr);
  EXPECT_EQ(b->getSize(b), 11);
  EXPECT_EQ(b->getLen(b), 7);
  EXPECT_EQ(0, memcmp(b->getBuf(b), "test123", 7));
  memcpy(a->getBuf(a), "123tes", 6);
  EXPECT_TRUE(a->setLen(a, 6));
  EXPECT_TRUE(b->copy(b, a));
  EXPECT_EQ(b->getSize(b), 11);
  EXPECT_EQ(b->getLen(b), 6);
  EXPECT_EQ(0, memcmp(b->getBuf(b), "123tes", 6));
  struct log_options_t lopt = {
    .log_level = LOG_DEBUG,
    .useSysLog = false,
    .useEcho = true
  };
  EXPECT_TRUE(setLog("", &lopt));
  useTestMode(true);
  EXPECT_FALSE(b->setLen(b, 15));
  EXPECT_EQ(0, strncmp(getErr(), "[4:buf.c:", 9));
  EXPECT_STREQ(strrchr(getErr(), ':'), ":_setLen] length exceed buffer size\n");
  EXPECT_TRUE(b->resize(b, 20));
  EXPECT_EQ(b->getSize(b), 20);
  EXPECT_EQ(b->getLen(b), 6);
  EXPECT_EQ(0, memcmp(b->getBuf(b), "123tes", 6));
  EXPECT_TRUE(b->setLen(b, 15));
  a->free(a);
  b->free(b);
}
