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
// PACKAGE     :: MAC LAYER
// DESCRIPTION :: This file describes data structures and functions used by the MAC Layer.
// **/

#ifndef MAC_H
#define MAC_H


#include "network.h"
#include "phy.h"
#include "list.h"
#include "random.h"

#define DEBUG_MACErr

// /**
// CONSTANT     :: MAC_PROPAGATION_DELAY : 1 * MICRO_SECOND
// DESCRIPTION  :: Peer to Peer Propogation delay in the MAC
// **/
#define MAC_PROPAGATION_DELAY         (1 * MICRO_SECOND)

// /**
// CONSTANT     :: MAC_ADDRESS_LENGTH_IN_BYTE : 6
// DESCRIPTION  :: MAC address length
// **/
#define MAC_ADDRESS_LENGTH_IN_BYTE  6

// /**
// CONSTANT     :: Max_MacAdress_Length : 16
// DESCRIPTION  :: Maximum MAC address length
// **/
#define MAX_MACADDRESS_LENGTH 16

// /**
// CONSTANT     :: MAC_ADDRESS_DEFAULT_LENGTH : 6
// DESCRIPTION  :: MAC address length in byte or octets
// **/
#define  MAC_ADDRESS_DEFAULT_LENGTH 6

// /**
// CONSTANT     :: MAC_CONFIGURATION_ATTRIBUTE : 5
// DESCRIPTION  :: Number of
//                 attribute of mac address file
// **/
#define  MAC_CONFIGURATION_ATTRIBUTE   5

// /**
// CONSTANT    :: HW_TYPE_NETROM : 0
// DESCRIPTION :: From KA9Q NET/ROM pseudo
//                Hardware type.
// **/
#define HW_TYPE_NETROM              0

// /**
// CONSTANT    :: HW_TYPE_ETHER : 1
// DESCRIPTION :: Ethernet 10/100Mbps
//                Hardware type Ethernet.
// **/
#define HW_TYPE_ETHER               1

// /**
// CONSTANT    :: HW_TYPE_EETHER : 2
// DESCRIPTION :: Hardware type Experimental Ethernet
// **/
#define ARP_HRD_EETHER   2

// /**
// CONSTANT    :: HW_TYPE_AX25 : 3
// DESCRIPTION :: Hardware type AX.25 Level 2
// **/
#define HW_TYPE_AX25     3

// /**
// CONSTANT    :: HW_TYPE_PRONET : 4
// DESCRIPTION :: Hardware type PROnet token ring
// **/
#define HW_TYPE_PRONET   4

// /**
// CONSTANT    :: HW_TYPE_CHAOS : 5
// DESCRIPTION :: Hardware type Chaosnet
// **/
#define HW_TYPE_CHAOS    5

// /**
// CONSTANT    :: HW_TYPE_IEEE802 : 6
// DESCRIPTION :: IEEE 802.2 Ethernet/TR/TB
// **/
#define HW_TYPE_IEEE802  6

// /**
// CONSTANT    :: HW_TYPE_ARCNET : 7
// DESCRIPTION :: Hardware type ARCnet
// **/
#define HW_TYPE_ARCNET      7

// /**
// CONSTANT    :: HW_TYPE_APPLETLK : 8
// DESCRIPTION :: Hardware type APPLEtalk
// **/
#define HW_TYPE_APPLETLK    8

// /**
// CONSTANT    :: HW_TYPE_DLCI : 15
// DESCRIPTION :: Frame Relay DLCI
// **/
#define HW_TYPE_DLCI        15

// /**
// CONSTANT    :: HW_TYPE_ATM : 19
// DESCRIPTION :: ATM 10/100Mbps
// **/
#define HW_TYPE_ATM         19

// /**
// CONSTANT    :: HW_TYPE_METRICOM : 23
// DESCRIPTION :: Hardware type HW_TYPE_METRICOM
// **/
#define HW_TYPE_METRICOM     23

// /**
// CONSTANT    :: HW_TYPE_IEEE_1394 : 24
// DESCRIPTION :: Hardware type IEEE_1394
// **/
#define HW_TYPE_IEEE_1394    24

// /**
// CONSTANT    :: HW_TYPE_EUI_64 : 27
// DESCRIPTION :: Hardware identifier
// **/
#define HW_TYPE_EUI_64       27

// /**
// CONSTANT    :: HW_TYPE_UNKNOWN : 0xffff
// DESCRIPTION :: Unknown Hardware type
//                MAC protocol HARDWARE identifiers.
// **/
#define HW_TYPE_UNKNOWN     0xffff

// /**
// CONSTANT    :: MAC_IPV4_LINKADDRESS_LENGTH : 4
// DESCRIPTION :: Length of 4 byte MacAddress
// **/
#define MAC_IPV4_LINKADDRESS_LENGTH 4

// /**
// CONSTANT    :: MAC_NODEID_LINKADDRESS_LENGTH : 2
// DESCRIPTION :: Length of 2 byte MacAddress
// **/
#define MAC_NODEID_LINKADDRESS_LENGTH 2

// /**
// CONSTANT    :: IPV4_LINKADDRESS : 28
// DESCRIPTION :: Hardware identifier
// **/
#define IPV4_LINKADDRESS 28

// /**
// CONSTANT    :: HW_NODE_ID : 29
// DESCRIPTION :: Hardware identifier
// **/
#define HW_NODE_ID  29

// /**
// STRUCT      :: MacHWAddress
// DESCRIPTION :: MAC hardware address of variable length
// **/
struct mac_hw_addr_struct
{
    unsigned char* byte;
    unsigned short hwLength;
    unsigned short hwType;

    mac_hw_addr_struct()
    {
        byte = NULL;
        hwLength = 0;
        hwType = HW_TYPE_UNKNOWN;
    }

    mac_hw_addr_struct(const mac_hw_addr_struct &srcAddr)
    {
        hwType = srcAddr.hwType;
        hwLength = srcAddr.hwLength;

        if (hwLength == 0)
        {
            byte = NULL;
        }
        else
        {
            byte = (unsigned char*) MEM_malloc(hwLength);
            memcpy(byte, srcAddr.byte, hwLength);
        }
    }

    inline mac_hw_addr_struct& operator =(const mac_hw_addr_struct& srcAddr)
    {

        if (!(hwLength == 0 ||
                srcAddr.hwLength == 0 ||
                hwLength == srcAddr.hwLength))
        {
            ERROR_ReportError("Incompatible Mac Address");
        }
        if (&srcAddr != this)
        {
        hwType = srcAddr.hwType;
        hwLength = srcAddr.hwLength;

        if (hwLength == 0)
        {
            if (byte != NULL)
            {
                MEM_free(byte);
                byte = NULL;
            }
        }
        else
        {
            if (byte == NULL)
            {
                byte = (unsigned char*) MEM_malloc(srcAddr.hwLength);
            }
            memcpy(byte, srcAddr.byte, hwLength);
            }
        }
        return *this;
    }

