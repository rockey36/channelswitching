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
// PACKAGE     :: API
// DESCRIPTION :: This file enumerates the basic message/events exchanged
// during the simulation process and the various layer functions (initialize, finalize, and event handling
// functions) and other miscellaneous routines and data structure definitions.
// **/

#ifndef API_H
#define API_H


#include "clock.h"
#include "qualnet_error.h"
#include "main.h"
#include "random.h"

#include "fileio.h"
#include "coordinates.h"
#include "splaytree.h"
#include "mapping.h"

#include "propagation.h"
#include "phy.h"
#include "network.h"
#include "mac.h"
#include "transport.h"
#include "application.h"
#include "user.h"

#include "message.h"

#include "gui.h"

#include "node.h"
#include "terrain.h"
#include "mobility.h"
#include "trace.h"

#define CHANSWITCH_TX_CLIENT 1
#define CHANSWITCH_RX_SERVER 2
#define CHANSWITCH_SINR_TX_CLIENT 3
#define CHANSWITCH_SINR_RX_SERVER 4


// /**
// ENUM        :: MESSAGE/EVENT
// DESCRIPTION :: Event/message types exchanged in the simulation
// **/
enum
{
    /* Special message types used for internal design. */
    MSG_SPECIAL_Timer                          = 0,

    /* Message Types for Environmental Effects/Weather */
    MSG_WEATHER_MobilityTimerExpired           = 50,

    /* Message Types for Channel layer */
    MSG_PROP_SignalArrival                     = 100,
    MSG_PROP_SignalEnd                         = 101,
    MSG_PROP_SignalReleased                    = 102,
    // Phy Connectivity Database
    MSG_PROP_SAMPLE_PATHLOSS                   = 103,
    MSG_PROP_SAMPLE_CONNECT                    = 104,
    // end PHY Connectivity Database
    /* Message Types for Phy layer */
    MSG_PHY_TransmissionEnd                    = 200,
    MSG_PHY_CollisionWindowEnd,

    /* Message Types for MAC layer */
    MSG_MAC_UMTS_LAYER2_HandoffPacket          = 287,
    MSG_MAC_DOT11_Scan_Start_Timer             = 288,
    MSG_MAC_DOT11_Reassociation_Start_Timer    = 289,
    MSG_MAC_DOT11_Authentication_Start_Timer   = 290,
    MSG_MAC_DOT11_Beacon_Wait_Timer            = 291,
    MSG_MAC_DOT11_Probe_Delay_Timer            = 292,
    MSG_MAC_DOT11_Active_Scan_Long_Timer       = 293,
    MSG_MAC_DOT11_Active_Scan_Short_Timer      = 294,
    MSG_MAC_DOT11_Station_Inactive_Timer       = 295,
    MSG_MAC_DOT11_Management_Authentication    = 296,
    MSG_MAC_DOT11_Management_Association       = 297,
    MSG_MAC_DOT11_Management_Probe             = 298,
    MSG_MAC_DOT11_Management_Reassociation     = 299,
    MSG_MAC_FromPhy                            = 300,
    MSG_MAC_FromNetwork                        = 301,
    MSG_MAC_ReportChannelStatus                = 302,
    MSG_MAC_TransmissionFinished               = 303,
    MSG_MAC_TimerExpired                       = 304,
    MSG_MAC_LinkToLink                         = 306,
    MSG_MAC_FrameStartOrEnd                    = 307,
    MSG_MAC_FromChannelForPacket               = 308,
    MSG_MAC_StartTransmission                  = 309,
    MSG_MAC_JamSequence                        = 310,
    MSG_MAC_StartFault                         = 311,
    MSG_MAC_EndFault                           = 312,
    MSG_MAC_FullDupToFullDup                   = 313,

    MSG_MAC_StartBGTraffic                     = 9333,
    MSG_MAC_EndBGTraffic                       = 9334,

    MSG_MAC_GroundToSatellite                  = 314,
    MSG_MAC_SatelliteToGround                  = 315,
    MSG_MAC_SwitchTick                         = 316,
    MSG_MAC_FromBackplane                      = 317,
    MSG_MAC_GarpTimerExpired                   = 318,

    MSG_MAC_802_11_Beacon                      = 320,
    MSG_MAC_802_11_CfpEnd                      = 321,

    MSG_MAC_SatTsm                             = 322,
    MSG_MAC_SatTsmShim                         = 323,

    // Message Types of DOT11 MAC
    MSG_MAC_DOT11_Enable_Management_Timer      = 329,
    MSG_MAC_DOT11_Beacon                       = 330,
    MSG_MAC_DOT11_CfpBeacon                    = 331,
    MSG_MAC_DOT11_CfpEnd                       = 332,
    MSG_MAC_DOT11_Management                   = 333,

    //Message Type for APP channel switching
    MSG_APP_TxChannelSelectionTimeout           = 376,
    MSG_APP_InitiateChannelScanRequest          = 377,
    MSG_MAC_DOT11_ChangeChannelRequest          = 378,
    MSG_MAC_FromAppChangeChannelRequest         = 379,
    MSG_APP_FromMac_MACAddressRequest           = 380,
    MSG_MAC_DOT11_MACAddressRequest             = 381,
    MSG_MAC_FromAppMACAddressRequest            = 382,
    MSG_APP_TxProbeWfAckTimeout                 = 383,
    MSG_APP_TxChangeWfAckTimeout                = 384,
    MSG_APP_TxVerifyWfAckTimeout                = 385,
    MSG_APP_RxProbeAckDelay                     = 386,
    MSG_APP_RxChangeAckDelay                    = 387,

	// Message Type for DOT11 channel switching
    MSG_MAC_DOT11_ChanswitchRequest             = 388,
    MSG_MAC_FromAppChanswitchRequest            = 389,
    MSG_APP_FromMacTxScanFinished               = 390,                   
    MSG_APP_FromMacRxScanFinished               = 391,
    MSG_MAC_DOT11_ChanSwitchRxReturnPrevChannel = 392,
    MSG_MAC_DOT11_ChanSwitchInitialDelay        = 393,
    MSG_MAC_DOT11_ChanSwitchRxProbe             = 394,
    MSG_MAC_DOT11_ChanSwitchTxDelay             = 395,
    MSG_MAC_DOT11_ChanSwitchSinrProbeChanSwitch = 396,
    MSG_MAC_DOT11_ChanSwitchWaitForTX           = 397,
	MSG_MAC_DOT11_ChanSwitchSinrProbe		    = 398,
	MSG_MAC_DOT11_ChanSwitchTimerExpired	    = 399,
//---------------------------Power-Save-Mode-Updates---------------------//
    MSG_BATTERY_TimerExpired                   = 334,
    MSG_MAC_DOT11_ATIMWindowTimerExpired       = 338,
    MSG_MAC_DOT11_ATIMPeriodExpired            = 339,
    MSG_MAC_DOT11_PSStartListenTxChannel       = 340,
//---------------------------Power-Save-Mode-End-Updates-----------------//

    // TADIL events
    MSG_MAC_TADIL_FromAppSend                  = 335,
    MSG_MAC_TADIL_Timer                        = 336,
    MSG_APP_FromMac                            = 337,

    // ALE events
    MSG_MAC_AleChannelScan                     = 341,
    MSG_MAC_AleChannelCheck                    = 342,
    MSG_MAC_AleStartCall                       = 343,
    MSG_MAC_AleEndCall                         = 344,
    MSG_MAC_AleCallSetupTimer                  = 345,
    MSG_MAC_AleSlotTimer                       = 346,
    MSG_MAC_AleClearCall                       = 347,



