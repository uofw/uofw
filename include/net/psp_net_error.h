/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/net/psp_net_error.h
 *
 * Defines PSP network specific error codes.
 *
 */

/**
 * PSP error value format
 *
 *  31  30  29   28 27              16 15                    0
 * +---+---+-------+------------------+-----------------------+
 * | E | C | Rsrvd | F A C I L I T Y  |  E R R O R   C O D E  |
 * +---+---+-------+------------------+-----------------------+
 *
 *  bit 31: Error
 *      1 = Error
 *      0 = Success
 *
 *  bit 30: Critical
 *      1 = Critical error
 *      0 = Normal error
 *
 *  bits 29-28: Reserved
 *      always zero
 *
 *  bits 27-16: Facility
 *      0x041 = SCE_ERROR_FACILITY_NETWORK
 *
 *  Error code format for psp_net:
 *
 *    15              8 7               0
 *    +----------------+----------------+
 *    |    MODULE ID   |   ERROR CODE   |
 *    +----------------+----------------+
 *
 *    bits 15-8: MODULE ID
 *        0x00    common error (NULL FACILITY)
 *        0x01    pspnet
 *        0x02    pspnet_inet
 *        0x03    poeclient
 *        0x04    pspnet_resolver
 *        0x05    dhcp
 *        0x06    pspnet_adhoc_auth
 *        0x07    pspnet_adhoc
 *        0x08    pspnet_adhoc_matching
 *        0x09    pspnet_netcnf
 *        0x0a    pspnet_apctl
 *        0x0b    pspnet_adhocctl
 *        0x0c    reserved01
 *        0x0d    wlan
 *        0x0e    eapol
 *        0x0f    8021x
 *        0x10    wpa_supplicant
 *        0x11    reserved02
 *        0x12    pspnet_adhoc_transfer (sample library)
 *        0x13    pspnet_adhoc_discover
 *        0x14    adhoc_dialog
 *        0x15    wispr
 *        0x16    reserved03
 *        0x17    reserved04
 *        0x18    reserved05
 *        0x19    reserved06
 *
 *    bits 7-0: PSP_net module specific error codes
 *        
 */

#ifndef PSP_NET_ERROR_H
#define	PSP_NET_ERROR_H

/* PSP_NET MODULE ID */

#define SCE_ERROR_NET_MODULE_ID_COMMON              0x00
#define SCE_ERROR_NET_MODULE_ID_CORE                0x01
#define SCE_ERROR_NET_MODULE_ID_INET                0x02
#define SCE_ERROR_NET_MODULE_ID_POECLIENT           0x03
#define SCE_ERROR_NET_MODULE_ID_RESOLVER            0x04
#define SCE_ERROR_NET_MODULE_ID_DHCP                0x05
#define SCE_ERROR_NET_MODULE_ID_ADHOC_AUTH          0x06
#define SCE_ERROR_NET_MODULE_ID_ADHOC               0x07
#define SCE_ERROR_NET_MODULE_ID_ADHOC_MATCHING      0x08
#define SCE_ERROR_NET_MODULE_ID_NETCNF              0x09
#define SCE_ERROR_NET_MODULE_ID_APCTL               0x0a
#define SCE_ERROR_NET_MODULE_ID_ADHOCCTL            0x0b
#define SCE_ERROR_NET_MODULE_ID_RESERVED01          0x0c
#define SCE_ERROR_NET_MODULE_ID_WLAN                0x0d
#define SCE_ERROR_NET_MODULE_ID_EAPOL               0x0e
#define SCE_ERROR_NET_MODULE_ID_8021x               0x0f
#define SCE_ERROR_NET_MODULE_ID_WPA                 0x10
#define SCE_ERROR_NET_MODULE_ID_RESERVED02          0x11
#define SCE_ERROR_NET_MODULE_ID_TRANSFER            0x12
#define SCE_ERROR_NET_MODULE_ID_ADHOC_DISCOVER      0x13
#define SCE_ERROR_NET_MODULE_ID_ADHOC_DIALOG        0x14
#define SCE_ERROR_NET_MODULE_ID_WISPR               0x15
#define SCE_ERROR_NET_MODULE_ID_RESERVED03          0x16
#define SCE_ERROR_NET_MODULE_ID_RESERVED04          0x17
#define SCE_ERROR_NET_MODULE_ID_RESERVED05          0x18
#define SCE_ERROR_NET_MODULE_ID_RESERVED06          0x19

/* PSP network specific error codes. */

/* 0x04 psp_net_resolver */

#define SCE_NET_RESOLVER_ERROR_ID_NOT_FOUND         0x80410408
#define SCE_NET_RESOLVER_ERROR_ALREADY_STOPPED      0x8041040A
#define SCE_NET_RESOLVER_ERROR_NO_HOST              0x80410414

/* 0x0d wlan */

#define SCE_NET_WLAN_ERROR_INVALID_ARG              0x80410D13

#endif	/* PSP_NET_ERROR_H */

