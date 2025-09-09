/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test main functions
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "libsys/libsys.h"

extern "C" {
#include "src/main.h"
}

// dummy MOCK of socket->poll
static bool dummy_poll(pcsock s, int t){return true;}
// dummy MOCK of socket->send
static bool dummy_send(pcsock s, pcbuffer b, pcipaddr a){return true;}

// printArr(b->getBuf(b), 0, 44);
static inline void printArr(uint8_t *t, size_t st, size_t sz)
{
  printf("start %zu, size %zu: ", st, sz);
  for(size_t i = st; i < st + sz; i++)
      printf("%d, ", t[i]);
  puts("");
}
static void utestClockInfo(struct service_opt *opt, struct ifClk_t *clk)
{
  clk->ifName = opt->ifName;
  memcpy(clk->clockIdentity, "\x1\x2\x3\xfe\xff\x4\x5\x6", 8);
  memcpy(clk->organizationId, "\x1\x2\x3", 3);
  memcpy(clk->organizationSubType, "\x4\x5\x6", 3);
  clk->networkProtocol = UDP_IPv4;
  clk->addressField = (uint8_t *)"\x1\x4\x7"; /* 1.4.7.0 */
  /* Clock parameters, comes from configuration */
  clk->priority1 = 127;
  clk->priority2 = 127;
  clk->clockQuality.clockClass = 12;
  clk->clockQuality.clockAccuracy = 73;
  clk->clockQuality.offsetScaledLogVariance = 7;
  clk->currentUtcOffset = 37;
  clk->keyField = 1;
  clk->currentOffset = 10800; // UTC + 3
  clk->jumpSeconds = 1;
  clk->timeOfNextJump = 175863;
  clk->TZName = "CEST";
}

// Test service create socket object
// psock service_main_create_socket(pcipaddr address)
TEST(mainServiceTest, createSocket)
{
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  EXPECT_TRUE(a->setIP4Str(a, "1.2.5.1"));
  useTestMode(true);
  psock s = service_main_create_socket(a);
  ASSERT_NE(s, nullptr);
  s->free(s);
  useTestMode(false);
  a->free(a);
}

// Test service received Request Sync message
// bool service_main_rcvReqSync(struct service_state_t *state, uint8_t *tlvReqFlags0)
TEST(mainServiceTest, rcvReqSync)
{
  uint8_t tlvReqFlags0;
  struct service_state_t st;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  st.message = m;
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  const static uint8_t d[160] = { // Sync message
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 127, 0, 0, 1, 0, 3, 0, 4, 0, 5, 0,
      // CSPTP_REQUEST 8 octets
      0xff, 0, 0, 4, 3, 0, 0, 0,
      // PAD of 108 octets
      0x80, 0x08, 0, 104
  };
  memcpy(b->getBuf(b), d, 160);
  ASSERT_TRUE(b->setLen(b, 160));
  struct ptp_params_t p;
  ASSERT_TRUE(m->parse(m, &p, b));
  EXPECT_TRUE(service_main_rcvReqSync(&st, &tlvReqFlags0));
  EXPECT_EQ(tlvReqFlags0, Flags0_Req_StatusTlv | Flags0_Req_AlternateTimeTlv);
  b->free(b);
  m->free(m);
}

// MOCK of socket->recv for responed Sync with one step
static bool recv_ReqSync(pcsock s, pbuffer b, pipaddr a, pts t)
{
  const static uint8_t d[160] = { // Sync message
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 127, 0, 0, 1, 0, 3, 0, 5, 0, 0, 0,
      // CSPTP_REQUEST 8 octets
      0xff, 0, 0, 4, 3, 0, 0, 0,
      // PAD of 108 octets
      0x80, 0x08, 0, 104
  };
  memcpy(b->getBuf(b), d, 160);
  return b->setLen(b, 160);
}