    /* Message Types for Network layer */
    MSG_NETWORK_BrpDeleteQueryEntry            = 350,
#ifdef ADDON_BOEINGFCS
     /* Message types for routing - Hsls */
    MSG_APP_CES_HSLS_Hello                         = 360,
    MSG_APP_CES_HSLS_T_P                           = 361,
    MSG_APP_CES_HSLS_T_E                           = 362,
    MSG_APP_CES_HSLS_CheckNeighbor                 = 363,
#endif

    MSG_NETWORK_FromApp                        = 400,
    MSG_NETWORK_FromMac                        = 401,
    MSG_NETWORK_FromTransportOrRoutingProtocol = 402,
    MSG_NETWORK_DelayedSendToMac               = 403,
    MSG_NETWORK_RTBroadcastAlarm               = 404,
    MSG_NETWORK_CheckTimeoutAlarm              = 405,
    MSG_NETWORK_TriggerUpdateAlarm             = 406,
    MSG_NETWORK_InitiateSend                   = 407,
    MSG_NETWORK_FlushTables                    = 408,
    MSG_NETWORK_CheckAcked                     = 409,
    MSG_NETWORK_CheckReplied                   = 410,
    MSG_NETWORK_DeleteRoute                    = 411,
    MSG_NETWORK_BlacklistTimeout               = 412,
    MSG_NETWORK_SendHello                      = 413,
    MSG_NETWORK_PacketTimeout                  = 414,
    MSG_NETWORK_RexmtTimeout                   = 415,
    MSG_NETWORK_PrintRoutingTable              = 416,
    MSG_NETWORK_Ip_Fragment                    = 417,
#ifdef MILITARY_RADIOS_LIB
    MSG_EPLRS_DelayedSendToMac                 = 418,
#endif

    MSG_NETWORK_JoinGroup                      = 420,
    MSG_NETWORK_LeaveGroup                     = 421,
    MSG_NETWORK_SendData                       = 422,
    MSG_NETWORK_SendRequest                    = 423,
    MSG_NETWORK_SendReply                      = 424,
    MSG_NETWORK_CheckFg                        = 425,

    MSG_NETWORK_Retx                           = 430,

    MSG_NETWORK_PacketDropped                  = 440,
    MSG_NETWORK_CheckRouteTimeout              = 441,
    MSG_NETWORK_CheckNeighborTimeout           = 442,
    MSG_NETWORK_CheckCommunityTimeout          = 443,

    /* Messages Types special for IP */
    MSG_NETWORK_BuffTimerExpired               = 450,
    MSG_NETWORK_Backplane                      = 451,

    /* Message Types for IPSec */
    MSG_NETWORK_IPsec                          = 455,

    MSG_NETWORK_IgmpData                       = 460,
    MSG_NETWORK_IgmpQueryTimer                 = 461,
    MSG_NETWORK_IgmpOtherQuerierPresentTimer   = 462,
    MSG_NETWORK_IgmpGroupReplyTimer            = 463,
    MSG_NETWORK_IgmpGroupMembershipTimer       = 464,
    MSG_NETWORK_IgmpJoinGroupTimer             = 465,
    MSG_NETWORK_IgmpLeaveGroupTimer            = 466,
    MSG_NETWORK_IgmpLastMemberTimer            = 467,

    /* Messages Types special for ICMP */
    MSG_NETWORK_IcmpEcho                       = 470,
    MSG_NETWORK_IcmpTimeStamp                  = 471,
    MSG_NETWORK_IcmpData                       = 472,
    MSG_NETWORK_IcmpRouterSolicitation         = 473,
    MSG_NETWORK_IcmpRouterAdvertisement        = 474,
    MSG_NETWORK_IcmpValidationTimer            = 475,

    MSG_NETWORK_AccessList                     = 480,

    /* LANMAR routing timers */
    MSG_FsrlScheduleSPF                        = 489,
    MSG_FsrlNeighborTimeout                    = 490,
    MSG_FsrlIntraUpdate                        = 491,
    MSG_FsrlLMUpdate                           = 492,

    MSG_NETWORK_RegistrationRequest            = 493,
    MSG_NETWORK_RegistrationReply              = 494,
    MSG_NETWORK_MobileIpData                   = 495,
    MSG_NETWORK_RetransmissionRequired         = 496,
    MSG_NETWORK_AgentAdvertisementRefreshTimer = 497,
    MSG_NETWORK_VisitorListRefreshTimer        = 498,
    MSG_NETWORK_BindingListRefreshTimer        = 499,

    /* Message Types for Transport layer */
    MSG_TRANSPORT_FromNetwork                  = 500,
    MSG_TRANSPORT_FromAppListen                = 501,
    MSG_TRANSPORT_FromAppOpen                  = 502,
    MSG_TRANSPORT_FromAppSend                  = 503,
    MSG_TRANSPORT_FromAppClose                 = 504,
    MSG_TRANSPORT_TCP_TIMER_FAST               = 505,
    MSG_TRANSPORT_TCP_TIMER_SLOW               = 506,
    MSG_TRANSPORT_Tcp_CheckTcpOutputTimer      = 507,

    /* Message Types for Setting Timers */
    MSG_TRANSPORT_TCP_TIMER                    = 510,

    /* Messages Types for Transport layer with NS TCP */
    MSG_TCP_SetupConnection                    = 520,

    /* Messages Types for RSVP & RSVP/TE */
    MSG_TRANSPORT_RSVP_InitApp                 = 540,
    MSG_TRANSPORT_RSVP_PathRefresh             = 541,
    MSG_TRANSPORT_RSVP_HelloExtension          = 542,
    MSG_TRANSPORT_RSVP_InitiateExplicitRoute   = 543,
    MSG_TRANSPORT_RSVP_ResvRefresh             = 544,

    /* Message Types for Application layer */
    MSG_APP_FromTransListenResult              = 600,
    MSG_APP_FromTransOpenResult                = 601,
    MSG_APP_FromTransDataSent                  = 602,
    MSG_APP_FromTransDataReceived              = 603,
    MSG_APP_FromTransCloseResult               = 604,
    MSG_APP_TimerExpired                       = 605,
    MSG_APP_SessionStatus                      = 606,

    /* Messages Types for Application layer from UDP */
    MSG_APP_FromTransport                      = 610,

    /* Messages Types for Application layer from NS TCP */
    MSG_APP_NextPkt                            = 620,
    MSG_APP_SetupConnection                    = 621,

    /* Messages Type for Application layer directly from IP */
    MSG_APP_FromNetwork                        = 630,

    /* MGEN specific event */
    MSG_APP_DrecEvent                          = 645,


    /* Message Types for MPLS LDP */
    MSG_APP_MplsLdpKeepAliveTimerExpired       = 650,
    MSG_APP_MplsLdpSendKeepAliveDelayExpired   = 651,
    MSG_APP_MplsLdpSendHelloMsgTimerExpired    = 652,
    MSG_APP_MplsLdpLabelRequestTimerExpired    = 653,
    MSG_APP_MplsLdpFaultMessageDOWN            = 654,
    MSG_APP_MplsLdpFaultMessageUP              = 655,

    /* Message Types for Promiscuous Routing Algorithms */
    MSG_ROUTE_FromTransport                    = 700,
    MSG_ROUTE_FromNetwork                      = 701,

#ifdef ADDON_BOEINGFCS
    /* Messages for FCS QoS */
    MSG_APP_UpdateTable                        = 702,
    MSG_APP_UpdateIngress                      = 703,
    MSG_APP_GotIngressPacket                   = 704,
    MSG_APP_RemoveFlow                         = 705,
    MSG_APP_RemarkIngress                      = 706,
        MSG_APP_NewSamplePeriod                    = 707,
#endif
    /* Message Types for Routing - BGP */
    MSG_APP_BGP_KeepAliveTimer                 = 710,
    MSG_APP_BGP_HoldTimer                      = 711,
    MSG_APP_BGP_ConnectRetryTimer              = 712,
    MSG_APP_BGP_StartTimer                     = 713,
    MSG_APP_BGP_RouteAdvertisementTimer        = 714,
    MSG_APP_BGP_IgpProbeTimer                  = 715,