    inline BOOL operator ==(const mac_hw_addr_struct& macAddr)
    {
        if(hwType == macAddr.hwType &&
           hwLength == macAddr.hwLength &&
           memcmp(byte, macAddr.byte,hwLength) == 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    ~mac_hw_addr_struct()
    {
        if (byte != NULL)
        {
            MEM_free(byte);
            byte = NULL;
        }
    }

};

// /**
// CONSTANT    :: INVALID_MAC_ADDRESS : mac_hw_addr_struct()
// DESCRIPTION :: INVALID MAC ADDRESS
// **/
#define INVALID_MAC_ADDRESS  mac_hw_addr_struct()



// /**
// STRUCT      :: Mac802Address
// DESCRIPTION :: MAC address of size MAC_ADDRESS_LENGTH_IN_BYTE.
//                It is default Mac address of type 802
// **/
typedef struct mac_addr_str
{
    unsigned char byte[MAC_ADDRESS_LENGTH_IN_BYTE];

    mac_addr_str()
    {
        memcpy(byte, INVALID_802ADDRESS, MAC_ADDRESS_LENGTH_IN_BYTE);
    }

    mac_addr_str(const mac_addr_str &srcAddr)
    {

        memcpy(byte, srcAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE);
    }

    mac_addr_str(const char* srcAddr)
    {

        memcpy(byte, srcAddr, MAC_ADDRESS_LENGTH_IN_BYTE);
    }


    inline mac_addr_str& operator =(const mac_addr_str& srcAddr)
    {
        memcpy(byte, srcAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE);
        return *this;
    }

    inline mac_addr_str& operator =(const char* srcAddr)
    {
        memcpy(byte, srcAddr, MAC_ADDRESS_LENGTH_IN_BYTE);
        return *this;
    }

    inline bool operator ==(const mac_addr_str& macAddr) const
    {
        if(memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return true;
        }
        return false;
    }

    inline bool operator ==(const char*  macAddr) const
    {
        if(memcmp(byte, macAddr, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return true;
        }
        return false;
    }

    inline bool operator !=(const mac_addr_str& macAddr) const
    {
        if(memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return false;
        }
        return true;
    }

    inline bool operator !=(const char*  macAddr) const
    {
        if(memcmp(byte, macAddr, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return false;
        }
        return true;
    }

    inline bool operator > (const mac_addr_str& macAddr) const
    {
        if(memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) > 0)
        {
            return true;
        }
        return false;
    }

    inline bool operator < (const mac_addr_str& macAddr) const
    {
        if(memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) < 0)
        {
            return true;
        }
        return false;
    }

    inline bool operator <= (const mac_addr_str& macAddr) const
    {
        if(memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) <= 0)
        {
            return true;
        }
        return false;
    }

    inline unsigned int hash(int hashSize)
    {
        unsigned int byteSum = byte[0] + byte[1] +
            byte[2] + byte[3] + byte[4] + byte[5];
        return byteSum % hashSize;
    }

} Mac802Address;


// /**
// ENUM         :: MacInterfaceState
// DESCRIPTION  :: Describes one out of two possible states
//                 of MAC interface - enable or disable
// **/
typedef enum
{
    MAC_INTERFACE_DISABLE,
    MAC_INTERFACE_ENABLE
} MacInterfaceState;


// /**
// ENUM         :: MacLinkType
// DESCRIPTION  :: Describes different link type
// **/
typedef enum
{
    WIRED = 0,
    WIRELESS = 1,
    MICROWAVE = 2
} MacLinkType;

// /**
// FUNCTION  :: MacReportInterfaceStatus
// PURPOSE   :: Callback funtion to report interface status
// PARAMETERS ::
// + node            : Node* : Pointer to a network node
// + interfaceIndex  : int   : index of interface
// + state           : MacInterfaceState : Wheather it enable or disable
// RETURN    :: void :
// **/
typedef void (* MacReportInterfaceStatus)(
    Node *node,
    int interfaceIndex,
    MacInterfaceState state);

// /**
// FUNCTION     :: MAC_SetInterfaceStatusHandlerFunction
// LAYER        :: MAC
// PURPOSE      :: Set the MAC interface handler function to be called
//                 when interface faults occurs
// PARAMETERS   ::
// + node            : Node* : Pointer to a network node
// + interfaceIndex  : int   : index of interface
// + statusHandler   : MacReportInterfaceStatus : Pointer to status Handler
// RETURN       :: void :
// **/
void
MAC_SetInterfaceStatusHandlerFunction(Node *node,
                                     int interfaceIndex,
                                     MacReportInterfaceStatus statusHandler);

// /**
// FUNCTION     :: MAC_GetInterfaceStatusHandlerFunction
// LAYER        :: MAC
// PURPOSE      :: To get the MACInterface status handling function
//                 for the system
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : index of interface
// RETURN       :: MacReportInterfaceStatus : Pointer to status handler
// **/
MacReportInterfaceStatus
MAC_GetInterfaceStatusHandlerFunction(Node *node, int interfaceIndex);


// /**
// FUNCTION  :: MacHasFrameToSendFn
// PURPOSE   :: Callback funtion for sending packet. It calls when network
//              layer has packet to send.
// PARAMETERS ::
// + node            : Node* : Pointer to node
// + interfaceIndex  : int   : index of interface
// RETURN    :: void : NULL
// **/
typedef void (* MacHasFrameToSendFn)(
    Node *node,
    int interfaceIndex);


// /**
// FUNCTION  :: MacReceiveFrameFn
// PURPOSE   :: Callback funtion to receive packet.
// PARAMETERS ::
// + node            : Node* : Pointer to node
// + interfaceIndex  : int   : index of interface
// + msg             : Message* : Pointer to the message
// RETURN    :: void : NULL
// **/
typedef void (* MacReceiveFrameFn)(
    Node *node,
    int interfaceIndex,
    Message *msg);


// /**
// CONSTANT     :: STATION_VLAN_TAGGING_DEFAULT : FALSE
// DESCRIPTION  :: Default VLAN TAGGING Value for a STATION node
// **/
#define STATION_VLAN_TAGGING_DEFAULT FALSE

// /**
// STRUCT       :: MacVlan
// DESCRIPTION  :: Structure of VLAN in MAC sublayer
// **/
typedef struct
{
    unsigned short vlanId;
    BOOL sendTagged;
} MacVlan;

// /**
// STRUCT       :: MacHeaderVlanTag
// DESCRIPTION  :: Structure of MAC sublayer VLAN header
// **/
typedef struct mac_header_vlan_tag
{
    unsigned int tagProtocolId:16,
                 userPriority:3,
                 canonicalFormatIndicator:1,
                 vlanIdentifier:12;
} MacHeaderVlanTag;

// /**
// ENUM         :: MAC_PROTOCOL
// DESCRIPTION  :: Specifies different MAC_PROTOCOLs used
// **/
typedef enum
{
    MAC_PROTOCOL_MPLS = 1,
    MAC_PROTOCOL_CSMA,
    MAC_PROTOCOL_FCSC_CSMA,
    MAC_PROTOCOL_MACA,
    MAC_PROTOCOL_FAMA,
    MAC_PROTOCOL_802_11,
    MAC_PROTOCOL_802_3,
    MAC_PROTOCOL_DAWN,
    MAC_PROTOCOL_LINK,
    MAC_PROTOCOL_ALOHA,
    MAC_PROTOCOL_GENERICMAC,
    MAC_PROTOCOL_SWITCHED_ETHERNET,
    MAC_PROTOCOL_TDMA,
    MAC_PROTOCOL_GSM,
    MAC_PROTOCOL_SPAWAR_LINK16,
    MAC_PROTOCOL_TADIL_LINK11,
    MAC_PROTOCOL_TADIL_LINK16,
    MAC_PROTOCOL_ALE,
    MAC_PROTOCOL_SATTSM,
    MAC_PROTOCOL_SATCOM,
#ifdef ADDON_BOEINGFCS
    MAC_PROTOCOL_BOEINGFCS_USAP,
    MAC_PROTOCOL_CES_WINTNCW,
    MAC_PROTOCOL_CES_WINTHNW,
    MAC_PROTOCOL_CES_WINTGBS,
    MAC_PROTOCOL_CES_USAP,
    MAC_PROTOCOL_BOEING_GENERICMAC,
    MAC_PROTOCOL_CES_SRW,
#endif // ADDON_BOEINGFCS
    MAC_PROTOCOL_USAP,
    MAC_PROTOCOL_SATELLITE_BENTPIPE,
    MAC_SWITCH,
    MAC_PROTOCOL_GARP,
    MAC_PROTOCOL_DOT11,
    MAC_PROTOCOL_DOT16,
    MAC_PROTOCOL_ABSTRACT,
    MAC_PROTOCOL_CELLULAR,
    MAC_PROTOCOL_ANE,
    MAC_PROTOCOL_WORMHOLE,
    MAC_PROTOCOL_ANODR,
    MAC_PROTOCOL_802_15_4,
    MAC_PROTOCOL_NONE // this must be the last one
}
MAC_PROTOCOL;

//--------------------------------------------------------------------------
// STRUCT      :: AddressResolutionModule
// DESCRIPTION :: Structure stored in MacData where data structures related to
//                Address Resolution Protocols like ARP, RARP, etc are stored
//--------------------------------------------------------------------------


#ifdef ADDON_NGCNMS
typedef void (*MacSetFunctionPtr)(Node*, int);
// Set rules defined
struct
struct_mac_reset_function_list
{
    // Corresponding set function
    MacSetFunctionPtr funcPtr;

    // the next match command
    struct struct_mac_reset_function_list* next;
};

typedef struct
struct_mac_reset_function_linkedList
{
    struct struct_mac_reset_function_list* first;
    struct struct_mac_reset_function_list* last;
} MacResetFunctionList;
#endif //ADDON_NGCNMS

// /**
// STRUCT       :: struct_mac_str
// DESCRIPTION  :: A composite structure representing MAC sublayer
//                 which is typedefed to MacData in main.h
// **/
struct struct_mac_str
{
    MAC_PROTOCOL macProtocol;
    D_UInt32     macProtocolDynamic;
    int          interfaceIndex;
    BOOL         macStats;
    BOOL         promiscuousMode;
    Int64        bandwidth;   // In bytes.
    clocktype    propDelay;
    BOOL         interfaceIsEnabled;
    int          phyNumber;
    void         *macVar;
    void         *mplsVar;

