// Copyright (c) 2001-2009, Scalable Network Technologies, Inc.  All Rights Reserved.
//                          6100 Center Drive
//                          Suite 1250
//                          Los Angeles, CA 90045
//                          sales@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

// /**
// PACKAGE     :: IP
// DESCRIPTION :: This file contains data structures and prototypes of functions used by IP.
// **/

// Objective: Header file for IP (Internet Protocol)
// Reference: RFC 791

#ifndef IP_H
#define IP_H
#define IP_TTL_DEC 1

#include <string>
#include "list.h"
#include "if_queue.h"
#include "if_scheduler.h"
#include "multicast_igmp.h"
#include "if_loopback.h"
#include "network_ipsec.h"
#include "dynamic.h"
#include "mac_llc.h"

#ifdef ENTERPRISE_LIB
#include "network_access_list.h"
#include "route_map.h"
#include "routing_policy_routing.h"
#include "route_parse_util.h"
#include "route_redistribution.h"
#endif // ENTERPRISE_LIB

#ifdef MILITARY_RADIOS_LIB
#include "mac_eplrs.h"
#include "routing_sdr.h"
#endif

#ifdef NETSEC_LIB
//#include "phy_jamming.h"
#include "network_secure_neighbor.h"
//#include "network_secure_community.h"
#include "network_isakmp.h"
#endif // NETSEC_LIB

#ifdef ADDON_BOEINGFCS

#include "network_ces_cluster.h"
#include "network_ces_region.h"
#include "network_ces_subnet.h"
#include "routing_ces_malsr.h"
#include "routing_ces_srw.h"
#include "routing_ces_rospf.h"
#include "routing_ces_mpr.h"
#include "mac_ces_sincgars.h"
#include "mac_ces_eplrs.h"
#include "routing_ces_sdr.h"
#endif  // ADDON_BOEINGFCS

#ifdef ADDON_NGCNMS
#include "subnet_abstract.h"
#include "network_ngc_haipe.h"
#endif
#ifdef MILITARY_RADIOS_LIB
#include "routing_sdr.h"
#endif

class D_IpPrint : public D_Command
{
private:
    Node *node;

public:
    D_IpPrint(Node *newNode) { node = newNode; }

    virtual void ExecuteAsString(const std::string& in, std::string& out);
};

//
// BSD-derived code START
//

/*
 * Ported from FreeBSD 2.2.2.
 * Definitions of the IP header and related information.
 */

//  Copyright (c) 1982, 1986, 1993
//   The Regents of the University of California.  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//  1. Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//  3. All advertising materials mentioning features or use of this software
//     must display the following acknowledgement:
//   This product includes software developed by the University of
//   California, Berkeley and its contributors.
//  4. Neither the name of the University nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
//  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
//  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
//  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
//  SUCH DAMAGE.
//
//   @(#)ip.h    8.2 (Berkeley) 6/1/94
//

/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 */


// /**
// CONSTANT    :: IPVERSION4 : 4
// DESCRIPTION :: Version of IP
// **/
#define IPVERSION4       4

//
// Structure of an internet header, naked of options.
//

// /**
// STRUCT      :: IpHeaderType
// DESCRIPTION :: QualNet typedefs struct ip_header_str to IpHeaderType.
//                struct ip_header_str is 20 bytes,just like in the BSD code.
// **/

typedef
struct ip_header_str
{
    UInt32 ip_v_hl_tos_len;//ip_v:4,        /* version */
                  //ip_hl:4;       /* header length */
    //unsigned char ip_tos;       /* type of service */
    //UInt16 ip_len;      /* total length */

    UInt16 ip_id;
    UInt16 ipFragment;
                 //ip_reserved:1,
                 //ip_dont_fragment:1,
                 //ip_more_fragments:1,
                 //ip_fragment_offset:13;

    unsigned char  ip_ttl;         /* time to live */
    unsigned char  ip_p;           /* protocol */
    unsigned short ip_sum;         /* checksum */
    unsigned       ip_src,ip_dst;  /* source and dest address */

} IpHeaderType;

// /**
// API                 :: IpHeaderSetVersion()
// LAYER               :: Network
// PURPOSE             :: Set the value of version number for IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32* :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// + version           : unsigned int : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetVersion(UInt32 *ip_v_hl_tos_len, unsigned int version)
{
    //masks version within boundry range
    version = version & maskInt(29, 32);

    //clears first four bits
    *ip_v_hl_tos_len = *ip_v_hl_tos_len & (~(maskInt(1, 4)));

    //setting the value of version number in ip_v_hl_tos_len
    *ip_v_hl_tos_len = *ip_v_hl_tos_len | LshiftInt(version, 4);
}


// /**
// API                 :: IpHeaderSetHLen()
// LAYER               :: Network
// PURPOSE             :: Set the value of header length for IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32* :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// + hlen              : unsigned int : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetHLen(UInt32 *ip_v_hl_tos_len, unsigned int hlen)
{
    //masks hlen within boundry range
    hlen = hlen & maskInt(29, 32);

    //clears last four bits
    *ip_v_hl_tos_len = *ip_v_hl_tos_len & (~(maskInt(5, 8)));

    //setting the value of header length in ip_v_hl_tos_len
    *ip_v_hl_tos_len = *ip_v_hl_tos_len | LshiftInt(hlen, 8);
}


// /**
// API                 :: IpHeaderSetTOS()
// LAYER               :: Network
// PURPOSE             :: Set the value of Type of Service for IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32* :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// + ipTos             : unsigned int : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetTOS(UInt32 *ip_v_hl_tos_len, unsigned int ipTos)
{
    //masks ipTos within boundry range
    ipTos = ipTos & maskInt(25, 32);

    //clears first four bits
    *ip_v_hl_tos_len = *ip_v_hl_tos_len & (~(maskInt(9, 16)));

    //setting the value of version number in ip_v_hl_tos_len
    *ip_v_hl_tos_len = *ip_v_hl_tos_len | LshiftInt(ipTos, 16);
}


// /**
// API                 :: IpHeaderSetIpLength()
// LAYER               :: Network
// PURPOSE             :: Set the value of ip length for IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32* :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// + ipLen             : unsigned int : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetIpLength(UInt32 *ip_v_hl_tos_len, unsigned int ipLen)
{
    //masks ipLen within boundry range
    ipLen = ipLen & maskInt(17, 32);

    //clears last four bits
    *ip_v_hl_tos_len = *ip_v_hl_tos_len & (maskInt(1, 16));

    //setting the value of header length in ip_v_hl_tos_len
    *ip_v_hl_tos_len = *ip_v_hl_tos_len | ipLen;
}


// /**
// API                 :: IpHeaderSetIpFragOffset()
// LAYER               :: Network
// PURPOSE             :: Set the value of ip_fragment_offset for
//                        IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16* : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// + offset            : UInt16 : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetIpFragOffset(UInt16* ipFragment, UInt16 offset)
{
    //masks offset within boundry range
    offset = offset & maskShort(4, 16);

    //clear last 13 bits of ipFragment
    *ipFragment = *ipFragment & maskShort(1, 3);

    //Setting the value of offset(last 13 bits) in ipFragment
    *ipFragment = *ipFragment | offset;
}


// /**
// API                 :: IpHeaderSetIpReserved()
// LAYER               :: Network
// PURPOSE             :: Set the value of ipReserved for IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16* : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// + ipReserved        : UInt16 : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetIpReserved(UInt16* ipFragment, BOOL ipReserved)
{
    UInt16 x = (UInt16)ipReserved;

    //masks ip_reserved within boundry range
    x = x & maskShort(16, 16);

    //clears the first bit of ipFragment
    *ipFragment = *ipFragment & (~(maskShort(1, 1)));

    //Setting the value of x at first position in ipFragment
    *ipFragment = *ipFragment | LshiftShort(x, 1);
}

// /**
// API                 :: IpHeaderSetIpDontFrag()
// LAYER               :: Network
// PURPOSE             :: Set the value of ip_dont_fragment for IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16* : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// + dontFrag          : UInt16 : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetIpDontFrag (UInt16* ipFragment, BOOL dontFrag)
{
    UInt16 x = (UInt16)dontFrag;

    //masks ip_dont_fragment within boundry range
    x = x & maskShort(16, 16);

    //clears the second bit of ipFragment
    *ipFragment = *ipFragment & (~(maskShort(2, 2)));

    //Setting the value of x at second position in ipFragment
    *ipFragment = *ipFragment | LshiftShort(x, 2);
}


// /**
// API                 :: IpHeaderSetIpMoreFrag()
// LAYER               :: Network
// PURPOSE             :: Set the value of ip_more_fragment for IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16* : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// + moreFrag          : UInt16 : Input value for set operation
// RETURN              :: void
// **/
static void IpHeaderSetIpMoreFrag(UInt16* ipFragment, BOOL moreFrag)
{
    UInt16 x = (UInt16) moreFrag;

    //masks ip_more_fragment within boundry range
    x = x & maskShort(16, 16);

    //clears the third bit of ipFragment
    *ipFragment = *ipFragment & (~(maskShort(3, 3)));

    //Setting the value of x at third position in ipFragment
    *ipFragment = *ipFragment | LshiftShort(x, 3);
}


// /**
// API                 :: IpHeaderGetVersion()
// LAYER               :: Network
// PURPOSE             :: Returns the value of version number for
//                        IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32 :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// RETURN              :: unsigned int
// **/
static unsigned int IpHeaderGetVersion(UInt32 ip_v_hl_tos_len)
{
    unsigned int version = ip_v_hl_tos_len;

    //clears last 4 bits
    version = version & maskInt(1, 4);

    //right shifts 4 bits so that last 4 bits contain the value of version
    version = RshiftInt(version, 4);

    return version;
}


// /**
// API                 :: IpHeaderGetHLen()
// LAYER               :: Network
// PURPOSE             :: Returns the value of header length for IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32 :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// RETURN              :: unsigned int
// **/
static unsigned int IpHeaderGetHLen(UInt32 ip_v_hl_tos_len)
{
    unsigned int hlen = ip_v_hl_tos_len;

    //clears first 4 bits
    hlen = hlen & maskInt(5, 8);

    //right shifts 8 bits so that last 4 bits contain the value of
    //header length
    hlen = RshiftInt(hlen, 8);

    return hlen;
}

// /**
// API                 :: IpHeaderGetTOS()
// LAYER               :: Network
// PURPOSE             :: Returns the value of Type of Service for
 //                       IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32 :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// RETURN              :: unsigned int
// **/
static unsigned int IpHeaderGetTOS(UInt32 ip_v_hl_tos_len)
{
    unsigned int ipTos = ip_v_hl_tos_len;

    //clears last 4 bits
    ipTos = ipTos & maskInt(9, 16);

    //right shifts 16 bits so that last 4 bits contain the value of ipTos
    ipTos = RshiftInt(ipTos, 16);

    return ipTos;
}


// /**
// API                 :: IpHeaderGetIpLength()
// LAYER               :: Network
// PURPOSE             :: Returns the value of ip length for IpHeaderType
// PARAMETERS          ::
// + ip_v_hl_tos_len   : UInt32 :The variable containing the value of ip_v
//                       ,ip_hl,ip_tos and ip_len
// RETURN              :: unsigned int
// **/
static unsigned int IpHeaderGetIpLength(UInt32 ip_v_hl_tos_len)
{
    unsigned int ipLen = ip_v_hl_tos_len;

    //clears first 4 bits
    ipLen = ipLen & maskInt(17, 32);

    return ipLen;
}


// /**
// API                 :: IpHeaderGetIpFragOffset()
// LAYER               :: Network
// PURPOSE             :: Returns the value of ip_fragment_offset for
//                        IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16 : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// RETURN              :: UInt16
// **/
static UInt16 IpHeaderGetIpFragOffset(UInt16 ipFragment)
{
    UInt16 offset = ipFragment ;

    //clears first three bits
    offset = offset & maskShort(4, 16);

    return offset;
}

// /**
// API                 :: IpHeaderGetIpDontFrag()
// LAYER               :: Network
// PURPOSE             :: Returns the value of ip_dont_fragment for
//                        IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16 : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// RETURN              :: BOOL
// **/
static BOOL IpHeaderGetIpDontFrag(UInt16 ipFragment)
{
    UInt16 dontFrag = ipFragment;

    //clears all bits except second bit
    dontFrag = dontFrag & maskShort(2, 2);

    //right shifts 14 bits so that last bit contains the value of dontFrag
    dontFrag = RshiftShort(dontFrag, 2);

    return (BOOL)dontFrag;
}


// /**
// API                 :: IpHeaderGetIpMoreFrag()
// LAYER               :: Network
// PURPOSE             :: Returns the value of ip_more_fragment for
//                        IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16 : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// RETURN              :: BOOL
// **/
static BOOL IpHeaderGetIpMoreFrag(UInt16 ipFragment)
{
    UInt16 moreFrag = ipFragment;

    //clears all bits except third bit
    moreFrag = moreFrag & maskShort(3, 3);

    //right shifts 13 bits so that last bit contains the value of moreFrag
    moreFrag = RshiftShort(moreFrag, 3);

    return (BOOL)moreFrag;
}


// /**
// API                 :: IpHeaderGetIpReserved()
// LAYER               :: Network
// PURPOSE             :: Returns the value of ipReserved for IpHeaderType
// PARAMETERS          ::
// + ipFragment        : UInt16 : The variable containing the value of
//                       ip_reserved,ip_dont_fragment,ip_more_fragment and
//                       ip_fragment_offset
// RETURN              :: BOOL
// **/
static BOOL IpHeaderGetIpReserved(UInt16 ipFragment)
{
    UInt16 frag_reserved = ipFragment;

    //clears all bits except first bit
    frag_reserved = frag_reserved & maskShort(1, 1);

    //right shifts 15 bits so that last bit contains the value of ipReserved
    frag_reserved = RshiftShort(frag_reserved, 1);

    return (BOOL) frag_reserved;
}



//  Definitions for IP type of service (ip_tos)

// /**
// CONSTANT    :: IPTOS_LOWDELAY : 0x10
// DESCRIPTION :: Type of service ( low delay)
// **/
#define IPTOS_LOWDELAY      0x10

// /**
// CONSTANT    :: IPTOS_THROUGHPUT  : 0x08
// DESCRIPTION :: Type of service ( throughput)
// **/
#define IPTOS_THROUGHPUT    0x08

// /**
// CONSTANT    :: IPTOS_RELIABILITY  : 0x04
// DESCRIPTION :: Type of service (reliability)
// **/
#define IPTOS_RELIABILITY   0x04

// /**
// CONSTANT    :: IPTOS_MINCOST  : 0x02
// DESCRIPTION :: Type of service ( minimum cost)
// **/
#ifndef IPTOS_MINCOST
#define IPTOS_MINCOST       0x02
#endif

// /**
// CONSTANT    :: IPTOS_ECT  : 0x02
// DESCRIPTION :: Bits 6 and 7 in the IPv4 TOS octet are designated as
//                the ECN field. Bit 6 is designated as the ECT bit.
// **/
#define IPTOS_ECT      0x02

// /**
// CONSTANT    :: IPTOS_CE   : 0x01
// DESCRIPTION :: Bits 6 and 7 in the IPv4 TOS octet are designated as
//                the ECN field. Bit 7 is designated as the CE bit.
// **/
#define IPTOS_CE       0x01

//
//  Bits 0 to 5 in the IPv4 TOS octet are designated as DSCP field.
//  The range for this 6-bit field is < 0 - 63 >.
//

// /**
// CONSTANT    :: IPTOS_DSCP_MAX  : 0x3f
// DESCRIPTION :: Bits 0 to 5 in the IPv4 TOS octet are designated as DSCP
//                field.The range for this 6-bit field is < 0 - 63 >.
// **/
#define IPTOS_DSCP_MAX         0x3f

// /**
// CONSTANT    :: IPTOS_DSCP_MIN  : 0x00
// DESCRIPTION :: Bits 0 to 5 in the IPv4 TOS octet are designated as DSCP
//                field.The range for this 6-bit field is < 0 - 63 >.
// **/
#define IPTOS_DSCP_MIN         0x00

//
//Definitions for IP precedence (also in ip_tos) (hopefully unused)
//

// /**
// CONSTANT    :: IPTOS_PREC_NETCONTROL  : 0xe0
// DESCRIPTION :: IP precedence 'net control'
// **/
#define IPTOS_PREC_NETCONTROL       0xe0

// /**
// CONSTANT    :: IPTOS_PREC_INTERNETCONTROL : 0xc0
// DESCRIPTION :: IP precedence 'internet control'
// **/
#define IPTOS_PREC_INTERNETCONTROL  0xc0

// /**
// CONSTANT    :: IPTOS_PREC_CRITIC_ECP   : 0xa0
// DESCRIPTION :: IP precedence 'critic ecp'
// **/
#define IPTOS_PREC_CRITIC_ECP       0xa0

// /**
// CONSTANT    :: IPTOS_PREC_FLASHOVERRIDE  : 0x80
// DESCRIPTION :: IP precedence 'flash override'
// **/

#define IPTOS_PREC_FLASHOVERRIDE    0x80

// /**
// CONSTANT    :: IPTOS_PREC_FLASH   : 0x60
// DESCRIPTION :: IP precedence 'flash'
// **/
#define IPTOS_PREC_FLASH            0x60

// /**
// CONSTANT    :: IPTOS_PREC_IMMEDIATE  : 0x40
// DESCRIPTION :: IP precedence 'immediate'
// **/

#define IPTOS_PREC_IMMEDIATE  0x40

// /**
// CONSTANT    :: IPTOS_PREC_PRIORITY   : 0x20
// DESCRIPTION :: IP precedence 'priority'
// **/
#define IPTOS_PREC_PRIORITY    0x20

// /**
// CONSTANT    :: IPTOS_PREC_ROUTINE  : 0x00
// DESCRIPTION :: IP precedence 'routing'
// **/
#define IPTOS_PREC_ROUTINE          0x00

//
// Definitions for options.
//

// /**
// MACRO       :: IPOPT_COPIED(o)
// DESCRIPTION :: IP option 'copied'.
// **/
#ifndef IPOPT_COPIED
#define IPOPT_COPIED(o)     ((o)&0x80)
#endif

// /**
// MACRO       :: IPOPT_CLASS(o)
// DESCRIPTION :: IP option 'class'
// **/
#ifndef IPOPT_CLASS
#define IPOPT_CLASS(o)      ((o)&0x60)
#endif

// /**
// MACRO       :: IPOPT_NUMBER(o)
// DESCRIPTION :: IP option 'number'
// **/
#ifndef IPOPT_NUMBER
#define IPOPT_NUMBER(o)     ((o)&0x1f)
#endif


// /**
// CONSTANT    :: IPOPT_CONTROL  : 0x00
// DESCRIPTION :: IP option 'control'
// **/
#define IPOPT_CONTROL       0x00

// /**
// CONSTANT    :: IPOPT_RESERVED1  :  0x20
// DESCRIPTION :: IP option 'reserved1'.
// **/
#define IPOPT_RESERVED1     0x20

// /**
// CONSTANT    :: IPOPT_DEBMEAS  : 0x40
// DESCRIPTION :: IP option 'debmeas'
// **/
#define IPOPT_DEBMEAS       0x40

// /**
// CONSTANT    :: IPOPT_RESERVED2  : 0x60
// DESCRIPTION :: IP option 'reserved2'
// **/
#define IPOPT_RESERVED2     0x60

// /**
// CONSTANT    :: IPOPT_EOL   : 0
// DESCRIPTION :: IP option 'end of option list'.
// **/
#define IPOPT_EOL       0

// /**
// CONSTANT    :: IPOPT_NOP   : 1
// DESCRIPTION :: IP option 'no operation'.
// **/
#define IPOPT_NOP       1

// /**
// CONSTANT    :: IPOPT_RR   : 7
// DESCRIPTION :: IP option 'record packet route'.
// **/
#define IPOPT_RR        7

// /**
// CONSTANT    :: IPOPT_TS  : 68
// DESCRIPTION :: IP option 'timestamp'.
// **/
#define IPOPT_TS        68

// /**
// CONSTANT    :: IPOPT_SECURITY  : 130
// DESCRIPTION :: IP option ' provide s,c,h,tcc'.
// **/
#define IPOPT_SECURITY  130

// /**
// CONSTANT    :: IPOPT_LSRR    : 131
// DESCRIPTION :: IP option 'loose source route'.
// **/
#define IPOPT_LSRR      131

// /**
// CONSTANT    :: IPOPT_SATID   : 136
// DESCRIPTION :: IP option 'satnet id'.
// **/
#define IPOPT_SATID     136

// /**
// CONSTANT    :: IPOPT_SSRR   : 137
// DESCRIPTION :: IP option 'strict source route '.
// **/
#define IPOPT_SSRR      137

//
// Offsets to fields in options other than EOL and NOP.
//

// /**
// CONSTANT    :: IPOPT_OPTVAL  : 0
// DESCRIPTION :: Offset to IP option 'option ID'
// **/
#define IPOPT_OPTVAL        0

// /**
// CONSTANT    :: IPOPT_OLEN   : 1
// DESCRIPTION :: Offset to IP option 'option length'
// **/
#define IPOPT_OLEN          1

// /**
// CONSTANT    :: IPOPT_OFFSET  :  2
// DESCRIPTION :: Offset to IP option 'offset within option'
// **/
#define IPOPT_OFFSET        2

// /**
// CONSTANT    :: IPOPT_MINOFF   : 4
// DESCRIPTION :: Offset to IP option 'min value of above'
// **/
#define IPOPT_MINOFF        4

