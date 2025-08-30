/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test mutex objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

extern "C" {
#include "src/mutex.h"
}

// Test mutex
// pmutex mutex_alloc()
// void free(pmutex self)
// bool unlock(pmutex self)
// bool lock(pmutex self)
TEST(mutexTest, mutex)
{
  pmutex m = mutex_alloc();
  ASSERT_NE(m, nullptr);
  EXPECT_TRUE(m->lock(m));
  EXPECT_TRUE(m->unlock(m));
  m->free(m);
}