    MacHasFrameToSendFn sendFrameFn;
    MacReceiveFrameFn receiveFrameFn;

    LinkedList  *interfaceStatusHandlerList;
    NodeAddress virtualMacAddress;
    MacVlan* vlan;

    void*       bgMainStruct; //ptr of background traffic main structure
    void*       randFault; //ptr of the random link fault data structure
    short       faultCount; //flag for link fault.

    MacHWAddress                macHWAddr;
    BOOL                        isLLCEnabled;
    BOOL                        interfaceCardFailed;

#ifdef NETSEC_LIB
    void         *encryptionVar;

    // This MAC interface is a wormhole eavesdropper/replayer.
    // This per-interface flag is added in particular for multi-channel
    // wormhole because of the channel-interface correspondence in QualNet.
    // Packets could be replayed in all physical channels in case
    // the victim network is using multiple channels
    BOOL        isWormhole;
#endif // NETSEC_LIB

    // For Mibs
    D_UInt32 ifInOctets;
    D_UInt32 ifOutOctets;
    D_UInt32 ifOutErrors;
    D_UInt32 ifInErrors;
    D_UInt64 ifHCInOctets;
    D_UInt64 ifHCOutOctets;
    D_UInt32 ifOperStatus;

    //subnet index for subnet list data structure
    //this way you can find which subnet list this interface belongs to
    short subnetIndex;

#ifdef ADDON_NGCNMS
    MacResetFunctionList *resetFunctionList;
#endif

};

#ifdef NETSEC_LIB
// /**
// ENUM         :: MAC_SECURITY
// DESCRIPTION  :: Specifies different MAC_SECURITY_PROTOCOLs used
// **/
typedef enum
{
    MAC_SECURITY_WEP = 1,
    MAC_SECURITY_CCMP
} MAC_SECURITY;
#endif // NETSEC_LIB


// MACROs for enabling and disabling and toggling between enabled
// and disabled interfaces.

// /**
// MACRO        :: MAC_EnableInterface(node, interfaceIndex)
// DESCRIPTION  :: Enable the MAC_interface
// **/
#define MAC_EnableInterface(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled = TRUE)

// /**
// MACRO        :: MAC_DisableInterface(node, interfaceIndex)
// DESCRIPTION  :: Disable the MAC_interface
// **/
#define MAC_DisableInterface(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled = FALSE)

// /**
// MACRO        :: MAC_ToggleInterfaceStatus(node, interfaceIndex)
// DESCRIPTION  :: Toggle the MAC_interface status
// **/
#define MAC_ToggleInterfaceStatus(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled ? \
         node->macData[interfaceIndex]->interfaceIsEnabled = FALSE : \
         node->macData[interfaceIndex]->interfaceIsEnabled = TRUE )

// /**
// MACRO        :: MAC_InterfaceIsEnabled(node, interfaceIndex)
// DESCRIPTION  :: To query MAC_interface status is enabled or not
// **/
#define MAC_InterfaceIsEnabled(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled)

// /**
// FUNCTION     :: MAC_NetworkLayerHasPacketToSend
// LAYER        :: MAC
// PURPOSE      :: Handles packets from the network layer when the
//                 network queue is empty
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : index of interface
// RETURN       :: void :
// **/
void
MAC_NetworkLayerHasPacketToSend(
    Node *node,
    int interfaceIndex);



// /**
// FUNCTION     :: MAC_NetworkLayerChanswitch
// LAYER        :: MAC
// PURPOSE      :: Network layer tells MAC layer to switch channels.               
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : index of interface
// RETURN       :: void :
// **/
void
MAC_NetworkLayerChanswitch(
    Node *node,
    int interfaceIndex);

// /**
// FUNCTION     :: MAC_SwitchHasPacketToSend
// LAYER        :: MAC
// PURPOSE      :: To inform MAC that the Switch has packets to
//                 to send
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : index of interface
// RETURN       :: void :
// **/
void MAC_SwitchHasPacketToSend(
    Node* node,
    int interfaceIndex);

// /**
// FUNCTION     :: MAC_ReceivePacketFromPhy
// LAYER        :: MAC
// PURPOSE      :: Handles packets received from physical layer
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : index of interface
// + packet         : Message* : Pointer to Message
// RETURN       :: void :
// **/
void
MAC_ReceivePacketFromPhy(
    Node *node,
    int interfaceIndex,
    Message *packet);


#ifdef ADDON_LINK16
// /**
// FUNCTION     :: MAC_ReceivePacketFromPhySpawar_Link16
// LAYER        :: MAC
// PURPOSE      :: Handles packets that was just received from the physical
//                 medium
// PARAMETERS   ::
// + node           : Node*    : Pointer to a network node
// + interfaceIndex : int      : index of interface
// + packet         : Message* : Pointer to Message
// + numError       : int      : Error number
// RETURN       :: void :
// **/
void
MAC_ReceivePacketFromPhySpawar_Link16(
    Node *node,
    int interfaceIndex,
    Message *packet,
    int numError);
#endif // ADDON_LINK16

// /**
// ENUM         :: ManagementRequestType
// DESCRIPTION  :: Type of management request message
// **/
typedef enum ManagementRequestTypeT {
    ManagementRequestUnspecified = 1,
    ManagementRequestEcho,
    ManagementRequestSetGroupMembership,
    ManagementRequestSetNewRap,
    ManagementRequestSetBandwidthLimit
} ManagementRequestType;

// /**
// STRUCT       :: ManagementRequest
// DESCRIPTION  :: data structure of management request
// **/
typedef struct ManagementRequestT {
    ManagementRequestType type;
    void *data;
} ManagementRequest;

// /**
// ENUM         :: ManagementResponseType
// DESCRIPTION  :: Type of management response message
// **/
typedef enum ManagementResponseTypeT {
    ManagementResponseOK = 1,
    ManagementResponseUnsupported,
    ManagementResponseIllformedRequest,
    ManagementResponseUnspecifiedFailure
} ManagementResponseType;

// /**
// STRUCT       :: ManagementResponse
// DESCRIPTION  :: data structure of management response
// **/
typedef struct ManagementResponseT {
    ManagementResponseType result;
    void *data;
} ManagementResponse;

// /**
// FUNCTION     :: MAC_ManagementRequest
// LAYER        :: MAC
// PURPOSE      :: Deliver a network management request to the MAC
// PARAMETERS   ::
// + node                  : Node* : Pointer to a network node
// + interfaceIndex        : int : index of interface
// + req                   : ManagementRequest* : Pointer to a management request
// + resp                  : ManagementResponse* : Pointer to a management response
// RETURN       :: void :
// **/
void
MAC_ManagementRequest(Node *node, int interfaceIndex,
                      ManagementRequest *req, ManagementResponse *resp);