// /**
// STRUCT      :: ip_timestamp
// DESCRIPTION :: Time stamp option structure.
// **/
struct ip_timestamp_str
{
    unsigned char ipt_code;       /* IPOPT_TS */
    unsigned char ipt_len;        /* size of structure (variable) */
    unsigned char ipt_ptr;        /* index of current entry */
    unsigned char flgOflw;
             //ipt_flg:4,        /* flags, see below */
             //ipt_oflw:4;       /* overflow counter */

    union ipt_timestamp
    {
        UInt32 ipt_time[1];
        struct ipt_ta
        {
            NodeAddress ipt_addr;
            UInt32 ipt_time;
        } ipt_ta[1];
    } ipt_timestamp;
};

// /**
// API                 :: Ip_timestampSetFlag()
// LAYER               :: Network
// PURPOSE             :: Set the value of flag for ip_timestamp_str
// PARAMETERS          ::
// + flgOflw           : unsigned char : The variable containing the value of flag and
//                       overflow counter
// + flag              : unsigned char: Input value for set operation
// RETURN              :: void
// **/
static void Ip_timestampSetFlag (unsigned char *flgOflw, unsigned char flag)
{
    //masks ipt_flg within boundry range
    flag = flag & maskChar(5, 8);

    //clears the first 4 bits of flgOflw
    *flgOflw = *flgOflw & maskChar(5, 8);

    //Setting the value of flag in flgOflw
    *flgOflw = *flgOflw | LshiftChar(flag, 4);
}


// /**
// API                 :: Ip_timestampSetOvflw()
// LAYER               :: Network
// PURPOSE             :: Set the value of ovflw for ip_timestamp_str
// PARAMETERS          ::
// + flgOflw           : unsigned char : The variable containing the value of flag and
//                       overflow counter
// + ovflw             : unsigned char : Input value for set operation
// RETURN              :: void
// **/
static void Ip_timestampSetOvflw (unsigned char *flgOflw,
                                  unsigned char ovflw)
{
    //masks ipt_oflw within boundry range
    ovflw = ovflw & maskChar(5, 8);

    //clears the last 4 bits of flgOflw
    *flgOflw = *flgOflw & maskChar(1, 4);

    //Setting the value of ovflw in flgOflw
    *flgOflw = *flgOflw | ovflw;
}


// /**
// API                 :: Ip_timestampGetFlag()
// LAYER               :: Network
// PURPOSE             :: Returns the value of flag for ip_timestamp_str
// PARAMETERS          ::
// + flgOflw           : unsigned char : The variable containing the value of flag and
//                       overflow counter
// RETURN              :: unsigned char
// **/
static unsigned char Ip_timestampGetFlag (unsigned char flgOflw)
{
    unsigned char flag = flgOflw;

    //clears last 4 bits
    flag = flag & maskChar(1, 4);

    //right shifts 4 bits so that last 4 bits contain the value of flag
    flag = RshiftChar(flag, 4);

    return flag;
}


// /**
// API                 :: Ip_timestampGetOvflw()
// LAYER               :: Network
// PURPOSE             :: Returns the value of overflow counter for
//                        ip_timestamp_str
// PARAMETERS          ::
// + flgOflw           : unsigned char : The variable containing the value of flag and
//                       overflow counter
// RETURN              :: unsigned char
// **/
static unsigned char Ip_timestampGetOvflw (unsigned char flgOflw)
{
    unsigned char oflwc = flgOflw;

    //clears first 4 bits
    oflwc = oflwc & maskChar(5, 8);

    return oflwc;
}


//
// flag bits for ipt_flg
//

// /**
// CONSTANT    :: IPOPT_TS_TSONLY  : 0
// DESCRIPTION :: Flag bits for ipt_flg (timestamps only );
// **/
#define IPOPT_TS_TSONLY     0

// /**
// CONSTANT    :: IPOPT_TS_TSANDADDR : 1
// DESCRIPTION :: Flag bits for ipt_flg (timestamps and addresses );
// **/
#define IPOPT_TS_TSANDADDR  1

// /**
// CONSTANT    :: IPOPT_TS_PRESPEC  : 3
// DESCRIPTION :: Flag bits for ipt_flg (specified modules only );
// **/
#define IPOPT_TS_PRESPEC    3

//
// bits for security (not byte swapped)
//

// /**
// CONSTANT    :: IPOPT_SECUR_UNCLASS  : 0x0000
// DESCRIPTION :: 'unclass' bits for security in IP option field
// **/
#define IPOPT_SECUR_UNCLASS 0x0000

// /**
// CONSTANT    :: IPOPT_SECUR_CONFID : 0xf135
// DESCRIPTION :: 'confid' bits for security in IP option field
// **/
#define IPOPT_SECUR_CONFID  0xf135

// /**
// CONSTANT    :: IPOPT_SECUR_EFTO  :  0x789a
// DESCRIPTION :: 'efto' bits for security in IP option field
// **/
#define IPOPT_SECUR_EFTO    0x789a

// /**
// CONSTANT    :: IPOPT_SECUR_MMMM  :  0xbc4d
// DESCRIPTION :: 'mmmm' bits for security in IP option field
// **/
#define IPOPT_SECUR_MMMM    0xbc4d

// /**
// CONSTANT    :: IPOPT_SECUR_RESTR  :  0xaf13
// DESCRIPTION :: 'restr' bits for security in IP option field
// **/
#define IPOPT_SECUR_RESTR   0xaf13

// /**
// CONSTANT    :: IPOPT_SECUR_SECRET : 0xd788
// DESCRIPTION :: 'secreat' bits for security in IP option field
// **/
#define IPOPT_SECUR_SECRET  0xd788

// /**
// CONSTANT    :: IPOPT_SECUR_TOPSECRET  : 0x6bc5
// DESCRIPTION :: 'top secret' bits for security in IP option field
// **/
#define IPOPT_SECUR_TOPSECRET   0x6bc5

// /**
// CONSTANT    :: MAXTTL  :  255
// DESCRIPTION :: Internet implementation parameters
//                (maximum time to live (seconds) )
// **/
#define MAXTTL      255

// /**
// CONSTANT    :: IPDEFTTL :  64
// DESCRIPTION :: Internet implementation parameters
//                (default ttl, from RFC 1340 )
// **/
#define IPDEFTTL    64

// /**
// CONSTANT    :: IPFRAGTTL :  60
// DESCRIPTION :: Internet implementation parameters
//                (time to live for frags, slowhz )
// **/
#define IPFRAGTTL   60

// /**
// CONSTANT    :: IPTTLDEC :  1
// DESCRIPTION :: Internet implementation parameters (subtracted
//                when forwarding)
// **/
#define IPTTLDEC    1

// /**
// CONSTANT    :: IPDEFTOS  :  0x10
// DESCRIPTION :: Internet implementation parameters (default TOS )
// **/
#define IPDEFTOS    0x10

// /**
// CONSTANT    :: IP_MSS  :  576
// DESCRIPTION :: Internet implementation parameters ( default maximum
//                segment size )
// **/
#define IP_MSS      576

//----------------------------------------------------------
// DEFINES
//----------------------------------------------------------

//----------------------------------------------------------
// IP protocol numbers
//----------------------------------------------------------

// IP protocol numbers for network-layer and transport-layer protocols

// /**
// CONSTANT    :: IPPROTO_IP : 0
// DESCRIPTION :: IP protocol numbers.
// **/
#ifndef IPPROTO_IP
#define IPPROTO_IP  0
#endif

// /**
// CONSTANT    :: IPPROTO_ICMP : 1
// DESCRIPTION :: IP protocol numbers for ICMP.
// **/
#ifndef IPPROTO_ICMP
#define IPPROTO_ICMP            1
#endif

// /**
// CONSTANT    :: IPPROTO_IGMP : 2
// DESCRIPTION :: IP protocol numbers for IGMP.
// **/
#ifndef IPPROTO_IGMP
#define IPPROTO_IGMP            2
#endif

// /**
// CONSTANT    :: IPPROTO_IPIP : 4
// DESCRIPTION :: IP protocol numbers for IP tunneling.
// **/
#ifndef IPPROTO_IPIP
#define IPPROTO_IPIP              4
#endif

// /**
// CONSTANT    :: IPPROTO_TCP : 6
// DESCRIPTION :: IP protocol numbers for TCP .
// **/
#ifndef IPPROTO_TCP
#define IPPROTO_TCP             6
#endif

// /**
// CONSTANT    :: IPPROTO_UDP : 17
// DESCRIPTION :: IP protocol numbers for UDP
// **/
#ifndef IPPROTO_UDP
#define IPPROTO_UDP             17
#endif

// /**
// CONSTANT    :: IPPROTO_IPV6 : 41
// DESCRIPTION :: IP protocol number for DUAL-IP.
// **/
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6  41
#endif

#ifndef IPPROTO_SDR  /*SDR*/
#define IPPROTO_SDR 42
#endif

#ifdef ADDON_BOEINGFCS
#ifndef IPPROTO_CES_SDR  // ROUTING_CES_SDR
#define IPPROTO_CES_SDR 43
#endif
#endif

// /**
// CONSTANT    :: IPPROTO_RSVP : 46
// DESCRIPTION :: IP protocol numbers for RSVP.
// **/
#ifndef IPPROTO_RSVP
#define IPPROTO_RSVP            46
#endif

// /**
// CONSTANT    :: IPPROTO_MOBILE_IP : 48
// DESCRIPTION :: IP protocol numbers for MOBILE_IP.
// **/
#define IPPROTO_MOBILE_IP       48

// /**
// CONSTANT    :: IPPROTO_ESP : 50
// DESCRIPTION :: IP protocol numbers for IPSEC.
// **/
#ifndef IPPROTO_ESP
#define IPPROTO_ESP             50
#endif

// /**
// CONSTANT    :: IPPROTO_AH : 51
// DESCRIPTION :: IP protocol numbers for IPSEC.
// **/
#ifndef IPPROTO_AH
#define IPPROTO_AH              51
#endif

#ifdef NETSEC_LIB
// /**
// CONSTANT    :: IPPROTO_ISAKMP : 52
// DESCRIPTION :: IP protocol numbers for IPSEC.
// **/
#define IPPROTO_ISAKMP          52
#endif // NETSEC_LIB

// /**
// CONSTANT    :: IPPROTO_OSPF : 89
// DESCRIPTION :: IP protocol numbers for OSPF .
// **/
#define IPPROTO_OSPF            89

// /**
// CONSTANT    :: IPPROTO_PIM : 103
// DESCRIPTION :: IP protocol numbers for PIM .
// **/
#ifndef IPPROTO_PIM
#define IPPROTO_PIM             103
#endif
#ifdef ADDON_BOEINGFCS
// /**
// CONSTANT    :: IPPROTO_RPIM : 104
// DESCRIPTION :: IP protocol numbers for PIM .
// **/
#define IPPROTO_RPIM             104
#endif

// /**
// CONSTANT    :: IPPROTO_IGRP : 100
// DESCRIPTION :: IP protocol numbers for IGRP .
// **/
#ifndef IPPROTO_IGRP
#define IPPROTO_IGRP            100
#endif

// /**
// CONSTANT    :: IPPROTO_EIGRP : 88
// DESCRIPTION :: IP protocol numbers for EIGRP .
// **/
#define IPPROTO_EIGRP           88

// /**
// CONSTANT    :: IPPROTO_BELLMANFORD : 150
// DESCRIPTION :: IP protocol numbers for BELLMANFORD.
// **/
#define IPPROTO_BELLMANFORD     150

#ifdef ADDON_NGCNMS
// /**
// CONSTANT    :: IPPROTO_IPIP_RED : 150
// DESCRIPTION :: IP protocol numbers for IP_RED.
// **/
#define IPPROTO_IPIP_RED       151
#endif

// /**
// CONSTANT    :: IPPROTO_FISHEYE : 160
// DESCRIPTION :: IP protocol numbers for FISHEYE .
// **/
#define IPPROTO_FISHEYE         160

// /**
// CONSTANT    :: IPPROTO_FSRL : 161
// DESCRIPTION :: IP protocol numbers for LANMAR .
// **/
#define IPPROTO_FSRL            161

#ifdef NETSEC_LIB
// /**
// CONSTANT    :: IPPROTO_ANODR : 162
// DESCRIPTION :: IP protocol numbers for ANODR .
// **/
#define IPPROTO_ANODR           162

// /**
// CONSTANT    :: IPPROTO_SECURE_NEIGHBOR : 163
// DESCRIPTION :: IP protocol numbers for secure neighbor discovery .
// **/
#define IPPROTO_SECURE_NEIGHBOR 163

// /**
// CONSTANT    :: IPPROTO_SECURE_COMMUNITY : 164
// DESCRIPTION :: IP protocol numbers for secure routing community
// **/
#define IPPROTO_SECURE_COMMUNITY 164
#endif // NETSEC_LIB

#ifdef ADDON_BOEINGFCS

// /**
// CONSTANT    :: IPPROTO_NETWORK_CES_CLUSTER : 165
// DESCRIPTION :: IP protocol numbers for clustering protocol.
// **/
#define IPPROTO_NETWORK_CES_CLUSTER               165


// CONSTANT    :: IPPROTO_ROUTING_CES_MALSR : 166
// DESCRIPTION :: IP protocol numbers for MALSR protocol.
// **/
#define IPPROTO_ROUTING_CES_MALSR                 166


// /**
// CONSTANT    :: IPPROTO_ROUTING_CES_ROSPF : 167
// DESCRIPTION :: IP protocol numbers for ROSPF protocol.
// **/
#define IPPROTO_ROUTING_CES_ROSPF                 167


// /**
// CONSTANT    :: IPPROTO_IPIP_ROUTING_CES_MALSR: 168
// DESCRIPTION :: IP protocol numbers for MALSR IP encapsulation.
// **/
#define IPPROTO_IPIP_ROUTING_CES_MALSR            168


// /**
// CONSTANT    :: IPPROTO_IPIP_ROUTING_CES_ROSPF: 169
// DESCRIPTION :: IP protocol numbers for ROSPF IP encapsulation.
// **/
#define IPPROTO_IPIP_ROUTING_CES_ROSPF            169

// /**
// CONSTANT    :: IPPROTO_NETWORK_CES_REGION: 170
// DESCRIPTION :: IP protocol numbers for RAP election protocol.
// **/
#define IPPROTO_NETWORK_CES_REGION                170

// /**
// CONSTANT    :: IPPROTO_MPR:    171
// DESCRIPTION :: IP protocol numbers for MPR
#define IPPROTO_ROUTING_CES_MPR                   171

// CONSTANT    :: IPPROTO_ROUTING_CES_SRW : 172
// DESCRIPTION :: IP protocol numbers for ROUTING_CES_SRW protocol.
// **/
#define IPPROTO_ROUTING_CES_SRW            172

// /**
// CONSTANT    :: IPPROTO_IPIP_ROUTING_CES_SRW: 173
// DESCRIPTION :: IP protocol numbers for ROUTING_CES_SRW IP encapsulation.
// **/
#define IPPROTO_IPIP_ROUTING_CES_SRW       173

// /**
// CONSTANT    :: IPPROTO_IPIP_SDR: 174
// DESCRIPTION :: IP protocol numbers for SDR IP encapsulation.
// **/
#define IPPROTO_IPIP_SDR       174

// /**
// CONSTANT    :: IPPROTO_IPIP_SDR: 175
// DESCRIPTION :: IP protocol numbers for SDR IP encapsulation.
// **/
#define IPPROTO_IPIP_CES_SDR       176

#endif  // ADDON_BOEINGFCS

// /**
// CONSTANT    :: IPPROTO_AODV : 123
// DESCRIPTION :: IP protocol numbers for AODV .
// **/
#define IPPROTO_AODV            123

// /**
// CONSTANT    :: IPPROTO_DYMO : 132
// DESCRIPTION :: IP protocol numbers for DYMO .
// **/
#define IPPROTO_DYMO            132

#ifdef ADDON_MAODV
// /**
// CONSTANT    :: IPPROTO_MAODV : 124
// DESCRIPTION :: IP protocol numbers for MAODV.
// **/
#define IPPROTO_MAODV           124
#endif // ADDON_MAODV
#ifdef MILITARY_RADIOS_LIB
#define IPPROTO_ODR             125
#define IPPROTO_IPIP_ODR        126
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_BOEINGFCS
#define IPPROTO_BOEING_ODR      127
#define IPPROTO_IPIP_BOEING_ODR 128
#endif

// /**
// CONSTANT    :: IPPROTO_DSR : 135
// DESCRIPTION :: IP protocol numbers for DSR .
// **/
#define IPPROTO_DSR             135

// /**
// CONSTANT    :: IPPROTO_ODMRP : 145
// DESCRIPTION :: IP protocol numbers for ODMRP .
// **/
#define IPPROTO_ODMRP           145

// /**
// CONSTANT    :: IPPROTO_LAR1 : 110
// DESCRIPTION :: IP protocol numbers for LAR1.
// **/
#define IPPROTO_LAR1            110

// /**
// CONSTANT    :: IPPROTO_STAR : 136
// DESCRIPTION :: IP protocol numbers for STAR.
// **/
#define IPPROTO_STAR            136

// /**
// CONSTANT    :: IPPROTO_DAWN : 120
// DESCRIPTION :: IP protocol numbers for DAWN.
// **/
#define IPPROTO_DAWN            120

// /**
// CONSTANT    :: IPPROTO_EPLRS : 174
// DESCRIPTION :: IP protocol numbers for EPLRS protocol.
// **/
#ifdef MILITARY_RADIOS_LIB
#define IPPROTO_EPLRS          174
#endif // MILITARY_RADIOS_LIB
#ifdef ADDON_BOEINGFCS
// /**
// CONSTANT    :: IPPROTO_EPLRS: 174
// DESCRIPTION :: IP protocol numbers for EPLRS protocol.
// **/
#define IPPROTO_CES_EPLRS          175
#endif

// /**
// CONSTANT    :: IPPROTO_DVMRP : 200
// DESCRIPTION :: IP protocol numbers for DVMRP.
// **/
#define IPPROTO_DVMRP           200

#ifdef CELLULAR_LIB
// /**
// CONSTANT    :: IPPROTO_GSM  : 202
// DESCRIPTION :: IP protocol numbers for GSM.
// **/
#define IPPROTO_GSM             202
#endif // CELLULAR_LIB

#ifdef ADVANCED_WIRELESS_LIB
// /*
// CONSTANT   :: IPPROTO_DOT16 : 249
// DESCRITION :: IP protocol number for dot16 backbone msg
// **/
#define IPPROTO_DOT16           249
#endif // ADVANCED_WIRELESS_LIB
// /**
// CONSTANT    :: IPPROTO_EXTERNAL  : 233
// DESCRIPTION :: IP protocol  for external interface.
// **/
#define IPPROTO_EXTERNAL        233

// /**
// CONSTANT    :: IPPROTO_NDP : 255
// DESCRIPTION :: IP protocol numbers for NDP.
// **/
#define IPPROTO_NDP             255

//startCellular
#define IPPROTO_CELLULAR 250
//endCellular

// /**
// CONSTANT    :: IPPROTO_BRP : 251
// DESCRIPTION :: IP protocol numbers for BRP .
// **/
#define IPPROTO_BRP             251

//StartZRP
#define IPPROTO_ZRP    252
//EndZRP
//StartIERP
#define IPPROTO_IERP    253
//EndIERP
//StartIARP
#define IPPROTO_IARP    254
//EndIARP
//InsertPatch ROUTING_IPPROTO

//----------------------------------------------------------
// IP header
//----------------------------------------------------------

// /**
// CONSTANT    :: IP_MIN_HEADER_SIZE : 20
// DESCRIPTION :: Minimum IP header size in bytes
// **/
#define IP_MIN_HEADER_SIZE       20

// /**
// CONSTANT    :: IP_MAX_HEADER_SIZE : 60
// DESCRIPTION :: Maximum IP header size in bytes
// **/
#define IP_MAX_HEADER_SIZE      60

// /**
// CONSTANT    :: IP_FRAGMENT_HOLD_TIME : 15 * SECOND
// DESCRIPTION :: Fragmented packets hold time.
// **/
#define IP_FRAGMENT_HOLD_TIME  15 * SECOND

// /**
// MACRO       :: IpHeaderSize(ipHeader)
// DESCRIPTION :: Returns IP header ip_hl field * 4, which is the size of
//                the IP header in bytes.
// **/

// PARAMETERS   IpHeaderType *ipHeader
//                  Pointer to the IP header.
//
// NOTES        (ipHeader)->ip_hl is the number of 4-byte words in the
//              IP header.
//

#define IpHeaderSize(ipHeader) (RshiftInt((ipHeader)->ip_v_hl_tos_len & maskInt(5,8),8) * 4)


// /**
// MACRO       :: SetIpHeaderSize(IpHeader, Size)
// DESCRIPTION :: Sets IP header ip_hl field (header length) to Size
//                divided by 4
// **/

// PARAMETERS   IpHeaderType *IpHeader
//                  Pointer to IP header.
//              unsigned Size
//                  Size of the IP header in bytes.
//
// NOTES        This macro checks that Size is evenly divisible by 4,
//              and also that Size falls within IP_MIN_HEADER_SIZE
//              and IP_MAX_HEADER_SIZE (inclusive).
//

/*#define SetIpHeaderSize(IpHeader, Size) \
    IpHeader->ip_hl = (Size) / 4; \
    ERROR_Assert(((Size) % 4 == 0) && \
                 (Size >= IP_MIN_HEADER_SIZE) && \
                 (Size <= IP_MAX_HEADER_SIZE), "Invalid IP header size");
*/
//----------------------------------------------------------
// Multicast
//----------------------------------------------------------

//
// Used to determine whether an IP address is multicast.
//

// /**
// CONSTANT    :: IP_MIN_MULTICAST_ADDRESS  : 0xE0000000
// DESCRIPTION :: Used to determine whether an IP address is multicast.
// **/
#define IP_MIN_MULTICAST_ADDRESS 0xE0000000    // 224.0.0.0

