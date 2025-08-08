/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test swap functions
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

extern "C" {
#include "src/swap.h"
}

// Test 64 bits
// uint64_t cpu_to_net64(uint64_t value)
// uint64_t net_to_cpu64(uint64_t value)
TEST(swapTest, u64)
{
  static const uint64_t v = 0x123456789abcdef0LL;
  const uint64_t n = cpu_to_net64(v);
  static const uint8_t p[8] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
  EXPECT_EQ(memcmp(p, &n, 8), 0);
  const uint64_t l = net_to_cpu64(n);
  EXPECT_EQ(net_to_cpu64(n), 0x123456789abcdef0LL);
}

// Test 32 bits
// uint32_t cpu_to_net32(uint32_t value)
// uint32_t net_to_cpu32(uint32_t value)
TEST(swapTest, u32)
{
  static const uint32_t v = 0x12345678;
  const uint32_t n = cpu_to_net32(v);
  static const uint8_t p[4] = { 0x12, 0x34, 0x56, 0x78 };
  EXPECT_EQ(memcmp(p, &n, 4), 0);
  const uint32_t l = net_to_cpu32(n);
  EXPECT_EQ(net_to_cpu32(n), 0x12345678);
}

// Test 16 bits
// uint16_t cpu_to_net16(uint16_t value)
// uint16_t net_to_cpu16(uint16_t value)
TEST(swapTest, u16)
{
  static const uint16_t v = 0x1234;
  const uint16_t n = cpu_to_net16(v);
  static const uint8_t p[2] = { 0x12, 0x34 };
  EXPECT_EQ(memcmp(p, &n, 2), 0);
  const uint16_t l = net_to_cpu16(n);
  EXPECT_EQ(net_to_cpu16(n), 0x1234);
}