// /**
// FUNCTION     :: MAC_ReceivePhyStatusChangeNotification
// LAYER        :: MAC
// PURPOSE      :: Handles status changes received from the physical layer
// PARAMETERS   ::
// + node                   : Node* : Pointer to a network node
// + interfaceIndex         : int : index of interface
// + oldPhyStatus           : PhyStatusType : Old status of physical layer
// + newPhyStatus           : PhyStatusType : New status of physical layer
// + receiveDuration        : clocktype : Duration after which received
// + potentialIncomingPacket: Message* : Pointer to incoming message
// RETURN       :: void :
// **/
void
MAC_ReceivePhyStatusChangeNotification(
    Node *node,
    int interfaceIndex,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus,
    clocktype receiveDuration,
    const Message *potentialIncomingPacket);

// /**
// STRUCT       :: MacToPhyPacketDelayInfoType
// DESCRIPTION  :: Specifies the MAC to Physical layer delay
// information structure
// **/
typedef struct
{
    clocktype theDelayUntilAirborn;
}
MacToPhyPacketDelayInfoType;

// /**
// FUNCTION     :: MAC_InitUserMacProtocol
// LAYER        :: MAC
// PURPOSE      :: Initialisation function for the User MAC_protocol
// PARAMETERS   ::
// + node            : Node* : Pointer to a network node
// + nodeInput       : NodeInput : Configured Inputs for the node
// + macProtocolName : const char* : MAC protocol name
// + interfaceIndex  : int : interface index
// RETURN       :: void :
// **/
void
MAC_InitUserMacProtocol(
    Node* node,
    const NodeInput*nodeInput,
    const char* macProtocolName,
    int interfaceIndex);

// /**
// FUNCTION     :: MacFinalizeUserMacProtocol
// LAYER        :: MAC
// PURPOSE      :: Finalization function for the User MAC_protocol
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : index of interface
// RETURN       :: void :
// **/
void
MacFinalizeUserMacProtocol(Node*node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_HandleUserMacProtocolEvent
// LAYER        :: MAC
// PURPOSE      :: Handles the MAC protocol event
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : index of interface
// + packet         : Message* : Pointer to Message
// RETURN       :: void :
// **/
void
MAC_HandleUserMacProtocolEvent(
    Node *node,
    int interfaceIndex,
    Message *packet);

// /**
// FUNCTION     :: MAC_OutputQueueIsEmpty
// LAYER        :: MAC
// PURPOSE      :: To check if Output queue for an interface of
//                 a node if empty or not
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : index of interface
// RETURN       :: BOOL : empty or not
// **/
BOOL
MAC_OutputQueueIsEmpty(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_NotificationOfPacketDrop
// LAYER        :: MAC
// PURPOSE      :: To notify MAC of packet drop
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + nextHopAddress : NodeAddress : Node address
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// RETURN       :: void :
// **/
void
MAC_NotificationOfPacketDrop(
    Node *node,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    Message *msg);

// /**
// FUNCTION     :: MAC_NotificationOfPacketDrop
// LAYER        :: MAC
// PURPOSE      :: To notify MAC of packet drop
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + nextHopAddress :MacHWAddress : Node address
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// RETURN       :: void :
// **/
void
MAC_NotificationOfPacketDrop(
    Node *node,
    MacHWAddress nextHopAddress,
    int interfaceIndex,
    Message *msg);

// /**
// FUNCTION     :: MAC_NotificationOfPacketDrop
// LAYER        :: MAC
// PURPOSE      :: To notify MAC of packet drop
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + nextHopAddress :Mac802Address : mac address
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// RETURN       :: void :
// **/
void
MAC_NotificationOfPacketDrop(
    Node *node,
    Mac802Address nextHopAddress,
    int interfaceIndex,
    Message *msg);

// /**
// FUNCTION     :: MAC_OutputQueueTopPacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To notify MAC of priority packet arrival
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType : Message Priority
// + msg            : Message** : Pointer to Message
// + nextHopAddress : NodeAddress: Next hop address
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    NodeAddress *nextHopAddress);

// /**
// FUNCTION     :: MAC_OutputQueueTopPacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To notify MAC of priority packet arrival
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType : Message Priority
// + msg            : Message** : Pointer to Message
// + nextHopAddress : Mac802Address*: Next hop mac address
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    Mac802Address *destMacAddr);

// /**
// FUNCTION     :: MAC_OutputQueueTopPacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To notify MAC of priority packet arrival
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType : Message Priority
// + msg            : Message** : Pointer to Message
// + nextHopAddress : MacHWAddress*: Next hop mac address
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    MacHWAddress *destHWAddr);


// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To remove the packet at the front of the specified priority
//                 output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType : Message Priority
// + msg            : Message** : Pointer to Message
// + nextHopAddress : NodeAddress* : Next hop address
// + networkType    : int* : network type
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType);


// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To remove the packet at the front of the specified
//                 priority output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType : Message Priority
// + msg            : Message** : Pointer to Message
// + nextHopAddress : MacHWAddress* : Next hop mac address
// + networkType    : int* : network type
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    MacHWAddress *destHWAddr,
    int *networkType);

// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To remove the packet at the front of the specified
//                 priority output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType : Message Priority
// + msg            : Message** : Pointer to Message
// + nextHopAddress : Mac802Address* : Next hop mac address
// + networkType    : int* : network type
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message** msg,
    Mac802Address* destMacAddr,
    int* networkType);

// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To allow a peek by network layer at packet
//                 before processing
//                 It is overloading function used for ARP packet
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType* : tos value
// + msg            : Message** : Pointer to Message
// + destMacAddr    : MacHWAddress* : Dest addr Pointer
// + networkType    : int* : Network Type pointer
// + packType       : int* : packet Type pointer
// RETURN       :: BOOL : If success TRUE
// NOTE         : Overloaded MAC_OutputQueueDequeuePacketForAPriority()
// **/
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node* node,
    int interfaceIndex,
    TosType* priority,
    Message** msg,
    NodeAddress *nextHopAddress,
    MacHWAddress* destMacAddr,
    int* networkType,
    int* packType);

// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To allow a peek by network layer at packet
//                 before processing
//                 It is overloading function used for ARP packet
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType* : tos value
// + msg            : Message** : Pointer to Message
// + destMacAddr    : Mac802Address* : Dest addr Pointer
// + networkType    : int* : Network Type pointer
// + packType       : int* : packet Type pointer
// RETURN       :: BOOL : If success TRUE
// NOTE         : Overloaded MAC_OutputQueueDequeuePacketForAPriority()
// **/
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType* priority,
    Message** msg,
    NodeAddress *nextHopAddress,
    Mac802Address* destMacAddr,
    int* networkType,
    int* packetType);


// /**
// FUNCTION     :: MAC_SneakPeekAtMacPacket
// LAYER        :: MAC
// PURPOSE      :: To allow a peek by network layer at
//                 packet before processing
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : const Message* : Pointer to Message
// + prevHop        : NodeAddress : Previous Node address
// + destAddr       : NodeAddress : Destination Node address
// + messageType : int : Distinguish between the ARP and general message
// RETURN       :: void : NULL
// **/
void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    NodeAddress prevHop,
    NodeAddress destAddr,
    int messageType = PROTOCOL_TYPE_IP);

// /**
// FUNCTION     :: MAC_SneakPeekAtMacPacket
// LAYER        :: MAC
// PURPOSE      :: To allow a peek by network layer at
//                 packet before processing
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : const Message* : Pointer to Message
// + prevHop        : MacHWAddress : Previous Node mac address
// + destAddr       : MacHWAddress : Destination Node mac address
// + arpMessageType : int : Distinguish between the ARP and general message
// RETURN       :: void : NULL
// **/
void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress& prevHop,
    MacHWAddress& destAddr,
    int messageType = PROTOCOL_TYPE_IP);

// /**
// FUNCTION     :: MAC_SneakPeekAtMacPacket
// LAYER        :: MAC
// PURPOSE      :: To allow a peek by network layer at
//                 packet before processing
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : const Message* : Pointer to Message
// + prevHop        : Mac802Address : Previous Node address
// + destAddr       : Mac802Address : Destination Node address
// + messageType : int : Distinguish between the ARP and general message
// RETURN       :: void : NULL
// **/

