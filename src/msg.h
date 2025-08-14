/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief PTP message for CSPTP
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_MSG_H_
#define __CSPTP_MSG_H_

#include "src/time.h"
#include "src/buf.h"

/** Maximum nubmert of TLVs in PTP message */
#define MAX_TLVS (4)

typedef struct ptp_msg_t *pmsg;
typedef const struct ptp_msg_t *pcmsg;
typedef struct ptp_params_t *pparms;
typedef const struct ptp_params_t *pcparms;

struct PTPText_t {
    uint8_t lengthField;
    char textField[0]; /**< variable length depend on lengthField */
} _PACKED__;

struct ClockIdentity_t {
    uint8_t clockIdentity[8];
} _PACKED__;

struct PortIdentity_t {
    uint8_t clockIdentity[8];
    uint16_t portNumber;
} _PACKED__;

struct ClockQuality_t {
    uint8_t clockClass;
    uint8_t clockAccuracy; /**< clock accuracy */
    uint16_t offsetScaledLogVariance;
} _PACKED__;

enum CSPTP_Req_Flags_e {
    /** client requests CSPTP_STATUS TLV in Sync responce */
    Flags0_Req_StatusTlv = 1 << 0,
    /* client requests ALTERNATE_TIME_OFFSET_INDICATOR in Sync responce */
    Flags0_Req_AlternateTimeTlv = 1 << 1,
};

struct PortAddress_t {
    uint16_t networkProtocol;
    uint16_t addressLength;
    uint8_t addressField[0]; /**< variable length depend on addressLength */
} _PACKED__;

enum messageType_e {
    Sync = 0,
    Delay_Req = 1,
    Pdelay_Req = 2,
    Pdelay_Resp = 3,
    Follow_Up = 8,
    Delay_Resp = 9,
    Pdelay_Resp_Follow_Up = 10,
    Announce = 11,
    Signaling = 12,
    Management = 13,
};

/* Flags from 8.2.4 timePropertiesDS data set member specifications */
enum flagField2_e {
    leap61 = 1 << 0, /**> a positive leap second is introduced at the end of the day */
    leap59 = 1 << 1, /**> a negative leap second is introduced at the end of the day */
    currentUtcOffsetValid = 1 << 2, /**> the UTC offset is considered to be valid */
    ptpTimescale = 1 << 3, /**> the PTP timescale is used */
    timeTraceable = 1 << 4, /**> time is traceable to a primary reference */
    frequencyTraceable = 1 << 5, /**> frequency is traceable to a primary reference */
};

struct msg_t {
    uint8_t messageType_majorSdoId;
    uint8_t versionPTP;
    uint16_t messageLength;
    uint8_t domainNumber;
    uint8_t minorSdoId;
    uint8_t flagField[2];
    int64_t correctionField;
    uint32_t messageTypeSpecific;
    struct PortIdentity_t sourcePortIdentity;
    uint16_t sequenceId;
    uint8_t controlField;
    uint8_t logMessageInterval;
    /*
     * Sync: originTimestamp
     * Follow_Up: preciseOriginTimestamp
     */
    struct Timestamp_t timestamp;
} _PACKED__;

enum tlv_type_id {
    Invalid_tlv_ID = -1,
    ALTERNATE_TIME_OFFSET_INDICATOR_id = 9, /* PTP standard */
    CSPTP_REQUEST_id = 0xff00, /* CSPTP TLV */
    CSPTP_RESPONSE_id = 0xff01, /* CSPTP TLV */
    CSPTP_STATUS_id = 0xf002, /* CSPTP TLV */
    PAD_id = 0x8008, /* PAD TLV */
};

struct tlv_hdr_t {
    uint16_t tlvType;
    uint16_t lengthField; /**> TLV length exclude the TLV header */
} _PACKED__;

struct ALTERNATE_TIME_OFFSET_INDICATOR_t {
    struct tlv_hdr_t hdr;
    uint8_t keyField;
    int32_t currentOffset;
    int32_t jumpSeconds;
    struct UInteger48_t timeOfNextJump;
    struct PTPText_t displayName; /**> Time zone acronyms */
} _PACKED__;

struct CSPTP_REQUEST_t {
    struct tlv_hdr_t hdr;
    uint8_t tlvRequestFlags[4];
} _PACKED__;

struct CSPTP_RESPONSE_t {
    struct tlv_hdr_t hdr;
    uint8_t organizationId[3];
    uint8_t organizationSubType[3];
    struct Timestamp_t reqIngressTimestamp;
    int64_t reqCorrectionField;
} _PACKED__;

struct CSPTP_STATUS_t {
    struct tlv_hdr_t hdr;
    uint8_t organizationId[3];
    uint8_t organizationSubType[3];
    uint8_t grandmasterPriority1;
    struct ClockQuality_t grandmasterClockQuality;
    uint8_t grandmasterPriority2;
    uint16_t stepsRemoved;
    int16_t currentUtcOffset;
    struct ClockIdentity_t grandmasterIdentity;
    struct PortAddress_t parentAddress;
} _PACKED__;

struct ptp_params_t {
    enum messageType_e type;
    uint8_t domainNumber;
    int64_t correctionField; /* in nanoseconds multiplied by 2^16 */
    uint16_t sequenceId;
    uint8_t flagField2; /* Used by Sync responce send from the service */
    struct Timestamp_t timestamp;
    bool useTwoSteps;
};

