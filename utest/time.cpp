/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test timestamp objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/time.h"
#include "src/log.h"
}

// Test timestamp
// void free(pts self)
// void fromTimespec(pts self, pctsp ts)
// void toTimespec(pcts self, ptsp ts)
// bool fromTimestamp(pts self, pcpts ts)
// bool toTimestamp(pcts self, ppts ts)
// void setTs(pts self, int64_t ts)
// int64_t getTs(pcts self)
// bool eq(pcts self, pcts other)
// void assign(pts self, pcts other)
// pts ts_alloc()
TEST(timestampTest, timestampTest)
{
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  struct timespec s = { 1, 105 };
  t->fromTimespec(t, &s);
  EXPECT_EQ(t->getTs(t), 1000000105);
  struct timespec s2;
  t->toTimespec(t, &s2);
  EXPECT_EQ(s2.tv_sec, 1);
  EXPECT_EQ(s2.tv_nsec, 105);
  pts t2 = ts_alloc();
  ASSERT_NE(t2, nullptr);
  struct Timestamp_t p;
  EXPECT_TRUE(t->toTimestamp(t, &p));
  EXPECT_TRUE(t2->fromTimestamp(t2, &p));
  EXPECT_TRUE(t->eq(t, t2));
  EXPECT_EQ(t2->getTs(t2), 1000000105);
  t->setTs(t, 975);
  EXPECT_EQ(t->getTs(t), 975);
  EXPECT_FALSE(t->eq(t, t2));
  t2->assign(t2, t);
  EXPECT_TRUE(t->eq(t, t2));
  t2->free(t2);
  t->free(t);
}

// Test negative timestamp
TEST(timestampTest, negative)
{
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  t->setTs(t, -975);
  EXPECT_EQ(t->getTs(t), -975);
  struct timespec s;
  t->toTimespec(t, &s);
  EXPECT_EQ(s.tv_sec, -1);
  EXPECT_EQ(s.tv_nsec, 999999025);
  struct Timestamp_t p;
  struct log_options_t lopt = {
    .log_level = LOG_DEBUG,
    .useSysLog = false,
    .useEcho = true
  };
  EXPECT_TRUE(setLog("", &lopt));
  /* Can not hold negative timestamp */
  useTestMode(true);
  EXPECT_FALSE(t->toTimestamp(t, &p));
  EXPECT_EQ(0, strncmp(getErr(), "[4:time.c:", 10));
  EXPECT_STREQ(strrchr(getErr(), ':'), ":_toTimestamp] PTP Timestamp do not support negitive time\n");
  t->free(t);
}

// Test add milliseconds to timestamp
// void addMilliseconds(pts self, int milliseconds)
TEST(timestampTest, addNilli)
{
  struct timespec s;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  t->setTs(t, 900000000);
  t->addMilliseconds(t, 1200);
  t->toTimespec(t, &s);
  EXPECT_EQ(s.tv_sec, 2);
  EXPECT_EQ(s.tv_nsec, 100000000);
  t->setTs(t, 100000000);
  t->addMilliseconds(t, -1200);
  t->toTimespec(t, &s);
  EXPECT_EQ(s.tv_sec, -2);
  EXPECT_EQ(s.tv_nsec, 900000000);
  t->setTs(t, -900000000);
  t->toTimespec(t, &s);
  EXPECT_EQ(s.tv_sec, -1);
  EXPECT_EQ(s.tv_nsec, 100000000);
  t->addMilliseconds(t, 1900);
  t->toTimespec(t, &s);
  EXPECT_EQ(s.tv_sec, 1);
  EXPECT_EQ(s.tv_nsec, 0);
  t->free(t);
}