    /* Message Types for Routing - OSPF , Q-OSPF*/
    MSG_ROUTING_OspfScheduleHello              = 720,
    MSG_ROUTING_OspfIncrementLSAge             = 721,
    MSG_ROUTING_OspfScheduleLSDB               = 722,
    MSG_ROUTING_OspfPacket                     = 723,
    MSG_ROUTING_OspfRxmtTimer                  = 724,
    MSG_ROUTING_OspfInactivityTimer            = 725,
    MSG_ROUTING_QospfSetNewConnection          = 726,
    MSG_ROUTING_QospfScheduleLSDB              = 727,
    MSG_ROUTING_QospfInterfaceStatusMonitor    = 9995,

    MSG_ROUTING_OspfWaitTimer                  = 728,
    MSG_ROUTING_OspfFloodTimer                 = 729,
    MSG_ROUTING_OspfDelayedAckTimer            = 9999,
    MSG_ROUTING_OspfScheduleSPF                = 9998,
    MSG_ROUTING_OspfNeighborEvent              = 9997,
    MSG_ROUTING_OspfInterfaceEvent             = 9996,
    MSG_ROUTING_OspfSchedASExternal            = 9994,

    /* Messages types for Multicast Routing - DVMRP */
    MSG_ROUTING_DvmrpScheduleProbe             = 730,
    MSG_ROUTING_DvmrpPeriodUpdateAlarm         = 731,
    MSG_ROUTING_DvmrpTriggeredUpdateAlarm      = 732,
    MSG_ROUTING_DvmrpNeighborTimeoutAlarm      = 733,
    MSG_ROUTING_DvmrpRouteTimeoutAlarm         = 734,
    MSG_ROUTING_DvmrpGarbageCollectionAlarm    = 735,
    MSG_ROUTING_DvmrpPruneTimeoutAlarm         = 736,
    MSG_ROUTING_DvmrpGraftRtmxtTimeOut         = 737,
    MSG_ROUTING_DvmrpPacket                    = 738,

    /* Message types for routing protocol IGRP     */
    MSG_ROUTING_IgrpBroadcastTimerExpired      = 9980,
    MSG_ROUTING_IgrpPeriodicTimerExpired       = 9981,
    MSG_ROUTING_IgrpHoldTimerExpired           = 9982,
    MSG_ROUTING_IgrpTriggeredUpdateAlarm       = 9983,

    /* Message types for routing protocol EIGRP    */
    MSG_ROUTING_EigrpHelloTimerExpired         = 1000,
    MSG_ROUTING_EigrpHoldTimerExpired          = 1001,
    MSG_ROUTING_EigrpRiseUpdateAlarm           = 1002,
    MSG_ROUTING_EigrpRiseQueryAlarm            = 1003,
    MSG_ROUTING_EigrpStuckInActiveRouteTimerExpired = 1004,

    /* Message types for Bellmanford. */
    MSG_APP_PeriodicUpdateAlarm                = 750,
    MSG_APP_CheckRouteTimeoutAlarm             = 751,
    MSG_APP_TriggeredUpdateAlarm               = 752,


    /* Used for debugging purposes. */
    MSG_APP_PrintRoutingTable                  = 770,

    /* Message types for routing - Fisheye */
    MSG_APP_FisheyeNeighborTimeout             = 780,
    MSG_APP_FisheyeIntraUpdate                 = 781,
    MSG_APP_FisheyeInterUpdate                 = 782,

    /* Messages for HSRP */
    MSG_APP_HelloTimer                         = 785,
    MSG_APP_ActiveTimer                        = 786,
    MSG_APP_StandbyTimer                       = 787,

    /* Message types for OLSR */
    MSG_APP_OlsrPeriodicHello                  = 790,
    MSG_APP_OlsrPeriodicTc                     = 791,
    MSG_APP_OlsrNeighHoldTimer                 = 792,
    MSG_APP_OlsrTopologyHoldTimer              = 793,
    MSG_APP_OlsrDuplicateHoldTimer             = 794,
    MSG_APP_OlsrPeriodicMid                    = 795,
    MSG_APP_OlsrPeriodicHna                    = 796,
    MSG_APP_OlsrMidHoldTimer                   = 797,
    MSG_APP_OlsrHnaHoldTimer                   = 798,

    /* Message Types for Application Layer CBR */
    MSG_APP_CBR_NEXT_PKT                       = 800,

    /* Message Types for Netwars*/
    MSG_APP_NW_SELF_INTERRUPT                  = 815,
    MSG_APP_NW_IER                             = 816,

    // Message types for H.323
    MSG_APP_H323_Connect_Timer                 = 820,
    MSG_APP_H323_Call_Timeout                  = 821,

    // Message types for H.225Ras
    MSG_APP_H225_RAS_GRQ_PeriodicRefreshTimer  = 830,   //ras
    MSG_APP_H225_RAS_GatekeeperRequestTimer    = 831,   //ras
    MSG_APP_H225_RAS_RegistrationRequestTimer  = 832,   //ras
    MSG_APP_H225_SETUP_DELAY_Timer             = 833,   //ras

    // Message types for SIP
    MSG_APP_SipConnectionDelay                 = 840,
    MSG_APP_SipCallTimeOut                     = 841,

    //Message Type RTP Jitter Buffer
    MSG_APP_JitterNominalTimer                 = 850,
    MSG_APP_TalkspurtTimer                     = 851,

    MSG_APP_RTP                                = 250,
    MSG_RTP_TerminateSession                   = 251,
    MSG_RTP_InitiateNewSession                 = 252,
    MSG_RTP_SetJitterBuffFirstPacketAsTrue     = 253,

    /* Message types for Messenger App */
    MSG_APP_SendRequest                        = 860,
    MSG_APP_SendResult                         = 861,
    MSG_APP_SendNextPacket                     = 862,
    MSG_APP_ChannelIsIdle                      = 863,
    MSG_APP_ChannelIsBusy                      = 864,
    MSG_APP_ChannelInBackoff                   = 865,

    /* Message types for Multicast Routing - PIM-DM */
    MSG_ROUTING_PimScheduleHello                 = 870,
    MSG_ROUTING_PimDmNeighborTimeOut             = 871,
    MSG_ROUTING_PimDmPruneTimeoutAlarm           = 872,
    MSG_ROUTING_PimDmAssertTimeOut               = 873,
    MSG_ROUTING_PimDmDataTimeOut                 = 874,
    MSG_ROUTING_PimDmGraftRtmxtTimeOut           = 875,
    MSG_ROUTING_PimDmJoinTimeOut                 = 876,
    MSG_ROUTING_PimDmScheduleJoin                = 877,
    MSG_ROUTING_PimPacket                        = 878,

    /* Message types for PIM-SM. */
    MSG_ROUTING_PimSmScheduleHello             = 880,
    MSG_ROUTING_PimSmScheduleCandidateRP       = 881,
    MSG_ROUTING_PimSmCandidateRPTimeOut        = 882,
    MSG_ROUTING_PimSmBootstrapTimeOut          = 883,
    MSG_ROUTING_PimSmRegisterStopTimeOut       = 884,
    MSG_ROUTING_PimSmExpiryTimerTimeout        = 885,
    MSG_ROUTING_PimSmPrunePendingTimerTimeout  = 886,
    MSG_ROUTING_PimSmJoinTimerTimeout          = 887,
    MSG_ROUTING_PimSmOverrideTimerTimeout      = 888,
    MSG_ROUTING_PimSmAssertTimerTimeout        = 889,
    MSG_ROUTING_PimSmKeepAliveTimeOut          = 890,