struct ptp_msg_t {
    pbuffer _buf; /**> pointer to buffer */
    enum messageType_e _type; /**> Message PTP type */
    struct msg_t *_msg; /**> pointer to used message */
    size_t _len; /**> message data length without padding */
    void *_end; /**> pointer to unused buffer memory */
    size_t _left; /**> left size available to use */
    struct tlvs_t {
        void *tlv; /**> Pointer to TLV or null */
        size_t len; /**> TLV size with the header */
        enum tlv_type_id id; /**> TLV ID */
    } _tlvs[MAX_TLVS];
    size_t _num_tlvs;

    /**
     * Free this message object
     * @param[in, out] self message object
     */
    void (*free)(pmsg self);

    /**
     * Initilize message with default values
     * @param[in, out] self message object
     * @param[in, out] buffer to parse
     * @param[in] params of message to initilize
     * @return true on success
     */
    bool (*init)(pmsg self, pcparms params, pbuffer buffer);

    /**
     * Get pointer to set a TLV
     * @param[in, out] self message object
     * @param[in] needed size for next TLV
     * @return pointer to add a new TLV or null
     * @note caller need to set all TLVs fields in host order, except
     *       the TLV header, which will be update when calling addTLV().
     * @note Use getTlvSize() and getCSPTPStatusTlvSize() to calculate needed size.
     *       For ALTERNATE_TIME_OFFSET_INDICATOR you should
     *       add the 'displayName' string length to needed size.
     */
    void *(*nextTlv)(pmsg self, size_t needed);

    /**
     * Add TLV
     * @param[in, out] self message object
     * @param[in] id type od TLV
     * @return true on success
     * @note caller should first call nextTlv() and fill the TLV
     *       before calling this method.
     */
    bool (*addTlv)(pmsg self, enum tlv_type_id id);

    /**
     * Add CSPTP_REQUEST TLV
     * @param[in, out] self message object
     * @param[in] tlvRequestFlags0 the tlvRequestFlags[0] value
     * @return true on success
     * @note No need to call nextTlv() or addTlv() after
     */
    bool (*addCSPTPReqTlv)(pmsg self, uint8_t tlvRequestFlags0);

    /**
     * Finish build and convert to network host ordered
     * @param[in, out] self message object
     * @param[in] size of message
     * @return true on success
     * @note The message will be pad to match the size.
     *       Unless size is zero.
     * @attention The size must be even!
     *            The PAD must be larger or equal to 4 octets.
     */
    bool (*buildDone)(pmsg self, size_t size);

    /**
     * Initilize message with default values
     * @param[in, out] self message object
     * @param[in, out] params of message to initilize
     * @param[in, out] buffer to parse
     * @return true if parsing success
     */
    bool (*parse)(pmsg self, pparms params, pbuffer buffer);

    /**
     * Copy message to another buffer
     * @param[in, out] self message object
     * @param[in, out] buffer to copy into
     * @return true if copy success
     * @note We do not copy padding!
     */
    bool (*copy)(pmsg self, pbuffer buffer);

    /**
     * Detach buffer
     * @param[in, out] self message object
     * @note can be used if the buffer is released to prevent missuse
     */
    void (*detach)(pmsg self);

    /**
     * Get last message type
     * @param[in] self message object
     * @return message type or -1
     */
    enum messageType_e(*getMsgType)(pcmsg self);

    /**
     * Get last message length
     * @param[in] self message object
     * @return message size
     * @note The message size exclude padding!
     */
    size_t (*getMsgLen)(pcmsg self);

    /**
     * Get PTP message size
     * @return size needed for using messages
     * @note Without any TLVs
     * @attention Header size is 33 octets + 10 octets for Timestamp
     */
    size_t (*getPTPMsgSize)();

    /**
     * Get TLV minimal size
     * @param[in] id of TLV
     * @return size of TLV
     */
    size_t (*getTlvSize)(enum tlv_type_id id);

    /**
     * Get CSPTP_STATUS TLV size
     * @param[in] protocol to use
     * @return size of TLV
     */
    size_t (*getCSPTPStatusTlvSize)(prot protocol);

    /**
     * Get number of TLVs
     * @param[in] self message object
     * @return number of TLVs
     */
    size_t (*getTlvs)(pcmsg self);

    /**
     * Get TLV length
     * @param[in] self message object
     * @param[in] index of TLV
     * @return TLV length including TLV header or zero if TLV is not exist
     */
    size_t (*getTlvLen)(pcmsg self, size_t index);

    /**
     * Get TLV ID
     * @param[in] self message object
     * @param[in] index of TLV
     * @return TLV ID or zero if TLV is not exist
     */
    enum tlv_type_id(*getTlvID)(pcmsg self, size_t index);

    /**
     * Get TLV using index
     * @param[in] self message object
     * @param[in] index of TLV
     * @return pointer to TLV or NULL
     */
    void *(*getTlv)(pcmsg self, size_t index);
};

/**
 * Allocate a message object
 * @return pointer to a new message object or null
 */
pmsg msg_alloc();

#endif /* __CSPTP_MSG_H_ */
