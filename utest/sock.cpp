/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test socket to communicate with CSPTP
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/sock.h"
}

#include <sys/socket.h>

// Test IPv4 address object
// void free(pipaddr self)
// const struct sockaddr *getAddr(pipaddr self)
// size_t getSize(pipaddr self)
// size_t getIPSize(pcipaddr self)
// uint16_t getPort(pcipaddr self)
// void setPort(pipaddr self, uint16_t port)
// void setAnyIP(pipaddr self)
// bool isAnyIP(pcipaddr self)
// prot getType(pcipaddr self)
// bool setIP4(pipaddr self, const uint8_t *ip)
// bool setIP4Val(pipaddr self, uint32_t ip)
// bool setIP4Str(pipaddr self, const char *ip)
// const uint8_t *getIP4(pcipaddr self)
// uint32_t getIP4Val(pcipaddr self)
// const char *getIP4Str(pipaddr self)
// pipaddr addr_alloc(prot type)
// const uint8_t *getIP(pcipaddr self)
// const char *getIPStr(pipaddr self)
// bool setIPStr(pipaddr self, const char *ip)
// bool eq(pcipaddr self, pcipaddr other)
TEST(addressTest, ipv4)
{
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(a->getSize(a), 16);
  EXPECT_EQ(a->getIPSize(a), 4);
  EXPECT_EQ(a->getPort(a), 320);
  a->setPort(a, 3232);
  EXPECT_EQ(a->getPort(a), 3232);
  EXPECT_EQ(a->getType(a), UDP_IPv4);
  EXPECT_TRUE(a->isAnyIP(a));
  EXPECT_TRUE(a->setIP4Val(a, 0x01020304));
  EXPECT_FALSE(a->isAnyIP(a));
  EXPECT_EQ(a->getIP4Val(a), 0x01020304);
  EXPECT_EQ(0, memcmp(a->getIP4(a), "\x1\x2\x3\x4", IPV4_ADDR_LEN));
  EXPECT_EQ(0, memcmp(a->getIP(a), "\x1\x2\x3\x4", IPV4_ADDR_LEN));
  EXPECT_STREQ(a->getIP4Str(a), "1.2.3.4");
  EXPECT_STREQ(a->getIPStr(a), "1.2.3.4");
  static const struct sockaddr s = {
    .sa_family = 2,
    .sa_data = { 12, -96, 1, 2, 3, 4 }
  };
  EXPECT_EQ(0, memcmp(a->getAddr(a), &s, 16));
  a->setAnyIP(a);
  EXPECT_TRUE(a->isAnyIP(a));
  EXPECT_TRUE(a->setIP4Str(a, "4.3.2.1"));
  EXPECT_FALSE(a->isAnyIP(a));
  EXPECT_EQ(a->getIP4Val(a), 0x04030201);
  EXPECT_EQ(0, memcmp(a->getIP4(a), "\x4\x3\x2\x1", IPV4_ADDR_LEN));
  EXPECT_EQ(0, memcmp(a->getIP(a), "\x4\x3\x2\x1", IPV4_ADDR_LEN));
  uint8_t ip[IPV4_ADDR_LEN] = { 7, 3, 4, 6 };
  EXPECT_TRUE(a->setIP4(a, ip));
  EXPECT_STREQ(a->getIP4Str(a), "7.3.4.6");
  EXPECT_STREQ(a->getIPStr(a), "7.3.4.6");
  pipaddr a2 = addr_alloc(UDP_IPv4);
  ASSERT_NE(a2, nullptr);
  EXPECT_TRUE(a2->setIPStr(a2, "7.3.4.6"));
  a2->setPort(a2, 3232);
  EXPECT_TRUE(a->eq(a, a2));
  a2->free(a2);
  a->free(a);
}

