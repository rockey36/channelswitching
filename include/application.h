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

#include "dynamic.h"

// /**
// PACKAGE     :: APPLICATION LAYER
// DESCRIPTION :: This file describes data structures and functions used by the Application Layer.
// **/

#ifndef _APPLICATION_H_
#define _APPLICATION_H_



// /**
// CONSTANT    :: APP_DEFAULT_TOS : Ox00
// DESCRIPTION :: Application default tos value
// **/
#define APP_DEFAULT_TOS  0x00


// /**
// CONSTANT    :: APP_MAX_DATA_SIZE : IP_MAXPACKET-MSG_MAX_HDR_SIZE
// DESCRIPTION :: Maximum size of data unit
// **/
#define APP_MAX_DATA_SIZE  ((IP_MAXPACKET)-(MSG_MAX_HDR_SIZE))

// /**
// CONSTANT    :: DEFAULT_APP_QUEUE_SIZE : 640000
// DESCRIPTION :: Default size of Application layer queue (in byte)
// **/
#define DEFAULT_APP_QUEUE_SIZE                  640000

// /**
// CONSTANT    :: DEFAULT_APP_QUEUE_SIZE : 640000
// DESCRIPTION :: Default size of Application layer queue (in byte)
// **/
#define DEFAULT_APP_QUEUE_SIZE                  640000

// /**
// CONSTANT    :: PORT_TABLE_HASH_SIZE : 503
// DESCRIPTION :: Prime number indicating port table size
#define PORT_TABLE_HASH_SIZE    503
// **/

#ifdef MILITARY_RADIOS_LIB

// /**
// CONSTANT    :: MAC_LINK16_FRAG_SIZE  :    72
// DESCRIPTION :: Maximum fragment size supported by LINK16 MAC protocol.
//                For Link16, it seems the fragment size should be
//                8 * 9 bytes = 72 bytes
// **/
#define     MAC_LINK16_FRAG_SIZE            72

// /**
// CONSTANT    :: MAC_DEFAULT_INTERFACE :   0
// DESCRIPTION :: Default interface of MAC layer.
// ASSUMPTION  :: Source and Destination node must have only one
//                interface with TADIL network.
// **/
#define     MAC_DEFAULT_INTERFACE           0

// /**
// STRUCT      :: Link16GatewayData
// DESCRIPTION :: Store Link16/IP gateway forwarding table
// **/
struct Link16GatewayData;
#endif // MILITARY_RADIOS_LIB

// /**
// ENUM        :: AppType
// DESCRIPTION :: Enumerates the type of application protocol
// **/
typedef
enum
{
    APP_FTP_SERVER_DATA = 20,
    APP_FTP_SERVER = 21,
    APP_FTP_CLIENT,
    APP_TELNET_SERVER = 23,
    APP_TELNET_CLIENT,
    APP_GEN_FTP_SERVER,
    APP_GEN_FTP_CLIENT,
    APP_CBR_SERVER = 59,
    APP_CBR_CLIENT = 60,
    APP_MCBR_SERVER,
    APP_MCBR_CLIENT,
    APP_MGEN3_SENDER,
    APP_DREC3_RECEIVER,
    APP_LINK_16_CBR_SERVER,
    APP_LINK_16_CBR_CLIENT,

    APP_HTTP_SERVER = 80,
    APP_HTTP_CLIENT,
    /* Application with random distribution */
    APP_TRAFFIC_GEN_CLIENT,
    APP_TRAFFIC_GEN_SERVER,
    APP_TRAFFIC_TRACE_CLIENT,
    APP_TRAFFIC_TRACE_SERVER,
    APP_NEIGHBOR_PROTOCOL,
    APP_LOOKUP_SERVER,
    APP_LOOKUP_CLIENT,
    /* Tutorial application type */
    APP_MY_APP,
    /* Application-layer routing protocols */
    APP_ROUTING_BELLMANFORD = 519,
    APP_ROUTING_FISHEYE = 160,      // IP protocol number
    /* Static routing */
    APP_ROUTING_STATIC,

    APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4 = 179,

    APP_ROUTING_HSRP = 1985,

    APP_MPLS_LDP = 646,
    APP_ROUTING_OLSR_INRIA = 698,

    APP_MESSENGER = 702,

    APP_H323,
    APP_SIP,
    APP_VOIP_CALL_INITIATOR,
    APP_VOIP_CALL_RECEIVER,
    APP_RTP,
    APP_RTCP,
    APP_H225_RAS_MULTICAST = 1718,
    APP_H225_RAS_UNICAST,
    APP_MGEN,


