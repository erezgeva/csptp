/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test interface object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/if.h"
}

// Test initialize with ifName
// void free(pif self)
// bool intIfName(pif self, const char *ifName, bool useTwoSteps)
// bool isInit(pcif self)
// const uint8_t *getMacAddr(pcif self)
// const uint8_t *getClockID(pcif self)
// uint32_t getIPv4(pcif self)
// const char *getIfName(pcif self)
// int getIfIndex(pcif self)
// int getPTPIndex(pcif self)
// pif if_alloc()
TEST(ifTest, ifName)
{
  pif i = if_alloc();
  ASSERT_NE(i, nullptr);
  useTestMode(true);
  EXPECT_TRUE(i->intIfName(i, "enp0s25", true));
  useTestMode(false);
  printf("%s\n", getErr());
  EXPECT_TRUE(i->isInit(i));
  EXPECT_EQ(i->getIfIndex(i), 2);
  EXPECT_STREQ(i->getIfName(i), "enp0s25");
  EXPECT_EQ(i->getPTPIndex(i), 3);
  EXPECT_EQ(0, memcmp(i->getMacAddr(i), "\x1\x2\x3\x4\x5\x6", 6));
  EXPECT_EQ(0, memcmp(i->getClockID(i), "\x1\x2\x3\xff\xfe\x4\x5\x6", 8));
  uint32_t ip = i->getIPv4(i);
  EXPECT_EQ(0, memcmp(&ip, "\xc0\xa8\x01\x34", 4));
  i->free(i);
}

// Test initialize with ifIndex
// bool intIfIndex(pif self, int ifIndex, bool useTwoSteps)
// bool bind(pcif self, int fd)
// bool getIPv6(pif self, uint8_t **ipv6)
TEST(ifTest, ifIndex)
{
  pif i = if_alloc();
  ASSERT_NE(i, nullptr);
  useTestMode(true);
  EXPECT_TRUE(i->intIfIndex(i, 2, false));
  useTestMode(false);
  EXPECT_TRUE(i->isInit(i));
  EXPECT_EQ(i->getIfIndex(i), 2);
  EXPECT_STREQ(i->getIfName(i), "enp0s25");
  EXPECT_EQ(i->getPTPIndex(i), 3);
  EXPECT_EQ(0, memcmp(i->getMacAddr(i), "\x1\x2\x3\x4\x5\x6", 6));
  EXPECT_EQ(0, memcmp(i->getClockID(i), "\x1\x2\x3\xff\xfe\x4\x5\x6", 8));
  uint32_t ip = i->getIPv4(i);
  EXPECT_EQ(0, memcmp(&ip, "\xc0\xa8\x01\x34", 4));
  uint8_t *ipv6 = nullptr;
  useTestMode(true);
  EXPECT_TRUE(i->bind(i, 7));
  EXPECT_TRUE(i->getIPv6(i, &ipv6));
  useTestMode(false);
  uint16_t *x = (uint16_t *)ipv6;
  uint8_t q[16] = { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0x42, 0xa5, 0xff, 0xfe, 0x51, 0x41, 0x50 };
  EXPECT_EQ(0, memcmp(q, ipv6, 16));
  i->free(i);
}
