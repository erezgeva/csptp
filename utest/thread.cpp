/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test thread objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifdef __GNUC__
#ifndef __clang__
// Somehow GCC C++ fail to understand C `atomic_bool`
// Anyhow, these varibles are used internaly in C only!!
typedef std::atomic<bool> atomic_bool;
#endif /* __clang__ */
#endif /* __GNUC__ */

extern "C" {
#include "src/thread.h"
}

static bool thread(void *cookie)
{
  int *a = (int *)cookie;
  /* Sleep so test can detect we run */
  timespec t = { .tv_sec = 0, .tv_nsec = 1000 };
  thrd_sleep(&t, nullptr);
  return *a == 17;
}

// Test thread
// pthread thread_create(const thread_f function, void *cookie)
// void free(pthread self)
// bool join(pcthread self)
// bool isRun(pcthread self)
// bool retVal(pcthread self)
TEST(threadTest, thread)
{
  int a = 17;
  pthread m = thread_create(thread, &a);
  ASSERT_NE(m, nullptr);
  EXPECT_TRUE(m->isRun(m));
  EXPECT_TRUE(m->join(m));
  // Thread exit!
  EXPECT_FALSE(m->isRun(m));
  EXPECT_TRUE(m->retVal(m));
  m->free(m);
}