// Test service received Request Sync message
// bool service_main_flow(struct service_state_t *state, bool useTxTwoSteps)
TEST(mainServiceTest, mainFlow)
{
  struct service_state_t st;
  struct ifClk_t clockInfo;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  st.message = m;
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  st.buffer = b;
  clockInfo.TZName = "CEST";
  clockInfo.addressField = (uint8_t*)"\x3\x2\x1";
  clockInfo.networkProtocol = UDP_IPv4;
  st.clockInfo = &clockInfo;
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  st.address = a;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  st.rxTs = t;
  pts t2 = ts_alloc();
  ASSERT_NE(t2, nullptr);
  st.t2 = t2;
  useTestMode(true);
  psock s = service_main_create_socket(a);
  ASSERT_NE(s, nullptr);
  st.socket = s;
  s->poll = dummy_poll; // dummy MOCK socket poll function!
  s->send = dummy_send; // dummy MOCK socket send function!
  s->recv = recv_ReqSync; // MOCK socket send function!
  setReal(2); // Set t2 value
  EXPECT_TRUE(service_main_flow(&st, false));
  EXPECT_EQ(t2->getTs(t2), 2000000000); // t2 value
  setReal(3); // Set t2 value
  EXPECT_TRUE(service_main_flow(&st, true));
  EXPECT_EQ(t2->getTs(t2), 3000000000); // t2 value
  s->free(s);
  useTestMode(false);
  t2->free(t2);
  t->free(t);
  a->free(a);
  b->free(b);
  m->free(m);
}

// Test service allocating objects
// bool service_main_allocObjs(struct service_opt *options)
// void service_main_clean()
TEST(mainServiceTest, createObjs)
{
  struct service_opt opt;
  struct service_state_t st;
  opt.useRxTwoSteps = true;
  opt.useRxTwoSteps = true;
  opt.type = UDP_IPv4;
  useTestMode(true);
  EXPECT_TRUE(service_main_allocObjs(&opt, &st));
  service_main_clean(&st);
  useTestMode(false);
}

// MOCK of socket->send
static bool sendRespSync(pcsock s, pcbuffer b, pcipaddr a)
{
  if(b->getLen(b) != 160)
      return false;
  const static uint8_t d[160] = { // Sync message
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // CSPTP_RESPONSE 28 octets
      0xff, 1, 0, 24, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // CSPTP_STATUS IPv4 36 octets
      0xf0, 2, 0, 32, 1, 2, 3, 4, 5, 6, 127, 12, 73, 0, 7, 127, 0, 0, 0, 37, 1, 2, 3, 254, 255, 4, 5, 6,
      0, 1, 0, 4, 1, 4, 7, 0,
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 4
      0, 9, 0, 20, 1, 0, 0, 42, 48, 0, 0, 0, 1, 0, 0, 0, 2, 174, 247, 4, 'C', 'E', 'S', 'T',
      // PAD of 28 octets
      0x80, 0x08, 0, 24
  };
  return memcmp(d, b->getBuf(b), 160) == 0;
}
// Test service send request Sync message to service
// bool service_main_sendRespSync(struct service_state_t *state, size_t size, uint8_t tlvRequestFlags0)
TEST(mainServiceTest, sendRespSync)
{
  struct service_state_t st;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  st.message = m;
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  st.address = a;
  EXPECT_TRUE(a->setIP4Str(a, "1.2.5.1"));
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  st.buffer = b;
  memset(&st.params, 0, sizeof(struct ptp_params_t));
  st.params.sequenceId = 71;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  st.t2 = t;
  pts rx = ts_alloc();
  ASSERT_NE(rx, nullptr);
  st.rxTs = rx;
  struct service_opt opt;
  opt.ifName = "eth0";
  struct ifClk_t clockInfo;
  utestClockInfo(&opt, &clockInfo);
  st.clockInfo = &clockInfo;
  useTestMode(true);
  psock s = service_main_create_socket(a);
  ASSERT_NE(s, nullptr);
  st.socket = s;
  s->send = sendRespSync; // MOCK socket send function!
  EXPECT_TRUE(service_main_sendRespSync(&st, 160, Flags0_Req_StatusTlv | Flags0_Req_AlternateTimeTlv));
  s->free(s);
  useTestMode(false);
  rx->free(rx);
  t->free(t);
  b->free(b);
  a->free(a);
  m->free(m);
}