    /*Message for route redstribution*/
    MSG_NETWORK_RedistributeData                = 895,

    /* Message Types for GSM */
    MSG_MAC_GsmSlotTimer                              = 900,
    MSG_MAC_GsmCellSelectionTimer                     = 901,
    MSG_MAC_GsmCellReSelectionTimer                   = 902,
    MSG_MAC_GsmRacchTimer                             = 903,
    MSG_MAC_GsmIdleSlotStartTimer                     = 904,
    MSG_MAC_GsmIdleSlotEndTimer                       = 905,
    MSG_MAC_GsmHandoverTimer                          = 906,
    MSG_MAC_GsmTimingAdvanceDelayTimer                = 907,
    MSG_NETWORK_GsmCallStartTimer                     = 910,
    MSG_NETWORK_GsmCallEndTimer                       = 911,
    MSG_NETWORK_GsmSendTrafficTimer                   = 912,
    MSG_NETWORK_GsmHandoverTimer                      = 913,

    // Call Control Timers
    MSG_NETWORK_GsmAlertingTimer_T301                 = 920,
    MSG_NETWORK_GsmCallPresentTimer_T303              = 921,
    MSG_NETWORK_GsmDisconnectTimer_T305               = 922,
    MSG_NETWORK_GsmReleaseTimer_T308                  = 923,
    MSG_NETWORK_GsmCallProceedingTimer_T310           = 924,
    MSG_NETWORK_GsmConnectTimer_T313                  = 925,
    MSG_NETWORK_GsmModifyTimer_T323                   = 926,
    MSG_NETWORK_GsmCmServiceAcceptTimer_UDT1          = 927,
    // Radio Resource Management Timers
    MSG_NETWORK_GsmImmediateAssignmentTimer_T3101     = 930,
    MSG_NETWORK_GsmHandoverTimer_T3103                = 931,
    MSG_NETWORK_GsmMsChannelReleaseTimer_T3110        = 932,
    MSG_NETWORK_GsmBsChannelReleaseTimer_T3111        = 933,
    MSG_NETWORK_GsmPagingTimer_T3113                  = 934,
    MSG_NETWORK_GsmChannelRequestTimer                = 935,

    // Mobility Management Timers
    MSG_NETWORK_GsmLocationUpdateRequestTimer_T3210   = 941,
    MSG_NETWORK_GsmLocationUpdateFailureTimer_T3211   = 942,
    MSG_NETWORK_GsmPeriodicLocationUpdateTimer_T3212  = 943,
    MSG_NETWORK_GsmCmServiceRequestTimer_T3230        = 944,
    MSG_NETWORK_GsmTimer_T3240                        = 945,
    MSG_NETWORK_GsmTimer_T3213                        = 946,
    MSG_NETWORK_GsmTimer_BSSACCH                      = 947,

    // Message type for IPv6 (reserved from 950 to 999)
    MSG_NETWORK_Icmp6_NeighSolicit            = 950,
    MSG_NETWORK_Icmp6_NeighAdv                = 951,
    MSG_NETWORK_Icmp6_RouterSolicit           = 952,
    MSG_NETWORK_Icmp6_RouterAdv               = 953,
    MSG_NETWORK_Icmp6_Redirect                = 954,
    MSG_NETWORK_Icmp6_Error                   = 955,
    // IPV6 EVENT TYPE
    MSG_NETWORK_Ipv6_InitEvent                = 956,
    MSG_NETWORK_Ipv6_Rdvt                     = 957,
    MSG_NETWORK_Ipv6_Ndadv6                   = 958,
    MSG_NETWORK_Ipv6_Ndp_Process              = 959,
    MSG_NETWORK_Ipv6_RetryNeighSolicit        = 960,
    MSG_NETWORK_Ipv6_Fragment                 = 961,
    MSG_NETWORK_Ipv6_RSol                     = 962,
    MSG_NETWORK_Ipv6_NdAdvt                   = 963,

    MSG_NETWORK_Ipv6_Update_Address           = 999,

    MSG_ATM_ADAPTATION_SaalEndTimer           = 1100,
    MSG_NETWORK_FromAdaptation                = 1101,

    MSG_MAC_DOT11s_HwmpActiveRouteTimeout      = 1110,
    MSG_MAC_DOT11s_HwmpRreqReplyTimeout        = 1111,
    MSG_MAC_DOT11s_HwmpSeenTableTimeout        = 1112,
    MSG_MAC_DOT11s_HwmpDeleteRouteTimeout      = 1113,
    MSG_MAC_DOT11s_HwmpBlacklistTimeout        = 1114,

    MSG_MAC_DOT11s_HwmpTbrEventTimeout         = 1120,
    MSG_MAC_DOT11s_HwmpTbrRannTimeout          = 1121,
    MSG_MAC_DOT11s_HwmpTbrMaintenanceTimeout   = 1122,
    MSG_MAC_DOT11s_HmwpTbrRrepTimeout          = 1123,

    MSG_MAC_DOT11s_AssocRetryTimeout           = 1130,
    MSG_MAC_DOT11s_AssocOpenTimeout            = 1131,
    MSG_MAC_DOT11s_AssocCancelTimeout          = 1132,
    MSG_MAC_DOT11s_LinkSetupTimer              = 1133,
    MSG_MAC_DOT11s_LinkStateTimer              = 1134,
    MSG_MAC_DOT11s_PathSelectionTimer          = 1135,
    MSG_MAC_DOT11s_InitCompleteTimeout         = 1136,
    MSG_MAC_DOT11s_MaintenanceTimer            = 1137,
    MSG_MAC_DOT11s_PannTimer                   = 1138,
    MSG_MAC_DOT11s_PannPropagationTimeout      = 1139,
    MSG_MAC_DOT11s_LinkStateFrameTimeout       = 1140,
    MSG_MAC_DOT11s_QueueAgingTimer             = 1141,

/* Message Types Added Via Designer */

//StartNDP
    NETWORK_NdpPeriodicHelloTimerExpired,
    NETWORK_NdpHoldTimerExpired,
//EndNDP

//StartVBR
//EndVBR

//StartARP
    MSG_NETWORK_ArpTickTimer,
    MSG_NETWORK_ARPRetryTimer,
//EndARP
//StartAloha
    MSG_MAC_CheckTransmit,
//EndAloha
//StartGenericMac
    MAC_GenTimerExpired,
    MAC_GenExpBackoff,
//EndGenericMac
//StartSuperApplication
    MSG_APP_TALK_TIME_OUT,
//EndSuperApplication
//StartRIP
    MSG_APP_RIP_RegularUpdateAlarm,
    MSG_APP_RIP_TriggeredUpdateAlarm,
    MSG_APP_RIP_RouteTimerAlarm,
//EndRIP
//StartRIPng
    MSG_APP_RIPNG_RegularUpdateAlarm,
    MSG_APP_RIPNG_RouteTimerAlarm,
    MSG_APP_RIPNG_TriggeredUpdateAlarm,
//EndRIPng
//StartIARP
    ROUTING_IarpBroadcastTimeExpired,
    ROUTING_IarpRefraceTimeExpired,
//EndIARP
//StartZRP
//EndZRP
//StartIERP
    ROUTING_IerpRouteRequestTimeExpired,
    ROUTING_IerpFlushTimeOutRoutes,
//EndIERP
//InsertPatch EVENT_TYPES

    /* Message Types for OPNET support */
    MSG_OPNET_SelfTimer                        = 1700,