    // ADDON_HELLO
    APP_HELLO,
    APP_STOCHASTIC_SERVER,
    APP_STOCHASTIC_CLIENT,

    // HLA_INTERFACE
    APP_HLA = 1800,

    //APP_ROUTING_RIP,
    APP_ROUTING_RIP=520,
    APP_ROUTING_RIPNG=1807,

#ifdef ADDON_BOEINGFCS
    APP_CES_CLIENT,
    APP_CES_SERVER,
    APP_CES_PLATFORM,
    APP_ROUTING_HSLS,
#endif // ADDON_BOEINGFCS
    APP_FORWARD,

    APP_VBR_CLIENT,
    APP_VBR_SERVER,

    APP_SUPERAPPLICATION_CLIENT,
    APP_SUPERAPPLICATION_SERVER,

    APP_ROUTING_OLSRv2_NIIGATA,

    // TUTORIAL_INTERFACE
    APP_INTERFACETUTORIAL = 6001,

#ifdef ADDON_BOEINGFCS
    APP_CES_QOS,
#endif
    APP_TRIGGER_APP,

    APP_CELLULAR_ABSTRACT,
    APP_PHONE_CALL,

    // ADDON_SDF_PDEF
    APP_THREADEDAPP_CLIENT,
    APP_THREADEDAPP_SERVER,
    APP_DYNAMIC_THREADEDAPP,

#ifdef ADDON_DB
    APP_STATSDB_AGGREGATE,
    APP_STATSDB_SUMMARY,
#endif
	APP_VIDEO_CLIENT,
	APP_VIDEO_SERVER,
	APP_RTCP_CLIENT,
	APP_RTCP_SERVER,
	APP_WBEST_CLIENT_UDP,
	APP_WBEST_SERVER_UDP,
	APP_WBEST_CLIENT_TCP,
	APP_WBEST_SERVER_TCP,
    APP_CHANSWITCH_CLIENT,
    APP_CHANSWITCH_SERVER,
    APP_PLACEHOLDER
}
AppType;

// /**
// STRUCT      :: AppInfo
// DESCRIPTION :: Information relevant to specific app layer protocol
// **/
typedef
struct app_info
{
    AppType appType;          /* type of application */
    void *appDetail;          /* statistics of the application */
    struct app_info *appNext; /* link to the next app of the node */
}
AppInfo;


// /**
// STRUCT      :: PortInfo
// DESCRIPTION :: Store port related information
// **/
struct PortInfo
{
    AppType appType;
    short portNumber;
    PortInfo* next;
};


//-------- Function Pointer definitions for multimedia signalling ---------

// /**
// FUNCTION  :: InitiateConnectionType
// PURPOSE   :: Multimedia callback funtion to open request for a TCP
//              connection from the initiating terminal
// PARAMETERS ::
// + node     : Node*       : Pointer to the node
// + voip     : void*       : Pointer to the voip application
// RETURN    :: void        : NULL
// **/
typedef
void (*InitiateConnectionType) (Node* node, void* voip);


// /**
// FUNCTION  :: TerminateConnectionType
// PURPOSE   :: Multimedia callback funtion to close TCP connection as
//              requested by VOIP application
// PARAMETERS ::
// + node     : Node* : Pointer to the node
// + voip     : void* : Pointer to the voip application
// RETURN    :: void  : NULL
// **/
typedef
void (*TerminateConnectionType)(Node* node, void* voip);


// /**
// FUNCTION  :: IsHostCallingType
// PURPOSE   :: Multimedia callback funtion to check whether node is
//              in initiator mode
// PARAMETERS ::
// + node     : Node* : Pointer to the node
// RETURN    :: BOOL  : TRUE if the node is initiator, FALSE otherwise
// **/
typedef
BOOL (*IsHostCallingType) (Node* node);


// /**
// FUNCTION  :: IsHostCalledType
// PURPOSE   :: Multimedia callback funtion to check whether node is
//              in receiver mode
// PARAMETERS ::
// + node     : Node* : Pointer to the node
// RETURN    :: BOOL  : TRUE if the node is receiver, FALSE otherwise
// **/
typedef
BOOL (*IsHostCalledType) (Node* node);


// /**
// STRUCT      :: AppMultimedia
// DESCRIPTION :: Store multimedia signalling related information
// **/
typedef struct struct_multimedia_info
{
    // MULTIMEDIA SIGNALLING related structures
    // 0-INVALID, 1-H323, 2-SIP
    unsigned char sigType;

    // currently SIP and H323 are available, H323Data and SipData
    // structure pointer is stored depending on the signalling type
    void* sigPtr;

    InitiateConnectionType  initiateFunction;
    TerminateConnectionType terminateFunction;

    IsHostCallingType       hostCallingFunction;
    IsHostCalledType        hostCalledFunction;

    double                  totalLossProbablity;
    clocktype               connectionDelay;
    clocktype               callTimeout;
} AppMultimedia;