// MOCK of socket->send
static bool respFollowUp(pcsock s, pcbuffer b, pcipaddr a)
{
  if(b->getLen(b) != 160)
      return false;
  const static uint8_t d[160] = { // FollowUp message
      // Header 44 octests
      0x38, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 2, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // PAD of 116 octets
      0x80, 0x08, 0, 112
  };
  return memcmp(d, b->getBuf(b), 160) == 0;
}
// Test service send FollowUp message to service
// bool service_main_sendFollowUp(struct service_state_t *state, size_t size)
TEST(mainServiceTest, sendFollowUp)
{
  struct service_state_t st;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  st.message = m;
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  st.address = a;
  EXPECT_TRUE(a->setIP4Str(a, "1.2.5.1"));
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  st.buffer = b;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  st.t2 = t;
  memset(&st.params, 0, sizeof(struct ptp_params_t));
  st.params.sequenceId = 71;
  useTestMode(true);
  psock s = service_main_create_socket(a);
  ASSERT_NE(s, nullptr);
  st.socket = s;
  s->send = respFollowUp; // MOCK socket send function!
  EXPECT_TRUE(service_main_sendFollowUp(&st, 160));
  s->free(s);
  useTestMode(false);
  t->free(t);
  b->free(b);
  a->free(a);
  m->free(m);
}

// Test client create address object
// pipaddr client_main_create_address(struct client_opt *option, prot *type)
TEST(mainClientTest, createAddress)
{
  prot type;
  struct client_opt opt;
  opt.ip = "102:304::1";
  opt.domainNumber = 200;
  opt.type = Invalid_PROTO;
  struct log_options_t lopt = {
    .log_level = LOG_DEBUG,
    .useSysLog = false,
    .useEcho = true
  };
  EXPECT_TRUE(setLog("", &lopt));
  useTestMode(true);
  pipaddr a = client_main_create_address(&opt, &type);
  EXPECT_EQ(0, strncmp(getOut(), "[7:main_client.c:", 17));
  EXPECT_STREQ(strrchr(getOut(), ']'), "] Server host 102:304::1, IP 102:304::1\n");
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(type, UDP_IPv6);
  EXPECT_STREQ(a->getIPStr(a), "102:304::1");
  a->free(a);
  opt.ip = "1.2.3.4";
  useTestMode(true);
  a = client_main_create_address(&opt, &type);
  EXPECT_EQ(0, strncmp(getOut(), "[7:main_client.c:", 17));
  EXPECT_STREQ(strrchr(getOut(), ':'), ":client_main_create_address] Server host 1.2.3.4, IP 1.2.3.4\n");
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(type, UDP_IPv4);
  EXPECT_STREQ(a->getIPStr(a), "1.2.3.4");
  a->free(a);
}

// Test client create socket object
// psock client_main_create_socket(prot type)
TEST(mainClientTest, createSocket)
{
  useTestMode(true);
  psock s = client_main_create_socket(UDP_IPv4);
  ASSERT_NE(s, nullptr);
  s->free(s);
  useTestMode(false);
}

// Test client calculate message size
// size_t client_main_get_msg_size(struct client_opt *option, pmsg message, prot type)
TEST(mainClientTest, calcMsgSize)
{
  struct client_opt opt;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = true;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv4), 138);
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = false;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv4), 108);
  opt.useCSPTPstatus = false;
  opt.useAltTimeScale = true;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv4), 102);
  opt.useCSPTPstatus = false;
  opt.useAltTimeScale = false;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv4), 72);
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = true;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv6), 150);
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = false;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv6), 120);
  opt.useCSPTPstatus = false;
  opt.useAltTimeScale = true;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv6), 102);
  opt.useCSPTPstatus = false;
  opt.useAltTimeScale = false;
  EXPECT_EQ(client_main_get_msg_size(&opt, m, UDP_IPv6), 72);
  m->free(m);
}

// Test client smooth size
// size_t client_main_smooth_size(size_t size)
TEST(mainClientTest, smoothSize)
{
  EXPECT_EQ(client_main_smooth_size(138), 0xa0);
  EXPECT_EQ(client_main_smooth_size(108), 0x80);
  EXPECT_EQ(client_main_smooth_size(102), 0x70);
  EXPECT_EQ(client_main_smooth_size(72), 0x60);
  EXPECT_EQ(client_main_smooth_size(150), 0xa0);
  EXPECT_EQ(client_main_smooth_size(120), 0x90);
}

