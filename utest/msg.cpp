/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief test message objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

extern "C" {
#include "src/msg.h"
}

// Test sizes
// void free(pmsg self)
// size_t getPTPMsgSize()
// size_t getTlvSize(enum tlv_type_id id)
// size_t getCSPTPStatusTlvSize(prot protocol)
// pmsg msg_alloc()
TEST(messageTest, sizes)
{
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  // 20 TLV size + 10 maximum time zone
  EXPECT_EQ(m->getTlvSize(ALTERNATE_TIME_OFFSET_INDICATOR_id), 30);
  EXPECT_EQ(m->getTlvSize(CSPTP_REQUEST_id), 8);
  EXPECT_EQ(m->getTlvSize(CSPTP_RESPONSE_id), 28);
  EXPECT_EQ(m->getCSPTPStatusTlvSize(UDP_IPv4), 36);
  EXPECT_EQ(m->getCSPTPStatusTlvSize(UDP_IPv6), 48);
  EXPECT_EQ(m->getPTPMsgSize(), 44);
  m->free(m);
}

// Test parsing messages
// bool parse(pmsg self, pparms params, pbuffer buffer)
// enum messageType_e getMsgType(pcmsg self)
// size_t getMsgLen(pcmsg self)
// size_t getTlvs(pcmsg self)
// size_t getTlvLen(pcmsg self, size_t index)
// enum tlv_type_id getTlvID(pcmsg self, size_t index)
// void *getTlv(pcmsg self, size_t index)
// bool copy(pmsg self, pbuffer buffer)
TEST(messageTest, parse)
{
  pbuffer b = buffer_alloc(160);
  ASSERT_NE(b, nullptr);
  EXPECT_TRUE(b->setLen(b, 160));
  const static uint8_t d[160] = { // Use Sync
      // Header 44 octests
      0x30, 18, 0, 160, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 1, 0, 2, 0, 4, 0, 6, 0,
      // CSPTP_REQUEST 8 octets
      0xff, 0, 0, 4, 3, 0, 0, 0,
      // CSPTP_RESPONSE 28 octets
      0xff, 1, 0, 24, 1, 2, 3, 4, 5, 6, 0, 0, 4, 0, 3, 0, 0, 4, 0, 3, 1, 2, 3, 4, 5, 6, 7, 8,
      // CSPTP_STATUS IPv6 48 octets
      0xf0, 2, 0, 44, 1, 2, 3, 4, 5, 6, 127, 37, 0xfe, 1, 1, 201, 2, 2, 0xe, 0x78, 1, 2, 3, 4, 5, 6, 7, 8,
      0, 2, 0, 16, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
      // PAD of 32 octets
      0x80, 0x08, 0, 28
  };
  memcpy(b->getBuf(b), d, 160);
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  struct ptp_params_t p;
  EXPECT_TRUE(m->parse(m, &p, b));
  EXPECT_EQ(p.type, Sync);
  EXPECT_EQ(m->getMsgType(m), Sync);
  EXPECT_EQ(p.domainNumber, 0);
  EXPECT_EQ(p.correctionField, 0);
  EXPECT_EQ(p.sequenceId, 0);
  EXPECT_EQ(p.flagField2, 0);
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  EXPECT_TRUE(t->fromTimestamp(t, &p.timestamp));
  EXPECT_EQ(t->getTs(t), 16777728067110400);
  EXPECT_TRUE(p.useTwoSteps);
  EXPECT_EQ(m->getMsgLen(m), 160);
  EXPECT_EQ(m->getTlvs(m), 4);
  EXPECT_EQ(m->getTlvLen(m, 0), 8);
  EXPECT_EQ(m->getTlvID(m, 0), CSPTP_REQUEST_id);
  struct CSPTP_REQUEST_t *rq = (struct CSPTP_REQUEST_t *)m->getTlv(m, 0);
  ASSERT_NE(rq, nullptr);
  EXPECT_EQ(rq->hdr.tlvType, CSPTP_REQUEST_id);
  EXPECT_EQ(rq->hdr.lengthField, 4);
  EXPECT_EQ(rq->tlvRequestFlags[0], Flags0_Req_StatusTlv | Flags0_Req_AlternateTimeTlv);
  EXPECT_EQ(rq->tlvRequestFlags[1], 0);
  EXPECT_EQ(rq->tlvRequestFlags[2], 0);
  EXPECT_EQ(rq->tlvRequestFlags[3], 0);
  EXPECT_EQ(m->getTlvLen(m, 1), 28);
  EXPECT_EQ(m->getTlvID(m, 1), CSPTP_RESPONSE_id);
  struct CSPTP_RESPONSE_t *rp = (struct CSPTP_RESPONSE_t *)m->getTlv(m, 1);
  ASSERT_NE(rp, nullptr);
  EXPECT_EQ(0, memcmp(rp->organizationId, "\x1\x2\x3", 3));
  EXPECT_EQ(0, memcmp(rp->organizationSubType, "\x4\x5\x6", 3));
  EXPECT_TRUE(t->fromTimestamp(t, &rp->reqIngressTimestamp));
  struct timespec tp;
  t->toTimespec(t, &tp);
  EXPECT_EQ(tp.tv_sec, 0x4000300);
  EXPECT_EQ(tp.tv_nsec, 0x40003);
  EXPECT_EQ(rp->reqCorrectionField, 0x102030405060708);
  EXPECT_EQ(m->getTlvLen(m, 2), 48);
  EXPECT_EQ(m->getTlvID(m, 2), CSPTP_STATUS_id);
  struct CSPTP_STATUS_t *st = (struct CSPTP_STATUS_t *)m->getTlv(m, 2);
  ASSERT_NE(st, nullptr);
  EXPECT_EQ(0, memcmp(st->organizationId, "\x1\x2\x3", 3));
  EXPECT_EQ(0, memcmp(st->organizationSubType, "\x4\x5\x6", 3));
  EXPECT_EQ(st->grandmasterPriority1, 127);
  EXPECT_EQ(st->grandmasterClockQuality.clockClass, 37);
  EXPECT_EQ(st->grandmasterClockQuality.clockAccuracy, 0xfe);
  EXPECT_EQ(st->grandmasterClockQuality.offsetScaledLogVariance, 0x101);
  EXPECT_EQ(st->grandmasterPriority2, 201);
  EXPECT_EQ(st->stepsRemoved, 0x202);
  EXPECT_EQ(st->currentUtcOffset, 0xe78);
  static const uint8_t cid[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  EXPECT_EQ(0, memcmp(&st->grandmasterIdentity, cid, 8));
  EXPECT_EQ(st->parentAddress.networkProtocol, 2);
  EXPECT_EQ(st->parentAddress.addressLength, IPV6_ADDR_LEN);
  static const uint8_t ipv6[IPV6_ADDR_LEN] = { 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  EXPECT_EQ(0, memcmp(st->parentAddress.addressField, ipv6, IPV6_ADDR_LEN));
  EXPECT_EQ(m->getTlvLen(m, 3), 32);
  EXPECT_EQ(m->getTlvID(m, 3), PAD_id);
  //////////// Second packet
  const static uint8_t d2[160] = { // Use Follow_Up
      // Header 44 octests
      0x38, 18, 0, 160, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 127, 0, 0, 1, 0, 2, 0, 4, 0, 6, 0,
      // CSPTP_STATUS IPv4 36 octets
      0xf0, 2, 0, 32, 1, 2, 3, 4, 5, 6, 127, 37, 0xfe, 1, 1, 201, 2, 2, 0xe, 0x78, 1, 2, 3, 4, 5, 6, 7, 8,
      0, 1, 0, 4, 1, 2, 3, 4,
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 6 octets
      0, 9, 0, 22, 12, 0, 0x12, 0x85, 5, 0, 2, 0x7a, 4, 0x12, 0x23, 0x45, 0x67, 0x89, 0, 6, 't', 'e', 's', 't', '1', '2',
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 4 octets
      0, 9, 0, 20, 12, 0, 0x12, 0x85, 5, 0, 2, 0x7a, 4, 0x12, 0x23, 0x45, 0x67, 0x89, 0, 3, 't', 's', 't', 0,
      // PAD ID 30 octets
      0x80, 0x08, 0, 26
  };
  EXPECT_TRUE(b->setLen(b, 160));
  memcpy(b->getBuf(b), d2, 160);
  EXPECT_TRUE(m->parse(m, &p, b));
  EXPECT_EQ(m->getMsgLen(m), 160);
  EXPECT_EQ(p.type, Follow_Up);
  EXPECT_EQ(m->getMsgType(m), Follow_Up);
  EXPECT_EQ(p.domainNumber, 0);
  EXPECT_EQ(p.correctionField, 0);
  EXPECT_EQ(p.sequenceId, 0);
  EXPECT_EQ(p.flagField2, 0);
  EXPECT_TRUE(p.useTwoSteps);
  EXPECT_TRUE(t->fromTimestamp(t, &p.timestamp));
  EXPECT_EQ(t->getTs(t), 16777728067110400);
  EXPECT_EQ(m->getTlvs(m), 4);
  EXPECT_EQ(m->getTlvLen(m, 0), 36);
  EXPECT_EQ(m->getTlvID(m, 0), CSPTP_STATUS_id);
  st = (struct CSPTP_STATUS_t *)m->getTlv(m, 0);
  ASSERT_NE(st, nullptr);
  EXPECT_EQ(0, memcmp(st->organizationId, "\x1\x2\x3", 3));
  EXPECT_EQ(0, memcmp(st->organizationSubType, "\x4\x5\x6", 3));
  EXPECT_EQ(st->grandmasterPriority1, 127);
  EXPECT_EQ(st->grandmasterClockQuality.clockClass, 37);
  EXPECT_EQ(st->grandmasterClockQuality.clockAccuracy, 0xfe);
  EXPECT_EQ(st->grandmasterClockQuality.offsetScaledLogVariance, 0x101);
  EXPECT_EQ(st->grandmasterPriority2, 201);
  EXPECT_EQ(st->stepsRemoved, 0x202);
  EXPECT_EQ(st->currentUtcOffset, 0xe78);
  EXPECT_EQ(0, memcmp(&st->grandmasterIdentity, cid, 8));
  EXPECT_EQ(st->parentAddress.networkProtocol, 1);
  EXPECT_EQ(st->parentAddress.addressLength, IPV4_ADDR_LEN);
  EXPECT_EQ(0, memcmp(st->parentAddress.addressField, "\x1\x2\x3\x4", IPV4_ADDR_LEN));
  EXPECT_EQ(m->getTlvLen(m, 1), 26);
  EXPECT_EQ(m->getTlvID(m, 1), ALTERNATE_TIME_OFFSET_INDICATOR_id);
  struct ALTERNATE_TIME_OFFSET_INDICATOR_t *a = (struct ALTERNATE_TIME_OFFSET_INDICATOR_t *)m->getTlv(m, 1);
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(a->keyField, 12);
  EXPECT_EQ(a->currentOffset, 0x128505);
  EXPECT_EQ(a->jumpSeconds, 0x27a04);
  EXPECT_EQ(get_uint48(&a->timeOfNextJump), 0x122345678900);
  EXPECT_EQ(a->displayName.lengthField, 6);
  EXPECT_EQ(0, memcmp(a->displayName.textField, "test12", 6));
  EXPECT_EQ(m->getTlvLen(m, 2), 24);
  EXPECT_EQ(m->getTlvID(m, 2), ALTERNATE_TIME_OFFSET_INDICATOR_id);
  a = (struct ALTERNATE_TIME_OFFSET_INDICATOR_t *)m->getTlv(m, 2);
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(a->keyField, 12);
  EXPECT_EQ(a->currentOffset, 0x128505);
  EXPECT_EQ(a->jumpSeconds, 0x27a04);
  EXPECT_EQ(get_uint48(&a->timeOfNextJump), 0x122345678900);
  EXPECT_EQ(a->displayName.lengthField, 3);
  // The pad uses zero octet, same as the null padding of C string!
  EXPECT_EQ(0, memcmp(a->displayName.textField, "tst", 4));
  EXPECT_EQ(m->getTlvLen(m, 3), 30);
  EXPECT_EQ(m->getTlvID(m, 3), PAD_id);
  pbuffer b2 = buffer_alloc(200);
  ASSERT_NE(b2, nullptr);
  EXPECT_TRUE(m->copy(m, b2));
  EXPECT_EQ(b2->getLen(b2), 160);
  EXPECT_EQ(0, memcmp(b->getBuf(b), b2->getBuf(b2), 160));
  t->free(t);
  m->free(m);
  b->free(b);
  b2->free(b2);
}

// Test build messages
// bool init(pmsg self, pcparms params, pbuffer buffer)
// void *nextTlv(pmsg self, size_t need)
// bool addTlv(pmsg self, enum tlv_type_id id))
// bool addCSPTPReqTlv(pmsg self, uint8_t tlvRequestFlags0)
// bool buildDone(pmsg self, size_t size)
// void detach(pmsg self)
TEST(messageTest, build)
{
  pbuffer b = buffer_alloc(200);
  ASSERT_NE(b, nullptr);
  uint8_t *x = b->getBuf(b);
  pmsg m = msg_alloc();
  ASSERT_NE(m, nullptr);
  struct ptp_params_t p;
  p.type = Sync;
  p.domainNumber = 10;
  p.correctionField = 0x5012;
  p.sequenceId = 57;
  p.flagField2 = leap61 | leap59 | currentUtcOffsetValid | ptpTimescale | timeTraceable | frequencyTraceable;
  p.useTwoSteps = true;
  p.timestamp = { { 1, 2 }, 3 };
  EXPECT_TRUE(m->init(m, &p, b));
  EXPECT_EQ(m->getMsgType(m), Sync);
  EXPECT_EQ(m->getMsgLen(m), 44);
  EXPECT_EQ(b->getLen(b), 44);
  static const uint8_t d_host[160] = { // Use Sync
      // Header 44 octests
      0x30, 18, 0, 0, 10, 0, 6, 63, 18, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 57, 0, 0, 127, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0,
      // CSPTP_REQUEST 8 octets
      0, 0xff, 4, 0, 3, 0, 0, 0,
      // CSPTP_RESPONSE 28 octets
      1, 0xff, 24, 0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 3, 0, 4, 3, 0, 4, 0, 8, 7, 6, 5, 4, 3, 2, 1,
      // CSPTP_STATUS IPv6 48 octets
      2, 0xf0, 44, 0, 1, 2, 3, 4, 5, 6, 127, 37, 0xfe, 1, 1, 201, 2, 2, 0x78, 0xe, 1, 2, 3, 4, 5, 6, 7, 8,
      2, 0, 16, 0, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
      // PAD of 32 octets
      0x08, 0x80, 28
  };
  EXPECT_EQ(0, memcmp(d_host, x, 44));
  EXPECT_EQ(m->getTlvs(m), 0);
  EXPECT_TRUE(m->addCSPTPReqTlv(m, 3));
  EXPECT_EQ(m->getTlvs(m), 1);
  EXPECT_EQ(m->getTlvLen(m, 0), 8);
  EXPECT_EQ(m->getTlvID(m, 0), CSPTP_REQUEST_id);
  EXPECT_EQ(m->getMsgLen(m), 52);
  EXPECT_EQ(b->getLen(b), 52);
  EXPECT_EQ(0, memcmp(d_host, x, 52));
  struct CSPTP_RESPONSE_t *rp = (struct CSPTP_RESPONSE_t *)m->nextTlv(m, m->getTlvSize(CSPTP_RESPONSE_id));
  ASSERT_NE(rp, nullptr);
  memcpy(rp->organizationId, "\x1\x2\x3", 3);
  memcpy(rp->organizationSubType, "\x4\x5\x6", 3);
  rp->reqCorrectionField = 0x102030405060708;
  pts t = ts_alloc();
  ASSERT_NE(t, nullptr);
  struct timespec tp;
  tp.tv_sec = 0x4000300;
  tp.tv_nsec = 0x40003;
  t->fromTimespec(t, &tp);
  EXPECT_TRUE(t->toTimestamp(t, &rp->reqIngressTimestamp));
  EXPECT_TRUE(m->addTlv(m, CSPTP_RESPONSE_id));
  EXPECT_EQ(m->getTlvs(m), 2);
  EXPECT_EQ(m->getTlvLen(m, 1), 28);
  EXPECT_EQ(m->getTlvID(m, 1), CSPTP_RESPONSE_id);
  EXPECT_EQ(m->getMsgLen(m), 80);
  EXPECT_EQ(b->getLen(b), 80);
  EXPECT_EQ(0, memcmp(d_host, x, 80));
  struct CSPTP_STATUS_t *st = (struct CSPTP_STATUS_t *)m->nextTlv(m, m->getCSPTPStatusTlvSize(UDP_IPv6));
  ASSERT_NE(st, nullptr);
  memcpy(st->organizationId, "\x1\x2\x3", 3);
  memcpy(st->organizationSubType, "\x4\x5\x6", 3);
  st->grandmasterPriority1 = 127;
  st->grandmasterClockQuality.clockClass = 37;
  st->grandmasterClockQuality.clockAccuracy = 0xfe;
  st->grandmasterClockQuality.offsetScaledLogVariance = 0x101;
  st->grandmasterPriority2 = 201;
  st->stepsRemoved = 0x202;
  st->currentUtcOffset = 0xe78;
  static const uint8_t cid[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  memcpy(&st->grandmasterIdentity, cid, 8);
  st->parentAddress.networkProtocol = UDP_IPv6;
  st->parentAddress.addressLength = IPV6_ADDR_LEN;
  static const uint8_t ipv6[IPV6_ADDR_LEN] = { 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  memcpy(st->parentAddress.addressField, ipv6, IPV6_ADDR_LEN);
  EXPECT_TRUE(m->addTlv(m, CSPTP_STATUS_id));
  EXPECT_EQ(m->getTlvs(m), 3);
  EXPECT_EQ(m->getTlvLen(m, 2), 48);
  EXPECT_EQ(m->getTlvID(m, 2), CSPTP_STATUS_id);
  EXPECT_EQ(m->getMsgLen(m), 128);
  EXPECT_EQ(b->getLen(b), 128);
  EXPECT_EQ(0, memcmp(d_host, x, 128));
  EXPECT_TRUE(m->buildDone(m, 160));
  static const uint8_t d[160] = { // Use Sync
      // Header 44 octests
      0x30, 18, 0, 160, 10, 0, 6, 63, 0, 0, 0, 0, 0, 0, 80, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 57, 0, 127, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3,
      // CSPTP_REQUEST 8 octets
      0xff, 0, 0, 4, 3, 0, 0, 0,
      // CSPTP_RESPONSE 28 octets
      0xff, 1, 0, 24, 1, 2, 3, 4, 5, 6, 0, 0, 4, 0, 3, 0, 0, 4, 0, 3, 1, 2, 3, 4, 5, 6, 7, 8,
      // CSPTP_STATUS IPv6 48 octets
      0xf0, 2, 0, 44, 1, 2, 3, 4, 5, 6, 127, 37, 0xfe, 1, 1, 201, 2, 2, 0xe, 0x78, 1, 2, 3, 4, 5, 6, 7, 8,
      0, 2, 0, 16, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
      // PAD of 32 octets
      0x80, 0x08, 0, 28
  };
  EXPECT_EQ(m->getMsgType(m), Sync);
  EXPECT_EQ(m->getTlvs(m), 4);
  EXPECT_EQ(m->getTlvLen(m, 3), 32);
  EXPECT_EQ(m->getTlvID(m, 3), PAD_id);
  EXPECT_EQ(m->getMsgLen(m), 160);
  EXPECT_EQ(b->getLen(b), 160);
  EXPECT_EQ(0, memcmp(d, x, 160));
  m->detach(m);
  EXPECT_EQ(m->getMsgType(m), -1);
  EXPECT_EQ(m->getMsgLen(m), 0);
  EXPECT_EQ(m->getTlvs(m), 0);
  /* Verify we get the same thing */
  EXPECT_TRUE(m->parse(m, &p, b));
  /* check and remove the size to compare to d2_host */
  EXPECT_EQ(x[2], 160);
  x[2] = 0;
  EXPECT_EQ(0, memcmp(d_host, x, 160));
  //////////// Second packet
  p.type = Follow_Up;
  EXPECT_TRUE(m->init(m, &p, b));
  EXPECT_EQ(m->getMsgType(m), Follow_Up);
  EXPECT_EQ(m->getMsgLen(m), 44);
  EXPECT_EQ(b->getLen(b), 44);
  const static uint8_t d2_host[160] = { // Use Follow_Up
      // Header 44 octests
      0x38, 18, 0, 0, 10, 0, 6, 63, 18, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 57, 0, 2, 127, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0,
      // CSPTP_STATUS IPv4 36 octets
      2, 0xf0, 32, 0, 1, 2, 3, 4, 5, 6, 127, 37, 0xfe, 1, 1, 201, 2, 2, 0x78, 0xe, 1, 2, 3, 4, 5, 6, 7, 8,
      1, 0, 4, 0, 1, 2, 3, 4,
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 6
      9, 0, 22, 0, 12, 5, 0x85, 0x12, 0, 4, 0x7a, 2, 0, 0x23, 0x12, 0, 0x89, 0x67, 0x45, 6, 't', 'e', 's', 't', '1', '2',
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 4
      9, 0, 20, 0, 12, 5, 0x85, 0x12, 0, 4, 0x7a, 2, 0, 0x23, 0x12, 0, 0x89, 0x67, 0x45, 3, 't', 's', 't', 0,
      // PAD of 30 octets
      0x08, 0x80, 26
  };
  EXPECT_EQ(0, memcmp(d2_host, x, 44));
  EXPECT_EQ(m->getTlvs(m), 0);
  st = (struct CSPTP_STATUS_t *)m->nextTlv(m, m->getCSPTPStatusTlvSize(UDP_IPv4));
  ASSERT_NE(st, nullptr);
  memcpy(st->organizationId, "\x1\x2\x3", 3);
  memcpy(st->organizationSubType, "\x4\x5\x6", 3);
  st->grandmasterPriority1 = 127;
  st->grandmasterClockQuality.clockClass = 37;
  st->grandmasterClockQuality.clockAccuracy = 0xfe;
  st->grandmasterClockQuality.offsetScaledLogVariance = 0x101;
  st->grandmasterPriority2 = 201;
  st->stepsRemoved = 0x202;
  st->currentUtcOffset = 0xe78;
  memcpy(&st->grandmasterIdentity, cid, 8);
  st->parentAddress.networkProtocol = UDP_IPv4;
  st->parentAddress.addressLength = IPV4_ADDR_LEN;
  memcpy(st->parentAddress.addressField, "\x1\x2\x3\x4", IPV4_ADDR_LEN);
  EXPECT_TRUE(m->addTlv(m, CSPTP_STATUS_id));
  EXPECT_EQ(m->getTlvs(m), 1);
  EXPECT_EQ(m->getTlvLen(m, 0), 36);
  EXPECT_EQ(m->getTlvID(m, 0), CSPTP_STATUS_id);
  EXPECT_EQ(m->getMsgLen(m), 80);
  EXPECT_EQ(b->getLen(b), 80);
  EXPECT_EQ(0, memcmp(d2_host, x, 80));
  const char str1[] = "test12";
  struct ALTERNATE_TIME_OFFSET_INDICATOR_t *a = (struct ALTERNATE_TIME_OFFSET_INDICATOR_t *)m->nextTlv(m, m->getTlvSize(ALTERNATE_TIME_OFFSET_INDICATOR_id) + strlen(str1));
  ASSERT_NE(a, nullptr);
  a->keyField = 12;
  a->currentOffset = 0x128505;
  a->jumpSeconds = 0x27a04;
  EXPECT_TRUE(set_uint48(&a->timeOfNextJump, 0x122345678900));
  a->displayName.lengthField = strlen(str1);
  memcpy(a->displayName.textField, str1, strlen(str1));
  EXPECT_TRUE(m->addTlv(m, ALTERNATE_TIME_OFFSET_INDICATOR_id));
  EXPECT_EQ(m->getTlvs(m), 2);
  EXPECT_EQ(m->getTlvLen(m, 1), 26);
  EXPECT_EQ(m->getTlvID(m, 1), ALTERNATE_TIME_OFFSET_INDICATOR_id);
  EXPECT_EQ(m->getMsgLen(m), 106);
  EXPECT_EQ(b->getLen(b), 106);
  EXPECT_EQ(0, memcmp(d2_host, x, 106));
  const char str2[] = "tst";
  a = (struct ALTERNATE_TIME_OFFSET_INDICATOR_t *)m->nextTlv(m, m->getTlvSize(ALTERNATE_TIME_OFFSET_INDICATOR_id) + strlen(str2));
  ASSERT_NE(a, nullptr);
  a->keyField = 12;
  a->currentOffset = 0x128505;
  a->jumpSeconds = 0x27a04;
  EXPECT_TRUE(set_uint48(&a->timeOfNextJump, 0x122345678900));
  a->displayName.lengthField = 3;
  memcpy(a->displayName.textField, str2, 4);
  EXPECT_TRUE(m->addTlv(m, ALTERNATE_TIME_OFFSET_INDICATOR_id));
  EXPECT_EQ(m->getTlvs(m), 3);
  EXPECT_EQ(m->getTlvLen(m, 2), 24);
  EXPECT_EQ(m->getTlvID(m, 2), ALTERNATE_TIME_OFFSET_INDICATOR_id);
  EXPECT_EQ(m->getMsgLen(m), 130);
  EXPECT_EQ(b->getLen(b), 130);
  EXPECT_EQ(0, memcmp(d2_host, x, 130));
  EXPECT_TRUE(m->buildDone(m, 160));
  const static uint8_t d2[160] = { // Use Follow_Up
      // Header 44 octests
      0x38, 18, 0, 160, 10, 0, 6, 63, 0, 0, 0, 0, 0, 0, 80, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 57, 2, 127, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3,
      // CSPTP_STATUS IPv4 36 octets
      0xf0, 2, 0, 32, 1, 2, 3, 4, 5, 6, 127, 37, 0xfe, 1, 1, 201, 2, 2, 0xe, 0x78, 1, 2, 3, 4, 5, 6, 7, 8,
      0, 1, 0, 4, 1, 2, 3, 4,
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 6
      0, 9, 0, 22, 12, 0, 0x12, 0x85, 5, 0, 2, 0x7a, 4, 0x12, 0x23, 0x45, 0x67, 0x89, 0, 6, 't', 'e', 's', 't', '1', '2',
      // ALTERNATE_TIME_OFFSET_INDICATOR 20 + 4
      0, 9, 0, 20, 12, 0, 0x12, 0x85, 5, 0, 2, 0x7a, 4, 0x12, 0x23, 0x45, 0x67, 0x89, 0, 3, 't', 's', 't', 0,
      // PAD ID 30 octets
      0x80, 0x08, 0, 26
  };
  EXPECT_EQ(m->getMsgType(m), Follow_Up);
  EXPECT_EQ(m->getTlvs(m), 4);
  EXPECT_EQ(m->getMsgLen(m), 160);
  EXPECT_EQ(b->getLen(b), 160);
  EXPECT_EQ(0, memcmp(d2, x, 160));
  EXPECT_TRUE(m->parse(m, &p, b));
  /* check and remove the size to compare to d2_host */
  EXPECT_EQ(x[2], 160);
  x[2] = 0;
  EXPECT_EQ(0, memcmp(d2_host, x, 160));
  t->free(t);
  m->free(m);
  b->free(b);
}