typedef
enum
{
    MOP_APPLICATION_STATS = 0,
    MOP_STAT_CATEGORY_STATS = 1,
    MOP_DSCP_STATS = 2,
    NUM_MOP_TYPES = 3
} MopTypes;

#ifdef ADDON_NGCNMS
typedef void (*AppSetFunctionPtr)(Node*,
                                const NodeInput*);
// Set rules defined
struct
struct_app_reset_function
{
    // Corresponding set function
    AppSetFunctionPtr setFuncPtr;

    // the next match command
    struct struct_app_reset_function* next;
};

typedef struct
struct_app_reset_function_linkedList
{
    struct struct_app_reset_function* first;
    struct struct_app_reset_function* last;
} AppResetFunctionList;
#endif // ADDON_NGCNMS


// /**
// STRUCT      :: struct_app_str
// DESCRIPTION :: Details of application data structure in node
//                structure, typedef to AppData in main.h
// **/
struct struct_app_str
{
    AppInfo *appPtr;         /* pointer to the list of app info */
    PortInfo *portTable;     /* pointer to the port table */
    short    nextPortNum;    /* next available port number */
    BOOL     appStats;       /* whether app stat is enabled */
    AppType exteriorGatewayProtocol;
    BOOL routingStats;
    void *routingVar;        /* deprecated */
    void *bellmanford;
#ifdef ADDON_BOEINGFCS
    void *routingCesHsls;
#endif
    void *olsr;
    void *olsrv2;

    void *hsrp;
    void *exteriorGatewayVar;
    void *userApplicationData;

    void* voipCallReceiptList;  // Maintain a list of receipt call.
    void* rtpData;

    AppMultimedia* multimedia;

//startCellular
    void *appCellularAbstractVar;
    void *phonecallVar;
//endCellular

    /*
     * Application statistics for the node using TCP.
     */
    Int32 numAppTcpFailure;     /* # of apps terminated due to failure */

    /* Used to determine unique client/server pair. */
    Int32 uniqueId;

    /*
     * User specified session parameters.
     */
    clocktype telnetSessTime;/* duration of a telnet session */

    void *triggerAppData;

#ifdef MILITARY_RADIOS_LIB
    Link16GatewayData* link16Gateway;   // data for link16 gateway
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_BOEINGFCS
    void *networkCesQosData;
    D_BOOL useNetworkCesQos;
    //BOOL useFcsQos;
#endif
#ifdef ADDON_NGCNMS
    void *mopData;
    AppResetFunctionList *resetFunctionList;
#endif

    void **hopCountPerDescriptorList;
    D_BOOL *hopCountPerDescriptorStats;

    void **tputPerDescriptorList;
    D_BOOL *tputPerDescriptorStats;

    void **e2eDelayPerDescriptorList;
    D_BOOL *e2eDelayPerDescriptorStats;

    void **jitterPerDescriptorList;
    D_BOOL *jitterPerDescriptorStats;

    void **mcrPerDescriptorList;
    D_BOOL *mcrPerDescriptorStats;

  // superapplication client pointer list
  // pointer to a structure which is a vector of client list
  void *clientPtrList;

};

#define APP_TIMER_SEND_PKT     (1)  /* for sending a packet */
#define APP_TIMER_UPDATE_TABLE (2)  /* for updating local tables */
#define APP_TIMER_CLOSE_SESS   (3)  /* for closing a session */

// /**
// STRUCT      :: AppTimer
// DESCRIPTION :: Timer structure used by applications
// **/
typedef
struct app_timer
{
    int type;        /* timer type */
    int connectionId;         /* which connection this timer is meant for */
    unsigned short sourcePort;         /* the port of the session this timer is meant for */
    NodeAddress address;      /* address and port combination identify the session */
} AppTimer;

// STRUCT      :: AppGenericTimer
// DESCRIPTION :: Timer structure used by applications
// Added to support generic address for IPv6
typedef
struct app_generic_timer
{
    int type;        /* timer type */
    int connectionId;         /* which connection this timer is meant for */
    unsigned short sourcePort;         /* the port of the session this timer is meant for */
    Address address;      /* address and port combination identify the session */
} AppGenericTimer;

#ifdef ADDON_NGCNMS
void
APP_Reset(Node *node, const NodeInput *nodeInput);

void APP_AddResetFunctionList(
        Node* node, void *param);
#endif

#endif /* _APPLICATION_H_ */
