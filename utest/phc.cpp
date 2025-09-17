/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test PTP Hardware clock objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/phc.h"
}

// Test fullPath
// void free(pphc self)
// bool initDev(pphc self, const char *device, bool readonly)
// bool getTime(pcphc self, pts timestamp)
// bool setTime(pphc self, pcts timestamp)
// bool offsetClock(pphc self, pcts offset)
// bool setPhase(pphc self, pcts offset)
// bool getFreq(pcphc self, double *frequancy)
// bool setFreq(pphc self, double frequancy)
// int fileno(pcphc self)
// clockid_t clkId(pcphc self)
// int ptpIndex(pcphc self)
// const char *device(pcphc self)
// pphc phc_alloc()
TEST(phcTest, fullPath)
{
  useTestMode(true);
  pphc p = phc_alloc();
  ASSERT_NE(p, nullptr);
  EXPECT_TRUE(p->initDev(p, "/dev/ptp0", true));
  EXPECT_EQ(p->fileno(p), 7);
  EXPECT_EQ(p->clkId(p), -61);
  EXPECT_EQ(p->ptpIndex(p), 0);
  EXPECT_STREQ(p->device(p), "/dev/ptp0");
  pts t = ts_alloc();
  setReal(4);
  EXPECT_TRUE(p->getTime(p, t));
  EXPECT_EQ(t->getTs(t), 4000000000);
  t->setTs(t, 4000000004);
  EXPECT_TRUE(p->setTime(p, t));
  t->setTs(t, 93000000571);
  EXPECT_TRUE(p->offsetClock(p, t));
  double f;
  EXPECT_TRUE(p->getFreq(p, &f));
  EXPECT_DOUBLE_EQ(f, 9.979248046875);
  EXPECT_TRUE(p->setFreq(p, f));
  t->setTs(t, 265963);
  EXPECT_TRUE(p->setPhase(p, t));
  t->free(t);
  p->free(p);
  useTestMode(false);
}

// Test fileOnly
TEST(phcTest, fileOnly)
{
  useTestMode(true);
  pphc p = phc_alloc();
  EXPECT_EQ(p->fileno(p), -1);
  EXPECT_EQ(p->clkId(p), -1);
  EXPECT_EQ(p->ptpIndex(p), -1);
  EXPECT_EQ(p->device(p), nullptr);
  EXPECT_TRUE(p->initDev(p, "ptp0", true));
  EXPECT_EQ(p->fileno(p), 7);
  EXPECT_EQ(p->clkId(p), -61);
  EXPECT_EQ(p->ptpIndex(p), 0);
  EXPECT_STREQ(p->device(p), "/dev/ptp0");
  p->free(p);
  useTestMode(false);
}

// Test index
// bool initIndex(pphc self, int ptpIndex, bool readonly)
TEST(phcTest, index)
{
  useTestMode(true);
  pphc p = phc_alloc();
  EXPECT_TRUE(p->initIndex(p, 0, true));
  EXPECT_EQ(p->fileno(p), 7);
  EXPECT_EQ(p->clkId(p), -61);
  EXPECT_EQ(p->ptpIndex(p), 0);
  EXPECT_STREQ(p->device(p), "/dev/ptp0");
  p->free(p);
  useTestMode(false);
}