    MSG_EXTERNAL_HLA_HierarchyMobility         = 1800,
    MSG_EXTERNAL_HLA_ChangeMaxTxPower          = 1801,
    MSG_EXTERNAL_HLA_AckTimeout                = 1802,
    MSG_EXTERNAL_HLA_CheckMetricUpdate         = 1803,
//HlaLink11Begin
    MSG_EXTERNAL_HLA_SendRtss                  = 1804,
//HlaLink11End
    MSG_EXTERNAL_HLA_StartMessengerForwarded   = 1805,
    MSG_EXTERNAL_HLA_SendRtssForwarded         = 1806,
    MSG_EXTERNAL_HLA_CompletedMessengerForwarded = 1807,

    MSG_EXTERNAL_DIS_HierarchyMobility         = 1900,
    MSG_EXTERNAL_DIS_ChangeMaxTxPower          = 1901,

#ifdef ADDON_BOEINGFCS
    /* FCS data structure */
    MSG_BoeingFcsSetSlaveSlot,
    MSG_BoeingFcsDnsUpdate,
    MSG_BoeingFcsDnsFromHostUpdate,
    MSG_BoeingFcsHostRoutingUpdate,
    MSG_BoeingFcsHelloBroadcastTimeout,
    MSG_RoutingCesMalsrLsuBroadcastTimeout,
    MSG_RoutingCesSrwBroadcastTimeout,
    MSG_RoutingCesRospfLsuBroadcastTimeout,
    MSG_NetworkCesClusterHelloBroadcastTimeout,
    MSG_NetworkCesRegionRapHelloBroadcastTimeout,
    MSG_APP_CES_COMM_Instantiate,
    MSG_APP_CES_COMM_Listen,
    MSG_APP_CES_COMM_CommEffectsRequest,
    MSG_BoeingFcsDelayedMessage,
    MSG_BoeingFcsMulticastGroup,
    MSG_RoutingCesMalsrSimulatedHelloTimeout,
    MSG_RoutingCesSrwToWnwIntfTimeout,
    MSG_RoutingCesSrwUgsSleepTimeout,
    MSG_RoutingCesSrwUgsHibernateTimeout,
    MSG_RoutingCesSrwUgsC2WakeUpTimeout,
    MSG_RoutingCesRospfDRElectionTimer,
    MSG_RoutingCesRospfDRResignationTimer,

    MSG_RoutingCesSdrLsuBroadcastTimeout,
    MSG_NetworkCesSincgarsNADTimerExpired,
    MSG_NetworkCesSincgarsVoiceTimerExpired,
    MSG_NetworkCesSincgarsReXmitTimerExpired,
    MSG_NetworkCesSincgarsNeighborAgentTimerExpired,
    MSG_NETWORK_CES_SINCGARS_FMU,
    MSG_NetworkCesSincgarsTimeSendSAUpFrame,
    MSG_NetworkCesSincgarsTimeSendSADownFrame,

    MSG_NETWORK_CES_EPLRS_DelayedSendToMac,
    MSG_NETWORK_CES_EPLRS_BufferTimerExpired,
    MSG_NETWORK_CES_EPLRS_TimerExpired,
    MSG_NETWORK_CES_EPLRS_GlobalTimerExpired,
    MSG_NETWORK_CES_EPLRS_CRM_delay,

    MSG_BoeingGenericMacSlotTimerExpired,

    /* Message types for Multicast Routing - RPIM */
    MSG_ROUTING_RPimScheduleHello,
    MSG_ROUTING_RPimNeighborTimeOut,
    MSG_ROUTING_RPimPruneTimeoutAlarm,
    MSG_ROUTING_RPimAssertTimeOut,
    MSG_ROUTING_RPimDataTimeOut,
    MSG_ROUTING_RPimGraftRtmxtTimeOut,
    MSG_ROUTING_RPimJoinTimeOut,
    MSG_ROUTING_RPimScheduleJoin,
    MSG_ROUTING_RPimPacket,
    MSG_EXTERNAL_RemoteMessage, // A message sent from the remote partition

    MSG_MAC_ABSTRACT_TimerExpired,

    MSG_MAC_CES_SRW,
    MSG_MAC_CES_SRW_CmmaResponseTimer,
    MSG_MAC_CES_SRW_SlotTimerExpired,
    MSG_MAC_CES_SRW_CircuitDataFlowStopTimer,
    MSG_MAC_CES_SRW_ProcessPendingListTimer,
    //MSG_MAC_WINT_SatelliteToGround,

    MSG_MULTICAST_CES_FlushCache,
    MSG_MULTICAST_CES_RescheduleJoinOrLeave,

#endif // ADDON_BOEINGFCS

    MSG_MAC_WINT_SatelliteToGround,

    // SINCGARS events
    MSG_SdrLsuBroadcastTimeout,

    // STK events
    MSG_EXTERNAL_StkUpdatePosition,

#ifdef ADDON_NGCNMS
    MSG_EXTERNAL_HistStatUpdate,
    MSG_NETWORK_NGC_HAIPE_SenderProcessingDelay,
    MSG_NETWORK_NGC_HAIPE_ReceiverProcessingDelay,
#endif

    MSG_EXTERNAL_SendPacket,
    MSG_EXTERNAL_ForwardInstantiate,
    MSG_EXTERNAL_ForwardTcpListen,
    MSG_EXTERNAL_ForwardTcpConnect,
    MSG_EXTERNAL_ForwardSendUdpData,
    MSG_EXTERNAL_ForwardBeginExternalTCPData,
    MSG_EXTERNAL_ForwardSendTcpData,
    MSG_EXTERNAL_Heartbeat,
    MSG_EXTERNAL_PhySetTxPower,

    // Messages used by the dynamic API
    MSG_DYNAMIC_Command,
    MSG_DYNAMIC_CommandOob,
    MSG_DYNAMIC_Response,
    MSG_DYNAMIC_ResponseOob,

#ifdef DXML_INTERFACE
    MSG_EXTERNAL_DxmlCommand,
#endif // DXML_INTERFACE

    // MAODV events
    MSG_ROUTING_MaodvFlushMessageCache,
    MSG_ROUTING_MaodvTreeUtilizationTimer,
    MSG_ROUTING_MaodvCheckMroute,
    MSG_ROUTING_MaodvCheckNextHopTimeout,
    MSG_ROUTING_MaodvRetransmitTimer,
    MSG_ROUTING_MaodvPruneTimeout,
    MSG_ROUTING_MaodvDeleteMroute,
    MSG_ROUTING_MaodvSendGroupHello,

    MSG_EXTERNAL_Mobility,
//startCellular
    //application
    MSG_APP_CELLULAR_FromNetworkCallAccepted,
    MSG_APP_CELLULAR_FromNetworkCallRejected,
    MSG_APP_CELLULAR_FromNetworkCallArrive,
    MSG_APP_CELLULAR_FromNetworkCallEndByRemote,
    MSG_APP_CELLULAR_FromNetworkCallDropped,
    MSG_APP_CELLULAR_FromNetworkPktArrive,

    //layer3
    MSG_NETWORK_CELLULAR_FromAppStartCall,
    MSG_NETWORK_CELLULAR_FromAppEndCall,
    MSG_NETWORK_CELLULAR_FromAppCallAnswered,
    MSG_NETWORK_CELLULAR_FromAppCallHandup,
    MSG_NETWORK_CELLULAR_FromAppPktArrival,
    MSG_NETWORK_CELLULAR_FromMacNetworkNotFound,
    MSG_NETWORK_CELLULAR_FromMacMeasurementReport,
    MSG_NETWORK_CELLULAR_TimerExpired,
    MSG_NETWORK_CELLULAR_PollHandoverForCallManagement,