// /**
// CONSTANT    :: IP_MAX_MULTICAST_ADDRESS  : 0xEFFFFFFF
// DESCRIPTION :: Used to determine whether an IP address is multicast.
// **/
#define IP_MAX_MULTICAST_ADDRESS 0xEFFFFFFF    // 239.255.255.255

// /**
// CONSTANT    :: MULTICAST_DEFAULT_INTERFACE_ADDRESS : 3758096384u
// DESCRIPTION :: Default multicast interface address (224.0.0.0).
// **/
#define MULTICAST_DEFAULT_INTERFACE_ADDRESS  3758096384u    // 224.0.0.0

// /**
// CONSTANT    :: IP_MIN_RESERVED_MULTICAST_ADDRESS : 0xE0000000
// DESCRIPTION :: Minimum reserve multicast address (224.0.0.0).
// **/
#define IP_MIN_RESERVED_MULTICAST_ADDRESS 0xE0000000  // 224.0.0.0

// /**
// CONSTANT    :: IP_MAX_RESERVED_MULTICAST_ADDRESS : 0xE00000FF
// DESCRIPTION :: Maximum reserve multicast address (224.0.0.255).
// **/
#define IP_MAX_RESERVED_MULTICAST_ADDRESS 0xE00000FF  // 224.0.0.255

// /**
// CONSTANT    :: MULTICAST_DEFAULT_NUM_HOST_BITS   :   27
// DESCRIPTION :: Multicast default num host bit
// **/
#define MULTICAST_DEFAULT_NUM_HOST_BITS      27

// STATS DB CODE
#define DEFAULT_CONNECTIVITY_INTERVAL 500 // (in seconds)
//----------------------------------------------------------
// Routing
//----------------------------------------------------------


// /**
// CONSTANT    :: NETWORK_UNREACHABLE  :  -2
// DESCRIPTION :: Network unreachable.
// **/
#define NETWORK_UNREACHABLE     -2

// /**
// CONSTANT    :: DEFAULT_INTERFACE  : 0
// DESCRIPTION :: Default interface index
// **/
#define DEFAULT_INTERFACE       0

//----------------------------------------------------------
// STRUCTS, ENUMS, AND TYPEDEFS
//----------------------------------------------------------
//-----------------------------------------------------------------------------
// Info field
//-----------------------------------------------------------------------------

//
// Used by NetworkIpSendPacketOnInterfaceWithDelay().
//
typedef struct
{
    int incomingInterface;
    int outgoingInterface;
    NodeAddress nextHop;
}
DelayedSendToMacLayerInfoType;


//
// Extra information stored in a message while it's in an IP queue.
//
// Currently contains only the next hop address.
// This is not really part of an IP packet, and for efficiency do not
// want to add another header just for this field, hence, the reason
// for this struct.
//

typedef
struct
{
    // Used incase of IPv6 packet.
    in6_addr ipv6NextHopAddr;
    NodeAddress nextHopAddress;
    //MacHWAddress destAddr;
    unsigned char macAddress [MAX_MACADDRESS_LENGTH];
    unsigned short hwLength;
    unsigned short hwType;

    union
    {
        NodeAddress ipv4DestAddr;
        in6_addr    ipv6DestAddr;
    } destinationAddress;

    // What type of packet is it?  IP, Link-16 and others...
    int networkType;

    // Used to determine where packet will go after passing through
    // the backplane.
    int outgoingInterface;

    // Used to restore user priority
    TosType userTos;

    // Used for CENTRAL backplane to get the information of the
    // incoming interface of the packet. At present all packets
    // are store into CPU queue instead of internal scheduler
    // to handles all INPUT and CPU queues.
    int incomingInterface;
} QueuedPacketInfo;


// /**
// STRUCT      :: NetworkIpBackplaneInfo
// DESCRIPTION :: Structure maintaining IP Back plane Information
// **/
typedef struct networkIpBackplaneInfoStruct
{
    int incomingInterface;
    NodeAddress hopAddr;
    MacHWAddress hopMacAddr;

} NetworkIpBackplaneInfo;


//----------------------------------------------------------
// Function Pointers
//----------------------------------------------------------

typedef
void (*RouterFunctionType)(
    Node *node,
    Message *msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL *PacketWasRouted);

typedef
void (*MacLayerStatusEventHandlerFunctionType)(
    Node *node,
    const Message *msg,
    const NodeAddress nextHop,
    const int interfaceIndex);

typedef
void (*PromiscuousMessagePeekFunctionType)(
    Node *node,
    const Message *msg,
    NodeAddress prevHop);

typedef
void (*MacLayerAckHandlerType) (
    Node* node,
    int interfaceIndex,
    const Message* msg,
    NodeAddress nextHop);

typedef
void (*NetworkRouteUpdateEventType)(
    Node *node, NodeAddress destAddr, NodeAddress destAddrMask,
    NodeAddress nextHop, int interfaceId, int metric,
    NetworkRoutingAdminDistanceType adminDistance);



//----------------------------------------------------------
// Multicast
//----------------------------------------------------------

typedef
void (*MulticastRouterFunctionType)(
    Node *node,
    Message *msg,
    NodeAddress destAddr,
    int interfaceIndex,
    BOOL *PacketWasRouted,
    NodeAddress prevHop);

// /**
// STRUCT      :: NetworkMulticastForwardingTableRow
// DESCRIPTION :: Structure of an entity of multicast forwarding table.
// **/
typedef
struct
{
    NodeAddress sourceAddress;
    NodeAddress sourceAddressMask;        // Not used
    NodeAddress multicastGroupAddress;
    LinkedList *outInterfaceList;
} NetworkMulticastForwardingTableRow;

// /**
// STRUCT      :: NetworkMulticastForwardingTable
// DESCRIPTION :: Structure of multicast forwarding table
// **/
typedef
struct
{
    int size;
    int allocatedSize;
    NetworkMulticastForwardingTableRow *row;
} NetworkMulticastForwardingTable;

// /**
// STRUCT      :: NetworkIpMulticastGroupEntry
// DESCRIPTION :: Structure for Multicast Group Entry
// **/
typedef struct
{
    NodeAddress groupAddress;
    int memberCount;

    // Not used.
    int interfaceIndex;

} NetworkIpMulticastGroupEntry;

//----------------------------------------------------------
// Per Hop Behavior (PHB)
//----------------------------------------------------------

// /**
// STRUCT      :: IpPerHopBehaviorInfoType
// DESCRIPTION :: Structure to maintain DS priority queue mapping
// **/
typedef
struct
{
    unsigned char ds;
    QueuePriorityType priority;
} IpPerHopBehaviorInfoType;

//----------------------------------------------------------
// Diffserv Multi-Field Traffic Conditioner
//----------------------------------------------------------

// Function pointer definition for Diffserv Meters
typedef BOOL (*MeterFunctionType)(
    Node* node,
    struct struct_network_ip_str* ip,
    Message* msg,
    int conditionClass,
    int* preference);

// Variables of the structure define a unique condition class

// /**
// STRUCT      :: IpMftcParameter
// DESCRIPTION :: Variables of the structure define a unique condition class
// **/
typedef
struct
{
    Address             srcAddress; //Address
    Address             destAddress; //Address
    unsigned char       ds;
    unsigned char       protocolId;
    int                 sourcePort;
    int                 destPort;
    int                 incomingInterface;
    int                 conditionClass;
} IpMftcParameter;

// /**
// STRUCT      :: IpMultiFieldTrafficConditionerInfo
// DESCRIPTION :: Structure used to store traffic condition.
// **/
typedef
struct
{
    int                 conditionClass;

    // Variables used for Meter or Policer.
    Int32               committedBurstSize;
    Int32               excessBurstSize;    // also used for peak burst size
    BOOL                colorAware;
    Int32               tokens;
    Int32               largeBucketTokens;  // also used for peak bucket size
    Int32               rate;
    Int32               peakInformationRate;
    clocktype           lastTokenUpdateTime;
    double              avgRate;            // Used for TSW metering
    double              winLen;             // Used for TSW metering
    MeterFunctionType   meterFunction;

    // Variables used for statistics collection.
    unsigned int        numPacketsIncoming;
    unsigned int        numPacketsConforming;
    unsigned int        numPacketsPartConforming;
    unsigned int        numPacketsNonConforming;

    // Variables used for action taken on Out-profile packet.
    BOOL                dropOutProfilePackets;
    unsigned char       dsToMarkOutProfilePackets;

    // Variables used for PHB Service.
    unsigned char       serviceClass;
}
IpMultiFieldTrafficConditionerInfo;


//----------------------------------------------------------
// IP options
//----------------------------------------------------------

// /**
// STRUCT      :: IpOptionsHeaderType
// DESCRIPTION :: Structure of optional header for IP source route
// **/
typedef
struct ip_options  /* options header */
{
    unsigned char code;
    unsigned char len;
    unsigned char ptr;
}
IpOptionsHeaderType;


//----------------------------------------------------------
// Router Model
//----------------------------------------------------------
// /**
// ENUM        :: BackplaneType
// DESCRIPTION :: NetworkIp backplane type(either CENTRAL or DISTRIBUTED)
// **/

typedef enum
{
    BACKPLANE_TYPE_CENTRAL = 0,
    BACKPLANE_TYPE_DISTRIBUTED = 1
}
BackplaneType;


//------------------------------------------------------------
// Statistics
//------------------------------------------------------------


// /**
// STRUCT      :: NetworkIpStatsType
// DESCRIPTION :: Structure used to keep track of all stats of network layer.
// **/
typedef
struct
{
    // SNMP statistics for IP.

    // Total number of received IP datagrams from all interfaces.
    D_UInt32 ipInReceives;
    int ipInReceivesId;
    // Number of IP datagrams discarded because of header errors
    // (e.g., checksum error, version number mismatch, TTL exceeded,
    // etc.).
    UInt32 ipInHdrErrors;
    int ipInHdrErrorsId;
    // Number of IP datagrams discarded because of incorrect
    // destination address.
    UInt32 ipInAddrErrors;
    // Number of IP datagrams for which an attempt was made to forward.
    UInt32 ipInForwardDatagrams;
    // Number of received IP datagrams discarded because of a lack of
    // buffer space.
    UInt32 ipInDiscards;
    // Number of IP datagrams delivered to appropriate protocol module.
    UInt32 ipInDelivers;
    // Total number of IP datagrams passed to IP for transmission.
    // Does not include those counted in ipForwardDatagrams.
    UInt32 ipOutRequests;
    // Number of output IP datagrams discarded because of a lack of
    // buffer space.
    UInt32 ipOutDiscards;
    // Number of IP datagrams discarded because no route could be found.
    UInt32 ipOutNoRoutes;
    // Number of IP fragments received that needed to be reassembled.
    D_UInt32 ipReasmReqds;
    // Number of IP datagrams successfully reassembled.
    UInt32 ipReasmOKs;
    // Number of failures by IP reassembly algorithm.
    UInt32 ipReasmFails;
    // Number of IP datagrams that have been successfully fragmented.
    UInt32 ipFragOKs;
    // Number of IP fragmented datagrams that are in fragments buffer.
    UInt32 ipFragsInBuff;
    // Number of IP fragments created.
    D_UInt32 ipFragsCreated;
    // Number of IP datagrams after joining fragments.
    UInt32 ipPacketsAfterFragsReasm;

    // ATM : Number of Ip datagrams routed thru gateway
    UInt32 ipRoutePktThruGt;
    UInt32 ipSendPktToOtherNetwork;
    UInt32 ipRecvdPktFromOtherNetwork;
    //end for ATM

    // Variables used for run-time statistics.

    UInt32 ipInReceivesLastPeriod;
    UInt32 ipInHdrErrorsLastPeriod;
    UInt32 ipInAddrErrorsLastPeriod;
    UInt32 ipInForwardDatagramsLastPeriod;
    UInt32 ipInDiscardsLastPeriod;
    UInt32 ipInDeliversLastPeriod;
    UInt32 ipOutRequestsLastPeriod;
    UInt32 ipOutDiscardsLastPeriod;
    UInt32 ipOutNoRoutesLastPeriod;
    UInt32 ipReasmReqdsLastPeriod;
    UInt32 ipReasmOKsLastPeriod;
    UInt32 ipReasmFailsLastPeriod;
    UInt32 ipFragOKsLastPeriod;

    // Total value of TTLs for packets delivered to a node.  This is
    // used to calculate an "ipInDelivers TTL-based average hop count"
    // metric.
    UInt32 deliveredPacketTtlTotal;

    // Keep track of how many packets are dropped due to backplane limit.
    UInt32 ipNumDroppedDueToBackplaneLimit;

    // Fragment related statistics.
    UInt32 ipNoOfPacketsAfterJoiningFragments;
    BOOL bufferSizeStats;
    //BOOL packetsDroppedStats;
    //D_Float64 dropsFromRouting;

    // Stats for Network Aggregate Tables
#ifdef ADDON_DB
    StatsDBNetworkAggregate aggregateStats;
#endif
}
NetworkIpStatsType;

//----------------------------------------------------------
// Routing table (forwarding table)
//----------------------------------------------------------

// /**
// STRUCT      :: NetworkForwardingTableRow
// DESCRIPTION :: Structure of an entity of forwarding table.
// **/
typedef
struct
{
    NodeAddress destAddress;        // destination address
    NodeAddress destAddressMask;    // subnet destination Mask
    int interfaceIndex;           // index of outgoing interface
    NodeAddress nextHopAddress;     // next hop IP address

    int cost;

    // routing protocol type
    NetworkRoutingProtocolType protocolType;

    // administrative distance for the routing protocol
    NetworkRoutingAdminDistanceType adminDistance;

    BOOL interfaceIsEnabled;
}
NetworkForwardingTableRow;

// /**
// STRUCT      :: NetworkForwardingTable
// DESCRIPTION :: Structure of forwarding table.
// **/
typedef
struct
{
    int size;                        // number of entries
    int allocatedSize;
    NetworkForwardingTableRow *row;  // allocation in Init function in Ip
}
NetworkForwardingTable;

//-----------------------------------------------------------
// Interface info
//-----------------------------------------------------------

// /**
// STRUCT      :: IpInterfaceInfoType
// DESCRIPTION :: Structure for maintaining IP interface informations.  This
//                struct must be allocated by new, not MEM_malloc.  All
//                member variables MUST be initialized in the constructor.
// **/
struct IpInterfaceInfoType
{
    // Constructor.  All member variables MUST be initialized in the
    // constructor.
    IpInterfaceInfoType();

    Scheduler* scheduler;
    Scheduler* inputScheduler;
    NetworkIpBackplaneStatusType backplaneStatus;

    D_NodeAddress ipAddress;
    int numHostBits;
    char interfaceName[MAX_STRING_LENGTH];

    // to accomodate all the possible ports..etc
    char* intfNumber;

    RouterFunctionType                      routerFunction;
    NetworkRoutingProtocolType              routingProtocolType;
    void*                                   routingProtocol;
#ifdef ADDON_BOEINGFCS
    RouterFunctionType                      intraRegionRouterFunction;
    NetworkRoutingProtocolType              intraRegionRoutingProtocolType;
    void*                                   intraRegionRoutingProtocol;
#endif

    BOOL multicastEnabled;
    MulticastRouterFunctionType multicastRouterFunction;
    NetworkRoutingProtocolType multicastProtocolType;
    void *multicastRoutingProtocol;

    MacLayerStatusEventHandlerFunctionType  macLayerStatusEventHandlerFunction;
    PromiscuousMessagePeekFunctionType      promiscuousMessagePeekFunction;
    MacLayerAckHandlerType                  macAckHandler;

    // For IPsec
    IPsecSecurityPolicyDatabase* spdIN;
    IPsecSecurityPolicyDatabase* spdOUT;

    BOOL hsrpEnabled;
    // IPv6 interface information
    NetworkType interfaceType;
    BOOL isVirtualInterface;
    //for Unnumbered interface support
    BOOL isUnnumbered;

#ifdef ENTERPRISE_LIB
    // enum defined in /network/rt_parse_util.h
    RtInterfaceType intfType;

    // Access list
    AccessListPointer *accessListInPointer;
    AccessListPointer *accessListOutPointer;
    AccessListStats accessListStat;

    // Route Redistribute
    RoutingTableUpdateFunction routingTableUpdateFunction;

    // For policy based routing
    RouteMap *rMapForPbr;
    PbrStat pbrStat;
#endif // ENTERPRISE_LIB

    // For MPR
    BOOL useRoutingCesMpr;

    struct ipv6_interface_struct* ipv6InterfaceInfo;

    //Tunnel information
    LinkedList* InterfacetunnelInfo;

    // Need to use clocktype since some router distributed backplane
    // throughput are larger than long type.
    clocktype disBackplaneCapacity;

#ifdef TRANSPORT_AND_HAIPE
    // For HAIPE specification
    HAIPESpec haipeSpec;
#endif // TRANSPORT_AND_HAIPE
#ifdef NETSEC_LIB
    // Should count PHY turnaround time for the (wormhole victim) node?
    BOOL countWormholeVictimTurnaroundTime;
    // the turnaround time can also be specified by user
    clocktype wormholeVictimTurnaroundTime;

    // False if no ISAKMP configuration is present for the interface
    BOOL isISAKMPEnabled;
    // ISAKMP data structure
    ISAKMPData* isakmpdata;

    // For eavesdropping
    FILE *eavesdropFile;

    // For auditing
    FILE *auditFile;

    // Each network interface/address has a valid certificate
    // to prove the authenticity of its key
    char *certificate;
    int certificateLength;
    BOOL certificateFileLog;

    // For jamming
    //PhyJammingData *jammingData;
#ifdef DO_ECC_CRYPTO
    // Each network interface/address has an ECC key
    MPI eccKey[12];  // the 1st 10 elements construct the public key
#endif // DO_ECC_CRYPTO
#endif // NETSEC_LIB

#ifdef ADDON_BOEINGFCS
    D_Int32 ipFragUnit;
#endif

    // Interface based stats. MIBS
    D_UInt32 ifInUcastPkts;
    D_UInt32 ifInNUcastPkts;
    D_String ifDescr;
    D_UInt32 ifOutUcastPkts;
    D_UInt32 ifOutNUcastPkts;
    D_UInt32 ifInMulticastPkts;
    D_UInt32 ifInBroadcastPkts;
    D_UInt32 ifOutMulticastPkts;
    D_UInt32 ifOutBroadcastPkts;
    D_UInt32 ifInDiscards;
    D_UInt32 ifOutDiscards;

    D_UInt64 ifHCInUcastPkts;
    D_UInt64 ifHCInMulticastPkts;
    D_UInt64 ifHCInBroadcastPkts;
    D_UInt64 ifHCOutUcastPkts;
    D_UInt64 ifHCOutMulticastPkts;
    D_UInt64 ifHCOutBroadcastPkts;

    D_UInt32 ipAddrIfIdx;
    D_NodeAddress ipAddrNetMask;
    D_UInt32 ipAddrBcast;

    // Stats for NAIL Analysis
    D_UInt32 ifInUcastDataPackets;
    D_UInt32 ifOutUcastDataPackets;
    D_UInt32 inUcastDataPacketSize;
    D_UInt32 inUcastPacketSize;
    D_UInt32 inNUcastPacketSize;
    D_UInt32 inMulticastPacketSize;
    D_UInt32 inBroadcastPacketSize;
    clocktype firstInUcastPacketTime;
    clocktype lastInUcastPacketTime;
    clocktype firstInUcastDataPacketTime;
    clocktype lastInUcastDataPacketTime;
    clocktype firstInNUcastPacketTime;
    clocktype lastInNUcastPacketTime;
    clocktype firstInMulticastPacketTime;
    clocktype lastInMulticastPacketTime;
    clocktype firstInBroadcastPacketTime;
    clocktype lastInBroadcastPacketTime;

    D_UInt32 outUcastDataPacketSize;
    D_UInt32 outUcastPacketSize;
    D_UInt32 outNUcastPacketSize;
    D_UInt32 outMulticastPacketSize;
    D_UInt32 outBroadcastPacketSize;
    clocktype firstOutUcastPacketTime;
    clocktype lastOutUcastPacketTime;
    clocktype firstOutUcastDataPacketTime;
    clocktype lastOutUcastDataPacketTime;
    clocktype firstOutNUcastPacketTime;
    clocktype lastOutNUcastPacketTime;
    clocktype firstOutMulticastPacketTime;
    clocktype lastOutMulticastPacketTime;
    clocktype firstOutBroadcastPacketTime;
    clocktype lastOutBroadcastPacketTime;

#ifdef ADDON_DB
    clocktype delay;
    clocktype jitter;
    clocktype lastDelayTime;
#endif

};

//----------------------------------------------------------
// IP Fragmentation structure.
//----------------------------------------------------------
// /**
// STRUCT      :: ip_frag_data
// DESCRIPTION :: QualNet typedefs struct ip_frag_data to
//                IpFragData. is a simple queue to hold
//                fragmented packets.
// **/
typedef struct ip_frag_data
{
    Message* msg;
    struct ip_frag_data* nextMsg;
}IpFragData;



// /**
// STRUCT      :: Ipv6FragQueue
// DESCRIPTION :: Ipv6 fragment queue structure.
// **/
typedef struct ip_frag_q_struct
{
    UInt32 ipFrg_id;
    NodeAddress ipFrg_src;
    NodeAddress ipFrg_dst;
    clocktype fragHoldTime;
    IpFragData* firstMsg;
    IpFragData* lastMsg;

    unsigned int actualacketSize;
    unsigned int totalFragmentSize;

    struct ip_frag_q_struct* next;
}IpFragQueue;



// /**
// STRUCT      :: FragmetedMsg
// DESCRIPTION :: QualNet typedefs struct fragmeted_msg_struct to
//                ip6q. struct fragmeted_msg_struct is a simple
//                fragmented packets msg hold structure.
// **/
struct ip_fragmeted_msg_struct
{
    Message* msg;
    struct ip_fragmeted_msg_struct* next;
};

