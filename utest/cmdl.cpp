/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief command line parsing
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/cmdl.h"
}

static const struct opt_rec_t bopts[] = {
    KEY_INT("domainNumber", 'n', "<domain number> domainNumber", 128, 128, 239),
    KEY_LAST
};

// Test base command line parsing
TEST(cmdlTest, base)
{
  popt po;
  optRecVal v;
  const char *a[] = {
      "base",
      "-y",
      "-e",
      "-i", "eth0",
      "-n", "137",
      nullptr
  };
  EXPECT_EQ(CMD_OK, cmd_base(7, (char **)a, &po, bopts));
  EXPECT_TRUE(po->getValSkey(po, 'y', &v));
  EXPECT_TRUE(v.i);
  EXPECT_TRUE(po->getValSkey(po, 'e', &v));
  EXPECT_TRUE(v.i);
  EXPECT_TRUE(po->getValSkey(po, 'n', &v));
  EXPECT_EQ(v.i, 137);
  EXPECT_TRUE(po->getValSkey(po, 'i', &v));
  EXPECT_STREQ(v.s, "eth0");
  po->free(po);
}

// Test base command line parsing
TEST(cmdlTest, baseCfgFile)
{
  popt po;
  optRecVal v;
  const char *a[] = {
      "base",
      "-f", "utest/cmdl_base.cfg",
      nullptr
  };
  EXPECT_EQ(CMD_OK, cmd_base(3, (char **)a, &po, bopts));
  EXPECT_TRUE(po->getValSkey(po, 'f', &v));
  EXPECT_STREQ(v.s, "utest/cmdl_base.cfg");
  EXPECT_TRUE(po->getValSkey(po, 'n', &v));
  EXPECT_EQ(v.i, 153);
  po->free(po);
  const char *a2[] = {
      "base",
      "-f", "utest/cmdl_base.cfg",
      "-n", "137",
      nullptr
  };
  EXPECT_EQ(CMD_OK, cmd_base(5, (char **)a2, &po, bopts));
  EXPECT_TRUE(po->getValSkey(po, 'f', &v));
  EXPECT_STREQ(v.s, "utest/cmdl_base.cfg");
  EXPECT_TRUE(po->getValSkey(po, 'n', &v));
  EXPECT_EQ(v.i, 137); /* Command line take precedence */
  po->free(po);
}

// Test base version
TEST(cmdlTest, baseVer)
{
  popt po;
  const char *a_v[] = {
      "base",
      "-v",
      nullptr
  };
  useTestMode(true);
  EXPECT_EQ(CMD_MSG, cmd_base(2, (char **)a_v, &po, bopts));
  const char *r = getOut();
  EXPECT_EQ(0, strncmp("Version: ", r, 9));
}

// Test base help
TEST(cmdlTest, baseHelp)
{
  popt po;
  const char *a_h[] = {
      "base",
      "-h",
      nullptr
  };
  useTestMode(true);
  EXPECT_EQ(CMD_MSG, cmd_base(2, (char **)a_h, &po, bopts));
  const char *r = getOut();
  EXPECT_EQ(0, strncmp(a_h[0], r, strlen(a_h[0])));
}

// Test base error
TEST(cmdlTest, baseErr)
{
  popt po;
  optRecVal v;
  const char *a_e[] = {
      "base",
      "-t", "X",
      nullptr
  };
  useTestMode(true);
  EXPECT_EQ(CMD_ERR, cmd_base(3, (char **)a_e, &po, bopts));
  useTestMode(false);
}

// Test service command line parsing
TEST(cmdlTest, service)
{
  const char *a[] = {
      "service",
      "-y",
      "-e",
      "-i", "eth0",
      "-l", "3",
      "-r", "2",
      "-t", "2",
      "-6",
      nullptr
  };
  struct service_opt o;
  EXPECT_EQ(CMD_OK, cmd_service(12, (char **)a, &o));
  EXPECT_TRUE(o.useRxTwoSteps);
  EXPECT_TRUE(o.useRxTwoSteps);
  EXPECT_STREQ(o.ifName, "eth0");
  EXPECT_EQ(o.type, UDP_IPv6);
}

// Test service version
TEST(cmdlTest, serviceVer)
{
  const char *a_v[] = {
      "service",
      "-v",
      nullptr
  };
  struct service_opt o;
  useTestMode(true);
  EXPECT_EQ(CMD_MSG, cmd_service(2, (char **)a_v, &o));
  const char *r = getOut();
  EXPECT_EQ(0, strncmp("Version: ", r, 9));
}

// Test service help
TEST(cmdlTest, serviceHelp)
{
  const char *a_h[] = {
      "service",
      "-h",
      nullptr
  };
  struct service_opt o;
  useTestMode(true);
  EXPECT_EQ(CMD_MSG, cmd_service(2, (char **)a_h, &o));
  const char *r = getOut();
  EXPECT_EQ(0, strncmp(a_h[0], r, strlen(a_h[0])));
}

// Test service error
TEST(cmdlTest, serviceErr)
{
  const char *a_e[] = {
      "service",
      "-t", "X",
      nullptr
  };
  struct service_opt o;
  useTestMode(true);
  EXPECT_EQ(CMD_ERR, cmd_service(3, (char **)a_e, &o));
  useTestMode(false);
}

// Test client command line parsing
TEST(cmdlTest, client)
{
  const char *a[] = {
      "client",
      "-y",
      "-e",
      "-i", "eth0",
      "-l", "3",
      "-t",
      "-s",
      "-a",
      "-4",
      "-d", "4.3.2.1",
      "-n", "137",
      nullptr
  };
  struct client_opt o;
  EXPECT_EQ(CMD_OK, cmd_client(15, (char **)a, &o));
  EXPECT_EQ(o.type, UDP_IPv4);
  EXPECT_FALSE(o.useTwoSteps);
  EXPECT_TRUE(o.useCSPTPstatus);
  EXPECT_TRUE(o.useAltTimeScale);
  EXPECT_STREQ(o.ip, "4.3.2.1");
  EXPECT_STREQ(o.ifName, "eth0");
  EXPECT_EQ(o.domainNumber, 137);
}

// Test client version
TEST(cmdlTest, clientVer)
{
  const char *a_v[] = {
      "client",
      "-v",
      nullptr
  };
  struct client_opt o;
  useTestMode(true);
  EXPECT_EQ(CMD_MSG, cmd_client(2, (char **)a_v, &o));
  const char *r = getOut();
  EXPECT_EQ(0, strncmp("Version: ", r, 9));
}

// Test client help
TEST(cmdlTest, clientHelp)
{
  const char *a_h[] = {
      "client",
      "-h",
      nullptr
  };
  struct client_opt o;
  useTestMode(true);
  EXPECT_EQ(CMD_MSG, cmd_client(2, (char **)a_h, &o));
  const char *r = getOut();
  EXPECT_EQ(0, strncmp(a_h[0], r, strlen(a_h[0])));
}

// Test client error
TEST(cmdlTest, clientErr)
{
  const char *a_e[] = {
      "client",
      "-p",
      nullptr
  };
  struct client_opt o;
  useTestMode(true);
  EXPECT_EQ(CMD_ERR, cmd_client(3, (char **)a_e, &o));
  useTestMode(false);
}
