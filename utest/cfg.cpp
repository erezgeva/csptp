/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test configuration file parser
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

extern "C" {
#include "src/cfg.h"
}

static void *noSec(const char *name, void *cookie) { return nullptr; }
static bool noKey(const void *section, const char *key, const char *val, void *cookie) { return false; }

// Test comment and empty line
// void free(pcfg self)
// bool parseLine(pcfg self, const char *line, size_t size)
// pcfg cfg_alloc(setSection_t setSection, setKey_f setKey, void *cookie)
TEST(cfgTest, commentEmpty)
{
  pcfg c = cfg_alloc(noSec, noKey, nullptr);
  ASSERT_NE(c, nullptr);
  EXPECT_TRUE(c->parseLine(c, " # this is a comment", 0));
  EXPECT_TRUE(c->parseLine(c, " \t \t ", 0)); /* line with spaces */
  EXPECT_TRUE(c->parseLine(c, "", 0)); /* empty line */
  c->free(c);
}

static int secTest;
static void *setSec(const char *name, void *cookie)
{ return strcmp( "a b", name) == 0 ? &secTest : nullptr; }
static bool setKey(const void *section, const char *key, const char *val, void *cookie)
{ return section == &secTest && strcmp(key, "key 1") == 0 && strcmp(val, "val 2") == 0; }

// Test section
TEST(cfgTest, section)
{
  pcfg c = cfg_alloc(setSec, setKey, nullptr);
  ASSERT_NE(c, nullptr);
  EXPECT_TRUE(c->parseLine(c, " [ a b ] \t ", 0));
  EXPECT_TRUE(c->parseLine(c, " [  'a b' ] \t ", 0));
  EXPECT_TRUE(c->parseLine(c, " \t [ \t  \"a b\" ] \t ", 0));
  c->free(c);
}

// Test key-value pair
TEST(cfgTest, keyVal)
{
  pcfg c = cfg_alloc(setSec, setKey, nullptr);
  ASSERT_NE(c, nullptr);
  EXPECT_TRUE(c->parseLine(c, " [ a b ] \t ", 0));
  EXPECT_TRUE(c->parseLine(c, " key 1 \t = \t  val 2 \t ", 0));
  EXPECT_TRUE(c->parseLine(c, " 'key 1' \t = \t  val 2 \t ", 0));
  EXPECT_TRUE(c->parseLine(c, "\t \t \"key 1\" \t = \t  val 2 \t ", 0));
  c->free(c);
}

// Test parsing buffer
// bool parseBuf(pcfg self, const char *buffer)
TEST(cfgTest, parseBuf)
{
  pcfg c = cfg_alloc(setSec, setKey, nullptr);
  ASSERT_NE(c, nullptr);
  EXPECT_TRUE(c->parseBuf(c,
   " # This is a comment \n"
   "  [ a b ] \t  \r\n"
   "\r\n  \n\n"
   " key 1 \t = \t  val 2 \t \n"));
  c->free(c);
}

// Test parsing file
// bool parseFile(pcfg self, const char *name)
TEST(cfgTest, parseFile)
{
  pcfg c = cfg_alloc(setSec, setKey, nullptr);
  ASSERT_NE(c, nullptr);
  c->parseFile(c, "utest/cfg.cfg");
  c->free(c);
}


// Test value string striping
// const char *cfg_rmStrQuote(const char *value)
TEST(cfgTest, rmStrQuote)
{
  /* strip single quate mark */
  char *s = strdup("'This is a test'");
  ASSERT_NE(s, nullptr);
  const char *r = cfg_rmStrQuote(s);
  EXPECT_STREQ(r, "This is a test");
  free(s);
  /* strip double quate mark */
  s = strdup("\"This \"is a test\"");
  ASSERT_NE(s, nullptr);
  r = cfg_rmStrQuote(s);
  EXPECT_STREQ(r, "This \"is a test");
  free(s);
  /* Do not strip as the double quate mark is on end side only */
  s = strdup("This \"is\' a test\"");
  ASSERT_NE(s, nullptr);
  r = cfg_rmStrQuote(s);
  EXPECT_STREQ(r, "This \"is\' a test\"");
  free(s);
  /* Do not strip as the single quate mark is on begin side only */
  s = strdup("\'This \"is\' a test");
  ASSERT_NE(s, nullptr);
  r = cfg_rmStrQuote(s);
  EXPECT_STREQ(r, "\'This \"is\' a test");
  free(s);
}