typedef struct ip_fragmeted_msg_struct ipFragmetedMsg;

//----------------------------------------------------------
// IP
//----------------------------------------------------------

// /**
// STRUCT      :: NetworkDataIp;
// DESCRIPTION :: Main structure of network layer.
// **/
typedef
struct struct_network_ip_str
{
    unsigned short              packetIdCounter;  // Used for identifying
    // datagram
    NetworkForwardingTable      forwardTable;

    BOOL                        checkMessagePeekFunction;
    BOOL                        checkMacAckHandler;
    int maxPacketLength;

    // added for Gateway
    BOOL gatewayConfigured;
    NodeAddress defaultGatewayId;

// For Dymo
    BOOL    isManetGateway;
    UInt8   manetPrefixlength;
    Address manetPrefixAddr;
// end for Dymo
    // Fragmentation disabled.
    // IpReassemblyBufferListType reassemblyBufferList;

    IpInterfaceInfoType* interfaceInfo[MAX_NUM_INTERFACES];

    IpPerHopBehaviorInfoType    *phbInfo;
    int                         numPhbInfo;
    int                         maxPhbInfo;

    IpMultiFieldTrafficConditionerInfo  *mftcInfo;
    int                                 numMftcInfo;
    int                                 maxMftcInfo;

    BOOL                        diffservEnabled;
    BOOL                        isEdgeRouter;
    BOOL                        mftcStatisticsEnabled;
    IpMftcParameter             *ipMftcParameter;
    int                         numIpMftcParameters;
    int                         maxIpMftcParameters;

    BOOL                        ipForwardingEnabled;

    NetworkIpStatsType stats;

    LinkedList                          *multicastGroupList;
    NetworkMulticastForwardingTable     multicastForwardingTable;

    BOOL                                isIgmpEnable;
    IgmpData                            *igmpDataPtr;

    BOOL                                isIcmpEnable;
    void                                *icmpStruct;

    // BEGIN start NDP
    BOOL                                isNdpEnable;
    void                                *ndpData;
    // END start NDP

#ifdef MILITARY_RADIOS_LIB
    // BEGIN start SDR
    BOOL                                isSdrEnable;
    SdrData*                            sdrData;
#endif
    // END start SDR

    // BEGIN start ROUTING_CES_SDR
#ifdef ADDON_BOEINGFCS
    RoutingCesSdrData*                            networkCesSdrData;
#endif
    // END start ROUTING_CES_SDR
    // ECN related variables
    BOOL                                isPacketEcnCapable;

    NetworkRouteUpdateEventType routeUpdateFunction;

    BOOL traceEnabled;

    // Need to use clocktype since some router backplane throughput
    // are larger than long type.
    clocktype backplaneThroughputCapacity;

    Scheduler* cpuScheduler;
    NetworkIpBackplaneStatusType backplaneStatus;

#ifdef ENTERPRISE_LIB
    // Keep track of all name value lists
    AccessListParameterNameValue* allNameValueLists;

    // Keep track of head of access list.
    // This is for number access list.
    AccessList** accessList;

    // This is for name access list
    AccessListName* accessListName;

    // Has got the nested reflex in the next
    AccessListNest* nestReflex;

    // Keep track of the existing sessions
    AccessListSessionList* accessListSession;

    BOOL accessListTrace;

    // Used to determine if access list stats are printed at the end of
    // simulation.
    BOOL isACLStatOn;

    // List of route maps for this node
    RouteMapList* routeMapList;

    RouteMapHookList* rtMapList;

    // Link to the last route map entry
    RouteMapEntry* bufferLastEntryRMap;

    RouteRedistribute*      rtRedistributeInfo;
    BOOL                    rtRedistributeIsEnabled;
    BOOL                    isRtRedistributeStatOn;

    // For policy based routing
    RouteMap* rMapForPbr;
    // Is local PBR enabled
    BOOL local;

    BOOL isPBRStatOn;
    BOOL pbrTrace;

    // one more pair of stats for local PBR
    PbrStat pbrStat;
#endif // ENTERPRISE_LIB

    // Timeout time for reflex
    clocktype reflexTimeout;

    // IPv6 data structure
    struct ipv6_data_struct* ipv6;

    // dualIp data structure
    struct dual_ip_data_struct* dualIp;

    void *mobileIpStruct;

    // Fragment Id counter.
    UInt32 ipFragmentId;
    IpFragQueue* fragmentListFirst;
    clocktype ipFragHoldTime;

#ifndef ADDON_BOEINGFCS
    D_Int32 ipFragUnit;
#endif

    // Loopback
    BOOL isLoopbackEnabled;
    NetworkForwardingTable loopbackFwdTable;

    BOOL isIPsecEnabled;
    IPsecSecurityAssociationDatabase* sad;
#ifdef NETSEC_LIB
    // Each node has a (per-node) secure neighbor list
    BOOL isSecureneighborEnabled;
    SecureneighborData* neighborData;

    //BOOL isSecureCommunityEnabled;  // for Routing Security
    //SecureCommunityData* communityData;
#endif // NETSEC_LIB


#ifdef ADDON_BOEINGFCS
    NetworkCesClusterData* clusterData;
    NetworkCesRegionData* regionData;
    NetworkCesSubnetData* subnetData;
    RoutingCesMalsrData* routingCesMalsrData;
    RoutingCesSrwData* routingCesSrwData;
    RoutingCesRospfData* rospfData;
    RoutingCesMprData* routingCesMprData;
    MacCesSincgarsData* macCesSincgarsData;
    MacCesEplrsData* macCesEplrsData;
    BOOL ncwHandoff;
#endif  // ADDON_BOEINGFCS

#ifdef ADDON_NGCNMS
    BOOL collectRoutes;
    char malsrRouteFileName0[MAX_STRING_LENGTH];
    char rospfRouteFileName0[MAX_STRING_LENGTH];
    char malsrRouteFileName1[MAX_STRING_LENGTH];
    char rospfRouteFileName1[MAX_STRING_LENGTH];
    clocktype collectRoutesUpdateInterval;
    NodeAddress gatewayAddress;
    int numSubnets;
    BOOL outputStats;
    NetworkNgcHaipeData* haipeData;
#endif

    // Keep backplane type.information
    BackplaneType backplaneType;

    // This is here because diffserv doesn't have it's own data structure
    // and I don't want to create one.
    RandomSeed trafficConditionerSeed;
#ifdef MILITARY_RADIOS_LIB
    EplrsData* eplrsData;    
#endif // MILITARY_RADIOS_LIB
#ifdef ADDON_BOEINGFCS
    BOOL controlStats;
    //clocktype printTimeForTable;
    //clocktype nextPrintTimeForTable;
#endif

#ifdef ADDON_BOEINGFCS
    BOOL tosRoutingEnabled;
#endif

    // STATS DB CODE
#ifdef ADDON_DB
    clocktype ipConnectivityInterval;
    clocktype nextConnInsertTime;

    // For Network Session Table
    std::vector<OneHopNeighborData>* oneHopData;
#endif
}
NetworkDataIp;

//----------------------------------------------------------
// INLINED FUNCTIONS
//----------------------------------------------------------

//----------------------------------------------------------
// IP address
//----------------------------------------------------------


// /**
// API          :: ConvertNumHostBitsToSubnetMask
// LAYER        :: Network
// PURPOSE      :: To generate subnetmask using number of host bit
// PARAMETERS   ::
// + numHostBits : int         : number of host bit.
// RETURN       :: NodeAddress : subnetmask
// **/
static NodeAddress
ConvertNumHostBitsToSubnetMask(int numHostBits)
{
    if (numHostBits == 32)
    {
        return 0;
    }

    return 0xffffffff << numHostBits;
}

// /**
// API         :: ConvertSubnetMaskToNumHostBits
// LAYER       :: Network
// PURPOSE     :: To generate number of host bit using subnetmask.
// PARAMETERS  ::
// + subnetMask : NodeAddress : subnetmask.
// RETURN      :: int         : number of host bit.
// **/
static int
ConvertSubnetMaskToNumHostBits(NodeAddress subnetMask)
{
    int numHostBits = 0;
    unsigned mask = 1;

    if (subnetMask == 0)
    {
        return 32;
    }
    while (1)
    {
        if (subnetMask & mask)
        {
            return numHostBits;
        }
        numHostBits++;
        mask = mask << 1;
    }
}

// /**
// API         :: MaskIpAddress
// LAYER       :: Network
// PURPOSE     :: To mask a ip address.
// PARAMETERS  ::
// + address    : NodeAddress : address of a node
// + mask       : NodeAddress : mask of subnet.
// RETURN      :: NodeAddress : masked node address.
// **/
static NodeAddress
MaskIpAddress(NodeAddress address, NodeAddress mask)
{
    return (address & mask);
}

// /**
// API             :: MaskIpAddressWithNumHostBits
// LAYER           :: Network
// PURPOSE         :: To mask a ip address.
// PARAMETERS      ::
// + address        : NodeAddress : address of a node.
// + numHostBits    : int         : number of host bit.
// RETURN          :: NodeAddress : masked node address.
// **/
static NodeAddress
MaskIpAddressWithNumHostBits(NodeAddress address, int numHostBits)
{
    return MaskIpAddress(address,
                         ConvertNumHostBitsToSubnetMask(numHostBits));
}

// /**
// API             :: CalcBroadcastIpAddress
// LAYER           :: Network
// PURPOSE         :: To generate broadcast address.
// PARAMETERS      ::
// + address        : NodeAddress : address of a node.
// + numHostBits    : int         : number of host bit.
// RETURN          :: NodeAddress : Broadcast address.
// **/
static NodeAddress
CalcBroadcastIpAddress(NodeAddress address, int numHostBits)
{
    int i;
    if (numHostBits == 0)
    {
        return address;
    }
    for (i = 0; i < numHostBits; i++)
    {
        address |= (1 << i);
    }

    return address;
}

// /**
// API             :: IsIpAddressInSubnet
// LAYER           :: Network
// PURPOSE         :: To check if a ip address belongs to a subnet.
// PARAMETERS      ::
// + address        : NodeAddress : address of a node.
// + subnetAddress  : NodeAddress : address of a subnet.
// + numHostbits    : int         : number of host bit.
// RETURN          :: BOOL        : TRUE if ip address belongs to a subnet
//                                  else FALSE.
// **/
static BOOL
IsIpAddressInSubnet(
    NodeAddress address,
    NodeAddress subnetAddress,
    int numHostBits)
{
    NodeAddress subnetMask = ConvertNumHostBitsToSubnetMask(numHostBits);
    return (BOOL) (MaskIpAddress(address, subnetMask) == subnetAddress);
}

//----------------------------------------------------------
// IP header
//----------------------------------------------------------

// /**
// API                 :: NetworkIpAddHeader
// LAYER               :: Network
// PURPOSE             :: Add an IP packet header to a message.
//                        Just calls AddIpHeader.
// PARAMETERS          ::
// + node               : Node*         : Pointer to node.
// + msg                : Message*      : Pointer to message.
// + sourceAddress      : NodeAddress   : Source IP address.
// + destinationAddress : NodeAddress   : Destination IP address.
// + priority           : TosType       : Currently a TosType.
//                        (values are not standard for "IP type of
//                         service field" but has correct function)
// + protocol           : unsigned char : IP protocol number.
// + ttl                : unsigned      : Time to live.If 0, uses default
//                        value IPDEFTTL, as defined in include/ip.h.
// RETURN              :: void :
// **/
void
NetworkIpAddHeader(
    Node *node,
    Message *msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    TosType priority,
    unsigned char protocol,
    unsigned ttl);

// /**
// API        :: FindAnIpOptionField
// LAYER      :: Network
// PURPOSE    :: Searches the IP header for the option field with option
//               code that matches optionKey, and returns a pointer to
//               the option field header.
// PARAMETERS ::
// + ipHeader  : const IpHeaderType*  : Pointer to an IP header.
// + optionKey : const int            : Option code for desired option field.
// RETURN     :: IpOptionsHeaderType* : to the header of the desired option
//                                      field. NULL if no option fields, or
//                                      the desired option field cannot be
//                                      found.
// **/
static IpOptionsHeaderType *
FindAnIpOptionField(
    const IpHeaderType *ipHeader,
    const int optionKey)
{
    IpOptionsHeaderType *currentOption;

    // If the passed in IP header is the minimum size, return NULL.
    // (no option in IP header)
    if (IpHeaderSize(ipHeader) == sizeof(IpHeaderType))
    {
        return NULL;
    }

    // Move pointer over 20 bytes from start of IP header,
    // so currentOption points to first option.
    currentOption = (IpOptionsHeaderType *)
                    ((char *) ipHeader + sizeof(IpHeaderType));

    // Loop until an option code matches optionKey.
    while (currentOption->code != optionKey)
    {
        // Options should never report their length as 0.
        ERROR_Assert(currentOption->len != 0, "IP option field has length 0");

        // Current option code doesn't match; move pointer over to next option.

        currentOption =
            (IpOptionsHeaderType *) ((char *) currentOption + currentOption->len);

        // If we've run out of options, return NULL.
        if ((char *) currentOption
                >= (char *) ipHeader + IpHeaderSize(ipHeader))
        {
            return NULL;
        }
    }

    // Found option with code matching optionKey.  Return pointer to option.

    return currentOption;
}


//----------------------------------------------------------
// PROTOTYPES FOR FUNCTIONS WITH EXTERNAL LINKAGE
//----------------------------------------------------------

//----------------------------------------------------------
// Init functions
//----------------------------------------------------------

// /**
// FUNCTION   :: NetworkIpPreInit
// LAYER      :: Network
// PURPOSE    :: IP initialization required before any of the other
//               layers are initialized.
//               This is mainly for MAC initialization, which requires
//               certain IP structures be pre-initialized.
// PARAMETERS ::
// + node      : Node*  : pointer to node.
// RETURN     :: void :
// **/
void
NetworkIpPreInit(Node *node);

// /**
// FUNCTION   :: NetworkIpInit
// LAYER      :: Network
// PURPOSE    :: Initialize IP variables, and all network-layer IP
//               protocols..
// PARAMETERS ::
// + node      : Node*            : pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// RETURN  :: void :
// **/
void
NetworkIpInit(Node *node, const NodeInput *nodeInput);

//----------------------------------------------------------
// Layer function
//----------------------------------------------------------


// /**
// FUNCTION   :: NetworkIpLayer
// LAYER      :: Network
// PURPOSE    :: Handle IP layer events, incoming messages and messages
//               sent to itself (timers, etc.).
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Pointer to message.
// RETURN     :: void :
// **/
void
NetworkIpLayer(Node *node, Message *msg);

//----------------------------------------------------------
// Finalize function
//----------------------------------------------------------


// /**
// FUNCTION   :: NetworkIpFinalize
// LAYER      :: Network
// PURPOSE    :: Finalize function for the IP model.  Finalize functions
//               for all network-layer IP protocols are called here.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// RETURN     :: void :
// **/
void
NetworkIpFinalize(Node *node);

//----------------------------------------------------------
// Transport layer to IP, sends IP packets out to network
//----------------------------------------------------------


// /**
// API        :: NetworkIpReceivePacketFromTransportLayer
// LAYER      :: Network
// PURPOSE    :: Called by transport layer protocols (UDP, TCP) to send
//               UDP datagrams and TCP segments using IP.  Simply calls
//               NetworkIpSendRawMessage().
// PARAMETERS          ::
// + node               : Node*        : Pointer to node.
// + msg                : Message*     : Pointer to message from transport
//                        layer containing payload data (UDP datagram, TCP
//                        segment) for an IP packet. (IP header needs to be
//                        added)
// + sourceAddress      : NodeAddress  : Source IP address.
//                        See NetworkIpSendRawMessage() for special values
// + destinationAddress : NodeAddress  : Destination IP address.
// + outgoingInterface  : int          : outgoing interface to use to
//                                       transmit packet.
// + priority           : TosType       : Priority of packet.
// + protocol           : unsigned char : IP protocol number.
// + isEcnCapable       : BOOL          : Is this node ECN capable?
// RETURN              :: void :
// **/
void
NetworkIpReceivePacketFromTransportLayer(
    Node *node,
    Message *msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    TosType priority,
    unsigned char protocol,
    BOOL isEcnCapable,
    UInt8 ttl = IPDEFTTL);


void
NetworkIpReceivePacketFromTransportLayer(
    Node *node,
    Message *msg,
    Address sourceAddress,
    Address destinationAddress,
    int outgoingInterface,
    TosType priority,
    unsigned char protocol,
    BOOL isEcnCapable,
    UInt8 ttl = IPDEFTTL);

//----------------------------------------------------------
// Network layer to MAC layer, sends IP packets out to network
//----------------------------------------------------------


// /**
// API        :: NetworkIpSendRawMessage
// LAYER      :: Network
// PURPOSE    :: Called by NetworkIpReceivePacketFromTransportLayer() to
//               send to send UDP datagrams, TCP segments using IP.  Also
//               called by network-layer routing protocols (AODV, OSPF,
//               etc.) to send IP packets.  This function adds an IP
//               header and calls RoutePacketAndSendToMac().
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + msg                : Message*     : Pointer to message with payload data
//                                       for IP packet.(IP header needs to
//                                       be added)
// + sourceAddress      : NodeAddress  : Source IP address.
//                        If sourceAddress is ANY_IP, lets IP assign the
//                        source  address (depends on the route).
// + destinationAddress : NodeAddress  : Destination IP address.
// + outgoingInterface  : int          : outgoing interface to use to
//                                       transmit packet.
// + priority           : TosType       : Priority of packet.
// + protocol           : unsigned char : IP protocol number.
// + ttl                : unsigned      : Time to live.
//                                     See AddIpHeader() for special values.
// RETURN              :: void :
// **/
void
NetworkIpSendRawMessage(
    Node *node,
    Message *msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    TosType priority,
    unsigned char protocol,
    unsigned ttl);


// /**
// API        :: NetworkIpSendRawMessageWithDelay
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpSendRawMessage(), but schedules
//               event after a simulation delay.
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + msg                : Message*     : Pointer to message with payload data
//                                       for IP packet.(IP header needs to
//                                       be added)
// + sourceAddress      : NodeAddress  : Source IP address.
//                        If sourceAddress is ANY_IP, lets IP assign the
//                        source  address (depends on the route).
// + destinationAddress : NodeAddress  : Destination IP address.
// + outgoingInterface  : int          : outgoing interface to use to
//                                       transmit packet.
// + priority           : TosType       : TOS of packet.
// + protocol           : unsigned char : IP protocol number.
// + ttl                : unsigned      : Time to live.
//                                     See AddIpHeader() for special values.
// + delay              : clocktype     : Delay
// RETURN              :: void :
// **/
void
NetworkIpSendRawMessageWithDelay(
    Node *node,
    Message *msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    TosType priority,
    unsigned char protocol,
    unsigned ttl,
    clocktype delay);


// /**
// API        :: NetworkIpSendRawMessageToMacLayer
// LAYER      :: Network
// PURPOSE    :: Called by network-layer routing protocols (AODV, OSPF,
//               etc.) to add an IP header to payload data, and with
//               the resulting IP packet, calls
//               NetworkIpSendPacketOnInterface().
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + msg                : Message*     : Pointer to message with payload data
//                                       for IP packet.(IP header needs to
//                                       be added)
// + sourceAddress      : NodeAddress  : Source IP address.
// + destinationAddress : NodeAddress  : Destination IP address.
// + priority           : TosType      : TOS of packet.
// + protocol           : unsigned char : IP protocol number.
// + ttl                : unsigned      : Time to live.
//                                     See AddIpHeader() for special values.
// + interfaceIndex     : int           : Index of outgoing interface.
// + nextHop            : NodeAddress   : Next hop IP address.
// RETURN     :: void :
// **/
void
NetworkIpSendRawMessageToMacLayer(
    Node *node,
    Message *msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    TosType priority,
    unsigned char protocol,
    unsigned ttl,
    int interfaceIndex,
    NodeAddress nextHop);


// /**
// API        :: NetworkIpSendRawMessageToMacLayerWithDelay
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpSendRawMessageToMacLayer(),
//               but schedules the event after a simulation delay
//               by calling NetworkIpSendPacketOnInterfaceWithDelay().
// PARAMETERS ::
// + node               : Node*       : Pointer to node.
// + msg                : Message*    : Pointer to message with payload data
//                                       for IP packet.(IP header needs to
//                                       be added)
// + sourceAddress      : NodeAddress  : Source IP address.
// + destinationAddress : NodeAddress  : Destination IP address.
// + priority           : TosType      : TOS of packet.
// + protocol           : unsigned char : IP protocol number.
// + ttl                : unsigned      : Time to live.
//                                     See AddIpHeader() for special values.
// + interfaceIndex     : int           : Index of outgoing interface.
// + nextHop            : NodeAddress   : Next hop IP address.
// + delay              : clocktype     : delay.
// RETURN              :: void :
// **/
void
NetworkIpSendRawMessageToMacLayerWithDelay(
    Node *node,
    Message *msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    TosType priority,
    unsigned char protocol,
    unsigned ttl,
    int interfaceIndex,
    NodeAddress nextHop,
    clocktype delay);


// /**
// API        :: NetworkIpSendPacketToMacLayer
// LAYER      :: Network
// PURPOSE    :: This function is called once the outgoing interface
//               index and next hop address to which to route an IP
//               packet are known.
// PARAMETERS ::
// + node           : Node*        : Pointer to node.
// + msg            : Message*     : Pointer to message with ip packet.
// + interfaceIndex : int          : Index of outgoing interface.
// + nextHop        : NodeAddress  : Next hop IP address.
// RETURN     :: void :
// **/
void
NetworkIpSendPacketToMacLayer(
    Node *node,
    Message *msg,
    int interfaceIndex,
    NodeAddress nextHop);