    //Mac layer
    MSG_MAC_CELLULAR_FromNetworkScanSignalPerformMeasurement,
    MSG_MAC_CELLULAR_FromNetwork,
    MSG_MAC_CELLULAR_FromTch,
    MSG_MAC_CELLULAR_ScanSignalTimer,
    MSG_MAC_CELLULAR_FromNetworkCellSelected,
    MSG_MAC_CELLULAR_FromNetworkHandoverStart,
    MSG_MAC_CELLULAR_FromNetworkHandoverEnd,
    MSG_MAC_CELLULAR_FromNetworkTransactionActive,
    MSG_MAC_CELLULAR_FromNetworkNoTransaction,
    MSG_MAC_CELLULAR_ProcessTchMessages,

    //MISC
    MSG_CELLULAR_PowerOn,
    MSG_CELLULAR_PowerOff,

//endCellular

    //User Layer
    MSG_USER_StatusChange,
    MSG_USER_ApplicationArrival,
    MSG_USER_PhoneStartup,
    MSG_USER_TimerExpired,

    /* Message Types for Pedestrian Mobility */
    MSG_MOBILITY_PedestrianDynamicDataUpdate,
    MSG_MOBILITY_PedestrianPartitionDynamicDataUpdate,

    MSG_CONFIG_ChangeValueTimer,
    MSG_CONFIG_DisableNode,
    MSG_CONFIG_EnableNode,
    MSG_CONFIG_ContinueEnable,

    MSG_GenericMacSlotTimerExpired,

    MSG_UTIL_MemoryUtilization,
    MSG_UTIL_FsmEvent,
    MSG_UTIL_External,
    MSG_UTIL_AbstractEvent,
    MSG_UTIL_MobilitySample,
    MSG_UTIL_NameServiceDynamicParameterUpdated,

    /* Message types for OLSRv2 NIIGATA */
    MSG_APP_OLSRv2_NIIGATA_PeriodicHello,
    MSG_APP_OLSRv2_NIIGATA_PeriodicTc,
    MSG_APP_OLSRv2_NIIGATA_PeriodicMa,
    MSG_APP_OLSRv2_NIIGATA_TimeoutForward,
    MSG_APP_OLSRv2_NIIGATA_TimeoutTuples,

    //802.15.4 timers
    MSG_SSCS_802_15_4_TimerExpired,
    MSG_CSMA_802_15_4_TimerExpired,
    MSG_MAC_802_15_4_Frame,
#ifdef MILITARY_RADIOS_LIB
    MSG_EPLRS_TimerExpired                    = 2000,

    MSG_EPLRS_GlobalTimerExpired,
    MSG_EPLRS_BufferTimerExpired,

    MSG_ROUTING_SdrScheduleSPF,
#endif //MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
    MSG_CRYPTO_Overhead,

    MSG_NETWORK_CheckRrepAck,
    MSG_NETWORK_CheckRerrAck,
    MSG_NETWORK_CheckDataAck,

    MSG_NETWORK_ISAKMP_SchedulePhase1,
    MSG_NETWORK_ISAKMP_RxmtTimer,
    MSG_NETWORK_ISAKMP_RefreshTimer,
#endif // NETSEC_LIB

    // STATS DB CODE
    MSG_NETWORK_InsertConnectivity,
    MSG_STATS_APP_InsertAggregate,
    MSG_STATS_NETWORK_InsertAggregate,
    MSG_STATS_APP_InsertSummary,
    MSG_STATS_NETWORK_InsertSummary,

    /*
     * Any other message types which have to be added should be added before
     * MSG_DEFAULT. Otherwise the program will not work correctly.
     */
    MSG_DEFAULT                                = 10000
};


// --------------------------------------------------------------------------
// FUNCTION BLOCK    Layer_*Initialize
// PURPOSE           Initialization functions for various layers.
// --------------------------------------------------------------------------

