/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test options for configuration file and command line
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/opt.h"
}

#include <getopt.h>

enum test {
  E1 = 1,
  E2 = 2,
  E3 = 3,
  E4l = 4,
};
const char *e2test(int64_t value)
{
  switch(value) {
     case caseItem(E1);
     case caseItem(E2);
     case caseItem(E3);
     case caseItem(E4l);
  }
  return nullptr;
}
struct opt_rec_t rcs[] = {
  KEY_INT("key 1", 'k', "this is key 1", 10, -10, 100),
  KEY_STR("key 2", 'm', "this is key 2", "val0", 10),
  KEY_BOOL("have it", 'o', "this is a boolean key\nwith two lines", true),
  KEY_ENUM("eval", 'e', "use enum\nthis is seconds\nlimit", E2, e2test, 3, E1, E4l),
  KEY_FLT("float", 'f', "use floting value", 2.1, -4.5, 10.7),
  KEY_LAST
};

// Test getopt
// void free(popt self)
// bool addRecords(popt self, pcrec records)
// popt opt_alloc()
// const char *getopt(popt self, const char *preOptions)
TEST(optTest, getopt)
{
  popt o = opt_alloc();
  ASSERT_NE(o, nullptr);
  EXPECT_TRUE(o->addRecords(o, rcs));
  EXPECT_STREQ(o->getopt(o, "hv"), "hvk:m:oe:f:");
  o->free(o);
}

// Test getopt_long
// void *getoptLong(popt self, void *additional_long_options)
TEST(optTest, getopt_long)
{
  popt o = opt_alloc();
  ASSERT_NE(o, nullptr);
  EXPECT_TRUE(o->addRecords(o, rcs));
  struct option *ot = (struct option *)o->getoptLong(o, nullptr);
  EXPECT_STREQ(ot[0].name, "key 1");
  EXPECT_TRUE(ot[0].has_arg);
  EXPECT_EQ(ot[0].flag, nullptr);
  EXPECT_EQ(ot[0].val, 'k');
  EXPECT_STREQ(ot[1].name, "key 2");
  EXPECT_TRUE(ot[1].has_arg);
  EXPECT_EQ(ot[1].flag, nullptr);
  EXPECT_EQ(ot[1].val, 'm');
  EXPECT_STREQ(ot[2].name, "have it");
  EXPECT_FALSE(ot[2].has_arg);
  EXPECT_EQ(ot[2].flag, nullptr);
  EXPECT_EQ(ot[2].val, 'o');
  EXPECT_STREQ(ot[3].name, "eval");
  EXPECT_TRUE(ot[3].has_arg);
  EXPECT_EQ(ot[3].flag, nullptr);
  EXPECT_EQ(ot[3].val, 'e');
  EXPECT_STREQ(ot[4].name, "float");
  EXPECT_TRUE(ot[4].has_arg);
  EXPECT_EQ(ot[4].flag, nullptr);
  EXPECT_EQ(ot[4].val, 'f');
  EXPECT_STREQ(ot[5].name, nullptr);
  EXPECT_FALSE(ot[5].has_arg);
  EXPECT_EQ(ot[5].flag, nullptr);
  EXPECT_EQ(ot[5].val, 0);
  o->free(o);
}

// Test print help
// bool printHelp(pcopt self, FILE *out, int ident)
TEST(optTest, printHelp)
{
  popt o = opt_alloc();
  ASSERT_NE(o, nullptr);
  EXPECT_TRUE(o->addRecords(o, rcs));
  useTestMode(true);
  EXPECT_TRUE(o->printHelp(o, stdout, 3));
  EXPECT_STREQ(getOut(),
   "   -k  this is key 1\n"
   "   -m  this is key 2\n"
   "   -o  this is a boolean key\n"
   "       with two lines\n"
   "   -e  use enum\n"
   "       this is seconds\n"
   "       limit\n"
   "   -f  use floting value\n");
  o->free(o);
}