// Test client set TX parameters
// uint8_t client_main_set_tx_params(struct client_opt *opt, pparms parameters)
TEST(mainClientTest, setTxParams)
{
  struct client_opt opt = { 0 };
  struct ptp_params_t p;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  opt.useTwoSteps = true;
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = true;
  opt.domainNumber = 200;
  opt.type = Invalid_PROTO;
  uint8_t tlvRequestFlags0 = client_main_set_tx_params(&opt, &p);
  EXPECT_EQ(tlvRequestFlags0, Flags0_Req_StatusTlv | Flags0_Req_AlternateTimeTlv);
  EXPECT_TRUE(p.useTwoSteps);
  EXPECT_EQ(p.type, Sync);
  EXPECT_EQ(p.domainNumber, 200);
  EXPECT_EQ(p.correctionField, 0);
  EXPECT_EQ(p.sequenceId, 0);
  EXPECT_EQ(p.flagField2, 0);
  t->fromTimestamp(t, &p.timestamp);
  EXPECT_EQ(t->getTs(t), 0);
  opt.useTwoSteps = false;
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = false;
  tlvRequestFlags0 = client_main_set_tx_params(&opt, &p);
  EXPECT_EQ(tlvRequestFlags0, Flags0_Req_StatusTlv);
  EXPECT_FALSE(p.useTwoSteps);
  opt.useCSPTPstatus = false;
  opt.useAltTimeScale = true;
  tlvRequestFlags0 = client_main_set_tx_params(&opt, &p);
  EXPECT_EQ(tlvRequestFlags0, Flags0_Req_AlternateTimeTlv);
  opt.useCSPTPstatus = false;
  opt.useAltTimeScale = false;
  tlvRequestFlags0 = client_main_set_tx_params(&opt, &p);
  EXPECT_EQ(tlvRequestFlags0, 0);
  t->free(t);
}

// Test client receive responce Sync message
// bool client_main_rcvRespSync(struct client_state_t *state)
TEST(mainClientTest, rcvRespSync)
{
  struct client_state_t st;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  st.message = m;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  st.r1 = t;
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  st.buffer = b;
  const static uint8_t d[160] = { // Sync message
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // CSPTP_RESPONSE 28 octets
      0xff, 1, 0, 24, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // CSPTP_STATUS IPv4 36 octets
      0xf0, 2, 0, 32, 1, 2, 3, 4, 5, 6, 127, 12, 73, 0, 7, 127, 0, 0, 0, 37, 1, 2, 3, 254, 255, 4, 5, 6,
      0, 1, 0, 4, 1, 4, 7, 0,
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 4
      0, 9, 0, 20, 1, 0, 0, 42, 48, 0, 0, 0, 1, 0, 0, 0, 2, 174, 247, 4, 'C', 'E', 'S', 'T',
      // PAD of 28 octets
      0x80, 0x08, 0, 24
  };
  memcpy(b->getBuf(b), d, 160);
  ASSERT_TRUE(b->setLen(b, 160));
  struct ptp_params_t p;
  ASSERT_TRUE(m->parse(m, &p, b));
  t->setTs(t, 117);
  EXPECT_TRUE(client_main_rcvRespSync(&st));
  EXPECT_EQ(t->getTs(t), 0); // r1 timestamp in CSPTP_RESPONSE TLV
  b->free(b);
  t->free(t);
  m->free(m);
}

