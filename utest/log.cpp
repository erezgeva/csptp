/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test logger
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/log.h"
}

static const struct log_options_t opt = {
  .log_level = LOG_INFO,
  .useSysLog = true,
  .useEcho = true
};

// Test logger with error
// bool setLog(const char *name, const struct log_options_t *options)
// void doneLog()
// void log_err(const char* format, ...)
TEST(logTest, loggerErr)
{
  useTestMode(true);
  EXPECT_TRUE(setLog("test", &opt));
  log_err("test 1 %d", 2);
  doneLog();
  EXPECT_STREQ(getLog(), "[3:log.cpp:32:TestBody] test 1 2");
  EXPECT_STREQ(getErr(), "[3:log.cpp:32:TestBody] test 1 2\n");
  EXPECT_STREQ(getOut(), "");
}

// Test logger with error and system error
// void logp_err(const char* format, ...)
TEST(logTest, loggerErrp)
{
  useTestMode(true);
  EXPECT_TRUE(setLog("test", &opt));
  errno = EINTR;
  logp_err("test 2 %d", 4);
  doneLog();
  EXPECT_STREQ(getLog(), "[3:log.cpp:46:TestBody] test 2 4: Interrupted system call");
  EXPECT_STREQ(getErr(), "[3:log.cpp:46:TestBody] test 2 4: Interrupted system call\n");
  EXPECT_STREQ(getOut(), "");
}

// Test logger with information
// void log_info(const char* format, ...)
TEST(logTest, loggerInfo)
{
  useTestMode(true);
  EXPECT_TRUE(setLog("test", &opt));
  log_info("test x %d", 7);
  doneLog();
  EXPECT_STREQ(getLog(), "[6:log.cpp:59:TestBody] test x 7");
  EXPECT_STREQ(getErr(), "");
  EXPECT_STREQ(getOut(), "[6:log.cpp:59:TestBody] test x 7\n");
}

// Test skip logger on level
// void log_debug(const char* format, ...)
TEST(logTest, loggerSkip)
{
  useTestMode(true);
  EXPECT_TRUE(setLog("test", &opt));
  log_debug("test xX");
  doneLog();
  EXPECT_STREQ(getLog(), "");
  EXPECT_STREQ(getErr(), "");
  EXPECT_STREQ(getOut(), "");
}
