/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief socket to communicate with CSPTP
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_SOCK_H_
#define __CSPTP_SOCK_H_

#include "src/buf.h"
#include "src/time.h"
#include "src/if.h"

struct sockaddr;

typedef struct ipaddr_t *pipaddr;
typedef const struct ipaddr_t *pcipaddr;
typedef struct sock_t *psock;
typedef const struct sock_t *pcsock;

struct ipaddr_t {
    void *_addr; /**> pointer to BSD socket address object */
    char *_iPstr; /**> store last IP address string */
    int _domain; /**> BSD socet domain */
    prot _type; /**> socket type */

    /**
     * Free this address object
     * @param[in, out] self address object
     */
    void (*free)(pipaddr self);

    /**
     * get address object
     * @param[in] self address object
     * @return address object
     */
    struct sockaddr *(*getAddr)(pcipaddr self);

    /**
     * get address object size
     * @param[in] self address object
     * @return address object size
     */
    size_t (*getSize)(pcipaddr self);

    /**
     * get IP address size
     * @param[in] self address object
     * @return IP address size
     * @note return IPV4_ADDR_LEN or IPV6_ADDR_LEN
     */
    size_t (*getIPSize)(pcipaddr self);

    /**
     * Get port number
     * @param[in] self address object
     * @return port number
     */
    uint16_t (*getPort)(pcipaddr self);

    /**
     * Set port number
     * @param[in, out] self address object
     * @param[in] port number
     */
    void (*setPort)(pipaddr self, uint16_t port);

    /**
     * Set any IP address
     * @param[in, out] self address object
     */
    void (*setAnyIP)(pipaddr self);

    /**
     * Is any IP address
     * @param[in] self address object
     * @return true if use any address
     */
    bool (*isAnyIP)(pcipaddr self);

    /**
     * Get address type
     * @param[in] self address object
     * @return address type
     */
    prot(*getType)(pcipaddr self);

    /**
     * Set IPv4 address
     * @param[in, out] self address object
     * @param[in] ip address in network order!
     * @return true if success
     */
    bool (*setIP4)(pipaddr self, const uint8_t *ip);

    /**
     * Set IPv4 address using unsigned integer
     * @param[in, out] self address object
     * @param[in] ip address in host order!
     * @return true if success
     */
    bool (*setIP4Val)(pipaddr self, uint32_t ip);

    /**
     * Set IPv4 address using string
     * @param[in, out] self address object
     * @param[in] ip address string
     * @return true if success
     */
    bool (*setIP4Str)(pipaddr self, const char *ip);

    /**
     * Get IPv4 address in network order!
     * @param[in] self address object
     * @return ip address
     */
    const uint8_t *(*getIP4)(pcipaddr self);

    /**
     * Get IPv4 address as unsigned integer in host order!
     * @param[in] self address object
     * @return ip address
     */
    uint32_t (*getIP4Val)(pcipaddr self);

    /**
     * Get IPv4 address string
     * @param[in, out] self address object
     * @return ip address
     */
    const char *(*getIP4Str)(pipaddr self);

    /**
     * Set IPv6 address
     * @param[in, out] self address object
     * @param[in] ip address
     * @return true if success
     */
    bool (*setIP6)(pipaddr self, const uint8_t *ip);

    /**
     * Set IPv6 address using string
     * @param[in, out] self address object
     * @param[in] ip address string
     * @return true if success
     */
    bool (*setIP6Str)(pipaddr self, const char *ip);

    /**
     * Get IPv6 address
     * @param[in] self address object
     * @return ip address
     */
    const uint8_t *(*getIP6)(pcipaddr self);

    /**
     * Get IPv6 address string
     * @param[in, out] self address object
     * @return ip address
     */
    const char *(*getIP6Str)(pipaddr self);

    /**
     * Compare to another address object
     * @param[in] self address object
     * @param[in] other address object
     * @return true if equal addreses
     */
    bool (*eq)(pcipaddr self, pcipaddr other);

    /**
     * Get IP address in network order!
     * @param[in] self address object
     * @return ip address
     */
    const uint8_t *(*getIP)(pcipaddr self);

    /**
     * Set IP address
     * @param[in, out] self address object
     * @param[in] ip address
     * @return true if success
     */
    bool (*setIP)(pipaddr self, const uint8_t *ip);

    /**
     * Get IP address string
     * @param[in, out] self address object
     * @return ip address
     */
    const char *(*getIPStr)(pipaddr self);

    /**
     * Set IP address using string
     * @param[in, out] self address object
     * @param[in] ip address string
     * @return true if success
     */
    bool (*setIPStr)(pipaddr self, const char *ip);
};

struct sock_t {
    int _fd;
    prot _type;
    /**
     * Free this socket object
     * @param[in, out] self socket object
     */
    void (*free)(psock self);

    /**
     * close socket and release its resources
     * @param[in, out] self socket object
     * @return true on success
     */
    bool (*close)(psock self);

    /**
     * Get file description
     * @param[in] self socket object
     * @return file description
     */
    int (*fileno)(pcsock self);

    /**
     * Allocate the socket and initialize it with current parameters
     * @param[in, out] self socket object
     * @param[in] type select which class to use
     * @return true if socket creation success
     */
    bool (*init)(psock self, prot type);

    /**
     * Allocate the socket and initialize it with current parameters
     * @param[in, out] self socket object
     * @param[in] address socket object
     * @param[in] interface Network interface to bind socket to
     * @return true if socket creation success
     */
    bool (*initSrv)(psock self, pcipaddr address, pif interface);

    /**
     * Send message
     * @param[in] self socket object
     * @param[in] buffer to send
     * @param[in] address of peer to send
     * @return true if send success
     */
    bool (*send)(pcsock self, pcbuffer buffer, pcipaddr address);

    /**
     * Receive message
     * @param[in] self socket object
     * @param[in] buffer to receive
     * @param[in, out] address of peer receive from
     * @return true if receive success
     */
    bool (*recv)(pcsock self, pbuffer buffer, pipaddr address, pts ts);

    /**
     * poll socket, wait for receive
     * @param[in] self socket object
     * @param[in] timeout in milliseconds
     * @return true if there is receive waiting
     * @note negitive timeout will wait infinite
     */
    bool (*poll)(pcsock self, int timeout);

    /**
     * Get IP protocol
     * @param[in] self address object
     * @return IP protocol
     */
    prot(*getType)(pcsock self);
};

/**
 * Allocate a new address object
 * @param[in] type soket type
 * @param[in] port UDP port to use
 * @return pointer to a new address object or null
 */
pipaddr addr_alloc(prot type);

/**
 * Allocate a new socket object
 * @return pointer to a new socket new object or null
 */
psock sock_alloc();

/**
 * Convert Address or host name string tp binary
 * @param[in] string containing the address or host name
 * @param[in, out] type of address
 * @param[out] binary address
 * @return tru if convert success
 * @note binary address need to be of size IPV6_ADDR_LEN to support both IPv4 and IPv6
 * @note type UDP_IPv4, UDP_IPv6 force, Invalid_PROTO allow both depends on string
 */
bool addressStringToBinary(const char *string, prot *type, uint8_t *binary);

#endif /* __CSPTP_SOCK_H_ */