// Test timestamp compare
// bool less(pcts self, pcts other)
// bool lessEq(pcts self, pcts other)
TEST(timestampTest, timestampCmpTest)
{
  pts t1 = ts_alloc();
  ASSERT_NE(t1, nullptr);
  pts t2 = ts_alloc();
  ASSERT_NE(t2, nullptr);
  struct timespec s = { 10, 105 };
  t1->fromTimespec(t1, &s);
  t2->fromTimespec(t2, &s);
  EXPECT_TRUE(t1->eq(t1, t2));
  EXPECT_TRUE(t1->lessEq(t1, t2));
  EXPECT_FALSE(t1->less(t1, t2));
  EXPECT_TRUE(t2->eq(t2, t1));
  EXPECT_TRUE(t2->lessEq(t2, t1));
  EXPECT_FALSE(t2->less(t2, t1));
  s = { 10, 104 };
  t1->fromTimespec(t1, &s);
  EXPECT_FALSE(t1->eq(t1, t2));
  EXPECT_TRUE(t1->lessEq(t1, t2));
  EXPECT_TRUE(t1->less(t1, t2));
  EXPECT_FALSE(t2->eq(t2, t1));
  EXPECT_FALSE(t2->lessEq(t2, t1));
  EXPECT_FALSE(t2->less(t2, t1));
  s = { 9, 105 };
  t1->fromTimespec(t1, &s);
  EXPECT_FALSE(t1->eq(t1, t2));
  EXPECT_TRUE(t1->lessEq(t1, t2));
  EXPECT_TRUE(t1->less(t1, t2));
  EXPECT_FALSE(t2->eq(t2, t1));
  EXPECT_FALSE(t2->lessEq(t2, t1));
  EXPECT_FALSE(t2->less(t2, t1));
  s = { 10, 106 };
  t1->fromTimespec(t1, &s);
  EXPECT_FALSE(t1->eq(t1, t2));
  EXPECT_FALSE(t1->lessEq(t1, t2));
  EXPECT_FALSE(t1->less(t1, t2));
  // Get big or equall by reverse!
  EXPECT_FALSE(t2->eq(t2, t1));
  EXPECT_TRUE(t2->lessEq(t2, t1));
  EXPECT_TRUE(t2->less(t2, t1));
  s = { 11, 105 };
  t1->fromTimespec(t1, &s);
  EXPECT_FALSE(t1->eq(t1, t2));
  EXPECT_FALSE(t1->lessEq(t1, t2));
  EXPECT_FALSE(t1->less(t1, t2));
  // Get big or equall by reverse!
  EXPECT_FALSE(t2->eq(t2, t1));
  EXPECT_TRUE(t2->lessEq(t2, t1));
  EXPECT_TRUE(t2->less(t2, t1));
  t2->free(t2);
  t1->free(t1);
}

// Test unsigned 48 bits integer
// uint64_t get_uint48(pcuint48 num)
// bool set_uint48(puint48 num, uint64_t value)
// bool cpu_to_net48(puint48 num)
// bool net_to_cpu48(puint48 num)
TEST(timestampTest, u48)
{
  struct UInteger48_t n;
  EXPECT_TRUE(set_uint48(&n, 0x123456789abc));
  EXPECT_EQ(get_uint48(&n), 0x123456789abc);
  EXPECT_TRUE(cpu_to_net48(&n));
  uint8_t a[6] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc };
  EXPECT_EQ(memcmp(&n, a, 6), 0);
  EXPECT_TRUE(net_to_cpu48(&n));
  EXPECT_EQ(get_uint48(&n), 0x123456789abc);
  struct log_options_t lopt = {
    .log_level = LOG_DEBUG,
    .useSysLog = false,
    .useEcho = true
  };
  EXPECT_TRUE(setLog("", &lopt));
  useTestMode(true);
  EXPECT_FALSE(set_uint48(&n, 0x1000000000000LL));
  EXPECT_EQ(0, strncmp(getErr(), "[3:time.c:", 10));
  EXPECT_STREQ(strrchr(getErr(), ':'), ":set_uint48] Value is too big for unsigned 48 bits 0x1000000000000\n");
}

// Test Timestamp_t functions
// bool net_to_cpu_ts(ppts timestamp)
// bool cpu_to_net_ts(ppts timestamp)
TEST(timestampTest, Timestamp)
{
  struct Timestamp_t p = { .nanosecondsField = 984 };
  EXPECT_TRUE(set_uint48(&p.secondsField, 10));
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  EXPECT_TRUE(t->fromTimestamp(t, &p));
  EXPECT_EQ(t->getTs(t), 10000000984);
  struct Timestamp_t p2;
  EXPECT_TRUE(t->toTimestamp(t, &p2));
  t->free(t);
  EXPECT_TRUE(net_to_cpu_ts(&p));
  uint8_t a[10] = { 0, 0, 0, 0, 0, 10, 0, 0, 3, 216 };
  EXPECT_EQ(memcmp(&p, a, 10), 0);
  EXPECT_TRUE(cpu_to_net_ts(&p));
  pts t1 = ts_alloc();
  ASSERT_NE(t1, nullptr);
  pts t2 = ts_alloc();
  ASSERT_NE(t2, nullptr);
  EXPECT_TRUE(t1->fromTimestamp(t1, &p));
  EXPECT_TRUE(t2->fromTimestamp(t2, &p2));
  EXPECT_TRUE(t1->eq(t1, t2));
  t1->free(t1);
  t2->free(t2);
}

// Test sleep
// void sleep(pts self)
TEST(timestampTest, sleep)
{
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  t->setTs(t, 1);
  t->sleep(t);
  t->free(t);
}

// Test locat time zone
// struct time_zone_t *getLocalTZ()
TEST(timestampTest, tz)
{
  useTestMode(true);
  struct time_zone_t *z = getLocalTZ();
  useTestMode(false);
  ASSERT_NE(z, nullptr);
  EXPECT_EQ(z[0].offset, 1 * 3600);
  EXPECT_STREQ(z[0].name, "CET");
  EXPECT_EQ(z[1].offset, 2 * 3600);
  EXPECT_STREQ(z[1].name, "CEST");
};