void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address prevHop,
    Mac802Address destAddr,
    int messageType = PROTOCOL_TYPE_IP);

// /**
// FUNCTION     :: MAC_MacLayerAcknowledgement
// LAYER        :: MAC
// PURPOSE      :: To send acknowledgement from MAC
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : interfaceIndex
// + msg            : Message* : Pointer to Message
// + nextHop        : NodeAddress: Pointer to Node address
// RETURN       :: void :
// **/
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    NodeAddress nextHop);

// /**
// FUNCTION     :: MAC_MacLayerAcknowledgement
// LAYER        :: MAC
// PURPOSE      :: To send acknowledgement from MAC
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : interfaceIndex
// + msg            : Message* : Pointer to Message
// + nextHop        : MacHWAddress&: Pointer to Node address
// RETURN       :: void :
// **/
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    MacHWAddress& nextHop);

// /**
// FUNCTION     :: MAC_MacLayerAcknowledgement
// LAYER        :: MAC
// PURPOSE      :: To send acknowledgement from MAC
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : interfaceIndex
// + msg            : Message* : Pointer to Message
// + nextHop        : Mac802Address&: Pointer to nexthop mac address
// RETURN       :: void :
// **/
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    Mac802Address& nextHop);

// /**
// FUNCTION     :: MAC_HandOffSuccessfullyReceivedPacket
// LAYER        :: MAC
// PURPOSE      :: Pass packet successfully up to the network layer
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// + lastHopAddress : NodeAddress: Node address
// RETURN       :: void :
// **/
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    NodeAddress lastHopAddress);


// /**
// FUNCTION     :: MAC_HandOffSuccessfullyReceivedPacket
// LAYER        :: MAC
// PURPOSE      :: Pass packet successfully up to the network layer
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// + lastHopAddr : MacHWAddress*: mac address
// RETURN       :: void :
// **/


void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress* lastHopAddr);

// /**
// FUNCTION     :: MAC_HandOffSuccessfullyReceivedPacket
// LAYER        :: MAC
// PURPOSE      :: Pass packet successfully up to the network layer
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// + lastHopAddr : Mac802ddress*: mac address
// RETURN       :: void :
// **/
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address* lastHopAddr);


// /**
// FUNCTION     :: MAC_HandOffSuccessfullyReceivedPacket
// LAYER        :: MAC
// PURPOSE      :: Pass packet successfully up to the network layer
//                 It is overloading function used for ARP packet
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// + lastHopAddress : MacHWAddress*: mac address
// + arpMessageType : int : Distinguish between ARP and general message
// RETURN       :: void : NULL
// **/
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress* macAddr,
    int arpMessageType);

// /**
// FUNCTION     :: MAC_HandOffSuccessfullyReceivedPacket
// LAYER        :: MAC
// PURPOSE      :: Pass packet successfully up to the network layer
//                 It is overloading function used for ARP packet
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg : Message* : Pointer to Message
// + lastHopAddress : Mac802Address*: mac address
// + arpMessageType : int : Distinguish between ARP and general message
// RETURN       :: void : NULL
// **/
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address* lastHopAddr,
    int messageType);

// /**
// FUNCTION     :: MAC_OutputQueueTopPacket
// LAYER        :: MAC
// PURPOSE      :: To check packet at the top of output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : Message** : Pointer to Message
// + nextHopAddress : NodeAddress* :  Next hop address
// + networkType    : int : network type
// + priority       : TosType* : Message Priority
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int* networkType,
    TosType *priority);

// /**
// FUNCTION     :: MAC_OutputQueueTopPacket
// LAYER        :: MAC
// PURPOSE      :: To check packet at the top of output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : Message** : Pointer to Message
// + nextHopAddress : MacHWAddress* :  Next hop address
// + networkType    : int : network type
// + priority       : TosType* : Message Priority
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    MacHWAddress *destHWAddr,
    int* networkType,
    TosType *priority);

// /**
// FUNCTION     :: MAC_OutputQueueTopPacket
// LAYER        :: MAC
// PURPOSE      :: To check packet at the top of output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : Message** : Pointer to Message
// + nextHopAddress : Mac802Address* :  Next hop address
// + networkType    : int : network type
// + priority       : TosType* : Message Priority
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
// **/
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    Mac802Address* destMacAddr,
    int* networkType,
    TosType *priority);

// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacket
// LAYER        :: MAC
// PURPOSE      :: To remove packet from front of output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : Message** : Pointer to Message
// + nextHopAddress : NodeAddress* : Pointer to Node address
// + networkType    : int : network type
// + priority       : TosType * : Pointer to queuing priority type
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType,
    TosType *priority);

// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacket
// LAYER        :: MAC
// PURPOSE      :: To remove packet from front of output queue
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : Message** : Pointer to Message
// + nextHopAddress : MacHWAddress* : Pointer to Mac address
// + networkType    : int : network type
// + priority       : TosType * : Pointer to queuing priority type
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/
BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    MacHWAddress *destHWAddr,
    int *networkType,
    TosType *priority);

// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacket
// LAYER        :: MAC
// PURPOSE      :: To remove packet from front of output queue,
//                 Its a overloaded function
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : Message** : Pointer to Message
// + nextHopAddress : Mac802Address* : Pointer to MacAddress address
// + networkType    : int : network type
// + priority       : TosType * : Pointer to queuing priority type
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/

BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    Mac802Address* destmacAddr,
    int *networkType,
    TosType *priority);

// /**
// FUNCTION     :: MAC_IsMyUnicastFrame
// LAYER        :: MAC
// PURPOSE      :: Check if a packet (or frame) belongs to this node
//                 Should be used only for four byte mac address
// PARAMETERS   ::
// + node     : Node* : Pointer to a network node
// + destAddr : NodeAddress : Destination Address
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsMyUnicastFrame(Node *node, NodeAddress destAddr);