// /**
// API        :: NetworkIpSendPacketOnInterface
// LAYER      :: Network
// PURPOSE    :: This function is called once the outgoing interface
//               index and next hop address to which to route an IP
//               packet are known.  This queues an IP packet for delivery
//               to the MAC layer.  This functions calls
//               QueueUpIpFragmentForMacLayer().
//               This function is used to initiate fragmentation if
//               required, but since fragmentation has been disabled, all
//               it does is assert false if the IP packet is too big
//               before calling the next function.
// PARAMETERS          ::
// + node               : Node*       : Pointer to node.
// + msg                : Message*    : Pointer to message with ip packet.
// + incomingInterface : int          : Index of incoming interface.
// + outgoingInterface  : int         : Index of outgoing interface.
// + nextHop            : NodeAddress : Next hop IP address.
// RETURN     :: void :
// **/
void
NetworkIpSendPacketOnInterface(
    Node *node,
    Message *msg,
    int incomingInterface,
    int outgoingInterface,
    NodeAddress nextHop);

// /**
// API        :: NetworkIpSendPacketToMacLayerWithDelay
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpSendPacketOnInterface(), but schedules
//               event after a simulation delay.
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + msg                : Message*     : Pointer to message with ip packet.
// + interfaceIndex     : int          : Index of outgoing interface.
// + nextHop            : NodeAddress  : Next hop IP address.
// + delay              : clocktype    : delay
// RETURN     :: void :
// **/
void
NetworkIpSendPacketToMacLayerWithDelay(
    Node *node,
    Message *msg,
    int interfaceIndex,
    NodeAddress nextHop,
    clocktype delay);


// /**
// API        :: NetworkIpSendPacketOnInterfaceWithDelay
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpSendPacketOnInterface(), but schedules
//               event after a simulation delay.
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + msg                : Message*     : Pointer to message with ip packet.
// + incommingInterface : int          : Index of incomming interface.
// + outgoingInterface  : int          : Index of outgoing interface.
// + nextHop            : NodeAddress  : Next hop IP address.
// + delay              : clocktype    : delay
// RETURN              :: void :
// **/
void
NetworkIpSendPacketOnInterfaceWithDelay(
    Node *node,
    Message *msg,
    int incomingInterface,
    int outgoingInterface,
    NodeAddress nextHop,
    clocktype delay);


// /**
// API        :: NetworkIpSendPacketToMacLayerWithNewStrictSourceRoute
// LAYER      :: Network
// PURPOSE    :: Tacks on a new source route to an existing IP packet and
//               then sends the packet to the MAC layer.
// PARAMETERS ::
// + node                : Node*        : Pointer to node.
// + msg                 : Message*     : Pointer to message with ip packet.
// + newRouteAddresses   : NodeAddress[]: Source route (address array).
// + numNewRouteAddresses: int          : Number of array elements.
// + removeExistingRecordedRoute : BOOL : Flag to indicate previous record
//                                        route should be removed or not
//                                        TRUE ->removed else FALSE
// RETURN     :: void :
// **/
void
NetworkIpSendPacketToMacLayerWithNewStrictSourceRoute(
    Node *node,
    Message *msg,
    NodeAddress newRouteAddresses[],
    int numNewRouteAddresses,
    BOOL removeExistingRecordedRoute);

//----------------------------------------------------------
// MAC layer to IP, receives IP packets from the MAC layer
//----------------------------------------------------------

// /**
// API        :: NetworkIpReceivePacketFromMacLayer
// LAYER      :: Network
// PURPOSE    :: IP received IP packet from MAC layer. Updates the Stats
//               database and then calls NetworkIpReceivePacket.
//
// PARAMETERS ::
// + node              : Node*        : Pointer to node.
// + msg               : Message*     : Pointer to message with ip packet.
// + previousHopNodeId : NodeAddress  : nodeId of the previous hop.
// + interfaceIndex    : int          : Index of interface on which packet arrived.
// RETURN     :: void :
// **/
void
NetworkIpReceivePacketFromMacLayer(
    Node *node,
    Message *msg,
    NodeAddress previousHopNodeId,
    int interfaceIndex);

// /**
// API        :: NetworkIpReceivePacket
// LAYER      :: Network
// PURPOSE    :: IP received IP packet.  Determine whether
//               the packet is to be delivered to this node, or needs to
//               be forwarded.
//               ipHeader->ip_ttl is decremented here, instead of the
//               way BSD TCP/IP does it, which is to decrement TTL right
//               before forwarding the packet.  QualNet's alternative
//               method suits its network-layer ad hoc routing protocols,
//               which may do their own forwarding.
// PARAMETERS ::
// + node              : Node*        : Pointer to node.
// + msg               : Message*     : Pointer to message with ip packet.
// + previousHopNodeId : NodeAddress  : nodeId of the previous hop.
// + interfaceIndex    : int          : Index of interface on which packet arrived.
// RETURN     :: void :
// **/
void
NetworkIpReceivePacket(
    Node *node,
    Message *msg,
    NodeAddress previousHopNodeId,
    int interfaceIndex);
//----------------------------------------------------------
// MAC-layer packet drop callbacks
//----------------------------------------------------------

// /**
// API        :: NetworkIpNotificationOfPacketDrop
// LAYER      :: Network
// PURPOSE    :: Invoke callback functions when a packet is dropped.
// PARAMETERS ::
// + node              : Node*        : Pointer to node.
// + msg               : Message*     : Pointer to message with ip packet.
// + nextHopNodeAddres : NodeAddress  : next hop address of dropped packet.
// + interfaceIndex    : int          : interface that experienced the packet drop.
// RETURN     :: void :
// **/
void
NetworkIpNotificationOfPacketDrop(Node *node,
                                  Message *msg,
                                  NodeAddress nextHopAddres,
                                  int interfaceIndex);

// /**
// API        :: NetworkIpGetMacLayerStatusEventHandlerFunction
// LAYER      :: Network
// PURPOSE    :: Get the status event handler function pointer.
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : interface associated with the status
//                            handler function.
// RETURN     :: MacLayerStatusEventHandlerFunctionType : Status event
//                                                        handler function.
// **/
MacLayerStatusEventHandlerFunctionType
NetworkIpGetMacLayerStatusEventHandlerFunction(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpSetMacLayerStatusEventHandlerFunction
// LAYER      :: Network
// PURPOSE    :: Allows the MAC layer to send status messages (e.g.,
//               packet drop, link failure) to a network-layer routing
//               protocol for routing optimization.
// PARAMETERS ::
// + node                  : Node*    : Pointer to node.
// + StatusEventHandlerPtr : MacLayerStatusEventHandlerFunctionType : interface
//                               that experienced the packet drop.
// + interfaceIndex        : int      : interface associated with the status
//                                      event handler.
// RETURN     :: void :
// **/
void
NetworkIpSetMacLayerStatusEventHandlerFunction(
    Node *node,
    MacLayerStatusEventHandlerFunctionType StatusEventHandlerPtr,
    int interfaceIndex);

//------------------------------------------------------------------------
// MAC-layer promiscuous mode callbacks
//------------------------------------------------------------------------

// /**
// API        :: NetworkIpSneakPeekAtMacPacket
// LAYER      :: Network
// PURPOSE    :: Called Directly by the MAC layer, this allows a routing
//               protocol to "sneak a peek" or "tap" messages it would not
//               normally see from the MAC layer.  This function will
//               possibly unfragment such packets and call the function
//               registered by the routing protocol to do the "Peek".
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + msg            : const Message* : The message being peeked at from the
//                                     MAC layer.Must not be freed
//                                     or modified!.
// + interfaceIndex : int            : The interface of which the "peeked" message belongs to.
//                                     packet drop.
// + prevHop        : NodeAddress    : next hop address of dropped packet.
// RETURN     :: void :
// **/
void
NetworkIpSneakPeekAtMacPacket(Node *node,
                              const Message *msg,
                              int interfaceIndex,
                              MacHWAddress& prevHopHwAddr);

// /**
// API        :: NetworkIpGetPromiscuousMessagePeekFunction
// LAYER      :: Network
// PURPOSE    :: Returns the network-layer function which will
//               promiscuously inspect packets.
//               See NetworkIpSneakPeekAtMacPacket().
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface associated with the peek function.
// RETURN     ::PromiscuousMessagePeekFunctionType : Function pointer
// **/
PromiscuousMessagePeekFunctionType
NetworkIpGetPromiscuousMessagePeekFunction(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpSetPromiscuousMessagePeekFunction
// LAYER      :: Network
// PURPOSE    :: Sets the network-layer function which will
//               promiscuously inspect packets.
//               See NetworkIpSneakPeekAtMacPacket().
// PARAMETERS ::
// + node            : Node* : Pointer to node.
// + PeekFunctionPtr : PromiscuousMessagePeekFunctionType : Peek function.
// + interfaceIndex  : int   : Interface associated with the peek function.
// RETURN     :: void :
// **/
void
NetworkIpSetPromiscuousMessagePeekFunction(
    Node *node,
    PromiscuousMessagePeekFunctionType PeekFunctionPtr,
    int interfaceIndex);

// Mac layer acknowledgement handlers

// /**
// API        :: NetworkIpReceiveMacAck
// LAYER      :: Network
// PURPOSE    :: MAC received an ACK, so call ACK handler function.
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface associated with the  ACK handler function.
// + msg            : const Message* :   Message that was ACKed.
// + nextHop        : NodeAddress  :   Next hop that sent the MAC layer ACK
// RETURN     :: void :
// **/
void
NetworkIpReceiveMacAck(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    NodeAddress nextHop);

// /**
// API        :: NetworkIpGetMacLayerAckHandler
// LAYER      :: Network
// PURPOSE    :: Get MAC layer ACK handler
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface associated with ACK handler function
// RETURN     :: MacLayerAckHandlerType : MAC acknowledgement function pointer
// **/
MacLayerAckHandlerType
NetworkIpGetMacLayerAckHandler(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpSetMacLayerAckHandler
// LAYER      :: Network
// PURPOSE    :: Set MAC layer ACK handler
// PARAMETERS ::
// + node             : Node* : Pointer to node.
// + macAckHandlerPtr : MacLayerAckHandlerType : Callback function handling
//                                    mac layer acknowledgement that it has
//                                    successfully transmitted one packet
// + interfaceIndex   : int   : Interface associated with the  ACK handler
//                              function.
// RETURN     :: void :
// **/
void
NetworkIpSetMacLayerAckHandler(
    Node *node,
    MacLayerAckHandlerType macAckHandlerPtr,
    int interfaceIndex);

//----------------------------------------------------------
// Network layer to transport layer
//----------------------------------------------------------
//
// ODMRP is the only model which uses these functions outside of ip.c!
//

// /**
// API        :: SendToUdp
// LAYER      :: Network
// PURPOSE    :: Sends a UDP packet to UDP in the transport layer.
//               The source IP address, destination IP address, and
//               priority of the packet are also sent.
// PARAMETERS ::
// + node                   : Node*    : Pointer to node.
// + msg                    : Message* : Pointer to message with UDP packet.
// + priority               : TosType  : TOS of UDP packet.
// + sourceAddress          : NodeAddress : Source IP address.
// + destinationAddress     : NodeAddress : Destination IP address.
// + incomingInterfaceIndex : int        :interface that received the packet
// RETURN     :: void :
// **/
void
SendToUdp(
    Node *node,
    Message *msg,
    TosType priority,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int incomingInterfaceIndex);

// /**
// API        :: SendToTcp
// LAYER      :: Network
// PURPOSE    :: Sends a TCP packet to TCP in the transport layer.
//               The source IP address, destination IP address, and
//               priority of the packet are also sent..
// PARAMETERS ::
// + node                   : Node*    : Pointer to node.
// + msg                    : Message* : Pointer to message with TCP packet.
// + priority               : TosType  : TOS of TCP packet.
// + sourceAddress          : NodeAddress : Source IP address.
// + destinationAddress     : NodeAddress : Destination IP address.
// + aCongestionExperienced : BOOL        : Determine if congestion is
//                                          experienced (via ECN)..
// RETURN     :: void :
// **/
void
SendToTcp(
    Node *node,
    Message *msg,
    TosType priority,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    BOOL aCongestionExperienced);

// /**
// API        :: SendToRsvp
// LAYER      :: Network
// PURPOSE    :: Sends a RSVP packet to RSVP in the transport layer.
//               The source IP address, destination IP address, and
//               priority of the packet are also sent.
// PARAMETERS ::
// + node                   : Node*    : Pointer to node.
// + msg                    : Message* : Pointer to message with RSVP packet.
// + priority               : TosType  : TOS of UDP packet.
// + sourceAddress          : NodeAddress : Source IP address.
// + destinationAddress     : NodeAddress : Destination IP address.
// + interfaceIndex         : int         : incoming interface index.
// + ttl                    : unsigned    : Receiving TTL
// RETURN     :: void :
// **/
void
SendToRsvp(
    Node *node,
    Message *msg,
    TosType priority,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceIndex,
    unsigned ttl);


//-----------------------------------------------------------------------
// IP header
//-----------------------------------------------------------------------

// /**
// API        :: NetworkIpRemoveIpHeader
// LAYER      :: Network
// PURPOSE    :: Removes the IP header from a message while also
//               returning all the fields of the header.
// PARAMETERS ::
// + node               : Node*    : Pointer to node.
// + msg                : Message* : Pointer to message
// + sourceAddress      : NodeAddress* : Storage for source IP address.
// + destinationAddress : NodeAddress* : Storage for destination IP
//                                       address.
// + priority           : TosType* : Storage for TosType.(values are
//                           not standard for "IP type of service field"
//                           but has correct function)
// + protocol           : unsigned char*: Storage for IP protocol number
// + ttl                : unsigned*     : Storage for time to live.
// RETURN     :: void :
// **/
void
NetworkIpRemoveIpHeader(
    Node *node,
    Message *msg,
    NodeAddress *sourceAddress,
    NodeAddress *destinationAddress,
    TosType *priority,
    unsigned char *protocol,
    unsigned *ttl);

//----------------------------------------------------------
// IP header option field
//----------------------------------------------------------

//
// ODMRP is the only model which uses these functions outside of ip.c!
//

// /**
// API        :: AddIpOptionField
// LAYER      :: Network
// PURPOSE    :: Inserts an option field in the header of an IP packet.
// PARAMETERS ::
// + node          : Node*    : Pointer to node.
// + msg           : Message* : Pointer to message
// + optionCode    : int      : The option code
// + optionSize    : int      : Size of the option
// RETURN     :: void :
// **/
void
AddIpOptionField(
    Node *node,
    Message *msg,
    int optionCode,
    int optionSize);

//----------------------------------------------------------
// Source route
//----------------------------------------------------------

// /**
// API        :: ExtractIpSourceAndRecordedRoute
// LAYER      :: Network
// PURPOSE    :: Retrieves a copy of the source and recorded route from
//               the options field in the header.
// PARAMETERS ::
// + msg               : Message* : Pointer to message with IP packet.
// + RouteAddresses    : NodeAddress[] : Storage for source/recorded route.
// + NumAddresses      : int* : Storage for size of RouteAddresses[] array.
// + RouteAddressIndex : int* : The index of the first address of the
//                              source  route; before this index is the
//                              recorded route.
// RETURN     :: void :
// **/
void
ExtractIpSourceAndRecordedRoute(
    Message *msg,
    NodeAddress RouteAddresses[],
    int *NumAddresses,
    int *RouteAddressIndex);

//----------------------------------------------------------
// Router info helper functions
//----------------------------------------------------------

// /**
// API        :: NetworkIpGetRouterFunction
// LAYER      :: Network
// PURPOSE    :: Get the router function pointer.
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : interface associated with router function
// RETURN     :: RouterFunctionType : router function pointer.
// **/
RouterFunctionType
NetworkIpGetRouterFunction(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpSetRouterFunction
// LAYER      :: Network
// PURPOSE    :: Allows a routing protocol to set the "routing function"
//               (one of its functions) which is called when a packet
//               needs to be routed.
//               NetworkIpSetRouterFunction() allows a routing protocol
//               to define the routing function.  The routing function
//               is called by the network layer to ask the routing
//               protocol to route the packet.  The routing function is
//               given the packet and its destination.  The routing
//               protocol can route the packet and set "PacketWasRouted"
//               to TRUE; or not route the packet and set to FALSE.  If
//               the packet, was not routed, then the network layer will
//               try to use the forwarding table or the source route the
//               source route in the IP header.  This function will also
//               be given packets for the local node the routing
//               protocols can look at packets for protocol reasons.  In
//               this case, the message should not be modified and
//               PacketWasRouted must be set to FALSE.
// PARAMETERS ::
// + node              : Node*   : Pointer to node.
// + RouterFunctionPtr : RouterFunctionType : Router function to set.
// + interfaceIndex    : int     : interface associated with router function.
// RETURN     :: void :
// **/
void
NetworkIpSetRouterFunction(
    Node *node,
    RouterFunctionType RouterFunctionPtr,
    int interfaceIndex);


// /**
// API        :: NetworkIpAddUnicastRoutingProtocolType
// LAYER      :: Network
// PURPOSE    :: Add unicast routing protocol type to interface.
// PARAMETERS ::
// + node              : Node*: Pointer to node.
// + routingProtocolType : NetworkRoutingProtocolType : Router function to add.
// + interfaceIndex    : int : Interface associated with the router function.
// RETURN     :: void :
// **/
void
NetworkIpAddUnicastRoutingProtocolType(
    Node *node,
    NetworkRoutingProtocolType routingProtocolType,
    int interfaceIndex,
    NetworkType networkType = NETWORK_IPV4);

#ifdef ADDON_BOEINGFCS
// /**
// API        :: NetworkIpAddUnicastIntraRegionRoutingProtocolType
// LAYER      :: Network
// PURPOSE    :: Add unicast intra region routing protocol type to interface.
// PARAMETERS ::
// + node              : Node*: Pointer to node.
// + routingProtocolType : NetworkRoutingProtocolType : Router function to add.
// + interfaceIndex    : int : Interface associated with the router function.
// RETURN     :: void :
// **/
void
NetworkIpAddUnicastIntraRegionRoutingProtocolType(
    Node *node,
    NetworkRoutingProtocolType routingProtocolType,
    int interfaceIndex);
#endif

// /**
// API        :: NetworkIpGetRoutingProtocol
// LAYER      :: Network
// PURPOSE    :: Get routing protocol structure associated with
//               routing protocol
//               running on this interface.
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + routingProtocolType : NetworkRoutingProtocolType : Router function to
//                                                      retrieve.
// RETURN     :: void* : Routing protocol structure requested.
// **/
void *
NetworkIpGetRoutingProtocol(
    Node *node,
    NetworkRoutingProtocolType routingProtocolType,
    NetworkType networkType = NETWORK_IPV4);

// /**
// API        :: NetworkIpGetUnicastRoutingProtocolType
// LAYER      :: Network
// PURPOSE    :: Get unicast routing protocol type on this interface.
// PARAMETERS ::
// + node           : Node*    : Pointer to node.
// + interfaceIndex : int      : network interface for request.
// RETURN     :: NetworkRoutingProtocolType : The unicast routing
//                                            protocol type.
// **/
NetworkRoutingProtocolType
NetworkIpGetUnicastRoutingProtocolType(
    Node *node,
    int interfaceIndex,
    NetworkType networkType = NETWORK_IPV4);

// /**
// API        :: NetworkIpSetHsrpOnInterface
// LAYER      :: Network
// PURPOSE    :: To enable hsrp on a interface
// PARAMETERS ::
// + node           : Node*    : Pointer to node.
// + interfaceIndex : int      : network interface.
// RETURN     :: void :
// **/
void NetworkIpSetHsrpOnInterface(Node *node, int interfaceIndex);


// /**
// API        :: NetworkIpIsHsrpEnabled
// LAYER      :: Network
// PURPOSE    :: To test if any interface is hsrp enabled.
// PARAMETERS ::
// + node           : Node*    : Pointer to node.
// + interfaceIndex : int      : network interface.
// RETURN     :: BOOL : return TRUE if any one interface is hsrp enabled
//                      else return FALSE.
// **/
BOOL NetworkIpIsHsrpEnabled(Node *node, int interfaceIndex);


//----------------------------------------------------------
// Interface creation
//----------------------------------------------------------

// /**
// API        :: NetworkIpAddNewInterface
// LAYER      :: Network
// PURPOSE    :: Add new interface to node.
// PARAMETERS ::
// + node                : Node*       : Pointer to node.
// + interfaceIpAddress  : NodeAddress :Interface to add.
// + numHostBits         : int : Number of host bits for the interface.
// + newInterfaceIndex   : int*: The interface number of the new interface.
// + nodeInput           : const NodeInput* : Provides access to
//                                            configuration file.
// RETURN     :: void :
// **/
void
NetworkIpAddNewInterface(
    Node *node,
    NodeAddress interfaceIpAddress,
    int numHostBits,
    int *newInterfaceIndex,
    const NodeInput *nodeInput,
    BOOL isNewInterface);

//----------------------------------------------------------
// Queue setup
//----------------------------------------------------------

void //inline//
IpInitPerHopBehaviors(
    Node *node,
    const NodeInput *nodeInput);

// /**
// API        :: NetworkIpInitCpuQueueConfiguration
// LAYER      :: Network
// PURPOSE    :: Initializes cpu queue parameters during startup.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// RETURN     :: void :
// */
//static
void
NetworkIpInitCpuQueueConfiguration(
    Node *node,
    const NodeInput *nodeInput);


// /**
// API        :: NetworkIpInitInputQueueConfiguration
// LAYER      :: Network
// PURPOSE    :: Initializes input queue parameters during startup.
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + nodeInput      : const NodeInput* : Pointer to node input.
// + interfaceIndex : int : interface associated with queue.
// RETURN     :: void :
// **/
void
NetworkIpInitInputQueueConfiguration(
    Node *node,
    const NodeInput *nodeInput,
    int interfaceIndex);


// /**
// API        :: NetworkIpInitOutputQueueConfiguration
// LAYER      :: Network
// PURPOSE    :: Initializes queue parameters during startup.
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + nodeInput      : const NodeInput* : Pointer to node input.
// + interfaceIndex : int : interface associated with queue.
// RETURN     :: void :
// **/
void
NetworkIpInitOutputQueueConfiguration(
    Node *node,
    const NodeInput *nodeInput,
    int interfaceIndex);


// /**
// API        :: NetworkIpCreateQueues
// LAYER      :: Network
// PURPOSE    :: Initializes input and output queue parameters during startup
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + nodeInput      : const NodeInput* : Pointer to node input.
// + interfaceIndex : int : interface associated with queue.
// RETURN     :: void :
// **/
void
NetworkIpCreateQueues(
    Node *node,
    const NodeInput *nodeInput,
    int interfaceIndex);


// /**
// API        :: NetworkIpSchedulerParameterInit
// LAYER      :: Network
// PURPOSE    :: Initialize the scheduler parameters and also allocate
//               memory for queues if require.
// PARAMETERS ::
// + schedulerPtr  : Scheduler* : pointer to schedular
// + numPriorities : const int      : Number of priorities available
// + queue         : Queue*         : pointer to ip queue.
// RETURN     :: void :
// **/
void
NetworkIpSchedulerParameterInit(
    Scheduler *schedulerPtr,
    const int numPriorities,
    Queue *queuePtr);


// /**
// API        :: NetworkIpSchedulerInit
// LAYER      :: Network
// PURPOSE    :: Call initialization function for appropriate scheduler.
// PARAMETERS ::
// + node                : Node* : pointer to node
// + nodeInput           : const NodeInput* : pointer to nodeinput
// + interfaceIndex      : int    : interface index
// + schedulerPtr        : Scheduler* : type of Scheduler
// + schedulerTypeString : const char* : Scheduler name
// RETURN     :: void :
// **/
void
NetworkIpSchedulerInit(
    Node *node,
    const NodeInput *nodeInput,
    int interfaceIndex,
    Scheduler *schedulerPtr,
    const char *schedulerTypeString);

//----------------------------------------------------------
// Network-layer enqueueing
//----------------------------------------------------------


// /**
// API        :: NetworkIpCpuQueueInsert
// LAYER      :: Network
// PURPOSE    :: Calls the cpu packet scheduler for an interface to retrieve
//               an IP packet from a queue associated with the interface.
//               The dequeued packet, since it's already been routed,
//               has an associated next-hop IP address.  The packet's
//               priority value is also returned.
// PARAMETERS ::
// + node               : Node*    : Pointer to node.
// + msg                : Message* : Pointer to message with IP packet.
// + nextHopAddress     : NodeAddress : Packet's next hop address.
// + destinationAddress : NodeAddress : Packet's destination address.
// + outgoingInterface  : int         : Used to determine where packet
//                                      should go after passing through
//                                      the backplane.
// + networkType        : int   : Type of network packet is using (IP,
//                                Link-16, ...)
// + queueIsFull        : BOOL* : Storage for boolean indicator.
//                                If TRUE, packet was not queued because
//                                scheduler reported queue was
//                                (or queues were) full.
// + incomingInterface  : int   : Incoming interface of packet.
// RETURN     :: void :
// **/
void
NetworkIpCpuQueueInsert(
    Node *node,
    Message *msg,
    NodeAddress nextHopAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    int networkType,
    BOOL *queueIsFull,
    int incomingInterface = ANY_INTERFACE);


// /**
// API        :: NetworkIpInputQueueInsert
// LAYER      :: Network
// PURPOSE    :: Calls input packet scheduler for an interface to retrieve
//               an IP packet from a queue associated with the interface.
//               The dequeued packet, since it's already been routed,
//               has an associated next-hop IP address.  The packet's
//               priority value is also returned.
// PARAMETERS ::
// + node               : Node*    : Pointer to node.
// + incomingInterface  : int      : interface of input queue.
// + msg                : Message* : Pointer to message with IP packet.
// + nextHopAddress     : NodeAddress : Packet's next hop address.
// + destinationAddress : NodeAddress : Packet's destination address.
// + outgoingInterface  : int         : Used to determine where packet
//                                      should go after passing through
//                                      the backplane.
// + networkType        : int   : Type of network packet is using (IP,
//                                Link-16, ...)
// + queueIsFull        : BOOL* : Storage for boolean indicator.
//                                If TRUE, packet was not queued because
//                                scheduler reported queue was
//                                (or queues were) full.
// RETURN     :: void :
// **/
void
NetworkIpInputQueueInsert(
    Node *node,
    int incomingInterface,
    Message *msg,
    NodeAddress nextHopAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    int networkType,
    BOOL *queueIsFull);


// /**
// API        :: NetworkIpOutputQueueInsert
// LAYER      :: Network
// PURPOSE    :: Calls output packet scheduler for an interface to retrieve
//               an IP packet from a queue associated with the interface.
//               The dequeued packet, since it's already been routed,
//               has an associated next-hop IP address.  The packet's
//               priority value is also returned.
//               Called by QueueUpIpFragmentForMacLayer().
// PARAMETERS ::
// + node               : Node*    : Pointer to node.
// + interfaceIndex     : int      : interface of input queue.
// + msg                : Message* : Pointer to message with IP packet.
// + nextHopAddress     : NodeAddress : Packet's next hop address.
// + destinationAddress : NodeAddress : Packet's destination address.
// + networkType        : int   : Type of network packet is using (IP,
//                                Link-16, ...)
// + queueIsFull        : BOOL* : Storage for boolean indicator.
//                                If TRUE, packet was not queued because
//                                scheduler reported queue was
//                                (or queues were) full.
// RETURN     :: void :
// **/
void
NetworkIpOutputQueueInsert(
    Node *node,
    int interfaceIndex,
    Message *msg,
    NodeAddress nextHopAddress,
    NodeAddress destinationAddress,
    int networkType,
    BOOL *queueIsFull);


//----------------------------------------------------------
// Network-layer dequeueing
//----------------------------------------------------------


// /**
// API        :: NetworkIpInputQueueDequeuePacket
// LAYER      :: Network
// PURPOSE    :: Calls the packet scheduler for an interface to retrieve
//               an IP packet from the input queue associated with the
//               interface.
// PARAMETERS ::
// + node              : Node*     : Pointer to node.
// + incomingInterface : int       : interface to dequeue from.
// + msg               : Message** : Storage for pointer to message
//                                   with IP packet.
// + nextHopAddress    : NodeAddress* : Storage for Packet's
//                                      next hop address.
// + outgoingInterface : int*      : Used to determine where packet
//                                   should go after passing
//                                   through the backplane.
// + networkType       : int*  : Type of network packet is using (IP,
//                               Link-16, ...)
// + priority          : QueuePriorityType* : Storage for
//                                                     priority of packet.
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL NetworkIpInputQueueDequeuePacket(
    Node *node,
    int incomingIterface,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int *outgoingInterface,
    int *networkType,
    QueuePriorityType *priority);


// /**
// API        :: NetworkIpOutputQueueDequeuePacket
// LAYER      :: Network
// PURPOSE    :: Calls the packet scheduler for an interface to retrieve
//               an IP packet from a queue associated with the interface.
//               The dequeued packet, since it's already been routed,
//               has an associated next-hop IP address.  The packet's
//               priority value is also returned.
//               This function is called by
//
//               MAC_OutputQueueDequeuePacket() (mac/mac.pc), which itself
//               is called from mac/mac_802_11.pc and other MAC protocol
//               source files.
//
//               This function will assert false if the scheduler cannot
//               return an IP packet for whatever reason.
// PARAMETERS ::
// + node           : Node*     : Pointer to node.
// + interfaceIndex : int       : index to interface .
// + msg            : Message** : Storage for pointer to message
//                                with IP packet.
// + nextHopAddress : NodeAddress* : Storage for Packet's next hop address.
// + networkType    : int*  : Type of network packet is using (IP,
//                            Link-16, ...)
// + priority       : QueuePriorityType* : Storage for priority
//                                                    of packet.
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL NetworkIpOutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int *networkType,
    QueuePriorityType *priority);


// /**
// API        :: NetworkIpOutputQueueDequeuePacketForAPriority
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpOutputQueueDequeuePacket(), except the
//               packet dequeued is requested by a specific priority,
//               instead of leaving the priority decision up to the
//               packet scheduler.
//               This function is called by
//               MAC_OutputQueueDequeuePacketForAPriority() (mac/mac.pc),
//               which itself is called from mac/mac_802_11.pc and other
//               MAC protocol source files.
//
//               This function will assert false if the scheduler cannot
//               return an IP packet for whatever reason.
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : index to interface .
// + priority        : QueuePriorityType : priority of packet.
// + msg             : Message** : Storage for pointer to message
//                                 with IP packet.
// + nextHopAddress  : NodeAddress* : Storage for Packet's next hop address.
// + networkType     : int*  : Type of network packet is using (IP,
//                             Link-16, ...)
// + posInQueue      : int : Position of packet in Queue.
//                          Added as part of IP-MPLS integration
//                          Default - DEQUEUE_NEXT_PACKET
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL NetworkIpOutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    QueuePriorityType priority,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int *networkType,
    int posInQueue = DEQUEUE_NEXT_PACKET);

// /**
// API        :: NetworkIpOutputQueueDequeuePacketWithIndex
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpOutputQueueDequeuePacket(), except the
//               packet dequeued is requested by a specific index
//               This function is called by
//               MAC_OutputQueueDequeuePacketForAPriority() (mac/mac.pc),
//               which itself is called from mac/mac_802_11.pc and other
//               MAC protocol source files.
//
//               This function will assert false if the scheduler cannot
//               return an IP packet for whatever reason.
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : index to interface .
// + msgIndex        : int       : index of packet.
// + msg             : Message** : Storage for pointer to message
//                                 with IP packet.
// + nextHopAddress  : NodeAddress* : Storage for Packet's next hop address.
// + networkType     : int*  : Type of network packet is using (IP,
//                             Link-16, ...)
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL NetworkIpOutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType);

// /**
// API        :: NetworkIpInputQueueTopPacket
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpInputQueueDequeuePacket(), except the
//               packet is not actually dequeued.  Note that the message
//               containing the packet is not copied; the contents may
//               (inadvertently or not) be directly modified.
// PARAMETERS ::
// + node              : Node*     : Pointer to node.
// + incomingInterface : int       : interface to get top packet from.
// + msg               : Message** : Storage for pointer to message
//                                   with IP packet.
// + nextHopAddress    : NodeAddress* : Storage for Packet's next hop addr.
// + outgoingInterface : int*  : Used to determine where packet should go
//                               after passing through the backplane.
// + networkType       : int*  : Type of network packet is using (IP,
//                               Link-16, ...)
// + priority          : QueuePriorityType* : Storage for priority
//                                                      of packet.
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL NetworkIpInputQueueTopPacket(
    Node *node,
    int incomingInterface,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int *outgoingInterface,
    int *networkType,
    QueuePriorityType *priority);

// /**
// API        :: NetworkIpOutputQueueTopPacket
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpOutputQueueDequeuePacket(), except the
//               packet is not actually dequeued.  Note that the message
//               containing the packet is not copied; the contents may
//               (inadvertently or not) be directly modified.
//
//               This function is called by MAC_OutputQueueTopPacket()
//               (mac/mac.pc), which itself is called from
//               mac/mac_802_11.pc and other MAC protocol source files.
//
//               This function will assert false if the scheduler cannot
//               return an IP packet for whatever reason.
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : index to interface .
// + msg             : Message** : Storage for pointer to message
//                                 with IP packet.
// + nextHopAddress  : NodeAddress* : Storage for Packet's next hop addr.
// + networkType     : int*      : Type of network of the packet
// + priority        : QueuePriorityType* : Storage for priority
//                                                    of packet.
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL NetworkIpOutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int* networkType,
    QueuePriorityType *priority);