// MOCK of socket->recv for FollowUp with one step
static bool recv_FollowUpOneStep(pcsock s, pbuffer b, pipaddr a, pts t)
{
  const static uint8_t d[160] = { // FollowUp message
      // Header 44 octests
      0x38, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 2, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // PAD of 116 octets
      0x80, 0x08, 0, 112
  };
  memcpy(b->getBuf(b), d, 160);
  return b->setLen(b, 160);
}
// MOCK of socket->recv for responed Sync with one step
static bool recv_RespSyncOneStep(pcsock s, pbuffer b, pipaddr a, pts t)
{
  const static uint8_t d[160] = { // Sync message
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 0, 127, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
      // CSPTP_RESPONSE 28 octets
      0xff, 1, 0, 24, 1, 2, 3, 4, 5, 6, 0, 0, 7, 0, 0, 6, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // CSPTP_STATUS IPv4 36 octets
      0xf0, 2, 0, 32, 1, 2, 3, 4, 5, 6, 127, 12, 73, 0, 7, 127, 0, 0, 0, 37, 1, 2, 3, 254, 255, 4, 5, 6,
      0, 1, 0, 4, 1, 4, 7, 0,
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 4
      0, 9, 0, 20, 1, 0, 0, 42, 48, 0, 0, 0, 1, 0, 0, 0, 2, 174, 247, 4, 'C', 'E', 'S', 'T',
      // PAD of 28 octets
      0x80, 0x08, 0, 24
  };
  setReal(3); // Set r2 value
  getUtcClock(t);
  psock s0 = (psock)s;
  s0->recv = recv_FollowUpOneStep; // MOCK socket send function!
  memcpy(b->getBuf(b), d, 160);
  return b->setLen(b, 160);
}
// Test client main flow with one step responce
// bool client_main_flow(struct client_state_t *state, uint8_t domainNumber, bool useTwoSteps, uint16_t *sequenceId)
TEST(mainClientTest, mainFlowOneStep)
{
  struct client_opt opt;
  struct client_state_t st;
  uint16_t sequenceId = 71;
  opt.ip = "1.2.3.4";
  opt.domainNumber = 200;
  opt.useTwoSteps = true;
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = true;
  opt.type = Invalid_PROTO;
  useTestMode(true);
  EXPECT_TRUE(client_main_allocObjs(&opt, &st));
  st.socket->poll = dummy_poll; // dummy MOCK socket poll function!
  st.socket->send = dummy_send; // dummy MOCK socket send function!
  st.socket->recv = recv_RespSyncOneStep; // MOCK socket send function!
  EXPECT_EQ(st.buffer->getSize(st.buffer), 160);
  EXPECT_TRUE(st.RxAddress->setIP(st.RxAddress, st.address->getIP(st.address)));
  setReal(2); // Set t1 value
  EXPECT_TRUE(client_main_flow(&st, 0, true, &sequenceId));
  EXPECT_EQ(sequenceId, 72);
  EXPECT_EQ(st.t1->getTs(st.t1), 2000000000);
  EXPECT_EQ(st.r1->getTs(st.r1), 117440518000001024);
  EXPECT_EQ(st.t2->getTs(st.t2), 1347513023544870154);
  EXPECT_EQ(st.r2->getTs(st.r2), 3000000000);
  client_main_clean(&st);
  useTestMode(false);
}

// MOCK of socket->recv for FollowUp with two steps
static bool recv_FollowUpTwoSteps(pcsock s, pbuffer b, pipaddr a, pts t)
{
  const static uint8_t d[160] = { // FollowUp message
      // Header 44 octests
      0x38, 18, 0, 160, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 2, 127, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
      // PAD of 116 octets
      0x80, 0x08, 0, 112
  };
  memcpy(b->getBuf(b), d, 160);
  return b->setLen(b, 160);
}
// MOCK of socket->recv for responed Sync with two steps
static bool recv_RespSyncTwoSteps(pcsock s, pbuffer b, pipaddr a, pts t)
{
  const static uint8_t d[160] = { // Sync message
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 0, 127, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
      // CSPTP_RESPONSE 28 octets
      0xff, 1, 0, 24, 1, 2, 3, 4, 5, 6, 0, 0, 7, 0, 0, 6, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // CSPTP_STATUS IPv4 36 octets
      0xf0, 2, 0, 32, 1, 2, 3, 4, 5, 6, 127, 12, 73, 0, 7, 127, 0, 0, 0, 37, 1, 2, 3, 254, 255, 4, 5, 6,
      0, 1, 0, 4, 1, 4, 7, 0,
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 4
      0, 9, 0, 20, 1, 0, 0, 42, 48, 0, 0, 0, 1, 0, 0, 0, 2, 174, 247, 4, 'C', 'E', 'S', 'T',
      // PAD of 28 octets
      0x80, 0x08, 0, 24
  };
  setReal(3); // Set r2 value
  getUtcClock(t);
  psock s0 = (psock)s;
  s0->recv = recv_FollowUpTwoSteps; // MOCK socket send function!
  memcpy(b->getBuf(b), d, 160);
  return b->setLen(b, 160);
}
// Test client main flow with one step responce
TEST(mainClientTest, mainFlowTwoSteps)
{
  struct client_opt opt;
  struct client_state_t st;
  uint16_t sequenceId = 71;
  opt.ip = "1.2.3.4";
  opt.domainNumber = 200;
  opt.useTwoSteps = true;
  opt.useCSPTPstatus = true;
  opt.useAltTimeScale = true;
  opt.type = Invalid_PROTO;
  useTestMode(true);
  EXPECT_TRUE(client_main_allocObjs(&opt, &st));
  st.socket->poll = dummy_poll; // dummy MOCK socket poll function!
  st.socket->send = dummy_send; // dummy MOCK socket send function!
  st.socket->recv = recv_RespSyncTwoSteps; // MOCK socket send function!
  EXPECT_EQ(st.buffer->getSize(st.buffer), 160);
  EXPECT_TRUE(st.RxAddress->setIP(st.RxAddress, st.address->getIP(st.address)));
  setReal(2); // Set t1 value
  EXPECT_TRUE(client_main_flow(&st, 0, true, &sequenceId));
  EXPECT_EQ(sequenceId, 72);
  EXPECT_EQ(st.t1->getTs(st.t1), 2000000000);
  EXPECT_EQ(st.r1->getTs(st.r1), 117440518000001024);
  EXPECT_EQ(st.t2->getTs(st.t2), 1347513023544870154);
  EXPECT_EQ(st.r2->getTs(st.r2), 3000000000);
  client_main_clean(&st);
  useTestMode(false);
}