// Test parse value
// bool parseSkey(popt self, int short_key, const char *str, uint8_t flags)
// bool parseKey(popt self, const char *key, const char *str, uint8_t flags)
// bool getValSkey(pcopt self, int short_key, precval value)
// bool getValKey(pcopt self, const char *key, precval value)
TEST(optTest, parse)
{
  optRecVal v;
  popt o = opt_alloc();
  ASSERT_NE(o, nullptr);
  EXPECT_TRUE(o->addRecords(o, rcs));
  EXPECT_TRUE(o->getValSkey(o, 'k', &v));
  EXPECT_EQ(v.i, 10);
  EXPECT_TRUE(o->parseSkey(o, 'k', "20", 0));
  EXPECT_TRUE(o->getValSkey(o, 'k', &v));
  EXPECT_EQ(v.i, 20);
  EXPECT_TRUE(o->getValKey(o, "key 2", &v));
  EXPECT_STREQ(v.s, "val0");
  EXPECT_TRUE(o->parseKey(o, "key 2", "xxx", 0));
  EXPECT_TRUE(o->getValKey(o, "key 2", &v));
  EXPECT_STREQ(v.s, "xxx");
  EXPECT_TRUE(o->getValSkey(o, 'o', &v));
  EXPECT_TRUE(v.i);
  EXPECT_TRUE(o->parseSkey(o, 'o', nullptr, OPT_BOOL_DEF));
  EXPECT_TRUE(o->getValSkey(o, 'o', &v));
  EXPECT_FALSE(v.i);
  EXPECT_TRUE(o->parseSkey(o, 'o', "on", OPT_REPLACE));
  EXPECT_TRUE(o->getValSkey(o, 'o', &v));
  EXPECT_TRUE(v.i);
  EXPECT_TRUE(o->getValSkey(o, 'e', &v));
  EXPECT_EQ(v.i, E2);
  EXPECT_TRUE(o->parseKey(o, "eval", "4", 0));
  EXPECT_TRUE(o->getValSkey(o, 'e', &v));
  EXPECT_EQ(v.i, E4l);
  EXPECT_TRUE(o->parseKey(o, "eval", "E3", OPT_REPLACE));
  EXPECT_TRUE(o->getValKey(o, "eval", &v));
  EXPECT_EQ(v.i, E3);
  EXPECT_TRUE(o->getValSkey(o, 'f', &v));
  EXPECT_FLOAT_EQ(v.f, 2.1);
  EXPECT_TRUE(o->parseSkey(o, 'f', "6.7", 0));
  EXPECT_TRUE(o->getValSkey(o, 'f', &v));
  EXPECT_FLOAT_EQ(v.f, 6.7);
  o->free(o);
}

// Test duplicate keys
// bool opt_duplicates(pcopt self, const char *preOptions, void *additional_long_options)
TEST(optTest, dupKeys)
{
  popt o = opt_alloc();
  ASSERT_NE(o, nullptr);
  EXPECT_TRUE(o->addRecords(o, rcs));
  EXPECT_TRUE(opt_duplicates(o, ":hv", nullptr));
  EXPECT_TRUE(opt_duplicates(o, nullptr, nullptr));
  useTestMode(true);
  EXPECT_FALSE(opt_duplicates(o, ":hvh", nullptr));
  EXPECT_STREQ(getOut(), "'h' appears twice in pre additional options.\n");
  useTestMode(true);
  EXPECT_FALSE(opt_duplicates(o, "o", nullptr));
  EXPECT_STREQ(getOut(), "'o' appears twice.\n");
  struct option a[] {
    { .name = "key1" },
    { .name = "key1" },
    { .name = "key2" },
    { .name = "eval" },
    { 0 }
  };
  useTestMode(true);
  EXPECT_FALSE(opt_duplicates(o, nullptr, a));
  EXPECT_STREQ(getOut(), "'key1' appears twice in additional keys.\n");
  struct option a2[] {
    { .name = "key1" },
    { .name = "key2" },
    { .name = "eval" },
    { 0 }
  };
  useTestMode(true);
  EXPECT_FALSE(opt_duplicates(o, nullptr, a2));
  EXPECT_STREQ(getOut(), "'eval' appears twice.\n");
  o->free(o);
}

// Test multiple records arrays
// const char *getKey(pcopt self, int short_key)
// bool isSkey(pcopt self, int short_key)
// bool isKey(pcopt self, const char *key)
// optType skeyType(pcopt self, int short_key)
// optType keyType(pcopt self, const char *key)
TEST(optTest, multiple)
{
  optRecVal v;
  popt o = opt_alloc();
  ASSERT_NE(o, nullptr);
  EXPECT_TRUE(o->addRecords(o, rcs));
  struct opt_rec_t rcs2[] = {
    KEY_INT("key 5", 'q', "this is key 1", 300, 50, 500),
    KEY_LAST
  };
  EXPECT_TRUE(o->addRecords(o, rcs2));
  EXPECT_TRUE(o->getValSkey(o, 'q', &v));
  EXPECT_EQ(v.i, 300);
  EXPECT_FALSE(o->parseKey(o, "key 5", "700", 0));
  EXPECT_TRUE(o->parseKey(o, "key 5", "400", 0));
  EXPECT_TRUE(o->getValSkey(o, 'q', &v));
  EXPECT_EQ(v.i, 400);
  EXPECT_STREQ(o->getKey(o, 'q'), "key 5");
  EXPECT_STREQ(o->getKey(o, 'o'), "have it");
  EXPECT_TRUE(o->isSkey(o, 'q'));
  EXPECT_TRUE(o->isSkey(o, 'o'));
  EXPECT_EQ(o->skeyType(o, 'q'), CFG_INT);
  EXPECT_EQ(o->skeyType(o, 'o'), CFG_BOOL);
  EXPECT_TRUE(o->isKey(o, "key 5"));
  EXPECT_TRUE(o->isKey(o, "have it"));
  EXPECT_EQ(o->keyType(o, "key 5"), CFG_INT);
  EXPECT_EQ(o->keyType(o, "have it"), CFG_BOOL);
  o->free(o);
}