// /**
// API        :: NetworkIpOutputQueuePeekWithIndex
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpOutputQueueDequeuePacket(), except the
//               packet is not actually dequeued.  Note that the message
//               containing the packet is not copied; the contents may
//               (inadvertently or not) be directly modified.
//
//               This function is called by MAC_OutputQueueTopPacket()
//               (mac/mac.pc), which itself is called from
//               mac/mac_802_11.pc and other MAC protocol source files.
//
//               This function will assert false if the scheduler cannot
//               return an IP packet for whatever reason.
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : index to interface .
// + msgIndex        : int       : index to message .
// + msg             : Message** : Storage for pointer to message
//                                 with IP packet.
// + nextHopAddress  : NodeAddress* : Storage for Packet's next hop addr.
// + priority        : QueuePriorityType* : Storage for priority
//                                                    of packet.
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL NetworkIpOutputQueuePeekWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    QueuePriorityType *priority);

// /**
// API        :: NetworkIpOutputQueueTopPacketForAPriority
// LAYER      :: Network
// PURPOSE    :: Same as NetworkIpOutputQueueDequeuePacketForAPriority(),
//               except the packet is not actually dequeued.  Note that
//               the message containing the packet is not copied; the
//               contents may (inadvertently or not) be directly
//               modified.
//
//               This function is called by
//               MAC_OutputQueueTopPacketForAPriority() (mac/mac.pc),
//               which itself is called from mac/mac_802_11.pc and other
//               MAC protocol source files.
//
//               This function will assert false if the scheduler cannot
//               return an IP packet for whatever reason.
// PARAMETERS ::
// + node           : Node*     : Pointer to node.
// + interfaceIndex : int       : index to interface .
// + priority       : QueuePriorityType : priority of packet
// + msg            : Message** : Storage for pointer to message
//                               with IP packet.
// + nextHopAddress : NodeAddress*: Storage for packet's next hop address.
// + posInQueue     : int : Position of packet in Queue.
//                          Added as part of IP-MPLS integration
// RETURN     : BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL NetworkIpOutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    QueuePriorityType priority,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int posInQueue = DEQUEUE_NEXT_PACKET);

// /**
// API        :: NetworkIpInputQueueIsEmpty
// LAYER      :: Network
// PURPOSE    :: Calls the packet scheduler for an interface to determine
//               whether the interface's input queue is empty
// PARAMETERS ::
// + node              : Node*: Pointer to node.
// + incomingInterface : int  : Index of interface.
// RETURN     :: BOOL  : TRUE if the scheduler says the interface's
//                       input queue is empty.
//                       FALSE if the scheduler says the interface's
//                       input queue is not empty.
// **/
BOOL
NetworkIpInputQueueIsEmpty(Node *node, int incomingInterface);


// /**
// API        :: NetworkIpOutputQueueIsEmpty
// LAYER      :: Network
// PURPOSE    :: Calls the packet scheduler for an interface to determine
//               whether the interface's output queue is empty.
// PARAMETERS ::
// + node           : Node*: Pointer to node.
// + interfaceIndex : int  : Index of interface.
// RETURN     :: BOOL : TRUE if the scheduler says the interface's output
//                      queue is empty.
//                      FALSE if the scheduler says the interface's output
//                      queue is not empty.
// **/
BOOL
NetworkIpOutputQueueIsEmpty(Node *node, int interfaceIndex);

// /**
// API        :: NetworkIpOutputQueueNumberInQueue
// LAYER      :: Network
// PURPOSE    :: Calls the packet scheduler for an interface to determine
//               how many packets are in a queue.  There may be multiple
//               queues on an interface, so the priority of the desired
//               queue is also provided.
// PARAMETERS ::
// + node           : Node*: Pointer to node.
// + interfaceIndex : int  : Index of interface.
// + specificPriorityOnly : BOOL : Should we only get the number of packets
//                                 in queue for the specified priority only
//                                 or for all priorities.
// + priority       : QueuePriorityType : Priority of queue.
// RETURN     :: int : Number of packets in queue.
// **/
int
NetworkIpOutputQueueNumberInQueue(
    Node *node,
    int interfaceIndex,
    BOOL specificPriorityOnly,
    QueuePriorityType priority);

// /**
// API        :: NetworkIpOutputQueueDropPacket
// LAYER      :: Network
// PURPOSE    :: Drop a packet from the queue.
// PARAMETERS ::
// + node           : Node*     : Pointer to node.
// + interfaceIndex : int       : index to interface .
// + msg            : Message** : Storage for pointer to message
//                                   with IP packet.
// RETURN     :: NodeAddress : Next hop of dropped packet.
// **/
NodeAddress
NetworkIpOutputQueueDropPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    MacHWAddress *nexthopmacAddr);


// /**
// API        :: NetworkIpDeleteOutboundPacketsToANode
// LAYER      :: Network
// PURPOSE    :: Deletes all packets in the queue going (probably broken),
//               to the specified next hop address.   There is option
//               to return all such packets back to the routing protocols.
//               via the usual mechanism (callback).
// PARAMETERS ::
// + node               : Node*             : Pointer to node.
// + nextHopAddress     : const NodeAddress : Next hop associated with
//                                            outbound packets.
// + destinationAddress : const NodeAddress : destination associated with
//                                            outbound packets.
// + returnPacketsToRoutingProtocol : const BOOL : Determine whether or not
//                                                dropped packets should be
//                                                returned to the routing
//                                                protocol for further
//                                                processing.
// RETURN     :: void :
// **/
void NetworkIpDeleteOutboundPacketsToANode(
    Node *node,
    const NodeAddress nextHopAddress,
    const NodeAddress destinationAddress,
    const BOOL returnPacketsToRoutingProtocol);


// /**
// FUNCTION   :: GetQueueNumberFromPriority
// LAYER      :: Network
// PURPOSE    :: Get queue number through which a given user priority
//               will be forwarded.
// PARAMETERS ::
// + userTos        : TosType   : user TOS.
// + numQueues      : int       : Number of queues.
// RETURN     :: unsigned : Index of the queue.
// **/
unsigned GetQueueNumberFromPriority(
    TosType userTos,
    int numQueues);


//----------------------------------------------------------
// Per hop behavior (PHB) routing
//----------------------------------------------------------


// /**
// API        :: ReturnPriorityForPHB
// LAYER      :: Network
// PURPOSE    :: Returns the priority queue corresponding to the
//               DS/TOS field.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + tos       : TosType       : TOS field
// RETURN     :: QueuePriorityType : priority queue
// **/
QueuePriorityType
ReturnPriorityForPHB(
    Node *node,
    TosType tos);

//----------------------------------------------------------
// Routing table (forwarding table)
//----------------------------------------------------------

// /**
// API        :: NetworkGetInterfaceAndNextHopFromForwardingTable
// LAYER      :: Network
// PURPOSE    :: Do a lookup on the routing table with a destination IP
//               address to obtain a route (index of an outgoing
//               interface and a next hop Ip address).
// PARAMETERS ::
// + node               : Node*       : Pointer to node.
// + destinationAddress : NodeAddress : Destination IP address.
// + interfaceIndex     : int*        : Storage for index of outgoing
//                                      interface.
// + nextHopAddress     : NodeAddress* : Storage for next hop IP address.
//                                       If no route can be found,
//                                       *nextHopAddress will be
//                                       set to NETWORK_UNREACHABLE.
// RETURN     :: void :
// **/
void NetworkGetInterfaceAndNextHopFromForwardingTable(
    Node *node,
    NodeAddress destinationAddress,
    int *interfaceIndex,
    NodeAddress *nextHopAddress);

// /**
// API        :: NetworkGetInterfaceAndNextHopFromForwardingTable
// LAYER      :: Network
// PURPOSE    :: Do a lookup on the routing table with a destination IP
//               address to obtain a route (index of an outgoing
//               interface and a next hop Ip address).
// PARAMETERS ::
// + node               : Node*       : Pointer to node.
// + destinationAddress : NodeAddress : Destination IP address.
// + interfaceIndex     : int*        : Storage for index of outgoing
//                                      interface.
// + nextHopAddress     : NodeAddress* : Storage for next hop IP address.
//                                       If no route can be found,
//                                       *nextHopAddress will be
//                                       set to NETWORK_UNREACHABLE.
// + testType : BOOL : Same protocol's routes if true
//                     different protocol's routes else
// + type : NetworkRoutingProtocolType : routing protocol type.
// RETURN     :: void :
// **/
void NetworkGetInterfaceAndNextHopFromForwardingTable
(
    Node *node,
    NodeAddress destinationAddress,
    int *interfaceIndex,
    NodeAddress *nextHopAddress,
    BOOL testType,
    NetworkRoutingProtocolType type);

// /**
// API        :: NetworkIpGetInterfaceIndexForNextHop
// LAYER      :: Network
// PURPOSE    :: This function looks at the network address of each of a
//               node's network interfaces.  When nextHopAddress is
//               matched to a network, the interface index corresponding
//               to the network is returned.
//               (used by NetworkUpdateForwardingTable() and ospfv2.pc)
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + nextHopAddress : NodeAddress : Destination IP address.
// RETURN     :: int    : Index of outgoing interface, if nextHopAddress
//                        is on a directly connected network. -1, otherwise
// **/
int
NetworkIpGetInterfaceIndexForNextHop(
    Node *node,
    NodeAddress nextHopAddress);


// /**
// API        :: NetworkGetInterfaceIndexForDestAddress
// LAYER      :: Network
// PURPOSE    :: Get interface for the destination address.
// PARAMETERS ::
// + node        : Node*      : Pointer to node.
// + destAddress : NodeAddress : Destination IP address.
// RETURN     :: int : interface index associated with destination.
// **/
int
NetworkGetInterfaceIndexForDestAddress(
    Node *node,
    NodeAddress destAddress);