// Test IPv6 address object
// bool setIP6(pipaddr self, const uint8_t *ip)
// bool setIP6Str(pipaddr self, const char *ip)
// const uint8_t *getIP6(pcipaddr self)
// const char *getIP6Str(pipaddr self)
// bool setIP(pipaddr self, const uint8_t *ip)
TEST(addressTest, ipv6)
{
  pipaddr a = addr_alloc(UDP_IPv6);
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(a->getSize(a), 28);
  EXPECT_EQ(a->getIPSize(a), 16);
  static const uint8_t ip[IPV6_ADDR_LEN] = { 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  EXPECT_EQ(a->getPort(a), 320);
  a->setPort(a, 3232);
  EXPECT_EQ(a->getPort(a), 3232);
  EXPECT_EQ(a->getType(a), UDP_IPv6);
  EXPECT_TRUE(a->isAnyIP(a));
  EXPECT_TRUE(a->setIP6(a, ip));
  EXPECT_FALSE(a->isAnyIP(a));
  EXPECT_TRUE(a->setIP(a, ip));
  EXPECT_EQ(0, memcmp(a->getIP6(a), ip, IPV6_ADDR_LEN));
  EXPECT_EQ(0, memcmp(a->getIP(a), ip, IPV6_ADDR_LEN));
  EXPECT_STREQ(a->getIP6Str(a), "102:304::1");
  EXPECT_STREQ(a->getIPStr(a), "102:304::1");
  static const uint8_t s[28] = { 10, 0, 12, 160, 0, 0, 0, 0, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  EXPECT_EQ(0, memcmp(a->getAddr(a), &s, 28));
  a->setAnyIP(a);
  EXPECT_TRUE(a->isAnyIP(a));
  EXPECT_TRUE(a->setIP6Str(a, "100:90::1"));
  EXPECT_FALSE(a->isAnyIP(a));
  EXPECT_STREQ(a->getIP6Str(a), "100:90::1");
  EXPECT_STREQ(a->getIPStr(a), "100:90::1");
  pipaddr a2 = addr_alloc(UDP_IPv6);
  ASSERT_NE(a2, nullptr);
  EXPECT_TRUE(a2->setIPStr(a2, "100:90::1"));
  a2->setPort(a2, 3232);
  EXPECT_TRUE(a->eq(a, a2));
  a2->free(a2);
  a->free(a);
}

// Test converting of adresses
// bool addressStringToBinary(const char *string, prot *type, uint8_t *binary)
TEST(sockTest, convert)
{
  uint8_t bin[IPV6_ADDR_LEN];
  prot type = Invalid_PROTO;
  static const uint8_t b1[IPV6_ADDR_LEN] = { 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  EXPECT_TRUE(addressStringToBinary("102:304::1", &type, bin));
  EXPECT_EQ(0, memcmp(bin, b1, IPV6_ADDR_LEN));
  EXPECT_EQ(type, UDP_IPv6);
  type = UDP_IPv6;
  EXPECT_TRUE(addressStringToBinary("102:304::1", &type, bin));
  EXPECT_EQ(0, memcmp(bin, b1, IPV6_ADDR_LEN));
  EXPECT_EQ(type, UDP_IPv6);
  type = UDP_IPv4;
  useTestMode(true);
  EXPECT_FALSE(addressStringToBinary("102:304::1", &type, bin));
  EXPECT_EQ(0, strncmp(getErr(), "[3:sock.c:", 10));
  EXPECT_STREQ(strrchr(getErr(), ']'), "] getaddrinfo: Address family for hostname not supported\n");
  type = Invalid_PROTO;
  EXPECT_TRUE(addressStringToBinary("1.2.3.4", &type, bin));
  EXPECT_EQ(0, memcmp(bin, "\x1\x2\x3\x4", IPV4_ADDR_LEN));
  EXPECT_EQ(type, UDP_IPv4);
  type = UDP_IPv4;
  EXPECT_TRUE(addressStringToBinary("1.2.3.4", &type, bin));
  EXPECT_EQ(0, memcmp(bin, "\x1\x2\x3\x4", IPV4_ADDR_LEN));
  EXPECT_EQ(type, UDP_IPv4);
  type = UDP_IPv4;
  EXPECT_TRUE(addressStringToBinary("localhost", &type, bin));
  EXPECT_EQ(0, memcmp(bin, "\x7f\x0\x0\x1", IPV4_ADDR_LEN));
  EXPECT_EQ(type, UDP_IPv4);
  static const uint8_t localhost6[IPV6_ADDR_LEN] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  type = Invalid_PROTO;
  EXPECT_TRUE(addressStringToBinary("::1", &type, bin));
  EXPECT_EQ(0, memcmp(bin, localhost6, IPV6_ADDR_LEN));
  EXPECT_EQ(type, UDP_IPv6);
}

// Tests socket object for client
// void free(psock self)
// void close(psock self)
// int fileno(pcsock self)
// bool init(psock self, prot type)
// bool send(pcsock self, pcbuffer buffer, pcipaddr address)
// bool recv(pcsock self, pbuffer buffer, pipaddr address, pts ts)
// bool poll(pcsock self, int timeout)
// prot getType(pcsock self)
// psock sock_alloc()
TEST(sockTest, client)
{
  psock s = sock_alloc();
  ASSERT_NE(s, nullptr);
  useTestMode(true);
  EXPECT_TRUE(s->init(s, UDP_IPv4));
  EXPECT_EQ(s->fileno(s), 7);
  EXPECT_EQ(s->getType(s), UDP_IPv4);
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  EXPECT_TRUE(a->setIP4Str(a, "1.2.5.1"));
  pbuffer b = buffer_alloc(10);
  ASSERT_NE(b, nullptr);
  memcpy(b->getBuf(b), "test1246xx", 10);
  EXPECT_TRUE(b->setLen(b, 10));
  EXPECT_TRUE(s->send(s, b, a));
  EXPECT_TRUE(s->poll(s, 17));
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  EXPECT_TRUE(s->recv(s, b, a, t));
  EXPECT_EQ(b->getLen(b), 4);
  EXPECT_EQ(memcmp(b->getBuf(b), "test", 4), 0);
  EXPECT_EQ(a->getPort(a), 2567);
  EXPECT_TRUE(a->setIPStr(a, "1.10.5.10"));
  EXPECT_TRUE(s->close(s));
  t->free(t);
  b->free(b);
  a->free(a);
  s->free(s);
  useTestMode(false);
}

// Tests socket object for service
// bool initSrv(psock self, pcipaddr address, pif interface)
TEST(sockTest, service)
{
  pif i = if_alloc();
  ASSERT_NE(i, nullptr);
  useTestMode(true);
  psock s = sock_alloc();
  ASSERT_NE(s, nullptr);
  EXPECT_TRUE(i->intIfIndex(i, 2, false));
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  EXPECT_TRUE(a->setIP4Str(a, "1.2.5.1"));
  EXPECT_TRUE(s->initSrv(s, a, i));
  EXPECT_EQ(s->fileno(s), 7);
  EXPECT_EQ(s->getType(s), UDP_IPv4);
  EXPECT_TRUE(s->close(s));
  a->free(a);
  s->free(s);
  useTestMode(false);
  i->free(i);
}