// /**
// FUNCTION     :: MAC_IsWiredNetwork
// LAYER        :: MAC
// PURPOSE      :: To check if an interface is a wired interface
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsWiredNetwork(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsPointToPointNetwork
// LAYER        :: MAC
// PURPOSE      :: Checks if an interface belongs to Point to PointNetwork
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : interfaceIndex
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsPointToPointNetwork(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsPointToMultiPointNetwork
// LAYER        :: MAC
// PURPOSE      :: Checks if an interface belongs to Point to Multi-Point
//                 network.
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int   : interfaceIndex
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsPointToMultiPointNetwork(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsWiredBroadcastNetwork
// LAYER        :: MAC
// PURPOSE      :: Determines if an interface is a wired broadcast interface
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsWiredBroadcastNetwork(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsWirelessNetwork
// LAYER        :: MAC
// PURPOSE      :: Determine if a node's interface is a wireless interface
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsWirelessNetwork(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsWirelessAdHocNetwork
// LAYER        :: MAC
// PURPOSE      :: Determine if a node's interface is a possible wireless
//                 ad hoc interface
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsWirelessAdHocNetwork(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsOneHopBroadcastNetwork
// LAYER        :: MAC
// PURPOSE      :: Determines if an interface is a single Hop
//                 Broadcast interface
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsOneHopBroadcastNetwork(Node *node, int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsASwitch
// LAYER        :: MAC
// PURPOSE      :: To check if a node is a switch
// PARAMETERS   ::
// + node : Node* : Pointer to a network node
// RETURN       :: BOOL : boolean
// **/
BOOL
MAC_IsASwitch(Node* node);

// /**
// FUNCTION     :: MAC_SetVirtualMacAddress
// LAYER        :: MAC
// PURPOSE      :: To set MAC address
// PARAMETERS   ::
// + node              : Node* : Pointer to a network node
// + interfaceIndex    : int : interface index
// + virtualMacAddress : NodeAddress : MAC address
// RETURN       :: void :
// **/
void MAC_SetVirtualMacAddress(Node *node,
                              int interfaceIndex,
                              NodeAddress virtualMacAddress);
// /**
// FUNCTION   :: MacSetDefaultHWAddress
// LAYER      :: MAC
// PURPOSE    :: Set Default interface Hardware Address of node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware structure
// + interfaceIndex : int : Interface on which the hardware address set
// RETURN     :: void : NULL
// **/
void MacSetDefaultHWAddress(
    Node* node,
    MacHWAddress* macAddr,
    int interfaceIndex);

// /**
// FUNCTION     :: MAC_IsMyMacAddress
// LAYER        :: MAC
// PURPOSE      :: To check if received mac address belongs to itself
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interface index
// + destAddr       : NodeAddress : dest address
// RETURN       :: NodeAddress : Node Address
// **/
BOOL MAC_IsMyMacAddress(Node *node,
                        int interfaceIndex,
                        NodeAddress destAddr);


// /**
// FUNCTION           :: MAC_IsMyHWAddress
// PURPOSE            :: Checks for own MAC address.
// PARAMETERS         ::
// + node              : Node* : Node pointer
// + interfaceIndex    : int : Interface index
// + macAddr           : MacAddress* : Mac Address
// RETURN             :: BOOL :
// NOTE               :: Overloaded MAC_IsMyHWAddress()
// **/

BOOL MAC_IsMyHWAddress(
    Node* node,
    int interfaceIndex,
    MacHWAddress* macAddr);

// /**
// FUNCTION   :: MacValidateAndSetHWAddress
// LAYER      :: MAC
// PURPOSE    :: Validate MAC Address String after fetching from user
// PARAMETERS ::
// + macAddrStr : char* : Pointer to address string
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: void : NULL
// **/
void MacValidateAndSetHWAddress(
    char* macAddrStr,
    MacHWAddress* macAddr);

// /**
// FUNCTION   :: DefaultMacHWAddressToIpv4Address
// LAYER      :: MAC
// PURPOSE    :: Retrieve the IP Address  from Default HW Address .
//               Default HW address is equal to 6 bytes
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: NodeAddress : Ip address
// **/

NodeAddress DefaultMacHWAddressToIpv4Address(
    Node* node,
    MacHWAddress* macAddr);



// /**
// FUNCTION   :: MacGetHardwareLength
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Hardware Length.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interface : int : interface whose hardware length required
// + hwLength  : unsigned short  : Pointer to hardware string
// RETURN     :: void : NULL
// **/
void MacGetHardwareLength(
    Node* node,
    int interfaceIndex,
    unsigned short *hwLength);


// /**
// FUNCTION   :: MacGetHardwareType
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Hardware Type.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interface : int : interface whose mac type requires
// + type : unsigned short* : Pointer to hardware type
// RETURN     :: void : NULL
// **/
void MacGetHardwareType(
    Node* node,
    int interfaceIndex,
    unsigned short* type);

// /**
// FUNCTION   :: MacGetHardwareAddressString
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Hardware Address String.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interface : int : interface whose  hardware address retrieved
// +macAddressString  : unsigned char* : Pointer to hardware string
// RETURN     :: void : NULL
// **/
void MacGetHardwareAddressString(
    Node* node,
    int interfaceIndex,
    unsigned char* macAddressString);

// /**
// FUNCTION     :: MacAddNewInterface
// LAYER        :: MAC
// PURPOSE      :: To add a new Interface at MAC
// PARAMETERS   ::
// + node             : Node* : Pointer to a network node
// + interfaceAddress : NodeAddress : interface IP add
// + numHostBits      : int : No of host bits
// + interfaceIndex   : int* : interface index
// + nodeInput        : const NodeInput : node input
// + macProtocolName  : char* : Mac protocol of interface
// RETURN       :: void :
// **/
void MacAddNewInterface(
    Node *node,
    NodeAddress interfaceAddress,
    int numHostBits,
    int *interfaceIndex,
    const NodeInput *nodeInput,
    const char *macProtocolName);

// /**
// FUNCTION     :: MacAddVlanInfoForThisInterface
// LAYER        :: MAC
// PURPOSE      :: Init and read VLAN configuration from user input for
//                           node and interface passed as arguments
// PARAMETERS   ::
// + node             : Node* : Pointer to a network node
// + interfaceIndex   : int* : interface index
// + interfaceAddress : NodeAddress : interface IP add
// + nodeInput        : const NodeInput : node input
// RETURN       :: void :
// **/
void MacAddVlanInfoForThisInterface(
    Node* node,
    int interfaceIndex,
    const Address* address,
    const NodeInput *nodeInput);

// /**
// FUNCTION     :: MacReleaseVlanInfoForThisInterface
// LAYER        :: MAC
// PURPOSE      :: To flush VLAN info for an interface
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interface index
// RETURN       :: NodeAddress : Node Address
// **/
void MacReleaseVlanInfoForThisInterface(
    Node* node,
    int interfaceIndex);



// /**
// FUNCTION   :: MAC_IsBroadcastHWAddress
// LAYER      :: MAC
// PURPOSE    :: Checks Broadcast MAC address
// PARAMETERS ::
// + macAddr : MacHWAddress* : structure to hardware address
// RETURN     :: BOOL : TRUE or FALSE
// **/
BOOL MAC_IsBroadcastHWAddress(
    MacHWAddress* macAddr);

// /**
// FUNCTION   :: MAC_IsIdenticalHWAddress
// LAYER      :: MAC
// PURPOSE    :: Compares two MAC addresses
// PARAMETERS ::
// + macAddr1 : MacHWAddress* : Pointer to harware address structure
// + macAddr2 : MacHWAddress* : Pointer to harware address structure
// RETURN     :: BOOL : TRUE or FALSE
// **/
BOOL MAC_IsIdenticalHWAddress(
    MacHWAddress* macAddr1,
    MacHWAddress* macAddr2);

// /**
// FUNCTION           :: MAC_PrintHWAddr
// PURPOSE            :: Prints interface Mac Address
// PARAMETERS         ::
// + macAddr : MacHWAddress* : Mac address
// RETURN             :: void :
// ASSUMPTION         : Byte separator is always '-'
// **/
void MAC_PrintHWAddr(MacHWAddress* macAddr);

// /**
// FUNCTION           :: MAC_PrintMacAddr
// PURPOSE            :: Prints interface Mac Address
// PARAMETERS         ::
// + macAddr : Mac802Address* : Mac address
// RETURN             :: void :
// ASSUMPTION         : Byte separator is always '-'
// **/
void MAC_PrintMacAddr(Mac802Address* macAddr);

#define RAND_TOKENSEP    " ,\t"

#define RAND_FAULT_GEN_SKIP   2


// /**
// ENUM         :: MacLinkType
// DESCRIPTION  :: Describes different fault type
// **/
typedef enum
{
    STATIC_FAULT,
    RANDOM_FAULT
} FaultType;

// /**
// STRUCT      :: MacFaultInfo
// DESCRIPTION :: Fields for keeping track of interface faults
// **/
struct MacFaultInfo
{
    FaultType  faultType;
    MacHWAddress macHWAddress;
    BOOL interfaceCardFailed;
};


// /**
// STRUCT      :: RandFault
// DESCRIPTION :: Structure containing random fault information.
// **/
typedef struct struct_random_fault
{
    int             repetition; //no. of interface fault
    clocktype       startTm;    //time to start first fault
    RandomDistribution<clocktype> randomUpTm;    //Up time distribution
    RandomDistribution<clocktype> randomDownTm;  //Down time distribution

    // Variables for statistics collection.
    int             numLinkfault;   // number of random link fault
    clocktype       totDuration;    // total time for random link fault
    int             totFramesDrop; //Total frames dropped due to fault
} RandFault;


// /**
// FUNCTION    :: MAC_RandFaultInit
// LAYER       :: MAC
// PURPOSE     :: Initialization the Random Fault structure from input file
// PARAMETERS  ::
// + node           : Node* : Node pointer
// + interfaceIndex : int   : Interface index
// + currentLine    : const char* : pointer to the input string
// RETURN      :: void     : NULL
// **/
void MAC_RandFaultInit(Node*       faultyNode,
                       int         interfaceIndex,
                       const char* currentLine);


// /**
// FUNCTION    :: MAC_RandFaultFinalize
// LAYER       :: MAC
// PURPOSE     :: IPrint the statistics of Random link fault.
// PARAMETERS  ::
// + node           : Node* : Node pointer
// + interfaceIndex : int   : Interface index
// RETURN      :: void     : NULL
// **/
void MAC_RandFaultFinalize(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: MAC_GetPacketsPriority
// LAYER      :: MAC
// PURPOSE    :: Returns the priority of the packet
// PARAMETERS ::
// + msg       : Message* :Node Pointer
// RETURN    :: TosType : priority
// NOTE: DOT11e updates
// **/
TosType MAC_GetPacketsPriority(Node *, Message* msg);

// /**
// FUNCTION   :: MAC_TranslateMulticatIPv4AddressToMulticastMacAddress
// LAYER      :: MAC
// PURPOSE    :: Convert the Multicast ip address to multicast MAC address
// PARAMETERS ::
// + multcastAddress : NodeAddress : Multicast ip address
// + macMulticast : MacHWAddress* : Pointer to mac hardware address
// RETURN     :: void : NULL
// **/
void MAC_TranslateMulticatIPv4AddressToMulticastMacAddress(
    NodeAddress multcastAddress,
    MacHWAddress* macMulticast);

// /**
// FUNCTION   :: MAC_OutputQueuePeekByIndex
// LAYER      :: MAC
// PURPOSE    :: Look at the packet at the index of the output queue.
// PARAMETERS ::
// + node              : Node*       : Node pointer
// + interfaceIndex    : int         : Interface index
// + msgIndex          : int         : Message index
// + msg              : Message**   :Double pointer to message
// + nextHopAddress   : NodeAddress : Next hop mac address
// + priority         : TosType     : priority
// RETURN     :: BOOL : TRUE if the messeage found, FALSE otherwise
// **/
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    TosType *priority);

// /**
// FUNCTION   :: MAC_OutputQueuePeekByIndex
// LAYER      :: MAC
// PURPOSE    :: Look at the packet at the index of the output queue.
// PARAMETERS ::
// + node              : Node*       : Node pointer
// + interfaceIndex    : int         : Interface index
// + msgIndex          : int         : Message index
// + msg              : Message**   :Double pointer to message
// + nextHopAddress   : Mac802Address* : Next hop mac address
// + priority         : TosType     : priority
// RETURN     :: BOOL : TRUE if the messeage found, FALSE otherwise
// **/
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    Mac802Address *nextHopMacAddr,
    TosType *priority);

// /**
// FUNCTION   :: MAC_OutputQueuePeekByIndex
// LAYER      :: MAC
// PURPOSE    :: Look at the packet at the index of the output queue.
// PARAMETERS ::
// +node              : Node*       : Node pointer
// + interfaceIndex    : int         : Interface index
// + msgIndex          : int         : Message index
// + msg              : Message**   :Double pointer to message
// + nextHopAddress   : MacHWAddress* : Next hop mac address
// + priority         : TosType     : priority
// RETURN     :: BOOL : TRUE if the messeage found, FALSE otherwise
// **/
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    MacHWAddress *nextHopAddr,
    TosType *priority);

// /**
// FUNCTION   :: MAC_OutputQueueDequeuePacketWithIndex
// LAYER      :: MAC
// PURPOSE    :: To remove the packet at specified index output queue.
// PARAMETERS ::
// + node              : Node*       : Node pointer
// + interfaceIndex    : int         : Interface index
// + msgIndex          : int         : Message index
// + msg              : Message**   :Double pointer to message
// + nextHopAddress   : NodeAddress : Next hop IP address
// + networkType      : int         : Type of network
// RETURN     :: BOOL : TRUE if the messeage dequeued properly,
//                      FALSE otherwise
// **/
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType);

// /**
// FUNCTION   :: MAC_OutputQueueDequeuePacketWithIndex
// LAYER      :: MAC
// PURPOSE    :: To remove the packet at specified index output queue.
// PARAMETERS ::
// + node              : Node*       : Node pointer
// + interfaceIndex    : int         : Interface index
// + msgIndex          : int         : Message index
// + msg              : Message**   :Double pointer to message
// + nextHopMacAddress   : Mac802Address* : Next hop mac address
// + networkType      : int         : Type of network
// RETURN     :: BOOL : TRUE if the messeage dequeued properly,
//                      FALSE otherwise
// **/
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    Mac802Address* nextHopMacAddr,
    int *networkType);

// /**
// FUNCTION   :: MAC_OutputQueueDequeuePacketWithIndex
// LAYER      :: MAC
// PURPOSE    :: To remove the packet at specified index output queue.
// PARAMETERS ::
// + node              : Node*       : Node pointer
// + interfaceIndex    : int         : Interface index
// + msgIndex          : int         : Message index
// + msg              : Message**   :Double pointer to message
// + nextHopMacAddress   : MacHWAddress : Next hop mac address
// + networkType      : int         : Type of network
// RETURN     :: BOOL : TRUE if the messeage dequeued properly,
//                      FALSE otherwise
// **/
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    MacHWAddress* nextHopMacAddr,
    int *networkType);


// /**
// FUNCTION   :: MAC_IPv4addressIsMulticastAddress
// LAYER      :: MAC
// PURPOSE    :: Check the given address is Multicast address or not.
// PARAMETERS ::
// + ipV4 : NodeAddress : ipV4 address
// RETURN     :: BOOL : TRUE or FALSE
// **/
BOOL MAC_IPv4addressIsMulticastAddress(
    NodeAddress ipV4);


// /**
// FUNCTION           :: MAC_IsBroadcastMacAddress
// PURPOSE            :: Checks Broadcast MAC address.
// PARAMETERS         ::
// + macAddr           : MacAddress* : Mac Address.
// RETURN             :: BOOL :
// **/

BOOL MAC_IsBroadcastMac802Address(Mac802Address *macAddr);

// /**
// FUNCTION   :: IPv4AddressToDefaultMac802Address
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Mac802Address  from IP address.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + index : int : Interface Index
// + ipv4Address : NodeAddress : Ipv4 address from which the
//                                hardware address resolved.
// + macAddr : Mac802Address* : Pointer to Mac802address structure
// RETURN     :: void : NULL
// **/

void IPv4AddressToDefaultMac802Address(
    Node *node,
    int index,
    NodeAddress ipv4Address,
    Mac802Address *macAddr);




// /**
// FUNCTION   :: ConvertVariableHWAddressTo802Address
// LAYER      :: MAC
// PURPOSE    :: Convert Variable Hardware address to Mac 802 addtess
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + mac802Addr : Mac802Address* : Pointer to mac 802 address structure
// RETURN     :: Bool :
// **/
BOOL ConvertVariableHWAddressTo802Address(
            Node *node,
            MacHWAddress* macHWAddr,
            Mac802Address* mac802Addr);


 /**
// FUNCTION   :: Convert802AddressToVariableHWAddress
// LAYER      :: MAC
// PURPOSE    :: Convert Mac 802 addtess to Variable Hardware address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + mac802Addr : Mac802Address* : Pointer to mac 802 address structure
// RETURN     :: Bool :
// **/
void Convert802AddressToVariableHWAddress (
            Node *node,
            MacHWAddress* macHWAddr,
            Mac802Address* mac802Addr);

// /**
// FUNCTION   :: MAC_CopyMacHWAddress
// LAYER      :: MAC
// PURPOSE    ::  Copies Hardware address address
// PARAMETERS ::
// + destAddr : MacHWAddress* : structure to destination hardware address
// + srcAddr : MacHWAddress* : structure to source hardware address
// RETURN     :: void : NULL
// **/

void MAC_CopyMacHWAddress(
            MacHWAddress* destAddr,
            MacHWAddress* srcAddr);


/**
// FUNCTION   :: MAC_CopyMacHWAddress
// LAYER      :: MAC
// PURPOSE    :: Convert Mac 802 addtess to Variable Hardware address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + mac802Addr : Mac802Address* : Pointer to mac 802 address structure
// RETURN     :: Bool :
// **/
void MAC_CopyMac802Address (
            Mac802Address* destAddr,
            Mac802Address* srcAddr);


// /**
// FUNCTION   :: DefaultMac802AddressToIpv4Address
// LAYER      :: MAC
// PURPOSE    :: Retrieve  IP address from.Mac802Address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : Mac802Address* : Pointer to hardware address structure
// RETURN     :: NodeAddress : Ipv4 Address
// **/
NodeAddress DefaultMac802AddressToIpv4Address (
            Node *node,
            Mac802Address* macAddr);

/**
// FUNCTION   :: MAC_IsMyAddress
// LAYER      :: MAC
// PURPOSE    :: Check if mac address is of node.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : int : Interface Index
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: Bool : True if mac address matches
// **/
BOOL MAC_IsMyAddress(
    Node* node,
    int interfaceIndex,
    MacHWAddress* macAddr);

/**
// FUNCTION   :: MAC_IsMyAddress
// LAYER      :: MAC
// PURPOSE    :: Check if mac address is of node. Overloaded function
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: Bool : True if mac address matches with any interface
// **/
BOOL MAC_IsMyAddress(
    Node* node,
    MacHWAddress* macAddr);



/**
// FUNCTION   :: MAC_IsIdenticalMac802Address
// LAYER      :: MAC
// PURPOSE    :: Compares two MAC addresses
// PARAMETERS ::
// + macAddr1         : Mac802Address* macAddr1 : Mac Address
// + macAddr2         : Mac802Address* macAddr2 : Mac Address
// RETURN   :: Bool :
// **/
BOOL MAC_IsIdenticalMac802Address(
    Mac802Address *macAddr1,
    Mac802Address *macAddr2);

// /**
// FUNCTION   :: IPv4AddressToHWAddress
// LAYER      :: MAC
// PURPOSE    :: Converts IP address.To MacHWAddress
// PARAMETERS ::
// + node           : Node* : Pointer to Node structure
// + interfaceIndex : int : interfcae index of a node
// + msg : Message* : Message pointer
// + ipv4Address    : NodeAddress : Ipv4 address from which the
//                               hardware address resolved.
//+ macAddr        : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: BOOL : Returns False when conversion fails
// **/
BOOL IPv4AddressToHWAddress(
    Node *node,
    int interfaceIndex,
    Message* msg,
    NodeAddress ipv4Address,
    MacHWAddress* macAddr);

// /**
// FUNCTION   :: MacHWAddressToIpv4Address
// LAYER      :: MAC
// PURPOSE    :: This functions converts variable length Mac address
//               to IPv4 address It checks the type of hardware address
//               and based on that conversion is done.
// PARAMETERS  ::
// + node       : Node * : Pointer to node which indicates the host
// + interfaceIndex : int : Interface index of a node
// + macAddr    : MacHWAddress* : Pointer to MacHWAddress Structure.
//
// RETURN     ::  NodeAddress : IP address
// **/
NodeAddress MacHWAddressToIpv4Address(
                Node *node,
                int interfaceIndex,
                MacHWAddress* macAddr);

// /**
// FUNCTION   :: decToHex
// LAYER      :: MAC
// PURPOSE    :: Convert one byte decimal number to hex number.
// PARAMETERS  ::
// + dec       : int : decimal number
// RETURN     :: char* : return correspondig hex digit string for
//                      one byte decimal number
// **/
char* decToHex(int dec);

// /**
// FUNCTION   :: MAC_FourByteMacAddressToVariableHWAddress
// LAYER      :: MAC
// PURPOSE:   ::  Convert 4 byte address to the variable hardware address
// PARAMETERS ::
// + node   : Node * : Pointer to node which indicates the host
// + interfaceIndex : int : Interface index of a node
// + macAddr : MacHWAddress * : Pointer to source MacHWAddress Structure
// + nodeAddr : NodeAddress : Ip address
// RETURN      :: void :
// **/
void MAC_FourByteMacAddressToVariableHWAddress(
                    Node* node,
                    int interfaceIndex,
                    MacHWAddress* macAddr,
                    NodeAddress nodeAddr);

// /**
// FUNCTION   :: MAC_VariableHWAddressToFourByteMacAddress
// LAYER      :: MAC
// PURPOSE    :: Retrieve  IP address from.MacHWAddress of type
//               IPV4_LINKADDRESS
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: NodeAddress : Ipv4 Address
// **/
NodeAddress MAC_VariableHWAddressToFourByteMacAddress (
    Node* node,
    MacHWAddress* macAddr);

// /**
// FUNCTION :: GetBroadCastAddress
// LAYER    :: MAC
// PURPOSE  :: Returns Broadcast Address of an interface
// PARAMETERS ::
// + node : Node* : Pointer to a node
// + interfaceIndex : int : Interface of a node
// RETURN  :: MacHWAddress : Broadcast mac address of a interface
// **/
MacHWAddress GetBroadCastAddress(Node *node, int InterfaceIndex);

// /**
// FUNCTION :: GetMacHWAddress
// LAYER    :: MAC
// PURPOSE ::  Returns MacHWAddress of an interface
// PARAMETERS ::
// + node : Node* : Pointer to a node
// + interfaceIndex : int : inetrface of a node
// RETURN :: MacHWAddress : Mac address of a interface
// **/
MacHWAddress GetMacHWAddress(Node* node, int interfaceIndex);

// /**
// FUNCTION :: MacGetInterfaceIndexFromMacAddress
// PURPOSE ::  Returns interfaceIndex at which Macaddress is configured
// PARAMETERS ::
// + node : Node* : Pointer to a node
// + macAddr : MacHWAddress: Mac Address of a node
// RETURN :: int : interfaceIndex of node
// **/
int MacGetInterfaceIndexFromMacAddress(Node* node, MacHWAddress& macAddr);

// /**
// FUNCTION :: MacGetInterfaceIndexFromMacAddress
// PURPOSE  ::  Returns interfaceIndex at which Macaddress is configured
// PARAMETERS   ::
// + node : Node* : Pointer to a node
// + macAddr : Mac802Address : Mac Address of a node
// RETURN :: int : interfaceIndex of node
// **/
int MacGetInterfaceIndexFromMacAddress(Node* node, Mac802Address& macAddr);

// /**
// FUNCTION :: MacGetInterfaceIndexFromMacAddress
// PURPOSE  ::  Returns interfaceIndex at which Macaddress is configured
// PARAMETERS ::
// + node : Node* : Pointer to a node
// + macAddr : NodeAddress : Mac Address of a node
// RETURN :: int : interfaceIndex of node
// **/
int MacGetInterfaceIndexFromMacAddress(Node* node, NodeAddress macAddr);


class D_Fault : public D_Object
{
    private:
        Node* m_Node;
        int m_InterfaceIndex;
        BOOL m_IsFault;

    public:
        D_Fault(Node* node, int interfaceIndex) : D_Object(D_VARIABLE)
        {
            readable = TRUE;
            executable = TRUE;
            m_Node = node;
            m_InterfaceIndex = interfaceIndex;
            m_IsFault = FALSE;
        }

        void ReadAsString(std::string& out);
        void ExecuteAsString(const std::string& in, std::string& out);
};

#ifdef ADDON_NGCNMS
// /**
// FUNCTION   :: MAC_Reset
// LAYER      :: MAC
// PURPOSE    :: Reset the Mac protocols use by the node
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + InterfaceIndex : int :  interface index
// RETURN :: void :
// **/
void
MAC_Reset(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: MAC_AddResetFunctionList
// LAYER      :: MAC
// PURPOSE    :: Add which protocols in the Mac layer to be reset to a
//              fuction list pointer.
// PARAMETERS ::
// + node      : Node* :Pointer to the node
// + InterfaceIndex : int :  interface index
// + param : void* : pointer to the protocols reset function
// RETURN :: void :
// **/
void
MAC_AddResetFunctionList(Node* node, int interfaceIndex, void *param);
#endif

//--------------------------------------------------------------------------
// FUNCTION :: MacSkipLLCandMPLSHeader
// PURPOSE ::  skips MPLS and LLC header if present.
// PARAMETERS ::
// + node : Node* : Pointer to a node
// + payload : char* : pointer to the data packet.
// RETURN  :: char* : Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
char* MacSkipLLCandMPLSHeader(Node* node, char* payload);
#endif /* MAC_H */