// /**
// API        :: NetworkRoutingGetAdminDistance
// LAYER      :: Network
// PURPOSE    :: Get the administrative distance of a routing protocol.
//               These values don't quite match those recommended by
//               Cisco.
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + type      : NetworkRoutingProtocolType : Type value of routing protocol.
// RETURN     :: NetworkRoutingAdminDistanceType : The administrative
//                                         distance of the routing protocol.
// **/
NetworkRoutingAdminDistanceType
NetworkRoutingGetAdminDistance(
    Node *node,
    NetworkRoutingProtocolType type);

// /**
// API        :: NetworkInitForwardingTable
// LAYER      :: Network
// PURPOSE    :: Initialize the IP fowarding table, allocate enough
//               memory for number of rows.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// RETURN     :: void :
// **/
void
NetworkInitForwardingTable(Node *node);


// /**
// API        :: NetworkUpdateForwardingTable
// LAYER      :: Network
// PURPOSE    :: Update or add entry to IP routing table.  Search the
//               routing table for an entry with an exact match for
//               destAddress, destAddressMask, and routing protocol.
//               Update this entry with the specified nextHopAddress
//               (the outgoing interface is automatically determined
//               from the nextHopAddress -- see code).  If no matching
//               entry found, then add a new route.
// PARAMETERS ::
// + node                   : Node*   : Pointer to node.
// + destAddress            : NodeAddress : IP address of destination
//                                          network or host.
// + destAddressMask        : NodeAddress : Netmask.
// + nextHopAddress         : NodeAddress : Next hop IP address.
// + outgoingInterfaceIndex : int         : outgoing interface.
// + cost                   : int         : Cost metric associated with
//                                          the route.
// + type                   : NetworkRoutingProtocolType : type value of
//                                                       routing protocol.
// RETURN     :: void :
// **/
void
NetworkUpdateForwardingTable(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int outgoingInterfaceIndex,
    int cost,
    NetworkRoutingProtocolType type);


// /**
// API        :: NetworkEmptyForwardingTable
// LAYER      :: Network
// PURPOSE    :: Remove entries in the routing table corresponding to a
//               given routing protocol.
// PARAMETERS ::
// + node      : Node*                      : Pointer to node.
// + type      : NetworkRoutingProtocolType : Type of routing protocol whose
//                                             entries are to be  removed.
// RETURN     :: void :
// **/
void
NetworkEmptyForwardingTable(
    Node *node,
    NetworkRoutingProtocolType type);


// /**
// API        :: NetworkPrintForwardingTable
// LAYER      :: Network
// PURPOSE    :: Display all entries in node's routing table.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// RETURN     :: void :
// **/
void
NetworkPrintForwardingTable(Node *node);

// /**
// API        :: NetworkGetMetricForDestAddress
// LAYER      :: Network
// PURPOSE    :: Get the cost metric for a destination from the
//               forwarding table.
// PARAMETERS ::
// + node        : Node* : Pointer to node.
// + destAddress : NodeAddress : destination to get cost metric from.
// RETURN     :: int : Cost metric associated with destination.
// **/
int
NetworkGetMetricForDestAddress(
    Node *node,
    NodeAddress destAddress);

// /**
// API        :: NetworkIpSetRouteUpdateEventFunction
// LAYER      :: Network
// PURPOSE    :: Set a callback fuction when a route changes from forwarding
//               table.
// PARAMETERS ::
// + node      : Node* :Pointer to node.
// + routeUpdateFunctionPtr : NetworkRouteUpdateEventType : Route update
//                                              callback function to set.
// RETURN     :: void :
// **/
void
NetworkIpSetRouteUpdateEventFunction(
    Node *node,
    NetworkRouteUpdateEventType routeUpdateFunctionPtr);


// /**
// API        :: NetworkIpGetRouteUpdateEventFunction
// LAYER      :: Network
// PURPOSE    :: Print packet headers when packet tracing is enabled.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// RETURN     :: NetworkRouteUpdateEventType : Route update callback
//                                             function to set.
// **/
NetworkRouteUpdateEventType
NetworkIpGetRouteUpdateEventFunction(
    Node *node);

//----------------------------------------------------------
// Interface IP addresses
//----------------------------------------------------------

// /**
// API        :: NetworkIpGetInterfaceAddress
// LAYER      :: Network
// PURPOSE    :: Get the interface address on this interface
// PARAMETERS ::
// + node           : Node* : Pointer to the node
// + interfaceIndex : int : Number of interface
// RETURN     :: NodeAddress : IP address associated with the interface
// **/
NodeAddress
NetworkIpGetInterfaceAddress(
    Node *node,
    int interfaceIndex);

NodeAddress
NetworkIpGetLinkLayerAddress(Node* node, int interfaceIndex);

int
NetworkIpGetInterfaceIndexFromLinkAddress(Node* node,
        NodeAddress ownMacAddr);


// /**
// API        :: NetworkIpGetInterfaceName
// LAYER      :: Network
// PURPOSE    :: To get the interface name associated with the interface
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + interfaceIndex : int : Number of interface
// RETURN     :: char* : interface name
// **/
char*
NetworkIpGetInterfaceName(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpGetInterfaceNetworkAddress
// LAYER      :: Network
// PURPOSE    :: To get network address associated with interface
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + interfaceIndex : int : Number of interface
// RETURN     :: NodeAddress : network address associated with interface
// **/
NodeAddress
NetworkIpGetInterfaceNetworkAddress(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpGetInterfaceSubnetMask
// LAYER      :: Network
// PURPOSE    :: To retrieve subnet mask of the node interface
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + interfaceIndex : int : Number of interface
// RETURN     :: NodeAddress : subnet mask of the specified interface
// **/
NodeAddress
NetworkIpGetInterfaceSubnetMask(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpGetInterfaceNumHostBits
// LAYER      :: Network
// PURPOSE    :: Get the number of host bits on this interface
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + interfaceIndex : int : Number of interface
// RETURN     :: int : Number of host bits on the specified interface
// **/
int
NetworkIpGetInterfaceNumHostBits(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpGetInterfaceBroadcastAddress
// LAYER      :: Network
// PURPOSE    :: Get broadcast address on this interface
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + interfaceIndex : int : Number of interface
// RETURN     :: NodeAddress : Broadcast address of specified interface
// **/
NodeAddress
NetworkIpGetInterfaceBroadcastAddress(
    Node *node,
    int interfaceIndex);

// /**
// API        :: IsOutgoingBroadcast
// LAYER      :: Network
// PURPOSE    :: Checks whether IP packet's destination address is broadcast
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + destAddress : NodeAddress : IP packet's destination IP address.
// + outgoingInterface : int* : Outgoing interface index.
// + outgoingBroadcastAddress : NodeAddress* : Broadcast address
//                                             of Outgoing interface
// RETURNS    :: BOOL : Returns true if destination is broadcast address
// **/

BOOL IsOutgoingBroadcast(Node *node,
                         NodeAddress destAddress,
                         int *outgoingInterface,
                         NodeAddress *outgoingBroadcastAddress);

//----------------------------------------------------------
// Miscellaneous
//----------------------------------------------------------

// /**
// API        :: NetworkIpIsMyIP
// LAYER      :: Network
// PURPOSE    :: In turn calls IsMyPacket()
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + ipAddress : NodeAddress : An IP packet's destination IP address.
// RETURN     :: BOOL : Returns if it belongs to it.
// **/
BOOL
NetworkIpIsMyIP(Node *node, NodeAddress ipAddress);

//----------------------------------------------------------
// Debugging
//----------------------------------------------------------

// /**
// API        :: NetworkIpConfigurationError
// LAYER      :: Network
// PURPOSE    :: Prints out the IP configuration error
// PARAMETERS ::
// + node           : Node* : Pointer to the node
// + parameterName  : char [] : Error message to print
// + interfaceIndex : int : interface number
// RETURN     :: void :
// **/
void
NetworkIpConfigurationError(
    Node *node,
    const char parameterName[],
    int interfaceIndex);

// /**
// API        :: NetworkPrintIpHeader
// LAYER      :: Network
// PURPOSE    :: To print the IP header
// PARAMETERS ::
// + msg       : Message* : Pointer to Message
// RETURN     :: void :
// **/
void
NetworkPrintIpHeader(Message *msg);

//----------------------------------------------------------
// Statistics
//----------------------------------------------------------

// */*
// API        :: NetworkIpRunTimeStat
// LAYER      :: Network
// PURPOSE    :: Print IP runtime statistics
// PARAMETERS ::
// + node      : Node* : pointer to node
// RETURN     :: void :
// **/
void
NetworkIpRunTimeStat(Node *node);

//----------------------------------------------------------
// Multicast
//----------------------------------------------------------

// /**
// API        :: NetworkIpAddToMulticastGroupList
// LAYER      :: Network
// PURPOSE    :: Add a specified node to a multicast group
// PARAMETERS ::
// + node         : Node* : Pointer to the node
// + groupAddress : NodeAddress : address of multicast group
// RETURN     :: void :
// **/
void
NetworkIpAddToMulticastGroupList(
    Node *node,
    NodeAddress groupAddress);

// /**
// API        :: NetworkIpRemoveFromMulticastGroupList
// LAYER      :: Network
// PURPOSE    :: To remove specified node from a multicast group
// PARAMETERS ::
// + node         : Node* : Pointer to the node
// + groupAddress : NodeAddress : address of multicast group
// RETURN     ::    void :
// **/
void
NetworkIpRemoveFromMulticastGroupList(
    Node *node,
    NodeAddress groupAddress);


// /**
// API        :: NetworkIpPrintMulticastGroupList
// LAYER      :: Network
// PURPOSE    :: To print the multicast grouplist
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// RETURN     :: void :
// **/
void
NetworkIpPrintMulticastGroupList(Node *node);

// /**
// API        :: NetworkIpIsPartOfMulticastGroup
// LAYER      :: Network
// PURPOSE    :: check if a node is part of specified multicast group
// PARAMETERS ::
// + node         : Node* : Pointer to the node
// + groupAddress : NodeAddress : group to check if node is part of
//                                multicast group
// RETURN     :: BOOL :
// **/
BOOL
NetworkIpIsPartOfMulticastGroup(Node *node, NodeAddress groupAddress);

// /**
// API        :: NetworkIpJoinMulticastGroup
// LAYER      :: Network
// PURPOSE    :: To join a multicast group
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + mcastAddr : NodeAddress : multicast group address
// + delay     : clocktype : delay after which to join
// RETURN     :: void :
// **/
void
NetworkIpJoinMulticastGroup(
    Node *node,
    NodeAddress mcastAddr,
    clocktype delay);

// /**
// API        :: NetworkIpLeaveMulticastGroup
// LAYER      :: Network
// PURPOSE    :: To leave a multicast group
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + mcastAddr : NodeAddress : multicast group address
// + delay     : clocktype : delay after which to leave
// RETURN     :: void :
// **/
void
NetworkIpLeaveMulticastGroup(
    Node *node,
    NodeAddress mcastAddr,
    clocktype delay);

// /**
// API        :: NetworkIpSetMulticastTimer
// LAYER      :: Network
// PURPOSE    :: To set a multicast timer to join or leave multicast groups
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + eventType : long : the event type
// + mcastAddr : NodeAddress : multicast group address
// + delay     : clocktype : delay after which to leave
// RETURN     :: void :
// **/
void
NetworkIpSetMulticastTimer(
    Node *node,
    Int32 eventType,
    NodeAddress mcastAddr,
    clocktype delay);

// /**
// API        :: NetworkIpSetMulticastRoutingProtocol
// LAYER      :: Network
// PURPOSE    :: Assign a multicast routing protocol to an interface
// PARAMETERS ::
// + node                     : Node*: Pointer to the node
// + multicastRoutingProtocol : void* : multicast routing protocol
// + interfaceIndex           : int : interface number
// RETURN     :: void :
// **/
void
NetworkIpSetMulticastRoutingProtocol(
    Node *node,
    void *multicastRoutingProtocol,
    int interfaceIndex);

// /**
// API        :: NetworkIpGetMulticastRoutingProtocol
// LAYER      :: Network
// PURPOSE    :: To get the Multicast Routing Protocol structure
// PARAMETERS ::
// + node                : Node* : Pointer to the node
// + routingProtocolType : NetworkRoutingProtocolType: routing protocol name
// RETURN     :: void * :
// **/
void *
NetworkIpGetMulticastRoutingProtocol(
    Node *node,
    NetworkRoutingProtocolType routingProtocolType);


// /**
// API        :: NetworkIpAddMulticastRoutingProtocolType
// LAYER      :: Network
// PURPOSE    :: Assign a multicast protocol type to an interface
// PARAMETERS ::
// + node                  : Node* : Pointer to this node
// + multicastProtocolType : NetworkRoutingProtocolType : routing protocol
// + interfaceIndex        : int : interface number of the node
// RETURN     :: void :
// **/
void
NetworkIpAddMulticastRoutingProtocolType(
    Node *node,
    NetworkRoutingProtocolType multicastProtocolType,
    int interfaceIndex);


// /**
// API        :: NetworkIpSetMulticastRouterFunction
// LAYER      :: Network
// PURPOSE    :: Set a multicast router function to an interface
// PARAMETERS ::
// + node              : Node* : Pointer to this node
// + routerFunctionPtr : MulticastRouterFunctionType : router Func pointer
// + interfaceIndex    : int : interface number of the node
// RETURN     :: void :
// **/
void
NetworkIpSetMulticastRouterFunction(
    Node *node,
    MulticastRouterFunctionType routerFunctionPtr,
    int interfaceIndex);


// /**
// API        :: NetworkIpGetMulticastRouterFunction
// LAYER      :: Network
// PURPOSE    :: Get the multicast router function for an interface
// PARAMETERS ::
// + node           : Node* : Pointer to this node
// + interfaceIndex : int : interface number of the node
// RETURN     :: MulticastRouterFunctionType : Multicast router function
//                                             on this interface.
// **/
MulticastRouterFunctionType
NetworkIpGetMulticastRouterFunction(
    Node *node,
    int interfaceIndex);

// /**
// API        :: NetworkIpUpdateMulticastRoutingProtocolAndRouterFunction
// LAYER      :: Network
// PURPOSE    :: Assign multicast routing protocol structure and router
//               function to an interface.  We are only allocating
//               the multicast routing protocol structure and router function
//               once by using pointers to the original structures.
// PARAMETERS ::
// + node                : Node* : this node
// + routingProtocolType : NetworkRoutingProtocolType : multicast routing
//                                                      protocol.
// + interfaceIndex      : int : interface index.
// RETURN     :: void :
// **/
void
NetworkIpUpdateMulticastRoutingProtocolAndRouterFunction(
    Node *node,
    NetworkRoutingProtocolType routingProtocolType,
    int interfaceIndex);


// /**
// API        :: NetworkIpUpdateUnicastRoutingProtocolAndRouterFunction
// LAYER      :: Network
// PURPOSE    :: Assign unicast routing protocol structure and router
//               function to an interface.  We are only allocating
//               the unicast routing protocol structure and router function
//               once by using pointers to the original structures.
// PARAMETERS ::
// + node                : Node* : this node
// + routingProtocolType : NetworkRoutingProtocolType : unicast routing
//                                                      protocol to add.
// + interfaceIndex      : int : interface associated with unicast protocol.
// RETURN     :: void :
// **/
void
NetworkIpUpdateUnicastRoutingProtocolAndRouterFunction(
    Node *node,
    NetworkRoutingProtocolType routingProtocolType,
    int interfaceIndex,
    NetworkType networkType = NETWORK_IPV4);

// /**
// API        :: NetworkIpGetInterfaceIndexFromAddress
// LAYER      :: Network
// PURPOSE    :: Get the interface index from an IP address.
// PARAMETERS ::
// + node      : Node*       : this node
// + address   : NodeAddress : address to determine interface index for
// RETURN     :: int         : interface index associated with specified
//                             address.
// **/
int
NetworkIpGetInterfaceIndexFromAddress(
    Node *node,
    NodeAddress address);

// /**
// API        :: NetworkIpGetInterfaceIndexFromSubnetAddress
// LAYER      :: Network
// PURPOSE    :: Get the interface index from an IP subnet address.
// PARAMETERS ::
// + node      : Node*       : this node
// + address   : NodeAddress : subnet address to determine interface
//                             index for.
// RETURN     :: int         : interface index associated with specified
//                             subnet address.
// **/
int
NetworkIpGetInterfaceIndexFromSubnetAddress(
    Node *node,
    NodeAddress address);



// /**
// API        :: NetworkIpIsMulticastAddress
// LAYER      :: Network
// PURPOSE    :: Check if an address is a multicast address.
// PARAMETERS ::
// + node      : Node*        : this node
// + address   : NodeAddress  : address to determine if multicast address.
// RETURN     :: BOOL         : TRUE if address is multicast address,
//                              FALSE, otherwise.
// **/
BOOL
NetworkIpIsMulticastAddress(
    Node *node,
    NodeAddress address);

// /**
// API        :: NetworkInitMulticastForwardingTable
// LAYER      :: Network
// PURPOSE    :: initialize the multicast fowarding table, allocate enough
//               memory for number of rows, used by ip
// PARAMETERS ::
// + node      : Node* : this node
// RETURN     :: void :
// **/
void
NetworkInitMulticastForwardingTable(Node *node);

// /**
// API        :: NetworkEmptyMulticastForwardingTable
// LAYER      :: Network
// PURPOSE    :: empty out all the entries in the multicast forwarding table.
//               basically set the size of table back to 0.
// PARAMETERS ::
// + node      : Node* : this node
// RETURN     :: void :
// **/
void
NetworkEmptyMulticastForwardingTable(Node *node);


// /**
// API        :: NetworkGetOutgoingInterfaceFromMulticastForwardingTable
// LAYER      :: Network
// PURPOSE    :: get the interface Id node that lead to the
//              (source, multicast group) pair.
// PARAMETERS ::
// + node          : Node*       : its own node
// + sourceAddress : NodeAddress : multicast source address
//                                 to foward to..
// + groupAddress  : NodeAddress : multicast group
// RETURN     :: LinkedList*     : interface Id from node to (source, multicast
//                           group), or NETWORK_UNREACHABLE (no such entry
//                           is found)
// **/
LinkedList *
NetworkGetOutgoingInterfaceFromMulticastForwardingTable(
    Node *node,
    NodeAddress sourceAddress,
    NodeAddress groupAddress);

// /**
// API        :: NetworkUpdateMulticastForwardingTable
// LAYER      :: Network
// PURPOSE    :: update entry with(sourceAddress,multicastGroupAddress) pair.
//               search for the row with(sourceAddress,multicastGroupAddress)
//               and update its interface.
// PARAMETERS ::
// + node                  : Node*       : its own node
// + sourceAddress         : NodeAddress : multicast source
// + multicastGroupAddress : NodeAddress : multicast group
// + interfaceIndex        : int     : interface to use for
//                                    (sourceAddress, multicastGroupAddress)
// RETURN     :: void :
// **/
void
NetworkUpdateMulticastForwardingTable(
    Node *node,
    NodeAddress sourceAddress,
    NodeAddress multicastGroupAddress,
    int interfaceIndex);

// /**
// API        :: NetworkPrintMulticastForwardingTable
// LAYER      :: Network
// PURPOSE    :: display all entries in multicast forwarding table of the
//               node.
// PARAMETERS ::
// + node      : Node* : this node
// RETURN     :: void :
// **/
void
NetworkPrintMulticastForwardingTable(Node *node);

// /**
// API        :: NetworkPrintMulticastOutgoingInterface
// LAYER      :: Network
// PURPOSE    :: Print mulitcast outgoing interfaces.
// PARAMETERS ::
// + node      : Node* : this node
// + list      : list* : list of outgoing interfaces.
// RETURN     :: void :
// **/
void
NetworkPrintMulticastOutgoingInterface(Node *node, LinkedList *list);


// /**
// API        :: NetworkInMulticastOutgoingInterface
// LAYER      :: Network
// PURPOSE    :: Determine if interface is in multicast outgoing interface
//               list.
// PARAMETERS ::
// + node           : Node* : this node
// + list           : List* : list of outgoing interfaces.
// + interfaceIndex : int   : interface to determine if in outgoing
//                            interface list.
// RETURN     :: BOOL : TRUE if interface is in multicast outgoing interface
//                      list, FALSE otherwise.
// **/
BOOL
NetworkInMulticastOutgoingInterface(
    Node *node,
    LinkedList *list,
    int interfaceIndex);

// /**
// API        :: NetworkIpPrintTraceXML
// LAYER      :: Network
// PURPOSE    :: Print packet trace information in XML format.
// PARAMETERS ::
// + node      : Node*          : this node
// + msg       : Message* : Packet to print headers from.
// RETURN     :: void :
// **/
void NetworkIpPrintTraceXML(Node *node, Message *msg);



void //inline//
NetworkIpSendOnBackplane(Node *node,
                         Message *msg,
                         int incomingInterface,
                         int outgoingInterface,
                         NodeAddress hopAddr);

void
NetworkIpUseBackplaneIfPossible(Node *node,
                                int incomingInterface);

void
NetworkIpReceiveFromBackplane(Node *node, Message *msg);

// FUNCTION     RoutePacketAndSendToMac()
// PURPOSE      "Route" a packet by determining the next hop IP address
//              and the outgoing interface.  It is assumed that the IP
//              packet already has an IP header at this point.  Figure
//              out the next hop in the route and send the packet.
//              First the "routing function" is checked and if that fails
//              the default source route or lookup table route is used.
//              [needs updating]
// PARAMETERS   Node *node
//                  Pointer to node.
//              Message *msg
//                  Pointer to message with IP packet.
//              int incomingInterface
//                  Index of interface on which packet arrived.
//                  [This is different if the packet originated from
//                  the network or transport layers.  This value is only
//                  for multicast packets, currently.]
//              int outgoingInterface
//                  Used only when the application specifies a specific
//                  interface to use to transmit packet.
//-----------------------------------------------------------------------------

void //inline//
RoutePacketAndSendToMac(Node *node,
                        Message *msg,
                        int incomingInterface,
                        int outgoingInterface,
                        NodeAddress previousHopAddress);

// /**
// API        :: RouteThePacketUsingLookupTable
// LAYER      :: Network
// PURPOSE    :: Tries to route and send the packet using the node's
//               forwarding table.
// PARAMETERS ::
// + node      : Node*    : this node
// + msg       : Message* : Pointer to message with IP packet.
// + incomingInterface : int : incoming interface of packet
// RETURN     :: void  : NULL
// **/
void RouteThePacketUsingLookupTable(Node *node,
                                    Message *msg,
                                    int incomingInterface);

#ifdef ADDON_BOEINGFCS
// /**
// API        :: GetNetworkIPFragUnit
// LAYER      :: Network
// PURPOSE    :: Returns the network ip fragmentation unit.
// PARAMETERS ::
// + node      : Node*          : this node
// + interfaceIndex : int       : interface of node
// RETURN     :: int :
// **/

inline int GetNetworkIPFragUnit(Node* node, int interfaceIndex)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    return (ip->interfaceInfo[interfaceIndex]->ipFragUnit);
}

inline int GetNetworkIPFragUnit(Node* node)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    return (ip->interfaceInfo[0]->ipFragUnit);
}
#else

// /**
// API        :: GetNetworkIPFragUnit
// LAYER      :: Network
// PURPOSE    :: Returns the network ip fragmentation unit.
// PARAMETERS ::
// + node      : Node*          : this node
// RETURN     :: int :
// **/
inline int GetNetworkIPFragUnit(Node* node)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    if (LlcIsEnabled(node,(int)DEFAULT_INTERFACE))
    {
        return (ip->ipFragUnit - sizeof(LlcHeader));
    }
    else
    {
        return (ip->ipFragUnit);
    }
}

#endif


#if 0

//----------------------------------------------------------
// Fragmentation (disabled)
//----------------------------------------------------------


// /**
// MACRO       :: FragmentOffset(ipHeader)
// DESCRIPTION :: Starting position of this fragment in actual packet.
// **/
#define FragmentOffset(ipHeader) (((ipHeader)->ip_fragment_offset) * 8)


// /**
// MACRO       :: SetFragmentOffset(ipHeader, offset)
// DESCRIPTION :: To set offset of fragment.
// **/
#define SetFragmentOffset(ipHeader, offset) \
    ERROR_Assert(((offset) % 8 == 0) && ((offset) < IP_MAXPACKET)); \
                 (ipHeader)->ip_fragment_offset = ((offset)/8, \
                 "Invalid fragment offset");


// /**
// CONSTANT    :: NETWORK_IP_REASS_BUFF_TIMER : (15 * SECOND)
// DESCRIPTION :: Max time data can stored in assembly buffer
// **/
#define NETWORK_IP_REASS_BUFF_TIMER (15 * SECOND)


// /**
// CONSTANT    :: MAX_IP_FRAGMENTS_SIMPLE_CASE : 64
// DESCRIPTION :: Max size of fragment allowed.
// **/
#define MAX_IP_FRAGMENTS_SIMPLE_CASE    64


// /**
// CONSTANT    :: SMALL_REASSEMBLY_BUFFER_SIZE : 2048
// DESCRIPTION :: Size of reassemble buffer
// **/
#define SMALL_REASSEMBLY_BUFFER_SIZE    2048

// /**
// CONSTANT    :: REASSEMBLY_BUFFER_EXPANSION_MULTIPLIER : 8
// DESCRIPTION :: Multiplier used for reassemble buffer expansion
// **/
#define REASSEMBLY_BUFFER_EXPANSION_MULTIPLIER  8


// /**
// STRUCT      :: IpReassemblyBufferType
// DESCRIPTION :: Structure of reassembly buffer
// **/
typedef
struct
{
    Message *packetUnderConstruction;
    int sizeLimit;
    clocktype expirationDate;
    unsigned short totalPacketLength;
    unsigned short fragmentationSize;
    unsigned char fragmentIsHereBitTable[MAX_IP_FRAGMENTS_SIMPLE_CASE / 8];
    BOOL endFragmentHasArrived;
    unsigned short endFragmentOffset;
}IpReassemblyBufferType;


// /**
// STRUCT      :: IpReassemblyBufferListCellType
// DESCRIPTION :: Structure of reassembly buffer cell listing
// **/
typedef
struct IpReassemblyBufferListCellStruct
{
    struct IpReassemblyBufferListCellStruct *nextPtr;
    IpReassemblyBufferType reassemblyBuffer;
}
IpReassemblyBufferListCellType;


// /**
// STRUCT      :: IpReassemblyBufferListType
// DESCRIPTION :: Structure of reassembly buffer list
// **/
typedef
struct
{
    IpReassemblyBufferListCellType *firstPtr;
    IpReassemblyBufferListCellType *freeListPtr;
}
IpReassemblyBufferListType;

//----------------------------------------------------------
// User functions (disabled)
//----------------------------------------------------------

// /**
// API        :: NetworkIpUserProtocolInit
// LAYER      :: Network
// PURPOSE    :: Initialization of user protocol(disabled)
// PARAMETERS ::
// + node                  : Node* : this node
// + nodeInput             : const NodeInput* : Provides access to
//                                              configuration file.
// + routingProtocolString : const char* : routing protocol
// + routingProtocolType   : NetworkRoutingProtocolType* : routing protocol
//                                                        type
// + routingProtocolData   : void** : Access to routing protocol data
// RETURN     :: void :
// **/
void
NetworkIpUserProtocolInit(
    Node *node,
    const NodeInput *nodeInput,
    const char *routingProtocolString,
    NetworkRoutingProtocolType *routingProtocolType,
    void **routingProtocolData);

// /**
// API        :: NetworkIpUserHandleProtocolEvent
// LAYER      :: Network
// PURPOSE    :: Event handler function of user protocol(disabled)
// PARAMETERS ::
// + node      : Node*    : The node that is handling the event.
// + msg       : Message* : the event that is being handled
// RETURN     :: void :
// **/
void
NetworkIpUserHandleProtocolEvent(
    Node *node,
    Message *msg);

// /**
// API        :: NetworkIpUserHandleProtocolPacket
// LAYER      :: Network
// PURPOSE    :: Process a user protocol generated control packet(disabled)
// PARAMETERS ::
// + node               : Node* :this node
// + msg                : Message* :  message that is being received.
// + ipProtocol         : unsigned char : ip protocol
// + sourceAddress      : NodeAddress : source address
// + destinationAddress : NodeAddress : destination address
// + ttl                : int : time to live
// RETURN     :: void :
// **/
void
NetworkIpUserHandleProtocolPacket(
    Node *node,
    Message *msg,
    unsigned char ipProtocol,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int ttl);

// /**
// API                 :: NetworkIpUserProtocolFinalize
// LAYER               :: Network
// PURPOSE             :: Finalization of user protocol(disabled)
// PARAMETERS          ::
// + node               : Node* : this node
// + userProtocolNumber : int   : protocol number
// RETURN              :: void :
// **/
void
NetworkIpUserProtocolFinalize(
    Node *node,
    int userProtocolNumber);


#endif // 0

// /**
// API                 :: Atm_RouteThePacketUsingLookupTable
// LAYER               :: Network
// PURPOSE             :: Routing packet received at ATM node
// PARAMETERS          ::
// + node               : Node* : this node
// + destAddr           : NodeAddress* : destination Address
// + outIntf            : int* : this node
// + nextHop            : NodeAddress* : nextHop address
// RETURN              :: void :
// **/
void
Atm_RouteThePacketUsingLookupTable(Node* node,
                                   NodeAddress destAddr,
                                   int* outIntf,
                                   NodeAddress* nextHop);

#ifdef ADDON_LINK16

void
NetworkQueueInsert(
    Node *node,
    Scheduler *scheduler,
    Message *msg,
    NodeAddress nextHopAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    int networkType,
    BOOL *queueIsFull);


void
NetworkOutputQueueInsert(
    Node *node,
    int outgoingInterface,
    Message *msg,
    NodeAddress nextHopAddress,
    NodeAddress destinationAddress,
    int networkType,
    BOOL *queueIsFull);

#endif // ADDON_LINK16

// Fragmentation code.

int
IpFragmentPacket(
    Node* node,
    Message* msg,
    int mtu,
    ipFragmetedMsg** fragmentHead,
    BOOL fragmentForMpls);

Message*
IpFragmentInput(
    Node* node,
    Message* msg,
    int interfaceId,
    BOOL* isReassembled);

Message*
IpFragementReassamble(
    Node* node,
    Message* msg,
    IpFragQueue* fp,
    int interfaceId);
BOOL NetworkIpOutputQueuePeekWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nextHopMacAddr,
    QueuePriorityType *priority);