// /**
// FUNCTION    :: CHANNEL_Initialize
// LAYER       :: PHYSICAL
// PURPOSE     :: Initialization function for channel
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing all the
// configuration file details
// RETURN      :: void :
// **/
void CHANNEL_Initialize(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: PHY_Init
// LAYER       :: PHYSICAL
// PURPOSE     :: Initialization function for physical layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing config file details
// RETURN      :: void :
// **/
void PHY_Init(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: MAC_Initialize
// LAYER       :: MAC
// PURPOSE     :: Initialization function for the MAC layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void MAC_Initialize(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: NETWORK_PreInit
// LAYER       :: NETWORK
// PURPOSE     :: Pre-Initialization function for Network layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void NETWORK_PreInit(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: NETWORK_Initialize
// LAYER       :: NETWORK
// PURPOSE     :: Initialization function for Network layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void NETWORK_Initialize(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: TRANSPORT_Initialize
// LAYER       :: TRANSPORT
// PURPOSE     :: Initialization function for transport layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void TRANSPORT_Initialize(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: APP_Initialize
// LAYER       :: APPLICATION
// PURPOSE     :: Initialization function for Application layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void APP_Initialize(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: USER_Initialize
// LAYER       :: USER
// PURPOSE     :: Initialization function for User layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void USER_Initialize(Node *node, const NodeInput *nodeInput);


// /**
// FUNCTION    :: APP_InitializeApplications
// LAYER       :: APPLICATION
// PURPOSE     :: Initialization function for applications in APPLICATION
// layer
// PARAMETERS  ::
// + firstNode : Node* : first node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void APP_InitializeApplications(Node* firstNode,
                                const NodeInput *nodeInput);

// /**
// FUNCTION: ATMLAYER2_Initialize
// PURPOSE:  Initialization function for the ATM Layer2.
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void ATMLAYER2_Initialize(Node* node, const NodeInput *nodeInput);

// /**
// FUNCTION    :: ADAPTATION_Initialize
// LAYER       :: ADAPTATION
// PURPOSE     :: Initialization function for Adaptation layer
// PARAMETERS  ::
// + node      : Node* : node being intialized
// + nodeInput : const NodeInput* : structure containing input file details
// RETURN      :: void :
// **/
void ADAPTATION_Initialize(Node *node, const NodeInput *nodeInput);


// --------------------------------------------------------------------------
// FUNCTION    Layer_*Finalize
// PURPOSE     Called at the end of simulation to collect the results of
//             the simulation of the various layers.
// --------------------------------------------------------------------------

// /**
// FUNCTION    :: CHANNEL_Finalize
// LAYER       :: PHYSICAL
// PURPOSE     :: To collect results of simulation at the end
// for channels
// PARAMETERS  ::
// + node : Node * : Node for which data is collected
// RETURN      :: void :
// **/
void CHANNEL_Finalize(Node *node);

// /**
// FUNCTION    :: PHY_Finalize
// LAYER       :: PHYSICAL
// PURPOSE     :: To collect results of simulation at the end
// for the PHYSICAL layer
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// and the statistical data being collected
// RETURN      :: void :
// **/
void PHY_Finalize(Node *node);

// /**
// FUNCTION    :: MAC_Finalize
// LAYER       :: MAC
// PURPOSE     :: To collect results of simulation at the end
// for the mac layers
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// and the statistical data being collected
// RETURN      :: void :
// **/
void MAC_Finalize(Node *node);

// /**
// FUNCTION    :: NETWORK_Finalize
// LAYER       :: NETWORK
// PURPOSE     :: To collect results of simulation at the end
// for network layers
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// and the statistical data being collected
// RETURN      :: void :
// **/
void NETWORK_Finalize(Node *node);

// /**
// FUNCTION    :: TRANSPORT_Finalize
// LAYER       :: TRANSPORT
// PURPOSE     :: To collect results of simulation at the end
// for transport layers
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// and the statistical data being collected
// RETURN      :: void :
// **/
void TRANSPORT_Finalize(Node *node);

// /**
// FUNCTION    :: APP_Finalize
// LAYER       :: APPLICATION
// PURPOSE     :: To collect results of simulation at the end
// for application layers
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// and the statistical data being collected
// RETURN      :: void :
// **/
void APP_Finalize(Node *node);

// /**
// FUNCTION    :: USER_Finalize
// LAYER       :: USER
// PURPOSE     :: To collect results of simulation at the end
// for user layers
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// and the statistical data being collected
// RETURN      :: void :
// **/
void USER_Finalize(Node *node);


// /**
// FUNCTION    :: ATMLAYER2_Finalize
// LAYER       :: Atm Layer2
// PURPOSE     :: To collect results at the end of the simulation.
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// RETURN      :: void :
// **/
void ATMLAYER2_Finalize(Node* node);

// /**
// FUNCTION    :: ADAPTATION_Finalize
// LAYER       :: ADAPTATION
// PURPOSE     :: To collect results of simulation at the end
// for network layers
// PARAMETERS  ::
// + node : Node * : Node for which finalization function is called
// and the statistical data being collected
// RETURN      :: void :
// **/
void ADAPTATION_Finalize(Node *node);


// --------------------------------------------------------------------------
// FUNCTION    Layer_*ProcessEvent
// PURPOSE     Models the behaviour of the various layers on receiving
//             message enclosed in msgHdr.
// --------------------------------------------------------------------------

// /**
// FUNCTION    :: CHANNEL_ProcessEvent
// LAYER       :: PHYSICAL
// PURPOSE     :: Processes the message/event of physical layer received
// by the node thus simulating the PHYSICAL layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void CHANNEL_ProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION    :: PHY_ProcessEvent
// LAYER       :: PHYSICAL
// PURPOSE     :: Processes the message/event of physical layer received
// by the node thus simulating the PHYSICAL layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void PHY_ProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION    :: MAC_ProcessEvent
// LAYER       :: MAC
// PURPOSE     :: Processes the message/event of MAC layer received
// by the node thus simulating the MAC layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void MAC_ProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION    :: NETWORK_ProcessEvent
// LAYER       :: NETWORK
// PURPOSE     :: Processes the message/event received by the node
// thus simulating the NETWORK layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void NETWORK_ProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION    :: TRANSPORT_ProcessEvent
// LAYER       :: TRANSPORT
// PURPOSE     :: Processes the message/event received by the node
// thus simulating the TRANSPORT layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void TRANSPORT_ProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION    :: APP_ProcessEvent
// LAYER       :: APPLICATION
// PURPOSE     :: Processes the message/event received by the node
// thus simulating the APPLICATION layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void APP_ProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION    :: USER_ProcessEvent
// LAYER       :: USER
// PURPOSE     :: Processes the message/event received by the node
// thus simulating the USER layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void USER_ProcessEvent(Node *node, Message *msg);


// /**
// FUNCTION    :: ATMLAYER2_ProcessEvent
// LAYER       :: ATM_LAYER2
// PURPOSE     :: Processes the message/event of ATM_LAYER2 layer received
// by the node thus simulating the ATM_LAYER2 layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void ATMLAYER2_ProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION    :: ADAPTATION_ProcessEvent
// LAYER       :: ADAPTATION
// PURPOSE     :: Processes the message/event received by the node
// thus simulating the ADAPTATION layer behaviour
// PARAMETERS  ::
// + node : Node* : node which receives the message
// + msg  : Message* : Received message structure
// RETURN      :: void :
// **/
void ADAPTATION_ProcessEvent(Node *node, Message *msg);


// --------------------------------------------------------------------------
// FUNCTION    Layer_*RunTimeStat
// PURPOSE     Prints runtime statistics for the node.
// --------------------------------------------------------------------------

// /**
// FUNCTION    :: MAC_RunTimeStat
// LAYER       :: MAC
// PURPOSE     :: To print runtime statistics for the MAC layer
// PARAMETERS  ::
// + node : Node* : node for which statistics to be printed
// RETURN      :: void :
// **/
void MAC_RunTimeStat(Node *node);

// /**
// FUNCTION    :: NETWORK_RunTimeStat
// LAYER       :: NETWORK
// PURPOSE     :: To print runtime statistics for the NETWORK layer
// PARAMETERS  ::
// + node : Node* : node for which statistics to be printed
// RETURN      :: void :
// **/
void NETWORK_RunTimeStat(Node *node);

// /**
// FUNCTION    :: TRANSPORT_RunTimeStat
// LAYER       :: TRANSPORT
// PURPOSE     :: To print runtime statistics for the TRANSPORT layer
// PARAMETERS  ::
// + node : Node* : node for which statistics to be printed
// RETURN      :: void :
// **/
void TRANSPORT_RunTimeStat(Node *node);

// /**
// FUNCTION    :: APP_RunTimeStat
// LAYER       :: APPLICATION
// PURPOSE     :: To print runtime statistics for the APPLICATION layer
// PARAMETERS  ::
// + node : Node* : node for which statistics to be printed
// RETURN      :: void :
// **/
void APP_RunTimeStat(Node *node);


// /**
// STRUCT      :: PhyBatteryPower
// DESCRIPTION :: Used by App layer and Phy layer to exchange battery power
// **/
typedef struct phy_battery_power_str {
    short layerType;
    short protocolType;
    double power;
} PhyBatteryPower;


// /**
// STRUCT      :: PacketNetworkToApp
// DESCRIPTION :: Network to application layer packet structure
// **/
typedef struct pkt_net_to_app {
    NodeAddress sourceAddr;    /* previous hop */
} PacketNetworkToApp;


// /**
// Define      :: TTL_NOT_SET
// DESCRIPTION :: TTL value for which we consider TTL not set.
//                Used in TCP/UDP app and transport layer TTL
// **/
#define TTL_NOT_SET 0

// /**
// STRUCT      :: NetworkToTransportInfo
// DESCRIPTION :: Network To Transport layer Information structure
// **/
typedef
struct
{
    Address sourceAddr;
    Address destinationAddr;
    TosType priority;
    int incomingInterfaceIndex;
    unsigned receivingTtl;
    BOOL isCongestionExperienced; /* ECN related variables */
} NetworkToTransportInfo;

// /**
// STRUCT      :: PacketTransportNetwork
// DESCRIPTION :: Transport to network layer packet structure
// **/
typedef struct pkt_nstcp_to_network {
    NodeAddress sourceId;
    NodeAddress destId;
    Int32 packetSize;
    Int32 agenttype;
    short sourcePort;
    short destPort;
    TosType priority;
    void *pkt;
} PacketTransportNetwork;

// /**
// STRUCT      :: TcpTimerPacket
// DESCRIPTION :: TCP timer packet
// **/
typedef struct tcp_timer__ {
    int timerId;
    int timerType;
    int connectionId;
} TcpTimerPacket;

// /**
// STRUCT      :: AppToUdpSend
// DESCRIPTION :: Additional information given to UDP from applications.
// This information is saved in the info field of a message.
// **/
typedef
struct app_to_udp_send
{
    Address sourceAddr;
    short sourcePort;
    Address destAddr;
    short destPort;
    TosType priority;
#ifdef ADDON_BOEINGFCS
    TosType origPriority;
#endif
    int outgoingInterface;
    UInt8 ttl;
} AppToUdpSend;

// /**
// STRUCT      :: UdpToAppRecv
// DESCRIPTION :: Additional information given to applications from UDP.
// This information is saved in the info field of a message.
// **/
typedef
struct udp_to_app_send
{
    Address sourceAddr;
    short sourcePort;
    Address destAddr;
    short destPort;
    int incomingInterfaceIndex;
    TosType priority;
} UdpToAppRecv;

// /**
// STRUCT      :: AppToRsvpSend
// DESCRIPTION :: send response structure from application layer
// **/
typedef struct {
    NodeAddress sourceAddr;
    NodeAddress destAddr;
    char*    upcallFunctionPtr;
} AppToRsvpSend;

// /**
// STRUCT      :: TransportToAppListenResult
// DESCRIPTION :: Report the result of application's listen request.
// **/
typedef struct transport_to_app_listen_result {
    Address localAddr;
    short localPort;
    int connectionId;     /* -1 - listen failed, >=0 - connection id */
} TransportToAppListenResult;

// /**
// STRUCT      :: TransportToAppOpenResult
// DESCRIPTION :: Report the result of opening a connection.
// **/
typedef struct transport_to_app_open_result {
    int type;             /* 1 - active open, 2 - passive open */
    Address localAddr;
    short localPort;
    Address remoteAddr;
    short remotePort;
    int connectionId;     /* -1 - open failed, >=0 - connection id */

    Int32 uniqueId;
} TransportToAppOpenResult;

// /**
// STRUCT      :: TransportToAppDataSent
// DESCRIPTION :: Report the result of sending application data.
// **/
typedef struct transport_to_app_data_sent {
    int connectionId;
    Int32 length;               /* length of data sent */
} TransportToAppDataSent;

// /**
// STRUCT      :: TransportToAppDataReceived
// DESCRIPTION :: Deliver data to application.
// **/
typedef struct transport_to_app_data_received {
    int connectionId;
    TosType priority;
} TransportToAppDataReceived;

// /**
// STRUCT      :: TransportToAppCloseResult
// DESCRIPTION :: Report the result of closing a connection.
// **/
typedef struct tcp_to_app_close_result {
    int type;                /* 1 - active close, 2 - passive close */
    int connectionId;
} TransportToAppCloseResult;

// /**
// STRUCT      :: AppToTcpListen
// DESCRIPTION :: Application announces willingness to accept connections
// on given port.
// **/
typedef struct app_to_tcp_listen {
    AppType appType;
    Address localAddr;
    short localPort;
    TosType priority;
    int uniqueId;
}
AppToTcpListen;

// /**
// STRUCT      :: AppToTcpOpen
// DESCRIPTION :: Application attempts to establish a connection
// **/
typedef struct app_to_tcp_open {
    AppType appType;
    Address localAddr;
    short localPort;
    Address remoteAddr;
    short remotePort;

    Int32 uniqueId;
    TosType priority;
    int outgoingInterface;
}
AppToTcpOpen;

// /**
// STRUCT      :: AppToTcpSend
// DESCRIPTION :: Application wants to send some data over the connection
// **/
typedef struct app_to_tcp_send {
    int connectionId;
    UInt8 ttl;
} AppToTcpSend;

// /**
// STRUCT      :: AppToTcpClose
// DESCRIPTION :: Application wants to release the connection
// **/
typedef struct app_to_tcp_close {
    int connectionId;
} AppToTcpClose;

// /**
// STRUCT      :: AppChanswitchTimeout
// DESCRIPTION :: Application sends a timeout message to itself
// Used in chanswitch
// **/
typedef struct app_chanswitch_timeout {
    int connectionId;
} AppChanswitchTimeout;

// /**
// STRUCT      :: AppToMacAddrRequest
// DESCRIPTION :: Application asks for its MAC address
// Used in chanswitch
// **/
typedef struct app_to_mac_addr_request {
    int connectionId;
    int appType;
    BOOL initial;
} AppToMacAddrRequest;

// /**
// STRUCT      :: MacToAppAddr
// DESCRIPTION :: MAC tells TX/RX its address
// Used in chanswitch
// **/
typedef struct mac_to_app_addr_request {
    int connectionId;
    Mac802Address myAddr;
    int numChannels;
    int currentChannel;
    D_BOOL* channelSwitch;
    double noise_mW;
    BOOL initial;
    BOOL asdcsInit;
} MacToAppAddrRequest;

// /**
// STRUCT      :: AppToMacStartProbe
// DESCRIPTION :: Application sends message to mac to start channel scan
// Used in chanswitch
// **/
typedef struct app_to_mac_start_probe{
    int connectionId;
    int appType;
    BOOL initial;
} AppToMacStartProbe;

// /**
// STRUCT      :: AppToMacChannelChange
// DESCRIPTION :: Application sends message to mac to change to the new channel
// Used in chanswitch
// **/
typedef struct app_to_mac_channel_change{
    int connectionId;
    int appType;
    int oldChannel;
    int newChannel;
} AppToMacChannelChange;

// /**
// STRUCT      :: MacToAppScanComplete
// DESCRIPTION :: MAC sends visible node list to App after finishing
// Used in chanswitch
// **/
typedef struct mac_to_app_scan_complete{
    int connectionId;
    int nodeCount;
    DOT11_VisibleNodeInfo* nodeInfo;
} MacToAppScanComplete;

// /**
// STRUCT      :: init_scan_request
// DESCRIPTION :: Can be sent from any layer to initiate a channel switch at App layer
// Used in chanswitch
// **/
typedef struct init_scan_request{
    int connectionId;
} AppInitScanRequest;

// /**
// STRUCT      :: AppToTcpConnSetup
// DESCRIPTION :: Application sets up connection at the local end
// Needed for NS TCP to fake connection setup
// **/
typedef struct app_to_tcp_conn_setup {
    NodeAddress localAddr;
    int       localPort;
    NodeAddress remoteAddr;
    int       remotePort;
    int       connectionId;
    int       agentType;
} AppToTcpConnSetup;

// /**
// STRUCT      :: AppQosToNetworkSend
// DESCRIPTION :: Application uses this structure in its info field to
// perform the initialization of a new QoS connection with
// its QoS requirements.
// **/
typedef struct app_qos_to_network_send {
    int   clientType;
    short sourcePort;
    short destPort;
    NodeAddress sourceAddress;
    NodeAddress destAddress;
    TosType priority;
    int bandwidthRequirement;
    int delayRequirement;
} AppQosToNetworkSend;

// /**
// STRUCT      :: NetworkToAppQosConnectionStatus
// DESCRIPTION :: Q-OSPF uses this structure to report status of a session
// requested by the application for Quality of Service.
// **/
typedef struct network_to_app_qos {
    short sourcePort;
    BOOL  isSessionAdmitted;
} NetworkToAppQosConnectionStatus;

#ifdef MILITARY_RADIOS_LIB
// /**
// STRUCT      :: TadilToAppRecv
// DESCRIPTION :: Additional information given to applications from TADIL.
//                This information is saved in the info field of a message.
// **/
typedef
struct tadil_to_app_rcvd
{
    NodeAddress             sourceId;
    unsigned short          sourcePort;
    NodeAddress             dstId;
    unsigned short          destPort;
} TadilToAppRecv;
#endif // MILITARY_RADIOS_LIB

// /**
// ENUM        ::TransportType
// DESCRIPTION :: Transport type to check reliable, unreliable or
//                TADIL network for Link16 or Link11
// **/
typedef enum {
    TRANSPORT_TYPE_RELIABLE     = 1,
    TRANSPORT_TYPE_UNRELIABLE   = 2,
    TRANSPORT_TYPE_MAC          = 3 // For LINK16 and LINK11
} TransportType;

// /**
// ENUM        ::DestinationType
// DESCRIPTION :: Interface IP address type
// **/
typedef enum {
    DEST_TYPE_UNICAST   = 0,
    DEST_TYPE_MULTICAST = 1,
    DEST_TYPE_BROADCAST = 2
} DestinationType;

#ifdef ADDON_NGCNMS
void
TRANSPORT_Reset(Node *node, const NodeInput *nodeInput);

void
TRANSPORT_AddResetFunctionList(Node* node, void *param);
#endif

#endif /*API_H*/