// Test client allocating objects
// bool client_main_allocObjs(struct client_opt *options, struct client_state_t *state)
// void client_main_clean(struct client_state_t *state)
TEST(mainClientTest, createObjs)
{
  struct client_opt opt;
  struct client_state_t st;
  opt.ip = "1.2.3.4";
  opt.domainNumber = 200;
  opt.useTwoSteps = true;
  opt.useCSPTPstatus = false;
  opt.useAltTimeScale = false;
  opt.type = Invalid_PROTO;
  useTestMode(true);
  EXPECT_TRUE(client_main_allocObjs(&opt, &st));
  EXPECT_EQ(st.type, UDP_IPv4);
  EXPECT_EQ(st.size, 0x60);
  client_main_clean(&st);
  useTestMode(false);
}

// MOCK of socket->send
static bool sendReqSync(pcsock s, pcbuffer b, pcipaddr a)
{
  if(b->getLen(b) != 160)
      return false;
  const static uint8_t d[160] = { // Sync message
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // CSPTP_REQUEST 8 octets
      0xff, 0, 0, 4, 3, 0, 0, 0,
      // PAD of 108 octets
      0x80, 0x08, 0, 104
  };
  return memcmp(d, b->getBuf(b), 160) == 0;
}
// Test client send request Sync message to service
// bool client_main_sendReqSync(struct client_state_t *state, uint16_t sequenceId)
TEST(mainClientTest, sendReqSync)
{
  struct client_state_t st;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  st.message = m;
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  st.address = a;
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  st.buffer = b;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  st.t1 = t;
  memset(&st.params, 0, sizeof(struct ptp_params_t));
  st.size = 160;
  st.tlvReqFlags0 = Flags0_Req_StatusTlv | Flags0_Req_AlternateTimeTlv;
  useTestMode(true);
  psock s = client_main_create_socket(UDP_IPv4);
  ASSERT_NE(s, nullptr);
  st.socket = s;
  s->send = sendReqSync; // MOCK socket send function!
  EXPECT_TRUE(client_main_sendReqSync(&st, 17));
  s->free(s);
  useTestMode(false);
  t->free(t);
  b->free(b);
  a->free(a);
  m->free(m);
}

// MOCK of socket->send
static bool sendFollowUp(pcsock s, pcbuffer b, pcipaddr a)
{
  if(b->getLen(b) != 160)
      return false;
  const static uint8_t d[160] = { // FollowUp message
      // Header 44 octests
      0x38, 18, 0, 160, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 2, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      // PAD of 116 octets
      0x80, 0x08, 0, 112
  };
  return memcmp(d, b->getBuf(b), 160) == 0;
}
// Test client send FollowUp message to service
// bool client_main_sendFollowUp(struct client_state_t *state, uint16_t sequenceId)
TEST(mainClientTest, sendFollowUp)
{
  struct client_state_t st;
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  st.message = m;
  pipaddr a = addr_alloc(UDP_IPv4);
  ASSERT_NE(a, nullptr);
  st.address = a;
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  st.buffer = b;
  memset(&st.params, 0, sizeof(struct ptp_params_t));
  st.size = 160;
  useTestMode(true);
  psock s = client_main_create_socket(UDP_IPv4);
  ASSERT_NE(s, nullptr);
  st.socket = s;
  s->send = sendFollowUp; // MOCK socket send function!
  EXPECT_TRUE(client_main_sendFollowUp(&st, 17));
  s->free(s);
  useTestMode(false);
  b->free(b);
  a->free(a);
  m->free(m);
}