BOOL NetworkIpOutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nextHopMacAddr,
    int *networkType);

// /**
// API                 :: RouteThePacketUsingMulticastForwardingTable
// LAYER               :: Network
// PURPOSE             :: Tries to route the multicast packet using the
//                        multicast forwarding table.
// PARAMETERS          ::
// + node               : Node*     : this node
// + msg                : Message*  : Pointer to Message
// + incomingInterface  : int       : Incomming Interface
// RETURN              :: void      : NULL.
// **/
void
RouteThePacketUsingMulticastForwardingTable(
    Node* node,
    Message* msg,
    int incomingInterface,
    NetworkType netType);

//-----------------------------------------------------------------------------
// FUNCTION     NetworkIpOutputQueueDequeuePacket()
// PURPOSE      Calls the packet scheduler for an interface to retrieve
//              an IP packet from a queue associated with the interface.
//              The dequeued packet, since it's already been routed,
//              has an associated next-hop IP address.  The packet's
//              priority value is also returned.
//              Addded function for IP+MPLS
// PARAMETERS   Node *node
//                  Pointer to node.
//              int interfaceIndex
//                  Index of interface.
//              Message **msg
//                  Storage for pointer to message with IP packet.
//              NodeAddress *nextHopAddress
//                  Storage for packet's next hop address.
//              QueuePriorityType *userPriority
//                  Storage for user priority of packet.
//              posInQueue
//                  Position of packet in Queue.
//                  Added as part of IP-MPLS integration
// RETURN       TRUE if dequeued successfully, FALSE otherwise.
//
// NOTES        This function is called by
//              MAC_OutputQueueDequeuePacket() (mac/mac.pc), which itself
//              is called from mac/mac_802_11.pc and other MAC protocol
//              source files.
//
//              This function will assert false if the scheduler cannot
//              return an IP packet for whatever reason.
//-----------------------------------------------------------------------------
BOOL NetworkIpOutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int *networkType,
    QueuePriorityType *userPriority,
    int posInQueue);


//-----------------------------------------------------------------------------
// FUNCTION     NetworkIpQueueTopPacket()
// PURPOSE      Same as NetworkIpQueueDequeuePacket(), except the
//              packet is not actually dequeued.  Note that the message
//              containing the packet is not copied; the contents may
//              (inadvertently or not) be directly modified.
// PARAMETERS   Node *node
//                  Pointer to node.
//              SchedulerType *scheduler
//                  queue to get top packet from.
//              Message **msg
//                  Storage for pointer to message with IP packet.
//              NodeAddress *nextHopAddress
//                  Storage for packet's next hop address.
//              int *outgoingInterface
//                  Used to determine where packet should go after passing
//                  through the backplane.
//              int *networkType
//                  Whether packet is associated with an IP network, Link-16
//                  nework, etc...
//              QueuePriorityType *priority
//                  Storage for priority of packet.
//              posInQueue
//                  Position of packet in Queue.
//                  Added as part of IP-MPLS integration
// RETURN       TRUE if there is a packet, FALSE otherwise.
//-----------------------------------------------------------------------------

BOOL NetworkIpQueueTopPacket(
    Node *node,
    Scheduler *scheduler,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress *nexthopmacAddr,
    int *outgoingInterface,
    int *networkType,
    QueuePriorityType *priority,
    int posInQueue = DEQUEUE_NEXT_PACKET);

//-----------------------------------------------------------------------------
// FUNCTION     NetworkIpOutputQueueTopPacket()
// PURPOSE      Same as NetworkIpOutputQueueDequeuePacket(), except the
//              packet is not actually dequeued.  Note that the message
//              containing the packet is not copied; the contents may
//              (inadvertently or not) be directly modified.
// PARAMETERS   Node *node
//                  Pointer to node.
//              int interfaceIndex
//                  Index of interface.
//              Message **msg
//                  Storage for pointer to message with IP packet.
//              NodeAddress *nextHopAddress
//                  Storage for packet's next hop address.
//              int *networkType
//                  Whether packet is associated with an IP network, Link-16
//                  nework, etc...
//              QueuePriorityType *priority
//                  Storage for priority of packet.
//              posInQueue
//                  Position of packet in Queue.
//                  Added as part of IP-MPLS integration
// RETURN       TRUE if there is a packet, FALSE otherwise.
//
// NOTES        This function is called by MAC_OutputQueueTopPacket()
//              (mac/mac.pc), which itself is called from
//              mac/mac_802_11.pc and other MAC protocol source files.
//
//              This function will assert false if the scheduler cannot
//              return an IP packet for whatever reason.
//-----------------------------------------------------------------------------

BOOL NetworkIpOutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    MacHWAddress* nexthopmacAddr,
    int* networkType,
    QueuePriorityType *priority,
    int posInQueue);

//-----------------------------------------------------------------------------
// FUNCTION     NetworkIpQueueInsert()
// PURPOSE      Calls the packet scheduler for an interface to retrieve
//              an IP packet from a queue associated with the interface.
//              The dequeued packet, since it's already been routed,
//              has an associated next-hop IP address.  The packet's
//              priority value is also returned.
// PARAMETERS   Node *node
//                  Pointer to node.
//              int incomingInterface
//                  interface of input queue.
//              Message *msg
//                  Pointer to message with IP packet.
//              NodeAddress nextHopAddress
//                  Packet's next hop address.
//              NodeAddress destinationAddress
//                  Packet's destination address.
//              int outgoingInterface
//                  Used to determine where packet should go after passing
//                  through the backplane.
//              int networkType
//                  Type of network packet is using (IP, Link-16, ...)
//              BOOL *queueIsFull
//                  Storage for boolean indicator.
//                  If TRUE, packet was not queued because scheduler
//                  reported queue was (or queues were) full.
//              int incomingInterface
//                  Incoming interface, Default argument used by backplane.
//              BOOL isOutputQueue
//                      Output Queue
// RETURN       None.
//--------------------------------------------------------------------------
void
NetworkIpQueueInsert(
    Node *node,
    Scheduler *scheduler,
    Message *msg,
    NodeAddress nextHopAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    int networkType,
    BOOL *queueIsFull,
    int incomingInterface = ANY_INTERFACE,
    BOOL isOutputQueue = FALSE);

#ifdef ADDON_BOEINGFCS
int NetworkIpGetOutgoingInterfaceFromAddr(Node* node,
        int interfaceIndex,
        NodeAddress addr);
#endif

#ifdef ADDON_NGCNMS
void NetworkIpReset(Node* node, int interfaceIndex);
#endif

// /**
// FUNCTION     :: NETWORKIpRoutingInit
// PURPOSE      :: Initialization function for network layer.
//                 Initializes IP.
// PARAMETERS   ::
// + node : Node * : Pointer to node.
// + nodeInput : const NodeInput *nodeInput : Pointer to node input.
// RETURN       :: int :
// **/
void
IpRoutingInit(Node *node,
              const NodeInput *nodeInput);

// /**
// API                 :: NetworkIpGetBandwidth
// LAYER               :: Network
// PURPOSE             :: getting the bandwidth information
// PARAMETERS          ::
// + node               : Node*   : the node who's bandwidth is needed.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: Int64: inverted bandwidth
// ASSUMPTION   :     Bandwidth read from interface is in from of bps unit.
//                    To invert the bandwidth we use the equation
//                    10000000 / bandwidth. Where bandwidth is in Kbps unit.
// **/
Int64 NetworkIpGetBandwidth(Node* node, int interfaceIndex);

// /**
// API                 :: NetworkIpGetPropDelay
// LAYER               :: Network
// PURPOSE             :: getting the propagation delay information
// PARAMETERS          ::
// + node               : Node*   : the node who's bandwidth is needed.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: clocktype: propagation delay
// ASSUMPTION           : Array is exactly 3-byte long.
// **/
clocktype NetworkIpGetPropDelay(Node* node, int interfaceIndex);

// /**
// API                 :: NetworkIpInterfaceIsEnabled
// LAYER               :: Network
// PURPOSE             :: To check the interface is enabled or not?
// PARAMETERS          ::
// + node               : Node*   : node structure pointer.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: BOOL :
// **/
BOOL NetworkIpInterfaceIsEnabled(Node* node, int interfaceIndex);

// /**
// API                 :: NetworkIpIsWiredNetwork
// LAYER               :: Network
// PURPOSE             :: Determines if an interface is a wired interface.
// PARAMETERS          ::
// + node               : Node*   : node structure pointer.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: BOOL
// **/
BOOL
NetworkIpIsWiredNetwork(Node *node, int interfaceIndex);

// /**
// API                 :: NetworkIpIsPointToPointNetwork
// LAYER               :: Network
// PURPOSE             :: Determines if an interface is a point-to-point.
// PARAMETERS          ::
// + node               : Node*   : node structure pointer.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: BOOL
// **/
BOOL
NetworkIpIsPointToPointNetwork(Node *node, int interfaceIndex);

// /**
// API                 :: IsIPV4MulticastEnabledOnInterface
// LAYER               :: Network
// PURPOSE             :: To check if IPV4 Multicast is enabled on interface?
// PARAMETERS          ::
// + node               : Node*   : node structure pointer.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: BOOL :
// **/
BOOL IsIPV4MulticastEnabledOnInterface(Node* node,
                                       int interfaceIndex);

// /**
// API                 :: IsIPV4RoutingEnabledOnInterface
// LAYER               :: Network
// PURPOSE             :: To check if IPV4 Routing is enabled on interface?
// PARAMETERS          ::
// + node               : Node*   : node structure pointer.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: BOOL :
// **/
BOOL IsIPV4RoutingEnabledOnInterface(Node* node,
                                     int interfaceIndex);

// /**
// FUNCTION             :: NetworkIpGetNetworkProtocolType
// PURPOSE              :: Get Network Protocol Type for the node
// PARAMETERS           ::
// + node               : Node* : node structure pointer.
// + nodeId             : NodeAddress : node id.
// RETURN   :: NetworkProtocolType :
// **/
NetworkProtocolType NetworkIpGetNetworkProtocolType(
    Node* node,
    NodeAddress nodeId);

// /**
// API          :: ResolveNetworkTypeFromSrcAndDestNodeId
// PURPOSE      :: Resolve the NetworkType from source and destination node id's.
// PARAMETERS   ::
// + node           : Node* : Pointer to the node.
// + sourceNodeId   : NodeId : Source node id.
// + destNodeId     : NodeId : Destination node id.
// RETURN   :: NetworkType :
// **/
NetworkType
ResolveNetworkTypeFromSrcAndDestNodeId(
    Node* node,
    NodeId sourceNodeId,
    NodeId destNodeId);

// FUNCTION            :: NetworkIpIsUnnumberedInterface
// LAYER               :: Network
// PURPOSE             :: checking for unnumbered Interface.
// PARAMETERS          ::
// + node              :: Node*   : Pointer to node structure.
// + intIndex        ::   int     :.Interface Index
// RETURN              :: BOOL
// **/
BOOL NetworkIpIsUnnumberedInterface(Node* node, int intIndex);

// /**
// API                 :: NetworkIpIsWiredBroadcastNetwork
// LAYER               :: Network
// PURPOSE             :: Determines if an interface is a wired interface.
// PARAMETERS          ::
// + node               : Node*   : node structure pointer.
// + interfaceIndex     : int     : interface Index.
// RETURN              :: BOOL
// **/
BOOL
NetworkIpIsWiredBroadcastNetwork(Node *node, int interfaceIndex);


// -----------------------------------------------------------------------------
// API :: GetDefaultInterfaceIndex
// PURPOSE :: Returns Default Interface index of depending on network type
// PARAMETERS ::
// + node : Node* : Pointer to the Node
// + sourceNodeId : NodeId
// + networkType : NetoworkType
// RETURN :: int
// -----------------------------------------------------------------------------
int
GetDefaultInterfaceIndex(
    Node* node,
    NetworkType netType);

// STATS DB CODE
#ifdef ADDON_DB
void NetworkIpConvertProtocolTypeToString(NetworkRoutingProtocolType type,
        std::string* protocol);

void NetworkIpConvertAdminDistanceToString(NetworkRoutingAdminDistanceType type,
        std::string adminDistString);
void NetworkIpConvertIpProtocolNumToString(unsigned char type,
        std::string* protocol);
void NetworkIpConvertMacProtocolTypeToString(
    MAC_PROTOCOL type,
    std::string *macProtocolStr);
BOOL IsDataPacket(IpHeaderType* IpHeader);
#endif

// -------------------------------------------------------------------------
// API :: GetDefaultInterfaceAddress
// PURPOSE :: Returns Default ipv4 Interface address
// PARAMETERS ::
// + node : Node* : Pointer to the Node
// RETURN :: NodeAddress
// -------------------------------------------------------------------------
NodeAddress GetDefaultIPv4InterfaceAddress(Node* node);
#endif // _IP_H_
