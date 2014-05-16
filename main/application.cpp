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

/*
 * This file contains initialization function, message processing
 * function, and finalize function used by application layer.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "clock.h"
#include "partition.h"
#include "external.h"

#include "network_ip.h"
#include "network_icmp.h"
#include "ipv6.h"
#include "app_util.h"
#include "if_loopback.h"

#include "app_ftp.h"
#include "app_gen_ftp.h"
#include "app_telnet.h"
#include "app_cbr.h"
#include "app_forward.h"
#include "app_http.h"
#include "app_traffic_gen.h"
#include "app_traffic_trace.h"
#include "app_mcbr.h"
#include "app_messenger.h"
#include "app_superapplication.h"
#include "app_vbr.h"
#include "app_video.h"
#include "app_wbest.h"
#include "app_chanswitch.h"
#include "app_chanswitch_sinr.h"

#include "app_lookup.h"

#include "routing_static.h"
#include "routing_bellmanford.h"
#include "routing_rip.h"
#include "routing_ripng.h"

#include "network_dualip.h"

#ifdef ENTERPRISE_LIB
#include "mpls.h"
#include "transport_rsvp.h"
#include "routing_hsrp.h"
#include "multimedia_h323.h"
#include "multimedia_h225_ras.h"
#include "multimedia_sip.h"
#include "app_voip.h"
#include "transport_rtp.h"
#include "routing_bgp.h"
#include "mpls_ldp.h"
#endif // ENTERPRISE_LIB

#ifdef CELLULAR_LIB
#include "cellular.h"
#include "cellular_gsm.h"
#include "cellular_abstract.h"
#include "app_cellular_abstract.h"
#include "cellular_layer3.h"
#endif

#ifdef UMTS_LIB
#include "cellular.h"
#include "app_phonecall.h"
#include "cellular_layer3.h"
#endif // UMTS_LIB

#ifdef ADDON_LINK16
#include "link16_cbr.h"
#endif // ADDON_LINK16

#ifdef ALE_ASAPS_LIB
#include "mac_ale.h"
#endif // ALE_ASAPS_LIB

#ifdef WIRELESS_LIB
#include "routing_fisheye.h"
#include "routing_olsr-inria.h"
#include "routing_olsrv2_niigata.h"
#include "network_neighbor_prot.h"
#endif // WIRELESS_LIB

#ifdef ADDON_MGEN4
#include "mgenApp.h"
#endif

#ifdef ADDON_MGEN3
#include "mgen3.h"
#include "drec.h"
#endif

#ifdef ADDON_BOEINGFCS
#include "network_ces_qos_manager.h"
#include "routing_ces_srw.h"
#endif
//InsertPatch HEADER_FILES

#ifdef ADDON_HELLO
#include "hello.h"
#endif /* ADDON_HELLO */

#ifdef ADDON_BOEINGFCS
#include "external_socket.h"
#include "cesmessages.h"
#include "routing_ces_hsls.h"
#include "change_param_value.h"
#endif /* ADDON_BOEINGFCS */
#ifdef TUTORIAL_INTERFACE
#include "interfacetutorial_app.h"
#endif /* TUTORIAL_INTERFACE */

#ifdef MILITARY_RADIOS_LIB
#include "tadil_util.h"
#include "app_threaded.h"
#define INPUT_LENGTH 1000
#endif

#ifdef ADDON_BOEINGFCS
#include "mop_stats.h"

#include "mac_ces_usap.h"
#endif /* ADDON_BOEINGFCS */

void
AppInputHttpError(void)
{
    char errorbuf[MAX_STRING_LENGTH];
    sprintf(errorbuf,
            "Wrong HTTP configuration format!\n"
            "HTTP <client> <num-servers> <server1> <server2> ... "
            "<start time> <threshhold time>\n");
    ERROR_ReportError(errorbuf);
}


// Checks and Returns equivalent 8-bit TOS value
BOOL
APP_AssignTos(
    char tosString[],
    char tosValString[],
    unsigned* tosVal)
{
    unsigned value = 0;

    if (IO_IsStringNonNegativeInteger(tosValString))
    {
        if (!strcmp(tosString, "PRECEDENCE"))
        {
            value = (unsigned) atoi(tosValString);

            if (/*value >= 0 &&*/ value <= 7)
            {
                // Equivalent TOS [8-bit] value [[PRECEDENCE << 5]]
                *tosVal = value << 5;
                return TRUE;
            }
            else
            {
                // PRECEDENCE [range < 0 to 7 >]
                ERROR_ReportError("PRECEDENCE value range < 0 to 7 >");
            }
        }

        if (!strcmp(tosString, "DSCP"))
        {
            value = (unsigned) atoi(tosValString);

            if (/*value >= 0 &&*/ value <= 63)
            {
                // Equivalent TOS [8-bit] value [[DSCP << 2]]
                *tosVal = value << 2;
                return TRUE;
            }
            else
            {
                // DSCP [range <0 to 63 >]
                ERROR_ReportError("DSCP value range < 0 to 63 >");
            }
        }

        if (!strcmp(tosString, "TOS"))
        {
            value = (unsigned) atoi(tosValString);

            if (/*value >= 0 &&*/ value <= 255)
            {
                // Equivalent TOS [8-bit] value
                *tosVal = value;
                return TRUE;
            }
            else
            {
                // TOS [range <0 to 255 >]
                ERROR_ReportError("TOS value range < 0 to 255 >");
            }
        }
    }
    return FALSE;
}

/*
 * NAME:        APP_InitiateIcmpMessage.
 * PURPOSE:     Call NetworkIcmpCreatePortUnreachableMessage()  to generate port
                unreachable message
 * PARAMETERS:  node - pointer to the node
                msg - pointer to the received message
 * RETURN:      none.
 */
void
APP_InitiateIcmpMessage(Node *node, Message *msg)
{
    UdpToAppRecv* udpToApp = (UdpToAppRecv *) MESSAGE_ReturnInfo(msg);

    // incoming interface received from info field
    int incomingInterface = udpToApp->incomingInterfaceIndex;

    NetworkIcmpCreatePortUnreachableMessage(node,
                    msg,
                    //GetIPv4Address(udpToApp->sourceAddr),
                    GetIPv4Address(udpToApp->destAddr),
                    incomingInterface);
}

/*
 * NAME:        APP_HandleLoopback.
 * PURPOSE:     Checks applications on nodes for loopback
 * PARAMETERS:  node - pointer to the node,
 * ASSUMPTION:  Network initialization is completed before
 *              Application initialization for a node.
 * RETURN:      BOOL.
 */
static BOOL
APP_SuccessfullyHandledLoopback(
    Node *node,
    const char *inputString,
    Address destAddr,
    NodeAddress destNodeId,
    Address sourceAddr,
    NodeAddress sourceNodeId)
{
    BOOL isLoopBack = FALSE;
    if ((sourceAddr.networkType == NETWORK_IPV6
        || destAddr.networkType == NETWORK_IPV6)
        && (Ipv6IsLoopbackAddress(node, GetIPv6Address(destAddr))
        || !CMP_ADDR6(GetIPv6Address(sourceAddr), GetIPv6Address(destAddr))
        || destNodeId == sourceNodeId))
    {
        isLoopBack = TRUE;
    }
    else if ((sourceAddr.networkType == NETWORK_IPV4
            || destAddr.networkType == NETWORK_IPV4)
            && (NetworkIpIsLoopbackInterfaceAddress(GetIPv4Address(destAddr))
            || (destNodeId == sourceNodeId)
            || (GetIPv4Address(sourceAddr) == GetIPv4Address(destAddr))))
    {
        isLoopBack = TRUE;
    }

    if (isLoopBack)
    {
        NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;

        if (!ip->isLoopbackEnabled)
        {
            char errorStr[MAX_STRING_LENGTH] = {0};
            // Error
            sprintf(errorStr, "IP loopback is disabled for"
                " node %d!\n  %s\n", node->nodeId, inputString);

            ERROR_ReportError(errorStr);
        }
        // Node pointer already found, skip scanning mapping
        // for destNodeId

        destNodeId = sourceNodeId;

        return TRUE;
    }
    return FALSE;
}

/*
 * NAME:        APP_TraceInitialize.
 * PURPOSE:     Initialize trace routine of desired application
 *              layer protocols.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void APP_TraceInitialize(Node *node, const NodeInput *nodeInput)
{
    BOOL retVal;
    char apStr[MAX_STRING_LENGTH];
    NodeInput appInput;

    // return if trace is  disabled
    if (!node->partitionData->traceEnabled)
    {
        return;
    }

    IO_ReadCachedFile(ANY_NODEID,
                      ANY_ADDRESS,
                      nodeInput,
                      "APP-CONFIG-FILE",
                      &retVal,
                      &appInput);

    if (retVal == TRUE)
    {
       for (int index = 0; index < appInput.numLines; index++)
       {
           sscanf(appInput.inputStrings[index], "%s", apStr);

           // Init relevant app layer prtocol trace routine
           if (!strcmp(apStr, "CBR"))
           {
               AppLayerCbrInitTrace(node, nodeInput);
           }
           else if (!strcmp(apStr, "TRAFFIC-GEN"))
           {
               TrafficGenInitTrace(node, nodeInput);
           }
           else if (!strcmp(apStr, "FTP/GENERIC"))
           {
               AppGenFtpInitTrace(node, nodeInput);
           }
           else if (!strcmp(apStr, "SUPER-APPLICATION"))
           {
               SuperApplicationInitTrace(node, nodeInput);
           }

        }

    }
}

//fix to read signalling parameters by all nodes
/*
 * NAME:        Multimedia_Initialize.
 * PURPOSE:     initializes multimedia related parameters according to
 *               user's specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
static void
Multimedia_Initialize(Node *node, const NodeInput *nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL gatekeeperFound = FALSE;
    H323CallModel callModel = Direct;
    if (node->appData.multimedia)
    {
        // Voip related intialization
        if (node->appData.multimedia->sigType == SIG_TYPE_H323)
        {
            // Gatekeeper of H323 should be created before starting
            // the application loop
            IO_ReadString(
                node->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "H323-CALL-MODEL",
                &retVal,
                buf);
            if (retVal == TRUE)
            {
                if (strcmp(buf, "DIRECT") == 0)
                {
                    callModel = Direct;
                }
                else if (strcmp(buf, "GATEKEEPER-ROUTED") == 0)
                {
                    callModel = GatekeeperRouted;
                }
                else
                {
                    /* invalid entry in config */
                    char errorBuf[MAX_STRING_LENGTH];
                    sprintf(errorBuf,
                        "Invalid call model (%s) for H323\n", buf);
                    ERROR_ReportError(errorBuf);
                }
            }
            gatekeeperFound = H323GatekeeperInit(node, nodeInput,
                                                 callModel);
        }
        else if (node->appData.multimedia->sigType == SIG_TYPE_SIP)
        {
            SipProxyInit(node, nodeInput);
        }
    }
}

/*
 * NAME:        APP_Initialize.
 * PURPOSE:     start applications on nodes according to user's
 *              specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void
APP_Initialize(Node *node, const NodeInput *nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    int i;
    AppMultimedia* multimedia = NULL;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
#ifdef MILITARY_RADIOS_LIB
    APPLink16GatewayInit(node, nodeInput);
#endif

#ifdef ADDON_NGCNMS
    if (!NODE_IsDisabled(node))
    {
        node->appData.resetFunctionList = new AppResetFunctionList;
        node->appData.resetFunctionList->first = NULL;
        node->appData.resetFunctionList->last = NULL;
    }
#endif // ADDON_NGCNMS

    /* Initialize application layer information.
       Note that the starting ephemeral port number is 1024. */

    node->appData.nextPortNum = 1024;
    node->appData.numAppTcpFailure = 0;

    // initialize finalize pointer in application

#ifdef ENTERPRISE_LIB
    // first check if the transport layer protocol for SIP is UDP
    IO_ReadString(node->nodeId,
              ANY_ADDRESS,
              nodeInput,
              "SIP-TRANSPORT-LAYER-PROTOCOL",
              &retVal,
              buf);

    if (retVal == TRUE)
    {
        if (!strcmp(buf, "UDP"))
        {

            // invalid entry in config
            char errBuf[MAX_STRING_LENGTH];
            sprintf(errBuf,
             " %s protocol is not supported in the current version!\n", buf);
            ERROR_ReportError(errBuf);
        }
    }
#endif // ENTERPRISE_LIB

    IO_ReadString(
                node->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "MULTIMEDIA-SIGNALLING-PROTOCOL",
                &retVal,
                buf);

    if (retVal)
    {
#ifdef ENTERPRISE_LIB
        multimedia = (AppMultimedia*) MEM_malloc(sizeof(AppMultimedia));
        memset(multimedia, 0, sizeof(AppMultimedia));

        if (!strcmp(buf, "H323"))
        {
            multimedia->sigType = SIG_TYPE_H323;
        }
        else if (!strcmp(buf, "SIP"))
        {
            multimedia->sigType = SIG_TYPE_SIP;
        }
        else
        {
            ERROR_ReportError("Invalid Multimedia Signalling protocol\n");
        }
#else // ENTERPRISE_LIB
        ERROR_ReportMissingLibrary(buf, "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
    }

    node->appData.multimedia = multimedia;

    #ifdef ENTERPRISE_LIB
        //fix to read signalling parameters by all nodes
        Multimedia_Initialize(node,nodeInput);
    #endif // ENTERPRISE_LIB

    node->appData.rtpData = NULL;

    // initialize and allocate memory to the port table
    node->appData.portTable = (PortInfo*) MEM_malloc(
        sizeof(PortInfo) * PORT_TABLE_HASH_SIZE);
    memset(node->appData.portTable,
       0,
       sizeof(PortInfo) * PORT_TABLE_HASH_SIZE);

    APP_TraceInitialize(node, nodeInput);

    /* Check if statistics needs to be printed. */

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "APPLICATION-STATISTICS",
        &retVal,
        buf);

    if (retVal == FALSE || strcmp(buf, "NO") == 0)
    {
        node->appData.appStats = FALSE;
    }
    else
    if (strcmp(buf, "YES") == 0)
    {
        node->appData.appStats = TRUE;
    }
    else
    {
        fprintf(stderr,
                "Expecting YES or NO for APPLICATION-STATISTICS parameter\n");
        abort();
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if (retVal == FALSE || strcmp(buf, "NO") == 0)
    {
        node->appData.routingStats = FALSE;
    }
    else
    if (strcmp(buf, "YES") == 0)
    {
        node->appData.routingStats = TRUE;
    }
    else
    {
        fprintf(stderr,
                "Expecting YES or NO for ROUTING-STATISTICS parameter\n");
        abort();
    }

#ifdef MILITARY_RADIOS_LIB
    node->appData.triggerAppData = NULL;
    ThreadedAppTriggerInit(node);
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_BOEINGFCS
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "NETWORK-CES-QOS-ENABLED",
        &retVal,
        buf);

    int priority = -1;
    ChangeParamValueNetworkCesQosEnabled(node,
                           priority,
                           &(node->appData.useNetworkCesQos));


    if (retVal == FALSE || strcmp(buf, "NO") == 0)
    {
        node->appData.useNetworkCesQos = FALSE;
        node->appData.networkCesQosData = NULL;
    }
    else
    {
        if (strcmp(buf, "YES") == 0)
        {
            node->appData.useNetworkCesQos = TRUE;
            if (node->appData.networkCesQosData==NULL)
                NetworkCesQosInit(node, nodeInput);
        }
        else
        {
            node->appData.useNetworkCesQos = FALSE;
            node->appData.networkCesQosData = NULL;
        }
    }
#endif
    node->appData.triggerAppData = NULL;
    SuperAppTriggerInit(node);

    node->appData.clientPtrList = NULL;
    SuperAppClientListInit(node);

    // Process static routes
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "STATIC-ROUTE",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "YES") == 0)
        {
            RoutingStaticInit(
                node,
                nodeInput,
                ROUTING_PROTOCOL_STATIC);
        }
    }

    // Process default routes
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "DEFAULT-ROUTE",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "YES") == 0)
        {
            RoutingStaticInit(
                node,
                nodeInput,
                ROUTING_PROTOCOL_DEFAULT);
        }
    }

    node->appData.hsrp = NULL;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        IO_ReadString(
                    node->nodeId,
                    NetworkIpGetInterfaceAddress(node, i),
                    nodeInput,
                    "HSRP-PROTOCOL",
                    &retVal,
                    buf);

        if (retVal == FALSE)
        {
            continue;
        }
        else
        {
            if (strcmp(buf, "YES") == 0)
            {
#ifdef ENTERPRISE_LIB
                RoutingHsrpInit(node, nodeInput, i);
#else // ENTERPRISE_LIB
                ERROR_ReportMissingLibrary("HSRP", "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
            }
        }
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NetworkType InterfaceType = NetworkIpGetInterfaceType(node, i);

        if (InterfaceType == NETWORK_IPV4 || InterfaceType == NETWORK_DUAL)
        {
            switch (ip->interfaceInfo[i]->routingProtocolType)
            {
                case ROUTING_PROTOCOL_BELLMANFORD:
                {
                    if (node->appData.bellmanford == NULL)
                    {
                        RoutingBellmanfordInit(node);
                        RoutingBellmanfordInitTrace(node, nodeInput);
                    }
                    break;
                }
                case ROUTING_PROTOCOL_RIP:
                {
                    RipInit(node, nodeInput, i);
                    break;
                }
#ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_FISHEYE:
                {
                    if (node->appData.routingVar == NULL)
                    {
                        RoutingFisheyeInit(node, nodeInput);
                    }
                    break;
                }
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                    if (node->appData.olsr == NULL)
                    {
                      RoutingOlsrInriaInit(node, nodeInput, i, NETWORK_IPV4);
                    }
                    else
                    {
                       if (((RoutingOlsr*)node->appData.olsr)->ip_version ==
                           NETWORK_IPV6)
                       {
                         ERROR_ReportError("Both IPV4 and IPv6 instance of"
                          "OLSR-INRIA can not run simultaneously on a node");
                       }

                     ((RoutingOlsr*)node->appData.olsr)->numOlsrInterfaces++;

                      NetworkIpUpdateUnicastRoutingProtocolAndRouterFunction(
                                node,
                                ROUTING_PROTOCOL_OLSR_INRIA,
                                i,
                                NETWORK_IPV4);
                    }
                    break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    if(node->appData.olsrv2 == NULL)
                    {
                        RoutingOLSRv2_Niigata_Init(node,
                                                   nodeInput,
                                                   i,
                                                   NETWORK_IPV4);
                    }
                    else
                    {
                        RoutingOLSRv2_Niigata_AddInterface(node,
                                                           nodeInput,
                                                           i,
                                                           NETWORK_IPV4);
                    }
                    break;
                }
#else // WIRELESS_LIB
                case ROUTING_PROTOCOL_FISHEYE:
                case ROUTING_PROTOCOL_OLSR_INRIA:
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    ERROR_ReportMissingLibrary(buf, "Wireless");
                }
#endif // WIRELESS_LIB
#ifdef ADDON_BOEINGFCS
                case ROUTING_PROTOCOL_CES_HSLS:
                {
                    RoutingCesHslsData* routingCesHsls = NULL;
                    if (node->appData.routingCesHsls == NULL)
                    {
                        RoutingCesHslsInit(node, nodeInput, i);
                    }

                    routingCesHsls =
                        (RoutingCesHslsData*) node->appData.routingCesHsls;
                    routingCesHsls->activeInterfaceIndexes[i] = 1;
                    routingCesHsls->numActiveInterfaces++;
                    break;
                }
#endif
                //InsertPatch APP_ROUTING_INIT_CODE

                default:
                {
                    break;
                }
            }
        }

        if (InterfaceType == NETWORK_IPV6 || InterfaceType == NETWORK_DUAL)
        {
            switch (ip->interfaceInfo[i]->ipv6InterfaceInfo->
                    routingProtocolType)
            {
                case ROUTING_PROTOCOL_RIPNG:
                {
                    RIPngInit(node, nodeInput, i);
                    break;
                }
#ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                   if (node->appData.olsr == NULL)
                   {
                      RoutingOlsrInriaInit(node, nodeInput, i, NETWORK_IPV6);
                   }
                   else
                   {
                       if (((RoutingOlsr*)node->appData.olsr)->ip_version ==
                            NETWORK_IPV4)
                       {
                         ERROR_ReportError("Both IPV4 and IPv6 instance of"
                          "OLSR-INRIA can not run simultaneously on a node");
                       }

                    ((RoutingOlsr* )node->appData.olsr)->numOlsrInterfaces++;

                      NetworkIpUpdateUnicastRoutingProtocolAndRouterFunction(
                                node,
                                ROUTING_PROTOCOL_OLSR_INRIA,
                                i,
                                NETWORK_IPV6);
                   }
                   break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    if(node->appData.olsrv2 == NULL)
                    {
                        RoutingOLSRv2_Niigata_Init(node,
                                                   nodeInput,
                                                   i,
                                                   NETWORK_IPV6);
                    }
                    else
                    {
                        RoutingOLSRv2_Niigata_AddInterface(node,
                                                           nodeInput,
                                                           i,
                                                           NETWORK_IPV6);
                    }
                    break;
                }
#else // WIRELESS_LIB
                case ROUTING_PROTOCOL_OLSR_INRIA:
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    ERROR_ReportMissingLibrary(buf, "Wireless");
                }
#endif // WIRELESS_LIB
        //InsertPatch APP_ROUTING_INIT_CODE

                default:
                {
                    break;
                }
            }
        }
    }

#ifdef ADDON_BOEINGFCS

    for (i=0; i< node->numberInterfaces; i++)
    {
        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            nodeInput,
            "INTRA-REGION-ROUTING-PROTOCOL",
            &retVal,
            buf);

        if (retVal && strcmp(buf, "ROUTING-CES-HSLS") == 0)
        {
            // This routing protocol is initialized in the app layer
            RoutingCesHslsData* routingCesHsls = NULL;
            NetworkIpAddUnicastIntraRegionRoutingProtocolType(
                                    node,
                                    ROUTING_PROTOCOL_CES_HSLS,
                                    i);

            if (node->appData.routingCesHsls == NULL)
            {
                RoutingCesHslsInit(node, nodeInput, i);
            }
#ifdef ADDON_NGCNMS
            else if (node->iface[i].disabled)
            {
                RoutingCesHslsReInitInterface(node, i);
            }
#endif
            routingCesHsls =
                (RoutingCesHslsData*) node->appData.routingCesHsls;
            routingCesHsls->activeInterfaceIndexes[i] = 1;
            routingCesHsls->numActiveInterfaces++;
        }
    }
#endif

    node->appData.uniqueId = 0;

    /* Setting up Border Gateway Protocol */
    node->appData.exteriorGatewayVar = NULL;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "EXTERIOR-GATEWAY-PROTOCOL",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "BGPv4") == 0)
        {
#ifdef ENTERPRISE_LIB
            node->appData.exteriorGatewayProtocol =
                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4;

            BgpInit(node, nodeInput);
#else // ENTERPRISE_LIB
            ERROR_ReportMissingLibrary("BGP", "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
        }
        else
        {
            // There is no other option now
            printf("The border gateway protocol should be BGPv4\n");
            assert(FALSE);
        }
    }

    /* Initialize Label Distribution Protocol, if necessary */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            nodeInput,
            "MPLS-LABEL-DISTRIBUTION-PROTOCOL",
            &retVal,
            buf);

        if (retVal)
        {
#ifdef ENTERPRISE_LIB
            if (strcmp(buf, "LDP") == 0)
            {
                // This has been added for IP-MPLS Integration to support
                // Dual IP Nodes.Initialize LDP only for IPv4 interfaces.
                if ((NetworkIpGetInterfaceType(node, i) ==
                        NETWORK_IPV4) || (NetworkIpGetInterfaceType(node, i) ==
                        NETWORK_DUAL))
                {
                    MplsLdpInit(node, nodeInput,
                            NetworkIpGetInterfaceAddress(node, i));
                }
            }
            else if (strcmp(buf, "RSVP-TE") == 0)
            {
                if (node->transportData.rsvpProtocol)
                {
                    // initialize RSVP/RSVP-TE variable and
                    // go out of the for loop
                    RsvpInit(node, nodeInput);
                    break;
                }
                else
                {
                    // ERROR : some-one has tried to initialize
                    // RSVP-TE witout Setting TRANSPORT-PROTOCOL-RSVP
                    // to YES in the cofiguration file.
                    ERROR_Assert(FALSE, "RSVP PROTOCOL IS OFF");
                }
            }
            else
            {
                fprintf(stderr, "Unknown MPLS-LABEL-DISTRIBUTION-PROTOCOL: "
                        "%s\n", buf);
                abort();
            }
#else // ENTERPRISE_LIB
            ERROR_ReportMissingLibrary(buf, "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
        } // end if (retVal)
    }// end for (i = 0; i < node->numberInterfaces; i++)

    if (!node->transportData.rsvpVariable)
    {
        // If either transport layer or RSVP-TE had not initialized
        // RSVP protocol, then set node->transportData.rsvpProtocol
        // to FALSE. This patch is needed because RSVP-TE is
        // implemented without implemeting RSVP. When RSVP will be
        // fully implemeted this patch will be removed.
        node->transportData.rsvpProtocol = FALSE;
    }

#ifdef ENTERPRISE_LIB
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RTP-ENABLED",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            RtpInit(node, nodeInput);
        }
        else if (strcmp(buf, "NO") == 0){}
        else
        {
             ERROR_ReportError("RTP-ENABLED should be "
                               "set to \"YES\" or \"NO\".\n");
        }
    }
#endif // ENTERPRISE_LIB

    // Initialize multicast group membership if required.
    APP_InitMulticastGroupMembershipIfAny(node, nodeInput);

    //AppLayerInitUserApplications(
    //   node, nodeInput, &node->appData.userApplicationData);
#ifdef ADDON_HELLO
    AppHelloInit(node, nodeInput);
#endif /* ADDON_HELLO */
#ifdef ADDON_BOEINGFCS
    D_Hierarchy *h = &node->partitionData->dynamicHierarchy;


    MopStatsHopCountPerDescriptorInit(node, nodeInput);

    MopStatsTputPerDescriptorInit(node, nodeInput);

    MopStatsE2EDelayPerDescriptorInit(node, nodeInput);

    MopStatsJitterPerDescriptorInit(node, nodeInput);

    MopStatsMcrPerDescriptorInit(node, nodeInput);
#endif
}


/*
 * NAME:        APP_ForbidSameSourceAndDest
 * PURPOSE:     Checks whether source node address and destination
 *              node address is same or not and if same it
 *              prints some warning messages and returns.
 * PARAMETERS:
 * RETURN:      none.
 */
static
BOOL APP_ForbidSameSourceAndDest(
    const char* inputString,
    const char* appName,
    NodeAddress sourceNodeId,
    NodeAddress destNodeId)
{
    if (sourceNodeId == destNodeId)
    {
#ifdef MILITARY_RADIOS_LIB
        char errorStr[INPUT_LENGTH] = {0};
#else
        char errorStr[MAX_STRING_LENGTH] = {0};
#endif /* MILITARY_RADIOS_LIB */

        sprintf(errorStr, "%s : Application source and destination"
                " nodes are identical!\n %s\n",
                appName, inputString);

        ERROR_ReportWarning(errorStr);

        return TRUE;
    }
    return FALSE;
}

#ifdef ADDON_NGCNMS
void
APP_ReInitializeApplications(Node* node, const NodeInput* nodeInput)
{
    NodeInput appInput;
    char appStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    BOOL retVal;
    int i;
    int numValues = 0;

    IO_ReadCachedFile(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "APP-CONFIG-FILE",
        &retVal,
        &appInput);

    if (retVal == FALSE)
    {
        return;
    }

    for (i = 0; i < appInput.numLines; i++)
    {
        sscanf(appInput.inputStrings[i], "%s", appStr);

        if (strcmp(appStr, "CBR") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char intervalStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            int itemsToSend;
            int itemSize;
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;
            unsigned tos = APP_DEFAULT_TOS;
            BOOL isRsvpTeEnabled = FALSE;
            char optionToken1[MAX_STRING_LENGTH];
            char optionToken2[MAX_STRING_LENGTH];
            char optionToken3[MAX_STRING_LENGTH];


            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %d %s %s %s %s %s %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            &itemSize,
                            intervalStr,
                            startTimeStr,
                            endTimeStr,
                            optionToken1,
                            optionToken2,
                            optionToken3);

            // dont need to check usage, because we are reading from
            // the same .app file as initialization. So if we get
            // to the restart point, the usage is correct.

            IO_AppParseSourceAndDestStrings(
                node,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            if (sourceNodeId == node->nodeId)
            {
                // reset the source address of cbr client
                AppCbrClientReInit(node, sourceAddr);
            }
        }
        else if (strcmp(appStr, "MCBR") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char intervalStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            int itemsToSend;
            int itemSize;
            NodeAddress sourceNodeId;
            Address sourceAddr;
            Address destAddr;
            NodeId destNodeId;
            BOOL isNodeId;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %d %s %s %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            &itemSize,
                            intervalStr,
                            startTimeStr,
                            endTimeStr);

            IO_AppParseSourceString(
                                node,
                                appInput.inputStrings[i],
                                sourceString,
                                &sourceNodeId,
                                &sourceAddr);



            if (MAPPING_GetNetworkType(destString) == NETWORK_IPV6)
            {
                destAddr.networkType = NETWORK_IPV6;

                IO_ParseNodeIdOrHostAddress(
                                destString,
                                &destAddr.interfaceAddr.ipv6,
                                &destNodeId);
            }
            else
            {
                destAddr.networkType = NETWORK_IPV4;

                IO_ParseNodeIdOrHostAddress(
                                destString,
                                &destAddr.interfaceAddr.ipv4,
                                &isNodeId);
            }

            // start modified to support IPv6 address
           if (destAddr.networkType == NETWORK_IPV4)
           {
               if (!(GetIPv4Address(destAddr) >= IP_MIN_MULTICAST_ADDRESS))
               {
                    fprintf(stderr, "MCBR: destination address is not a "
                             "multicast address\n");
               }
           }
           else
           {
                if(!IS_MULTIADDR6(GetIPv6Address(destAddr)))
                {
                    fprintf(stderr, "MCBR: destination address is not a "
                             "multicast address\n");
                }
                else if (!Ipv6IsValidGetMulticastScope(
                            node,
                            GetIPv6Address(destAddr)))
                {
                    fprintf(stderr, "MCBR: destination address is not a "
                             "valid multicast address\n");
                }
            }
            // end modification

            if (sourceNodeId == node->nodeId)
            {
                // reset the source address of cbr client
                AppMCbrClientReInit(node, sourceAddr);
            }

        }
        else if (strcmp(appStr, "SUPER-APPLICATION") == 0)
        {
            NodeAddress clientAddr;
            NodeAddress serverAddr;
            NodeAddress clientId;
            NodeAddress serverId;
            char clientString[MAX_STRING_LENGTH];
            char serverString[MAX_STRING_LENGTH];

            BOOL isSimulation = FALSE;
            BOOL wasFound;
            char simulationTimeStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            int numberOfReturnValue = 0;
            clocktype simulationTime = 0;
            double simTime = 0.0;
            clocktype sTime = 0;
            Node* newNode = NULL;

            // read in the simulation time from config file
            IO_ReadString( ANY_NODEID,
                           ANY_ADDRESS,
                           nodeInput,
                           "SIMULATION-TIME",
                           &wasFound,
                           simulationTimeStr);

            // read-in the simulation-time, convert into second, and store in simTime variable.
            numberOfReturnValue = sscanf(simulationTimeStr, "%s %s", startTimeStr, endTimeStr);
            if (numberOfReturnValue == 1) {
                simulationTime = (clocktype) TIME_ConvertToClock(startTimeStr);
            } else {
                sTime = (clocktype) TIME_ConvertToClock(startTimeStr);
                simulationTime = (clocktype) TIME_ConvertToClock(endTimeStr);
                simulationTime -= sTime;
            }

            TIME_PrintClockInSecond(simulationTime, simulationTimeStr);
            simTime = atof(simulationTimeStr);

            BOOL isStartArray = FALSE;
            BOOL isUNI = FALSE;
            BOOL isRepeat = FALSE;
            BOOL isSources = FALSE;
            BOOL isDestinations = FALSE;
            BOOL isConditions = FALSE;
            BOOL isSame = FALSE;
            BOOL isLoopBack   = FALSE;

            // check if the inputStrings has enable startArray, repeat, source or denstination.
            SuperApplicationCheckInputString(&appInput,
                                             i,
                                             &isStartArray,
                                             &isUNI,
                                             &isRepeat,
                                             &isSources,
                                             &isDestinations,
                                             &isConditions);
            if (isConditions){
                SuperApplicationHandleConditionsInputString(&appInput, i);
            }
            if (isStartArray){
                SuperApplicationHandleStartTimeInputString(nodeInput, &appInput, i, isUNI);
            }
            if (isSources){
                SuperApplicationHandleSourceInputString(nodeInput, &appInput, i);
            }
            if (isDestinations){
                SuperApplicationHandleDestinationInputString(nodeInput, &appInput, i);
            }
            if (isRepeat){
                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s",
                                clientString, serverString);

                IO_AppParseSourceAndDestStrings(
                    node,
                    appInput.inputStrings[i],
                    clientString,
                    &clientId,
                    &clientAddr,
                    serverString,
                    &serverId,
                    &serverAddr);

                //newNode = MAPPING_GetNodePtrFromHash(nodeHash, clientId);
                SuperApplicationHandleRepeatInputString(node, nodeInput, &appInput, i, isUNI, simTime);
            }

            retVal = sscanf(appInput.inputStrings[i],
                            "%*s %s %s",
                            clientString, serverString);

            if (retVal != 2) {
                fprintf(stderr,
                        "Wrong SUPER-APPLICATION configuration format!\n"
                        "SUPER-APPLICATION Client Server                        \n");
                ERROR_ReportError("Illegal transition");
            }

            IO_AppParseSourceAndDestStrings(
                node,
                appInput.inputStrings[i],
                clientString,
                &clientId,
                &clientAddr,
                serverString,
                &serverId,
                &serverAddr);

                if (node->nodeId != clientId && node->nodeId != serverId)
                {
                    continue;
                }

                if (APP_ForbidSameSourceAndDest(
                        appInput.inputStrings[i],
                        "SUPER-APPLICATION",
                        clientId,
                        serverId))
                {
                    isSame = TRUE;
                }

                if (isSame == FALSE){
                    // Handle Loopback Address
                    // TBD : must be handled by designner
                    if (NetworkIpIsLoopbackInterfaceAddress(serverAddr))
                    {
                        char errorStr[5 * MAX_STRING_LENGTH] = {0};
                        sprintf(errorStr, "SUPER-APPLICATION : Application doesn't support "
                                "Loopback Address!\n  %s\n", appInput.inputStrings[i]);

                        ERROR_ReportWarning(errorStr);
                        isLoopBack = TRUE;

                    }

                    if (isLoopBack == FALSE){
                            SuperApplicationInit(node,
                                                 clientAddr,
                                                 serverAddr,
                                                 appInput.inputStrings[i],
                                                 i,
                                                 nodeInput);
                        int resetIndex = -1;

                        // assumption is that clientAddr is this node's new address.
                        resetIndex = MAPPING_GetInterfaceIndexFromInterfaceAddress(node, clientAddr);
                        ERROR_Assert(resetIndex != -1, "Problem getting interface index!!\n");

                        NodeAddress resetAddress = node->iface[resetIndex].oldIpAddress;

                        IdToNodePtrMap* nodeHash;
                        nodeHash = node->partitionData->nodeIdHash;

                        newNode = MAPPING_GetNodePtrFromHash(nodeHash, serverId);
                        if (newNode != NULL) {
                                   SuperApplicationResetServerClientAddr(newNode,
                                                                         resetAddress,
                                                                         clientAddr);
                        }

                    }
                }
            }
        }

}
#endif

/*
 * NAME:        APP_InitializeApplications.
 * PURPOSE:     start applications on nodes according to user's
 *              specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void
APP_InitializeApplications(
    Node *firstNode,
    const NodeInput *nodeInput)
{
    NodeInput appInput;
    char appStr[MAX_STRING_LENGTH];

    BOOL retVal;
    int  i;
    int  numValues;
    Node* node = NULL;
    IdToNodePtrMap* nodeHash;

    BOOL gatekeeperFound = FALSE;
#ifdef ENTERPRISE_LIB
    H323CallModel callModel = Direct;
#endif // ENTERPRISE_LIB

#ifdef CELLULAR_LIB
    BOOL cellularAbstractApp = FALSE;
    CellularAbstractApplicationLayerData *appCellular;

    // initilize all the node's application data info
    // This segment code is moved from inside to here
    // to handle the case when no CELLULAR-ABSTARCT-APP is defined
    // and USER model is  not used, we can still power on the MS
    if (cellularAbstractApp == FALSE)
    {
        Node* nextNodePt  = NULL;

        nextNodePt=firstNode->partitionData->firstNode;

        // network preinit already before application initilization
        while (nextNodePt != NULL)
        {
            //check to see if we've initialized
            //if not, then do so
            appCellular =
                (CellularAbstractApplicationLayerData *)
                    nextNodePt->appData.appCellularAbstractVar;

            if (nextNodePt->networkData.cellularLayer3Var &&
                nextNodePt->networkData.cellularLayer3Var->cellularAbstractLayer3Data &&
                appCellular == NULL)
            {
                // USER layer initialized before application layer
                // if network is cellular model and has not been
                // initialized by USER layer
                // then initialize the cellular application
                CellularAbstractAppInit(nextNodePt, nodeInput);
            }
            nextNodePt = nextNodePt->nextNodeData;
        }
        cellularAbstractApp = TRUE;
    }
#endif // CELLULAR_LIB
#ifdef UMTS_LIB
    BOOL phoneCallApp = FALSE;
#endif

#ifdef ADDON_NGCNMS
    BOOL initialized = firstNode->enabled;
#endif

    if (firstNode == NULL)
        return; // this partition has no nodes.

    nodeHash = firstNode->partitionData->nodeIdHash;

    IO_ReadCachedFile(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "APP-CONFIG-FILE",
        &retVal,
        &appInput);
    // initialize appinput.numMaxLines to 0
    appInput.maxNumLines = 0;

    if (retVal == FALSE)
    {
        return;
    }

    for (i = 0; i < appInput.numLines; i++)
    {
        sscanf(appInput.inputStrings[i], "%s", appStr);

        if (firstNode->networkData.networkProtocol == IPV6_ONLY)
        {
            if (strcmp(appStr, "CBR") != 0
                && strcmp(appStr, "FTP/GENERIC") != 0
                && strcmp(appStr, "FTP") != 0
                && strcmp(appStr, "TELNET") != 0
                && strcmp(appStr, "HTTP") != 0
                && strcmp(appStr, "HTTPD") != 0
                && strcmp(appStr, "MCBR") !=0
                && strcmp(appStr, "TRAFFIC-GEN") !=0
                && strcmp(appStr, "SUPER-APPLICATION") !=0)
            {
                char buf[MAX_STRING_LENGTH];

                sprintf(buf,
                    "%s is not supported for IPv6 based Network.\n"
                    "Only CBR, FTP/GENERIC, FTP, TELNET ,HTTP ,MCBR,"
                    "SUPER-APPLICATION and TRAFFIC-GEN \n"
                    "applications are currently supported.\n", appStr);

                ERROR_ReportError(buf);
            }
        }

#ifdef DEBUG
        printf("Parsing for application type %s\n", appStr);
#endif /* DEBUG */

        if (strcmp(appStr, "FTP") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            int itemsToSend;
            char startTimeStr[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            startTimeStr);

            if (numValues != 4)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(errorString,
                        "Wrong FTP configuration format!\n"
                        "FTP <src> <dest> <items to send> <start time>\n");
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];

                printf("Starting FTP client with:\n");
                printf("  src nodeId:    %u\n", sourceNodeId);
                printf("  dst nodeId:    %u\n", destNodeId);
                IO_ConvertIpAddressToString(&destAddr, addrStr);
                printf("  dst address:   %s\n", addrStr);
                printf("  items to send: %d\n", itemsToSend);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
#endif /* DEBUG */

                AppFtpClientInit(
                    node, sourceAddr, destAddr, itemsToSend, startTime);
            }

            // Handle Loopback Address
            if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                     node,
                                     appInput.inputStrings[i],
                                     destAddr,
                                     destNodeId,
                                     sourceAddr,
                                     sourceNodeId))
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
            }

            if (node != NULL)
            {
                AppFtpServerInit(node, destAddr);
            }
        }

        //added - chanswitch

        if (strcmp(appStr, "CHANSWITCH") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            double hnThreshold;
            double csThreshold;
            char startTimeStr[MAX_STRING_LENGTH];
            char changeBackoffStr[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;


            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %lf %lf %s %s",
                            sourceString,
                            destString,
                            &hnThreshold,
                            &csThreshold,
                            changeBackoffStr,
                            startTimeStr);

            if (numValues != 6)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(errorString,
                        "Wrong CHANSWITCH configuration format!\n"
                        "CHANSWITCH <src> <dest> <sinr db threshold> <cs dbm threshold> <change backoff> <start time>\n");
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
                clocktype changeBackoffTime = TIME_ConvertToClock(changeBackoffStr);
// #ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];

                printf("Starting CHANSWITCH client with:\n");
                printf("  src nodeId:    %u\n", sourceNodeId);
                printf("  dst nodeId:    %u\n", destNodeId);
                IO_ConvertIpAddressToString(&destAddr, addrStr);
                printf("  dst address:   %s\n", addrStr);
                printf("  hidden node sinr threshold (dB): %lf\n",hnThreshold);
                printf("  cs signal strength threshold (dBm): %lf\n",csThreshold);
                ctoa(changeBackoffTime, clockStr);
                printf("  minimum delay btwn channel changes:    %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
// #endif /* DEBUG */

                AppChanswitchClientInit(
                    node, sourceAddr, destAddr, startTime, hnThreshold, csThreshold, changeBackoffTime);
            }

            // Handle Loopback Address
            if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                     node,
                                     appInput.inputStrings[i],
                                     destAddr,
                                     destNodeId,
                                     sourceAddr,
                                     sourceNodeId))
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
            }

            if (node != NULL)
            {
                AppChanswitchServerInit(node, destAddr);
            }
        }

        //end of added - chanswitch

        //added - chanswitch_sinr

        if (strcmp(appStr, "CHANSWITCH_SINR") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            int itemsToSend;
            char startTimeStr[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            startTimeStr);

            if (numValues != 4)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(errorString,
                        "Wrong CHANSWITCH_SINR configuration format!\n"
                        "CHANSWITCH_SINR <src> <dest> <items to send> <start time>\n");
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];

                printf("Starting CHANSWITCH_SINR client with:\n");
                printf("  src nodeId:    %u\n", sourceNodeId);
                printf("  dst nodeId:    %u\n", destNodeId);
                IO_ConvertIpAddressToString(&destAddr, addrStr);
                printf("  dst address:   %s\n", addrStr);
                printf("  items to send: %d\n", itemsToSend);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
#endif /* DEBUG */

                AppChanswitchSinrClientInit(
                    node, sourceAddr, destAddr, itemsToSend, startTime);
            }

            // Handle Loopback Address
            if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                     node,
                                     appInput.inputStrings[i],
                                     destAddr,
                                     destNodeId,
                                     sourceAddr,
                                     sourceNodeId))
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
            }

            if (node != NULL)
            {
                AppChanswitchSinrServerInit(node, destAddr);
            }
        }

        //end of added - chanswitch_sinr

        else
        if (strcmp(appStr, "FTP/GENERIC") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            int itemsToSend;
            int itemSize;
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            char tosStr[MAX_STRING_LENGTH];
            char tosValueStr[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;
            unsigned tos = APP_DEFAULT_TOS;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %d %s %s %s %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            &itemSize,
                            startTimeStr,
                            endTimeStr,
                            tosStr,
                            tosValueStr);

            switch (numValues)
            {
                case 6:
                    break;
                case 8 :
                    if (APP_AssignTos(tosStr, tosValueStr, &tos))
                    {
                        break;
                    } // else fall through default
                default:
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(errorString,
                            "Wrong FTP/GENERIC configuration format!\n"
                            "FTP/GENERIC <src> <dest> <items to send> "
                            "<item size> <start time> <end time> "
                            "[TOS <tos-value> | DSCP <dscp-value>"
                            " | PRECEDENCE <precedence-value>]\n");
                    ERROR_ReportError(errorString);
                }
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
                clocktype endTime = TIME_ConvertToClock(endTimeStr);

#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];

                printf("Starting FTP client with:\n");
                printf("  src nodeId:    %u\n", sourceNodeId);
                printf("  dst nodeId:    %u\n", destNodeId);
                IO_ConvertIpAddressToString(&destAddr, addrStr);
                printf("  dst address:   %s\n", addrStr);
                printf("  items to send: %d\n", itemsToSend);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
                ctoa(endTime, clockStr);
                printf("  end time:      %s\n", clockStr);
                printf("  tos:           %d\n", tos);
#endif /* DEBUG */

                AppGenFtpClientInit(
                    node,
                    sourceAddr,
                    destAddr,
                    itemsToSend,
                    itemSize,
                    startTime,
                    endTime,
                    tos);
            }

            // Handle Loopback Address
            if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                     node,
                                     appInput.inputStrings[i],
                                     destAddr,
                                     destNodeId,
                                     sourceAddr,
                                     sourceNodeId))
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
            }

            if (node != NULL)
            {
                AppGenFtpServerInit(node, destAddr);
            }
        }
        else if (strcmp(appStr, "TELNET") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char sessDurationStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %s %s",
                            sourceString,
                            destString,
                            sessDurationStr,
                            startTimeStr);

            if (numValues != 4)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(errorString,
                        "Wrong TELNET configuration format!\nTELNET"
                        " <src> <dest> <session duration> <start time>\n");
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime;
                clocktype sessDuration;

#ifdef DEBUG
                startTime = TIME_ConvertToClock(startTimeStr);
                startTime += getSimStartTime(firstNode);
                char startTimeStr2[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(startTime, startTimeStr2);

                char clockStr[MAX_CLOCK_STRING_LENGTH];
                printf("Starting TELNET client with:\n");
                printf("  src nodeId:       %u\n", sourceNodeId);
                printf("  dst nodeId:       %u\n", destNodeId);
                printf("  session duration: %s\n", sessDurationStr);
                printf("  start time:       %s\n", startTimeStr2);
#endif /* DEBUG */

                startTime    = TIME_ConvertToClock(startTimeStr);

#ifdef DEBUG
                {
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];
                printf("Starting TELNET client with:\n");
                printf("  src nodeId:       %u\n", sourceNodeId);
                printf("  dst nodeId:       %u\n", destNodeId);
                IO_ConvertIpAddressToString(&destAddr, addrStr);
                printf("  dst address:      %s\n", addrStr);
                ctoa(sessDuration, clockStr);
                printf("  session duration: %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:       %s\n", clockStr);
                }
#endif /* DEBUG */

                sessDuration = TIME_ConvertToClock(sessDurationStr);

#ifdef DEBUG
                {
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];
                printf("Starting TELNET client with:\n");
                printf("  src nodeId:       %u\n", sourceNodeId);
                printf("  dst nodeId:       %u\n", destNodeId);
                IO_ConvertIpAddressToString(&destAddr, addrStr);
                printf("  dst address:      %s\n", addrStr);
                ctoa(sessDuration, clockStr);
                printf("  session duration: %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:       %s\n", clockStr);
                }
#endif /* DEBUG */

                AppTelnetClientInit(
                    node,
                    sourceAddr,
                    destAddr,
                    sessDuration,
                    startTime);
            }

            // Handle Loopback Address
            if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                     node,
                                     appInput.inputStrings[i],
                                     destAddr,
                                     destNodeId,
                                     sourceAddr,
                                     sourceNodeId))
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
            }

            if (node != NULL)
            {
                AppTelnetServerInit(node, destAddr);
            }
        }
        else
        if (strcmp(appStr, "CBR") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char intervalStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            int itemsToSend;
            int itemSize;
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;
            unsigned tos = APP_DEFAULT_TOS;
            BOOL isRsvpTeEnabled = FALSE;
            char optionToken1[MAX_STRING_LENGTH];
            char optionToken2[MAX_STRING_LENGTH];
            char optionToken3[MAX_STRING_LENGTH];


            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %d %s %s %s %s %s %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            &itemSize,
                            intervalStr,
                            startTimeStr,
                            endTimeStr,
                            optionToken1,
                            optionToken2,
                            optionToken3);

            switch (numValues)
            {
                case 7:
                    break;
                case 8:
                    if (!strcmp(optionToken1, "RSVP-TE"))
                    {
                        isRsvpTeEnabled = TRUE;
                        break;
                    } // else fall through default
                case 9 :
                    if (APP_AssignTos(optionToken1, optionToken2, &tos))
                    {
                        break;
                    } // else fall through default
                case 10 :
                    if (!strcmp(optionToken1, "RSVP-TE"))
                    {
                        isRsvpTeEnabled = TRUE;

                        if (APP_AssignTos(optionToken2, optionToken3, &tos))
                        {
                            break;
                        }// else fall through default
                    }
                    else
                    {
                        if (APP_AssignTos(optionToken1, optionToken2, &tos))
                        {
                            if (!strcmp(optionToken3, "RSVP-TE"))
                            {
                                isRsvpTeEnabled = TRUE;
                                break;
                            }
                        }// else fall through default
                    }
                default:
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(errorString,
                        "Wrong CBR configuration format!\n"
                        "CBR <src> <dest> <items to send> "
                        "<item size> <interval> <start time> "
                        "<end time> [TOS <tos-value> | DSCP <dscp-value>"
                        " | PRECEDENCE <precedence-value>] [RSVP-TE]\n");
                    ERROR_ReportError(errorString);
                }
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

#ifdef ADDON_NGCNMS
            if (NODE_IsDisabled(firstNode) &&
                firstNode->nodeId != sourceNodeId)
            {
                continue;
            }
#endif

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
                clocktype endTime = TIME_ConvertToClock(endTimeStr);
                clocktype interval = TIME_ConvertToClock(intervalStr);

                if ((node->adaptationData.adaptationProtocol
                    != ADAPTATION_PROTOCOL_NONE)
                    && (!node->adaptationData.endSystem))
                {

                    char err[MAX_STRING_LENGTH];
                    sprintf(err,"Only end system can be a valid source\n");
                    ERROR_ReportWarning(err);

                    return;
                }

#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                printf("Starting CBR client with:\n");
                printf("  src nodeId:    %u\n", sourceNodeId);
                printf("  dst nodeId:    %u\n", destNodeId);
                printf("  dst address:   %u\n", destAddr);
                printf("  items to send: %d\n", itemsToSend);
                printf("  item size:     %d\n", itemSize);
                ctoa(interval, clockStr);
                printf("  interval:      %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
                ctoa(endTime, clockStr);
                printf("  end time:      %s\n", clockStr);
                printf("  tos:           %u\n", tos);
#endif /* DEBUG */

                AppCbrClientInit(
                    node,
                    sourceAddr,
                    destAddr,
                    itemsToSend,
                    itemSize,
                    interval,
                    startTime,
                    endTime,
                    tos,
                    isRsvpTeEnabled);
           }

            if (sourceAddr.networkType != destAddr.networkType)
            {
                char err[MAX_STRING_LENGTH];

                sprintf(err,
                        "CBR: At node %d, Source and Destination IP"
                        " version mismatch inside %s\n",
                    node->nodeId,
                    appInput.inputStrings[i]);

                ERROR_Assert(FALSE, err);
            }

            // Handle Loopback Address
            if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                    node,
                                    appInput.inputStrings[i],
                                    destAddr,
                                    destNodeId,
                                    sourceAddr,
                                    sourceNodeId))
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
            }

            if (node != NULL)
            {
                AppCbrServerInit(node);
            }
        }
        		else if(strcmp(appStr, "WBEST") ==  0)
		{
			NodeAddress sourceNodeId;
			Address		sourceAddr;
			NodeAddress destNodeId;
			Address		destAddr;

			char sourceString[MAX_STRING_LENGTH];
			char destString[MAX_STRING_LENGTH];

			retVal = sscanf(appInput.inputStrings[i],"%*s %s %s",sourceString,destString);

			IO_AppParseSourceAndDestStrings(firstNode,appInput.inputStrings[i],	sourceString,&sourceNodeId,	&sourceAddr,destString,&destNodeId,	&destAddr);
			node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);

			if (node != NULL) {
				AppWbestClientInit(node, sourceAddr, sourceNodeId, destAddr, destNodeId);
			}
			// Handle Loopback Address
			if (node == NULL || !APP_SuccessfullyHandledLoopback(node,appInput.inputStrings[i],destAddr,destNodeId,sourceAddr,sourceNodeId))
			{
				node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
			}

			if (node != NULL)
			{
				AppWbestServerInit(node, destAddr,sourceAddr, destNodeId,sourceNodeId);
			}
			
		}

		else if (strcmp(appStr, "VIDEO") == 0)
		{
			NodeAddress sourceNodeId;
			Address		sourceAddr;
			NodeAddress destNodeId;
			Address		destAddr;
			int			startTime;

            char startTimeStr[MAX_STRING_LENGTH];
			char sourceString[MAX_STRING_LENGTH];
			char destString[MAX_STRING_LENGTH];

			char strDXManagerPath[MAX_STRING_LENGTH];
			char strGRFPath[MAX_STRING_LENGTH];
			char strQoSPath[MAX_STRING_LENGTH];

			unsigned tos = APP_DEFAULT_TOS;

			int nFPS	= 30;
			int Width	= 352; // default value for video width
			int Height	= 288; // default value for video height
			int Jitter	= 150; // default value for jitter

			AppDataVideoClient*		pDataVideoClient;

			retVal = sscanf(appInput.inputStrings[i],"%*s %s %s %d %d %d %d %s %s %s %s"
				,sourceString,destString,&nFPS,&Jitter, &Width, &Height, startTimeStr,
				strDXManagerPath,strGRFPath,strQoSPath);

			if(retVal == FALSE) {
				ERROR_Assert(FALSE, "Input string doesn't match well.");
			}

			IO_AppParseSourceAndDestStrings(firstNode,appInput.inputStrings[i],	
				sourceString,&sourceNodeId,	&sourceAddr,destString,&destNodeId,	&destAddr);

			node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);

			if (node != NULL) {
				clocktype startTime = TIME_ConvertToClock(startTimeStr);

				pDataVideoClient = AppVideoClientInit(node, sourceAddr, sourceNodeId,destAddr, destNodeId, nFPS,
					Jitter, Width, Height, startTime,strDXManagerPath,strGRFPath,strQoSPath);  

				if(pDataVideoClient == NULL) {
					ERROR_Assert(FALSE, "AppVideoClientInit error!");
				}
			}

			// Handle Loopback Address
			if (node == NULL || !APP_SuccessfullyHandledLoopback(node,appInput.inputStrings[i],destAddr,destNodeId,sourceAddr,sourceNodeId))
			{
				node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);

				if (node != NULL) {
					AppVideoServerInit(node,destAddr,destNodeId,sourceAddr,sourceNodeId);
				}
			}
		}

        
        else
        if (strcmp(appStr, "LOOKUP") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char requestIntervalStr[MAX_STRING_LENGTH];
            char replyDelayStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            char tosStr[MAX_STRING_LENGTH];
            char tosValueStr[MAX_STRING_LENGTH];
            int requestsToSend;
            int requestSize;
            int replySize;
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;
            unsigned tos = APP_DEFAULT_TOS;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %d %d %s %s %s %s %s %s",
                            sourceString,
                            destString,
                            &requestsToSend,
                            &requestSize,
                            &replySize,
                            requestIntervalStr,
                            replyDelayStr,
                            startTimeStr,
                            endTimeStr,
                            tosStr,
                            tosValueStr);

            switch (numValues)
            {
                case 9:
                    break;
                case 11 :
                    if (APP_AssignTos(tosStr, tosValueStr, &tos))
                    {
                        break;
                    } // else fall through default
                default:
                {
                    char errorString[BIG_STRING_LENGTH];
                    sprintf(errorString,
                            "Wrong LOOKUP configuration format!\n"
                            "LOOKUP <src> <dest> <requests to send> "
                            "<request size> <reply size> <request interval> "
                            "<reply delay> <start time> <end time>"
                            " [TOS <tos-value> | DSCP <dscp-value>"
                            " | PRECEDENCE <precedence-value>]\n");
                    ERROR_ReportError(errorString);
                }
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
                clocktype endTime = TIME_ConvertToClock(endTimeStr);
                clocktype requestInterval
                    = TIME_ConvertToClock(requestIntervalStr);
                clocktype replyDelay = TIME_ConvertToClock(replyDelayStr);

#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                printf("Starting LOOKUP client with:\n");
                printf("  src nodeId:       %u\n", sourceNodeId);
                printf("  dst nodeId:       %u\n", destNodeId);
                printf("  dst address:      %u\n", destAddr);
                printf("  requests to send: %d\n", requestsToSend);
                printf("  request size:     %d\n", requestSize);
                printf("  reply size:       %d\n", replySize);
                ctoa(requestInterval, clockStr);
                printf("  request interval: %s\n", clockStr);
                ctoa(replyDelay, clockStr);
                printf("  reply delay:      %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:       %s\n", clockStr);
                ctoa(endTime, clockStr);
                printf("  end time:         %s\n", clockStr);
                printf("  tos:              %u\n", tos);
#endif /* DEBUG */

                AppLookupClientInit(
                    node,
                    GetIPv4Address(sourceAddr),
                    GetIPv4Address(destAddr),
                    requestsToSend,
                    requestSize,
                    replySize,
                    requestInterval,
                    replyDelay,
                    startTime,
                    endTime,
                    tos);
            }

            // Handle Loopback Address
            if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                     node,
                                     appInput.inputStrings[i],
                                     destAddr, destNodeId,
                                     sourceAddr, sourceNodeId))
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
            }

            if (node != NULL)
            {
                AppLookupServerInit(node);
            }
        }
        else
        if (strcmp(appStr, "MCBR") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char intervalStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            int itemsToSend;
            int itemSize;
            NodeAddress sourceNodeId;
            Address sourceAddr;
            Address destAddr;
            NodeId destNodeId;
            BOOL isNodeId;
            unsigned tos = APP_DEFAULT_TOS;
            char optionToken1[MAX_STRING_LENGTH];
            char optionToken2[MAX_STRING_LENGTH];

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %d %s %s %s %s %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            &itemSize,
                            intervalStr,
                            startTimeStr,
                            endTimeStr,
                            optionToken1,
                            optionToken2);

            switch (numValues)
            {
                case 7:
                    break;
                case 9 :
                    if (APP_AssignTos(optionToken1, optionToken2, &tos))
                    {
                        break;
                    } // else fall through default
                default:
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(errorString,
                        "Wrong MCBR configuration format!\n"
                        "MCBR <src> <dest> <items to send> <item size> "
                        "<interval> <start time> <end time> "
                        "[TOS <tos-value> | DSCP <dscp-value>"
                        " | PRECEDENCE <precedence-value>]\n");
                    ERROR_ReportError(errorString);
                }
            }


#ifdef ADDON_NGCNMS
            if (NODE_IsDisabled(firstNode) &&
                firstNode->nodeId != sourceNodeId)
            {
                continue;
            }
#endif


            if (MAPPING_GetNetworkType(destString) == NETWORK_IPV6)
            {
                destAddr.networkType = NETWORK_IPV6;

                IO_ParseNodeIdOrHostAddress(
                    destString,
                    &destAddr.interfaceAddr.ipv6,
                    &destNodeId);
            }
            else
            {
                destAddr.networkType = NETWORK_IPV4;

                IO_ParseNodeIdOrHostAddress(
                    destString,
                    &destAddr.interfaceAddr.ipv4,
                    &isNodeId);
            }

           if (destAddr.networkType == NETWORK_IPV4)
           {
               if (!(GetIPv4Address(destAddr) >= IP_MIN_MULTICAST_ADDRESS))
               {
                    fprintf(stderr, "MCBR: destination address is not a "
                             "multicast address\n");
               }
           }
           else
           {
                if(!IS_MULTIADDR6(GetIPv6Address(destAddr)))
                {
                    fprintf(stderr, "MCBR: destination address is not a "
                             "multicast address\n");
                }
                else if (!Ipv6IsValidGetMulticastScope(
                            node,
                            GetIPv6Address(destAddr)))
                {
                    fprintf(stderr, "MCBR: destination address is not a "
                             "valid multicast address\n");
                }
            }

            IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destAddr.networkType);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
                clocktype endTime = TIME_ConvertToClock(endTimeStr);
                clocktype interval = TIME_ConvertToClock(intervalStr);

#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                printf("Starting MCBR client with:\n");
                printf("  src nodeId:    %u\n", sourceNodeId);
                printf("  dst address:   %u\n", destAddr);
                printf("  items to send: %d\n", itemsToSend);
                printf("  item size:     %d\n", itemSize);
                ctoa(interval, clockStr);
                printf("  interval:      %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
                ctoa(endTime, clockStr);
                printf("  end time:      %s\n", clockStr);
                printf("  tos:           %d\n", tos);
#endif /* DEBUG */

                AppMCbrClientInit(
                    node,
                    sourceAddr,
                    destAddr,
                    itemsToSend,
                    itemSize,
                    interval,
                    startTime,
                    endTime,
                    tos);

                if (sourceAddr.networkType != destAddr.networkType)
                {
                    char err[MAX_STRING_LENGTH];

                    sprintf(err,
                            "MCBR: At node %d, Source and Destination IP"
                            " version mismatch inside %s\n",
                        node->nodeId,
                        appInput.inputStrings[i]);

                    ERROR_Assert(FALSE, err);
                }
            }
        }

        else
        if (strcmp(appStr, "mgen") == 0)
        {
#ifdef ADDON_MGEN4
            int sourceNodeId = ANY_SOURCE_ADDR;
            int destNodeId   = ANY_DEST;
            Node* node1 = NULL;
            retVal = sscanf(appInput.inputStrings[i], "%*s %u %u",
                            &sourceNodeId, &destNodeId);
            node1 = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node1 != NULL)
            {
                MgenInit(node1, appInput.inputStrings[i]);
            }
#else // ADDON_MGEN4
            ERROR_ReportError("MGEN4 addon required\n");
#endif // ADDON_MGEN4
        }
        else
            if (strcmp(appStr, "mgen3") == 0)
            {
#ifdef ADDON_MGEN3
                int sourceNodeId;

                 retVal = sscanf(appInput.inputStrings[i],
                                    "%*s %u", &sourceNodeId);

                 node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);

                 if (node != NULL)
                 {
                    AppMgenInit(node, appInput.inputStrings[i]);
                 }
#else // ADDON_MGEN3
            ERROR_ReportError("MGEN3 addon required\n");
#endif // ADDON_MGEN3
            }
            else
            if (strcmp(appStr, "drec") == 0)
            {
#ifdef ADDON_MGEN3
               int sourceNodeId;

               retVal = sscanf(appInput.inputStrings[i],
                    "%*s %u", &sourceNodeId);

               node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
               if (node != NULL)
               {
                   AppDrecInit(node, appInput.inputStrings[i]);
               }
#else // ADDON_MGEN3
            ERROR_ReportError("MGEN3 addon required\n");
#endif // ADDON_MGEN3
           }

#ifndef STANDALONE_MESSENGER_APPLICATION
        else
        if (strcmp(appStr, "MESSENGER-APP") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            NodeAddress sourceAddr;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s",
                            sourceString);
            assert(numValues == 1);

            IO_AppParseSourceString(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                MessengerInit(node);
            }
        }

#else /* STANDALONE_MESSENGER_APPLICATION */
        else
        if (strcmp(appStr, "MESSENGER-APP") == 0)
        {
            char srcStr[MAX_STRING_LENGTH];
            char destStr[MAX_STRING_LENGTH];
            char tTypeStr[MAX_STRING_LENGTH];
            char appTypeStr[MAX_STRING_LENGTH];
            char lifeTimeStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char intervalStr[MAX_STRING_LENGTH];
            char filenameStr[MAX_STRING_LENGTH];
            char errorString[MAX_STRING_LENGTH];
#ifdef MILITARY_RADIOS_LIB
            char destNPGStr[MAX_STRING_LENGTH];
            int destNPGId = 0;
#endif
            int fragSize = 0;
            int fragNum = 0;
            NodeAddress srcNodeId;
            NodeAddress srcAddr;
            NodeAddress destNodeId = 0;
            NodeAddress destAddr;
            TransportType t_type;
            MessengerAppType app_type = GENERAL;
            clocktype lifeTime = 0;
            clocktype startTime = 0;
            clocktype interval = 0;
            char* filename = NULL;
#ifdef MILITARY_RADIOS_LIB
            int tempnumValues = sscanf(appInput.inputStrings[i],
                                "%*s %s", srcStr);

            IO_AppParseSourceString(firstNode,
                appInput.inputStrings[i],
                srcStr,
                &srcNodeId,
                &srcAddr);
            node = MAPPING_GetNodePtrFromHash(nodeHash, srcNodeId);

            if(node == NULL){
                continue;
            }
            if (node != NULL)
            {
                int numValues = sscanf(appInput.inputStrings[i],
                                    "%*s %s %s %s %s %s %s %s %d %d %s %d %s",
                                    srcStr,
                                    destStr,
                                    tTypeStr,
                                    appTypeStr,
                                    lifeTimeStr,
                                    startTimeStr,
                                    intervalStr,
                                    &fragSize,
                                    &fragNum,
                                    destNPGStr,
                                    &destNPGId,
                                    filenameStr);

                if (numValues == 11)
                {
                    filename = NULL;
                }
                if (numValues == 12)
                {
                    filename = (char*) MEM_malloc(strlen(filenameStr) + 1);
                    strcpy(filename, filenameStr);
                }
                if (numValues == 10)
                {
                    filename = (char*) MEM_malloc(strlen(destNPGStr) + 1);
                    strcpy(filename, destNPGStr);
                }

                if((numValues > 9) && (strcmp(destNPGStr, "NPG") == 0))
                {
                    if (node->macData[MAC_DEFAULT_INTERFACE]->macProtocol
                        == MAC_PROTOCOL_TADIL_LINK16)
                    {
                        if (numValues != 11 && numValues != 12)
                        {
                            int n = 
                                snprintf(errorString,
                            	    MAX_STRING_LENGTH,
                                    "MESSENGER:Wrong MESSENGER configuration format!\n"
                                    "\tMESSENGER-APP <src> <dest> <transport_type> "
                                    "<life_time>\n\t\t<start_time> <interval> "
                                    "<fragment_size> <fragment_num> "
                                    "NPG <desttination NPGId> "
                                    "[additional_data_file_name]\n");
                            ERROR_ReportError(errorString);
                        }
                    }
                    else
                    {
                        // Ignore the Destination NPG id information
                        destNPGId = 0;
                        if (numValues == 10)
                        {
                            filename = NULL;
                        }
                    }
                }
                else
                {
                    if (node->macData[MAC_DEFAULT_INTERFACE]->macProtocol
                        == MAC_PROTOCOL_TADIL_LINK16)
                    {
                        int n = snprintf(errorString,
                            MAX_STRING_LENGTH,
                            "MESSENGER:Wrong MESSENGER configuration format!\n"
                            "\tMESSENGER-APP <src> <dest> <transport_type> "
                            "<life_time>\n\t\t<start_time> <interval> "
                            "<fragment_size> <fragment_num> "
                            "NPG <desttination NPGId> "
                            "[additional_data_file_name]\n");
                        ERROR_ReportError(errorString);
                    }
                    else
                    {
                        destNPGId = 0;
                        if (numValues != 9 && numValues != 10 && numValues != 12)
                        {
                            sprintf(errorString,
                                "MESSENGER:Wrong MESSENGER configuration format!\n"
                                "\tMESSENGER-APP <src> <dest> <transport_type> "
                                "<life_time>\n\t\t<start_time> <interval> "
                                "<fragment_size> <fragment_num> "
                                "[additional_data_file_name]\n");
                            ERROR_ReportError(errorString);
                        }
                        if (numValues == 9)
                        {
                            filename = NULL;
                        }

                    }
                }
            }
#else

            int numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %s %s %s %s %s %d %d %s",
                                srcStr,
                                destStr,
                                tTypeStr,
                                appTypeStr,
                                lifeTimeStr,
                                startTimeStr,
                                intervalStr,
                                &fragSize,
                                &fragNum,
                                filenameStr);

            if (numValues != 9 && numValues != 10)
            {
                sprintf(errorString,
                    "MESSENGER:Wrong MESSENGER configuration format!\n"
                    "\tMESSENGER-APP <src> <dest> <transport_type> "
                    "<life_time>\n\t\t<start_time> <interval> "
                    "<fragment_size> <fragment_num> "
                    "[additional_data_file_name]\n");
                ERROR_ReportError(errorString);
            }

            if (numValues == 9)
            {
                filename = NULL;
            }
            else
            {
                filename = (char*) MEM_malloc(strlen(filenameStr) + 1);
                strcpy(filename, filenameStr);
            }
#endif
            IO_AppParseSourceString(firstNode,
                                appInput.inputStrings[i],
                                srcStr,
                                &srcNodeId,
                                &srcAddr);

            if (strcmp(destStr, "255.255.255.255") == 0)
            {
                destAddr = ANY_DEST;
            }
            else
            {
                IO_AppParseDestString(firstNode,
                                      appInput.inputStrings[i],
                                      destStr,
                                      &destNodeId,
                                      &destAddr);
            }

            if (strcmp(tTypeStr, "UNRELIABLE") == 0)
            {
                t_type = TRANSPORT_TYPE_UNRELIABLE;
            }
            else if (strcmp(tTypeStr, "RELIABLE") == 0)
            {
                if (destAddr == ANY_DEST)
                {
                    sprintf(errorString, "MESSENGER:BROADCAST "
                            "is not assigned for RELIABLE service.\n");
                    ERROR_ReportError(errorString);
                }
                t_type = TRANSPORT_TYPE_RELIABLE;
            }
            else if (strcmp(tTypeStr, "MAC") == 0)
            {
                t_type = TRANSPORT_TYPE_MAC;
            }
            else
            {
                sprintf(errorString,
                        "MESSENGER: %s is not supported by TRANSPORT-TYPE.\n"
                            "Only UNRELIABLE / RELIABLE / TADIL"
                            " are currently supported.\n", tTypeStr);
                ERROR_ReportError(errorString);
            }

            if (strcmp(appTypeStr, "GENERAL") == 0)
            {
                app_type = GENERAL;
            }
            else if (strcmp(appTypeStr, "VOICE") == 0)
            {
                if (t_type == TRANSPORT_TYPE_RELIABLE ||
                    t_type == TRANSPORT_TYPE_MAC ||
                    filename != NULL)
                {
                    sprintf(errorString, "MESSENGER:VOICE application "
                            "only support UNRELIABLE transport type "
                            "and not support external file data.\n");
                    ERROR_ReportError(errorString);
                }
                app_type = VOICE_PACKET;
            }
            else
            {
                sprintf(errorString, "MESSENGER:%s is not supported by "
                        "APPLICATION-TYPE.\nOnly GENERAL/VOICE are "
                        "currently supported.\n", appTypeStr);
                ERROR_ReportError(errorString);
            }
            lifeTime = TIME_ConvertToClock(lifeTimeStr);
            startTime = TIME_ConvertToClock(startTimeStr);
            interval = TIME_ConvertToClock(intervalStr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, srcNodeId);

            if(node == NULL){
                continue;
            }
            if (node != NULL)
            {
#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                printf("Starting MESSENGER client with:\n");
                printf("  src nodeId:    %u\n", srcNodeId);

                if (destAddr == ANY_DEST)
                {
                    printf("  dst address:   255.255.255.255\n");
                }
                else
                {
                    printf("  dst address:   %u\n", destAddr);
                }

                printf("  transport type: %s\n", tTypeStr);
                ctoa(lifeTime, clockStr);
                printf("  life time:      %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
                ctoa(interval, clockStr);
                printf("  interval:      %s\n", clockStr);
                printf("  fragment size:      %d\n", fragSize);
                printf("  fragment number:      %d\n", fragNum);

                if (filename)
                {
                    printf("  file name:      %s\n", filename);
                }
#endif // DEBUG
                MessengerClientInit(
                    node,
                    srcAddr,
                    destAddr,
                    t_type,
                    app_type,
                    lifeTime,
                    startTime,
                    interval,
                    fragSize,
                    fragNum,
#ifdef MILITARY_RADIOS_LIB
                    (unsigned short)destNPGId,
#endif // MILITARY_RADIOS_LIB
                    filename);
        }
            if (destAddr != ANY_DEST)
            {
                node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);

                if (node != NULL)
                {
                    MessengerServerListen(node);
                }
            }
        }
#endif // STANDALONE_MESSENGER_APPLICATION

        else
        if (strcmp(appStr, "L16-CBR") == 0)
        {
#ifdef ADDON_LINK16
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char intervalStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            int itemsToSend;
            int itemSize;
            NodeAddress sourceNodeId;
            NodeAddress sourceAddr;
            NodeAddress destNodeId;
            NodeAddress destAddr;
            BOOL isNodeId;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %d %s %s %s",
                            sourceString,
                            destString,
                            &itemsToSend,
                            &itemSize,
                            intervalStr,
                            startTimeStr,
                            endTimeStr);

            if (numValues != 7)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(errorString,
                       "Wrong LINK16-CBR configuration format!\n"
                       "LINK16-CBR <src> <dest> <items to send> <item size> "
                       "<interval> <start time> <end time>\n");
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr);

            IO_AppParseHostString(
                    firstNode,
                    appInput.inputStrings[i],
                    destString,
                    &destNodeId,
                    &destAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);

            if (node != NULL)
            {
                clocktype startTime = TIME_ConvertToClock(startTimeStr);
                clocktype endTime = TIME_ConvertToClock(endTimeStr);
                clocktype interval = TIME_ConvertToClock(intervalStr);

#ifdef DEBUG
                char clockStr[MAX_CLOCK_STRING_LENGTH];
                printf("Starting LINK16-CBR client with:\n");
                printf("  src nodeId:    %u\n", sourceNodeId);
                printf("  dst address:   %u\n", destAddr);
                printf("  items to send: %d\n", itemsToSend);
                printf("  item size:     %d\n", itemSize);
                ctoa(interval, clockStr);
                printf("  interval:      %s\n", clockStr);
                ctoa(startTime, clockStr);
                printf("  start time:    %s\n", clockStr);
                ctoa(endTime, clockStr);
                printf("  end time:      %s\n", clockStr);
#endif /* DEBUG */

                AppSpawar_Link16CbrClientInit(
                    node,
                    sourceAddr,
                    destAddr,
                    itemsToSend,
                    itemSize,
                    interval,
                    startTime,
                    endTime);
            }
#else // ADDON_LINK16
            ERROR_ReportMissingAddon(appStr, "Link-16");
#endif // ADDON_LINK16
        }

        else
        if (strcmp(appStr, "INTERFACETUTORIAL") == 0)
        {
#ifdef TUTORIAL_INTERFACE
            char nodeString[MAX_STRING_LENGTH];
            char dataString[MAX_STRING_LENGTH];
            NodeAddress nodeId;
            NodeAddress nodeAddr;
            BOOL isNodeId;

            numValues = sscanf(appInput.inputStrings[i],
                               "%*s %s %s",
                               nodeString,
                               dataString);

            if (numValues != 2)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(errorString,
                        "Wrong INTERFACETUTORIAL configuration format!\n"
                        "INTERFACETUTORIAL <node> <data>");
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    nodeString,
                    &nodeId,
                    &nodeAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, nodeId);
            if (node != NULL)
            {
                AppInterfaceTutorialInit(node, dataString);
            }
#else /* TUTORIAL_INTERFACE */
            ERROR_ReportMissingInterface(appStr, "Interface Tutorial\n");
#endif /* TUTORIAL_INTERFACE */
        }

        else if (strcmp(appStr, "ALE") == 0)
        {
#ifdef ALE_ASAPS_LIB
            BOOL    errorOccured = FALSE;

            int lineLen = NI_GetMaxLen(&appInput);
            char *inputString;
            char* next;
            char *token;
            char iotoken[MAX_STRING_LENGTH];
            const char *delims = " \t\n";

            char    sourceString[MAX_STRING_LENGTH];
            char    destNumString[MAX_STRING_LENGTH];
            char    *destString[MAX_STRING_LENGTH];

            char    intervalStr[MAX_STRING_LENGTH];
            char    startTimeStr[MAX_STRING_LENGTH];
            char    endTimeStr[MAX_STRING_LENGTH];
            char    errorString[MAX_STRING_LENGTH];
            char    stdErrorString[MAX_STRING_LENGTH];

            int destNum = 1;    // Group calls are treated as Net calls
            int count;
            int sourceInterfaceIndex;
            int interfaceIndex;
            int requestedCallChannel;

            Node        *sourceNode;
            NodeAddress sourceNodeId;
            NodeAddress *destNodeId = NULL;

            clocktype   startTime;
            clocktype   endTime;

            sprintf(stdErrorString,
                "Incorrect ALE configuration format\n'%s'\n"
               "ALE <SourceNodeName> <DestNodeName(s)> "
               "<StartTime> <EndTime> [CH<OptionalChannelNum>]\n"
               "'ALE ET HOME 10S 50S CH3' or 'ALE 1 3 10S 50S'\n",
               appInput.inputStrings[i]);
            /*
               Group calls will have multiple destinations, and
               slots will be assigned from left-to-right,
               starting with slot 0 for source/calling node.
               A NET address can only appear as a destination and
               the slots are pre-assigned, based on the node's
               appearance order in ALE-CONFIG-FILE.
            */
            inputString = (char*) MEM_malloc(lineLen);
            memset(inputString,0,lineLen);
            strcpy(inputString, appInput.inputStrings[i]);
            // ALE
            token = IO_GetDelimitedToken(
                        iotoken, inputString, delims, &next);
            // Retrieve next token.
            token = IO_GetDelimitedToken(iotoken, next, delims, &next);

            if (!token)
            {
                ERROR_ReportWarning(stdErrorString);
                MEM_free(inputString);
                continue;
            }

            strcpy(sourceString, token);
            if (MacAleGetNodeNameMapping(
                    token,
                    &sourceNodeId,
                    &sourceInterfaceIndex) == FALSE)
            {
                sprintf(errorString,
                        "%s\n Non-existent ALE nodeId/name: %s",
                        appInput.inputStrings[i], token);
                MEM_free(inputString);
                ERROR_ReportError(errorString);
            }

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL) {

                // Group calls are modeled as Net calls, so only 1 dest, always.
                /*
                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token) {
                    ERROR_ReportError(stdErrorString);
                }

                destNum = strtoul(token, NULL, 10);
                */

                destString[0] = (char *) MEM_malloc(
                        sizeof(char) * MAX_STRING_LENGTH * destNum);

                destNodeId = (NodeAddress *) MEM_malloc(
                        sizeof(NodeAddress) * destNum);

                if (!token) {
                    ERROR_ReportError(stdErrorString);
                }

                for (count = 0; count < destNum; count++)
                {
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);
                    if (MacAleGetNodeNameMapping(
                            token,
                            &(destNodeId[count]),
                            &interfaceIndex) == FALSE)
                    {
                        sprintf(errorString,
                                "'%s'\nNon-existent ALE nodeId/name: %s",
                                appInput.inputStrings[i], token);
                        MEM_free(inputString);
                        ERROR_ReportError(errorString);
                    }

                    strcpy(destString[count], token);
                }

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token) {
                    ERROR_ReportError(stdErrorString);
                }

                strcpy(startTimeStr, token);
                startTime = TIME_ConvertToClock(startTimeStr);

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token) {
                    ERROR_ReportError(stdErrorString);
                }

                strcpy(endTimeStr, token);
                endTime = TIME_ConvertToClock(endTimeStr);

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (token != NULL &&
                    IO_IsStringNonNegativeInteger(token))
                {
                    requestedCallChannel = strtoul(token, NULL, 10);
                }
                else
                {
                    requestedCallChannel = -1;
                }
                /*
                // Optional keyword CH is used to get optional channel info.
                if (token != NULL && (token[0] == 'c' || token[0] == 'C')
                    && (token[1] == 'h' || token[1] == 'H') &&
                    strlen(token) >= 3 &&
                    IO_IsStringNonNegativeInteger(&(token[2])))
                {
                    requestedCallChannel = strtoul(&(token[2]), NULL, 10);
                } else {
                    requestedCallChannel = -1;
                }
                */

                MacAleSetupCall(
                    node,
                    sourceInterfaceIndex,
                    startTime,
                    endTime,
                    sourceString,
                    destNum,
                    destString,             // destination(s)
                    requestedCallChannel,   // -1 if not specified
                    NULL);                  // initial message to send
#ifdef DEBUG
                {
                    printf("--------\n");
                    if (destNum > 1)
                    {
                        printf("ALE Group Call:\n");
                    }
                    else {
                        printf("ALE Call:\n");
                    }

                    printf("start time: %s\n", startTimeStr);
                    printf("end time:   %s\n", endTimeStr);
                    printf("src[%ld]: %s\n", sourceNodeId, sourceString);
                    printf("dst[%ld]: %s\n", destNodeId[0], destString[0]);
                    printf("--------\n");
                }
#endif // DEBUG
            MEM_free(inputString);
            }
#else // ALE_ASAPS_LIB
            ERROR_ReportMissingLibrary(appStr, "ALE/ASAPS");
#endif // ALE_ASAPS_LIB
        }
        else
        if (strcmp(appStr, "HTTP") == 0)
        {

            int lineLen = NI_GetMaxLen(&appInput);
            char *inputString = (char*) MEM_malloc(lineLen);
            // For IO_GetDelimitedToken
            char* next;
            char *token;
            const char *delims = " \t\n";
            char iotoken[MAX_STRING_LENGTH];
            NodeAddress clientNodeId;
            Address clientAddr;

            memset(inputString,0,lineLen);
            strcpy(inputString, appInput.inputStrings[i]);
            // HTTP
            token = IO_GetDelimitedToken(
                        iotoken, inputString, delims, &next);
            // Retrieve next token.
            token = IO_GetDelimitedToken(iotoken, next, delims, &next);

            if (!token)
            {
                AppInputHttpError();
            }

            IO_AppParseSourceString(
                firstNode,
                appInput.inputStrings[i],
                token,
                &clientNodeId,
                &clientAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, clientNodeId);
            if (node != NULL)
            {
                char threshTimeStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                int numServerAddrs;
                NodeAddress *serverNodeIds;
                Address *serverAddrs;
                int count;
                clocktype threshTime;
                clocktype startTime;

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token)
                {
                    AppInputHttpError();
                }

                numServerAddrs = atoi(token);

                if (!token)
                {
                    AppInputHttpError();
                }

                serverNodeIds =
                    (NodeAddress *)
                    MEM_malloc(sizeof(NodeAddress) * numServerAddrs);
                serverAddrs =
                    (Address *)
                    MEM_malloc(sizeof(Address) * numServerAddrs);

                for (count = 0; count < numServerAddrs; count++)
                {
                    token = IO_GetDelimitedToken(
                                iotoken, next, delims, &next);

                    if (!token)
                    {
                        AppInputHttpError();
                    }

                    IO_AppParseDestString(
                        firstNode,
                        appInput.inputStrings[i],
                        token,
                        &serverNodeIds[count],
                        &serverAddrs[count]);
                }

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token)
                {
                    AppInputHttpError();
                }
                strcpy(startTimeStr, token);
                startTime = TIME_ConvertToClock(startTimeStr);

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token)
                {
                    AppInputHttpError();
                }
                strcpy(threshTimeStr, token);
                threshTime = TIME_ConvertToClock(threshTimeStr);

#ifdef DEBUG
                {
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    char addrStr[MAX_STRING_LENGTH];
                    printf("Starting HTTP client with:\n");
                    printf("  client nodeId:  %u\n", clientNodeId);
                    for (count = 0; count < numServerAddrs; count++)
                    {
                        printf("  server nodeId:  %u\n",
                            serverNodeIds[count]);
                        IO_ConvertIpAddressToString(&serverAddrs[count],
                            addrStr);
                        printf("  server address: %s\n", addrStr);
                    }
                    printf("  num servers:    %d\n", numServerAddrs);
                    ctoa(startTime, clockStr);
                    printf("  start time:     %s\n", clockStr);
                    ctoa(threshTime, clockStr);
                    printf("  threshold time: %s\n", clockStr);
                }
#endif /* DEBUG */

                MEM_free(serverNodeIds);

                AppHttpClientInit(
                    node,
                    clientAddr,
                    serverAddrs,
                    numServerAddrs,
                    startTime,
                    threshTime);
            MEM_free(inputString);
            }
        }
        else
        if (strcmp(appStr, "HTTPD") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s",
                            sourceString);
            assert(numValues == 1);

            IO_AppParseSourceString(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                AppHttpServerInit(node, sourceAddr);
            }
        }
        else
        if (strcmp(appStr, "TRAFFIC-GEN") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;
            DestinationType destType = DEST_TYPE_UNICAST;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s",
                            sourceString,
                            destString);

            if (numValues != 2)
            {
                char errorString[BIG_STRING_LENGTH];
                sprintf(errorString,
                        "Wrong TRAFFIC-GEN configuration format!\n"
                        TRAFFIC_GEN_USAGE
                        "\n");
                ERROR_ReportError(errorString);
            }

            // Parse and check given destination address.
            if (strcmp(destString, "255.255.255.255") == 0)
            {
                IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    NETWORK_IPV4);

                SetIPv4AddressInfo(&destAddr, ANY_DEST);
                destType = DEST_TYPE_BROADCAST;
                destNodeId = ANY_NODEID;
            }
            else
            {
                  //Get source and destination nodeId and address
                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                if (destAddr.networkType == NETWORK_IPV6)
                {
                    if (IS_MULTIADDR6(destAddr.interfaceAddr.ipv6) ||
                    Ipv6IsReservedMulticastAddress(
                            node, destAddr.interfaceAddr.ipv6) == TRUE)
                    {
                        destType= DEST_TYPE_MULTICAST;
                    }
                }
                else if (destAddr.networkType == NETWORK_IPV4)
                {
                    if (NetworkIpIsMulticastAddress(
                        firstNode, destAddr.interfaceAddr.ipv4))
                    {
                        destType= DEST_TYPE_MULTICAST;
                    }
                }
            }

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                //Call Traffic-Gen client initialization function
                TrafficGenClientInit(node,
                                     appInput.inputStrings[i],
                                     sourceAddr,
                                     destAddr,
                                     destType);
            }
            if (destType == DEST_TYPE_UNICAST)
            {
                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                         node,
                                         appInput.inputStrings[i],
                                         destAddr, destNodeId,
                                         sourceAddr, sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    TrafficGenServerInit(node);
                }
            }
        }
        else
        if (strcmp(appStr, "TRAFFIC-TRACE") == 0)
        {
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            NodeAddress sourceNodeId;
            Address sourceAddr;
            NodeAddress destNodeId;
            Address destAddr;
            memset(&destAddr, 0, sizeof(Address));
            memset(&sourceAddr, 0, sizeof(Address));
            BOOL isDestMulticast;

            numValues = sscanf(appInput.inputStrings[i],
                            "%*s %s %s",
                            sourceString,
                            destString);

            if (numValues != 2)
            {
                char errorString[BIG_STRING_LENGTH];
                sprintf(errorString,
                        "Wrong TRAFFIC-TRACE configuration format!\n"
                        TRAFFIC_TRACE_USAGE
                        "\n");
                ERROR_ReportError(errorString);
            }

            APP_CheckMulticastByParsingSourceAndDestString(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr.interfaceAddr.ipv4,
                destString,
                &destNodeId,
                &destAddr.interfaceAddr.ipv4,
                &isDestMulticast);

            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL)
            {
                TrafficTraceClientInit(node,
                                       appInput.inputStrings[i],
                                       GetIPv4Address(sourceAddr),
                                       GetIPv4Address(destAddr),
                                       isDestMulticast);
            }

            if (!isDestMulticast)
            {
                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                         node,
                                         appInput.inputStrings[i],
                                         destAddr, destNodeId,
                                         sourceAddr, sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    TrafficTraceServerInit(node);
                }
            }
        }
        else if (!strcmp(appStr, "VOIP"))
        {
#ifdef ENTERPRISE_LIB
            char srcStr[MAX_STRING_LENGTH];
            char destStr[MAX_STRING_LENGTH];
            char intervalStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            char option1Str[MAX_STRING_LENGTH];
            char option2Str[MAX_STRING_LENGTH];
            char option3Str[MAX_STRING_LENGTH];

            // For IO_GetDelimitedToken
            char iotoken[MAX_STRING_LENGTH];
            char viopAppString[MAX_STRING_LENGTH];
            char* next;
            char* token;
            char* p;
            const char* delims = " ";

            NodeAddress srcNodeId;
            NodeAddress srcAddr;
            NodeAddress destNodeId;
            NodeAddress destAddr;

            BOOL callAccept = TRUE;
            clocktype packetizationInterval = 0;
            unsigned int bitRatePerSecond = 0;
            clocktype startTime;
            clocktype endTime;
            clocktype interval;
            unsigned tos = APP_DEFAULT_TOS;
            BOOL newSyntax = FALSE;

            unsigned short srcPort;
            unsigned short callSignalPort;
            // Each line for VOIP describes a call, so we can
            // use it to assign ports
            srcPort = (unsigned short) (VOIP_PORT_NUMBER_BASE - (i * 2));
            callSignalPort = srcPort - 1;

            char sourceAliasAddr[MAX_ALIAS_ADDR_SIZE];
            char destAliasAddr[MAX_ALIAS_ADDR_SIZE];
            Node* destNode = NULL;

            char codecScheme[MAX_STRING_LENGTH] = {"G.711"};

            retVal = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %s %s %s %s %s %s",
                            srcStr, destStr, intervalStr,
                            startTimeStr, endTimeStr,
                            option1Str, option2Str, option3Str);

            strcpy(viopAppString, appInput.inputStrings[i]);
            p = viopAppString;

            token = IO_GetDelimitedToken(iotoken, p, delims, &next);

            while (token)
            {
                if (!strcmp(token, "CALL-STATUS") ||
                    !strcmp(token, "ENCODING") ||
                    !strcmp(token, "PACKETIZATION-INTERVAL") ||
                    !strcmp(token, "TOS") ||
                    !strcmp(token, "DSCP") ||
                    !strcmp(token, "PRECEDENCE"))
                {
                    newSyntax = TRUE;
                    break;
                }

                token = IO_GetDelimitedToken(
                            iotoken, next, delims, &next);
            }

            if (!newSyntax)
            {
                if (retVal < 5 || retVal > 7)
                {
                    char errStr[MAX_STRING_LENGTH];
                    sprintf(errStr, "Wrong VOIP configuration format!\n");
                    ERROR_ReportError(errStr);
                }
                else if ((retVal == 6) || (retVal == 7))
                {
                    // Process optional argument
                    if (!isdigit(*option1Str))
                    {
                        callAccept = VOIPsetCallStatus(option1Str);
                        if (retVal == 7)
                        {
                            packetizationInterval =
                                TIME_ConvertToClock(option2Str);
                        }
                        else
                        {
                            packetizationInterval = 0;
                        }
                    }
                    else
                    {
                        packetizationInterval =
                            TIME_ConvertToClock(option1Str);
                        if (retVal == 7)
                        {
                            callAccept = VOIPsetCallStatus(option2Str);
                        }
                    }
                }
            }
            else // New Syntax
            {
                int index;
                packetizationInterval = 0;
                bitRatePerSecond = 0;
                char tosType[MAX_STRING_LENGTH] = {0};
                token = IO_GetDelimitedToken(iotoken, p, delims, &next);

                for (index = 0; index < 6; index++)
                {
                    token = IO_GetDelimitedToken(
                                iotoken, next, delims, &next);
                }

                while (token)
                {
                    int optionValue = VOIPgetOption(token);
                    token = IO_GetDelimitedToken(
                                    iotoken, next, delims, &next);
                    switch (optionValue)
                    {
                        case VOIP_CALL_STATUS:
                        {
                            callAccept = VOIPsetCallStatus(token);
                            break;
                        }
                        case VOIP_ENCODING_SCHEME:
                        {
                            strcpy(codecScheme, token);
                            break;
                        }
                        case VOIP_PACKETIZATION_INTERVAL:
                        {
                            if (!strlen(codecScheme))
                            {
                                packetizationInterval = 0;
                            }
                            else
                            {
                                packetizationInterval =
                                        TIME_ConvertToClock(token);
                            }
                            break;
                        }
                        case VOIP_TOS:
                        {
                            strcpy(tosType, "TOS");
                            break;
                        }
                        case VOIP_DSCP:
                        {
                            strcpy(tosType, "DSCP");
                            break;
                        }
                        case VOIP_PRECEDENCE:
                        {
                            strcpy(tosType, "PRECEDENCE");
                            break;
                        }
                        default:
                        {
                            char errStr[MAX_STRING_LENGTH];
                            sprintf(errStr,"Invalid option: %s\n", token);
                            ERROR_ReportError(errStr);
                        }
                    }
                    if ((optionValue == VOIP_TOS) ||
                                    (optionValue == VOIP_DSCP)
                        || (optionValue == VOIP_PRECEDENCE))
                    {
                        if (!(APP_AssignTos(tosType, token, &tos)))
                        {
                            char errStr[MAX_STRING_LENGTH];
                            sprintf(errStr, "Invalid tos option: %s\n",
                                    token);
                            ERROR_ReportError(errStr);
                        }
                    }
                    if (token)
                    {
                        token = IO_GetDelimitedToken(
                            iotoken, next, delims, &next);
                    }
                }
            }

            IO_AppParseSourceAndDestStrings(
                                    firstNode,
                                    appInput.inputStrings[i],
                                    srcStr,
                                    &srcNodeId,
                                    &srcAddr,
                                    destStr,
                                    &destNodeId,
                                    &destAddr);

            if (APP_ForbidSameSourceAndDest(
                    appInput.inputStrings[i],
                    "VOIP",
                    srcNodeId,
                    destNodeId))
            {
                return;
            }

            // Handle Loopback Address
            if (NetworkIpIsLoopbackInterfaceAddress(destAddr))
            {
                char errorStr[MAX_STRING_LENGTH] = {0};
                sprintf(errorStr, "VOIP: Application doesn't support "
                    "Loopback Address!\n  %s\n", appInput.inputStrings[i]);

                ERROR_ReportWarning(errorStr);

                return;
            }

            node = MAPPING_GetNodePtrFromHash(nodeHash, srcNodeId);
            destNode = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);

            if (node != NULL || destNode != NULL) {
                VoipReadConfiguration(node, destNode, nodeInput, codecScheme,
                                    packetizationInterval, bitRatePerSecond);
            }

            startTime = TIME_ConvertToClock(startTimeStr);
            endTime = TIME_ConvertToClock(endTimeStr);
            interval = TIME_ConvertToClock(intervalStr);

            if (endTime == 0)
            {
                BOOL retVal;

                IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput,
                                 "SIMULATION-TIME", &retVal, endTimeStr);

                if (retVal == TRUE)
                {
                    endTime = TIME_ConvertToClock(endTimeStr);
                }
                else
                {
                    ERROR_Assert(FALSE,
                        "SIMULATION-TIME not found in config file.\n");
                }
            }

            if (interval < MINIMUM_TALKING_TIME)
            {
                interval = MINIMUM_TALKING_TIME;

                ERROR_ReportWarning("Minimum value of average talking time "
                    "is one second. default value used here.\n");
            }
            if (destNode != NULL)
            {

                AppMultimedia* multimedia = destNode->appData.multimedia;

                if (multimedia)
                {
                    if (multimedia->sigType == SIG_TYPE_H323)
                    {
                        H323Data* h323 = (H323Data*) multimedia->sigPtr;

                        if (h323 && h323->endpointType == Gatekeeper)
                        {
                            char err[MAX_STRING_LENGTH];
                            sprintf(err,"\nVOIP receiving Node Id %d"
                                " should not be Gatekeeper. "
                                "Connection canceled.\n", destNode->nodeId);
                            ERROR_ReportWarning(err);
                            continue;
                        }

                        //Find gatekeeper present or not
                        for (Int32 index = 0; index < nodeInput->numLines; index++)
                        {
                            if (!strcmp(nodeInput->variableNames[index],
                                "H323-GATEKEEPER") &&
                                !strcmp(nodeInput->values[index],
                                "YES"))
                            {
                                gatekeeperFound = TRUE;
                                break;
                            }
                        }

                        if (!h323)
                        {
                            H323Init(destNode,
                                nodeInput,
                                gatekeeperFound,
                                callModel,
                                callSignalPort);

                            h323 = (H323Data*) multimedia->sigPtr;
                        }
                        strcpy(
                            destAliasAddr,
                            h323->h225Terminal->terminalAliasAddr);
                    }
                    else if (multimedia->sigType == SIG_TYPE_SIP)
                    {
                        SipData* sip;
                        if (!multimedia->sigPtr)
                        {
                            SipInit(destNode, nodeInput, SIP_UA);
                        }

                        sip = (SipData*) multimedia->sigPtr;
                        if (!sip->SipGetOwnAliasAddr())
                        {
                            char err[MAX_STRING_LENGTH];
                            sprintf(err, "No data available in default.sip"
                                " for node: %d\n", destNode->nodeId);
                            ERROR_Assert(false, err);
                        }
                        strcpy(destAliasAddr, sip->SipGetOwnAliasAddr());
                    }
                }
                else
                {
                    char errorBuf[MAX_STRING_LENGTH] = {0};
                    sprintf(errorBuf,"No Multimedia Signalling Protocol Set At"
                            " Node %d\n Presently"
                            " H323 and SIP are Supported", destNode->nodeId);
                    ERROR_Assert(destNode->appData.multimedia, errorBuf);
                }
            }

            if (node != NULL)
            {
                // Initialize the source node's SIP application.
                // (the source node is on this partition)
                AppMultimedia* multimedia = node->appData.multimedia;

                if (multimedia)
                {
                    if (multimedia->sigType == SIG_TYPE_H323)
                    {
                        H323Data* h323 = (H323Data*) multimedia->sigPtr;
                        gatekeeperFound = FALSE;

                        if (h323 && h323->endpointType == Gatekeeper)
                        {
                            char err[MAX_STRING_LENGTH];
                            sprintf(err,"\nVOIP sending Node Id %d"
                                " should not be Gatekeeper. "
                                "Connection canceled.\n", node->nodeId);
                            ERROR_ReportWarning(err);
                            continue;
                        }
                        //Find gatekeeper present or not
                        for (Int32 index = 0; index < nodeInput->numLines; index++)
                        {
                            if (!strcmp(nodeInput->variableNames[index],
                                "H323-GATEKEEPER") &&
                                !strcmp(nodeInput->values[index],
                                "YES"))
                            {
                                gatekeeperFound = TRUE;
                                break;
                            }
                        }
                        if (!h323)
                        {
                            H323Init(node,
                                nodeInput,
                                gatekeeperFound,
                                callModel, callSignalPort);

                            h323 = (H323Data*) multimedia->sigPtr;
                        }
                        else
                        {
                            h323->h225Terminal->terminalCallSignalTransAddr.port =
                                callSignalPort;
                        }
                        strcpy(sourceAliasAddr,
                            h323->h225Terminal->terminalAliasAddr);
                    }
                    else if (multimedia->sigType == SIG_TYPE_SIP)
                    {
                        SipData* sip;

                        if (!multimedia->sigPtr)
                        {
                            SipInit(node, nodeInput, SIP_UA);
                        }
                        //callSignalPort = node->appData.nextPortNum++;
                        sip = (SipData*) multimedia->sigPtr;
                        strcpy(sourceAliasAddr, sip->SipGetOwnAliasAddr());
                    }
                }
                else
                {
                    char errorBuf[MAX_STRING_LENGTH] = {0};
                    sprintf(errorBuf,"No Multimedia Signalling Protocol Set At"
                            " Node %d\n Presently"
                            " H323 and SIP are Supported", node->nodeId);
                    ERROR_Assert(node->appData.multimedia, errorBuf);
                }

                if (multimedia)
                {
                    // src and dest node's signaling protocols have to match
                    // Determine the alias for the destination (because
                    // destNode may be NULL -- on a different partition)
                    if (multimedia->sigType == SIG_TYPE_SIP)
                    {
                        SIPDATA_fillAliasAddrForNodeId (destNodeId, nodeInput,
                            destAliasAddr);
                    }
                    else if (multimedia->sigType == SIG_TYPE_H323)
                    {
                        // Determine the alias for our destination.
                        H323ReadEndpointList(node->partitionData,
                            destNodeId, nodeInput, destAliasAddr);
                    }
                }

                VoipCallInitiatorInit(
                    node,
                    srcAddr,
                    destAddr,
                    callSignalPort,
                    srcPort,
                    sourceAliasAddr,
                    destAliasAddr,
                    interval,
                    startTime,
                    endTime,
                    packetizationInterval,
                    bitRatePerSecond,
                    tos);
            }

            if (callAccept)
            {
                if (destNode)
                {
                    VoipAddNewCallToList(
                            destNode,
                            srcAddr,
                            srcPort,
                            interval,
                            startTime,
                            endTime,
                            packetizationInterval);

                    VoipCallReceiverInit(destNode,
                                         destAddr,
                                         srcAddr,
                                         srcPort,
                                         destAliasAddr,
                                         callSignalPort,
                                         INVALID_ID,
                                         packetizationInterval,
                                         bitRatePerSecond,
                                         tos);
                }
            }
#else // ENTERPRISE_LIB
            ERROR_ReportMissingLibrary(appStr, "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
        }
        else if (strcmp(appStr, "GSM") == 0)
        {
#ifdef CELLULAR_LIB
            char startTimeStr[MAX_STRING_LENGTH];
            char callDurationTimeStr[MAX_STRING_LENGTH];
            char simEndTimeStr[MAX_STRING_LENGTH];
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char errorString[MAX_STRING_LENGTH];

            unsigned int *info;

            clocktype callStartTime;
            clocktype callEndTime;
            clocktype simEndTime;
            clocktype *endTime;

            NodeAddress sourceNodeId;
            NodeAddress sourceAddr;
            NodeAddress destNodeId;
            NodeAddress destAddr;

            Message *callStartTimerMsg;
            Message *callEndTimerMsg;

            // For Mobile Station nodes only

            retVal = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %s %s",
                            sourceString,
                            destString,
                            startTimeStr,
                            callDurationTimeStr);

            if (retVal != 4)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(errorString,
                        "Incorrect GSM call setup: %s\n"
                        "'GSM <src> <dest> <call start time> <call duration>'",
                        appInput.inputStrings[i]);
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            // TBD : for loopBack

            IO_AppForbidSameSourceAndDest(
                    appInput.inputStrings[i],
                    sourceNodeId,
                    destNodeId);

            // Should check if the source & dest nodes are
            // configured for GSM (check interfaces) ???

            // Call Terminating(MT) node
            if (PARTITION_NodeExists(firstNode->partitionData, destNodeId)
                == FALSE)
            {
               sprintf(errorString,
                    "%s: Node %d does not exist",
                    appInput.inputStrings[i],
                    destNodeId);
                ERROR_ReportError(errorString);
            }

            // Call Originating(MO) node
            if (PARTITION_NodeExists(firstNode->partitionData, sourceNodeId)
                == FALSE)
            {
               sprintf(errorString,
                    "%s: Node %d does not exist",
                    appInput.inputStrings[i],
                    sourceNodeId);
                ERROR_ReportError(errorString);
            }
            node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node == NULL)
            {
                // not on this partition
                continue;
            }

            callStartTime = TIME_ConvertToClock(startTimeStr);
            callEndTime = callStartTime +
                TIME_ConvertToClock(callDurationTimeStr);
            callStartTime -= getSimStartTime(firstNode);
            callEndTime -= getSimStartTime(firstNode);

            simEndTime = node->partitionData->maxSimClock;

            // printf("GSM_MS[%ld] -> %d, Call start %s, duration %s\n",
            //     node->nodeId, destNodeId,
            //     startTimeStr, callDurationTimeStr);

            if (callStartTime <= 0 || callStartTime >= simEndTime ||
                 callEndTime <= 0 || callEndTime >= simEndTime)
            {
                sprintf(errorString,
                    "Illegal Call Time:\n'%s'"
                    "\nTotal Simulation Time = %s",
                    appInput.inputStrings[i], simEndTimeStr);
                ERROR_ReportError(errorString);
            }

            if ((callEndTime - callStartTime) < GSM_MIN_CALL_DURATION)
            {
                char clockStr2[MAX_STRING_LENGTH];
                char clockStr3[MAX_STRING_LENGTH];


                ctoa(GSM_MIN_CALL_DURATION / SECOND, clockStr2);
                ctoa(GSM_AVG_CALL_DURATION / SECOND, clockStr3);

                callEndTime = callStartTime + GSM_AVG_CALL_DURATION;

                sprintf(errorString,
                    "Mininum call duration is %s second(s).\n"
                    "Default value %sS will be used.",
                    clockStr2, clockStr3);

                ERROR_ReportWarning(errorString);
            }

            // Setup Call Start Timer with info on dest & call end time
            callStartTimerMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    0,
                                    MSG_NETWORK_GsmCallStartTimer);

            MESSAGE_InfoAlloc(node,
                callStartTimerMsg,
                sizeof(unsigned int) + sizeof(clocktype));
            info = (unsigned int*) MESSAGE_ReturnInfo(callStartTimerMsg);
            *info = (unsigned int) destNodeId;
            endTime = (clocktype *)(info + sizeof(unsigned int));
            *endTime = callEndTime;
            callStartTimerMsg->protocolType = NETWORK_PROTOCOL_GSM;

            MESSAGE_Send(node,
                    callStartTimerMsg,
                    callStartTime);

            // Set a timer to initiate call disconnect
            callEndTimerMsg = MESSAGE_Alloc(node,
                                NETWORK_LAYER,
                                0,
                                MSG_NETWORK_GsmCallEndTimer);

            callEndTimerMsg->protocolType = NETWORK_PROTOCOL_GSM;

            MESSAGE_Send(node,
                    callEndTimerMsg,
                    callEndTime);
#else // CELLULAR_LIB
            ERROR_ReportMissingLibrary(appStr, "Cellular");
#endif // CELLULAR_LIB

        } // End of GSM //

//StartVBR
        else if (strcmp(appStr, "VBR") == 0) {
            NodeAddress clientAddr;
            NodeAddress serverAddr;
            int itemSize;
            clocktype meanInterval;
            clocktype startTime;
            clocktype endTime;
            NodeAddress clientId;
            NodeAddress serverId;
            char clientString[MAX_STRING_LENGTH];
            char serverString[MAX_STRING_LENGTH];
            char meanIntervalString[MAX_STRING_LENGTH];
            char startTimeString[MAX_STRING_LENGTH];
            char endTimeString[MAX_STRING_LENGTH];
            char tosStr[MAX_STRING_LENGTH];
            char tosValueStr[MAX_STRING_LENGTH];
            unsigned tos = APP_DEFAULT_TOS;

            retVal = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %d %s %s %s %s %s",
                            clientString,
                            serverString,
                            &itemSize,
                            meanIntervalString,
                            startTimeString,
                            endTimeString,
                            tosStr,
                            tosValueStr);

            switch (retVal)
            {
                case 6:
                    break;
                case 8 :
                    if (APP_AssignTos(tosStr, tosValueStr, &tos))
                    {
                        break;
                    } // else fall through default
                default:
                {
                    fprintf(stderr,
                            "Wrong VBR configuration format!\n"
                            "VBR Client Server item Size mean Interval start Time end Time [tos]                       \n");
                    ERROR_ReportError("Illegal transition");
                }
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                clientString,
                &clientId,
                &clientAddr,
                serverString,
                &serverId,
                &serverAddr);

            node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);

            meanInterval = TIME_ConvertToClock(meanIntervalString);
            startTime = TIME_ConvertToClock(startTimeString);
            endTime = TIME_ConvertToClock(endTimeString);

            if (node != NULL) {
                VbrInit(node,
                        clientAddr,
                        serverAddr,
                        itemSize,
                        meanInterval,
                        startTime,
                        endTime,
                        tos);
            }

            node = MAPPING_GetNodePtrFromHash(nodeHash, serverId);
            if (node != NULL) {
                VbrInit(node,
                        clientAddr,
                        serverAddr,
                        itemSize,
                        meanInterval,
                        startTime,
                        endTime,
                        tos);
            }
        }
//EndVBR
        else if (strcmp(appStr, "CELLULAR-ABSTRACT-APP") == 0)
        {
#ifdef CELLULAR_LIB
            char sourceString[MAX_STRING_LENGTH];
            char destString[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char appDurationTimeStr[MAX_STRING_LENGTH];
            char simEndTimeStr[MAX_STRING_LENGTH];
            char appTypeString[MAX_STRING_LENGTH];
            char errorString[MAX_STRING_LENGTH];

            NodeAddress sourceNodeId;
            NodeAddress sourceAddr;
            NodeAddress destNodeId;
            NodeAddress destAddr;
            clocktype appStartTime;
            clocktype appEndTime;
            clocktype appDuration;
            clocktype simEndTime;
            CellularAbstractApplicationType appType;
            short numChannelReq;
            float bandwidthReq;

            CellularAbstractApplicationLayerData *appCellularVar;

            // For Mobile Station nodes only
            retVal = sscanf(appInput.inputStrings[i],
                            "%*s %s %s %s %s %s %f",
                            sourceString,
                            destString,
                            startTimeStr,
                            appDurationTimeStr,
                            appTypeString,
                            &bandwidthReq);

            //this one is used when application is specified by  channel #
            numChannelReq = 1;

            if (retVal != 6)
            {
                char errorString[MAX_STRING_LENGTH];
                sprintf(
                    errorString,
                    "Incorrect CELLULAR ABSTRACT APP setup: %s\n"
                    "'CELLULAR-ABSTRACT-APP <src> <dest> <app start time>"
                    "<appcall duration> <app type> <bandwidth required>'",
                    appInput.inputStrings[i]);
                ERROR_ReportError(errorString);
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                sourceString,
                &sourceNodeId,
                &sourceAddr,
                destString,
                &destNodeId,
                &destAddr);

            IO_AppForbidSameSourceAndDest(
                    appInput.inputStrings[i],
                    sourceNodeId,
                    destNodeId);

            // Should check if the source & dest nodes are
            // configured for CELLULAR MS or aggregated node???

            // APP Terminating(MT) node
            if (PARTITION_NodeExists(firstNode->partitionData, destNodeId)
                == FALSE)
            {
               sprintf(
                    errorString,
                    "%s: Node %d does not exist",
                    appInput.inputStrings[i],
                    destNodeId);
                ERROR_ReportError(errorString);
            }

            // APP Originating(MO) node
            if (PARTITION_NodeExists(firstNode->partitionData, sourceNodeId)
                == FALSE)
            {
               sprintf(
                   errorString,
                    "%s: Node %d does not exist",
                    appInput.inputStrings[i],
                    sourceNodeId);
                ERROR_ReportError(errorString);
            }

            appStartTime = TIME_ConvertToClock(startTimeStr);
            appEndTime = appStartTime +
                TIME_ConvertToClock(appDurationTimeStr);

            simEndTime = firstNode->partitionData->maxSimClock;
#if 0
            printf(
                "CELLULAR_MS[%ld] -> %d, %s start %s, "
                "duration %s numchannel %d bandwidth %f\n",
                node->nodeId, destNodeId, appTypeString,
                startTimeStr, appDurationTimeStr,
                numChannelReq,bandwidthReq);
#endif /* 0 */


            if (appStartTime <= 0 || appStartTime >= simEndTime ||
                 appEndTime <= 0 || appEndTime >= simEndTime)
            {
                sprintf(
                    errorString,
                    "Illegal Call Time:\n'%s'"
                    "\nTotal Simulation Time = %s",
                    appInput.inputStrings[i], simEndTimeStr);
                ERROR_ReportError(errorString);
            }

            appDuration = TIME_ConvertToClock(appDurationTimeStr);

            if (strcmp(appTypeString,"VOICE") == 0)
            {
                appType = CELLULAR_ABSTRACT_VOICE_PHONE;
            }
            else if (strcmp(appTypeString,"VIDEO-PHONE") == 0)
            {
                appType=CELLULAR_ABSTRACT_VIDEO_PHONE;
            }
            else if (strcmp(appTypeString,"TEXT-MAIL") == 0)
            {
                appType = CELLULAR_ABSTRACT_TEXT_MAIL;
            }
            else if (strcmp(appTypeString,"PICTURE-MAIL") == 0)
            {
                appType = CELLULAR_ABSTRACT_PICTURE_MAIL;
            }
            else if (strcmp(appTypeString,"ANIMATION-MAIL") == 0)
            {
                appType = CELLULAR_ABSTRACT_ANIMATION_MAIL;
            }
            else if (strcmp(appTypeString,"WEB") == 0)
            {
                appType = CELLULAR_ABSTRACT_WEB;
            }
            else
            {
                sprintf(
                    errorString,
                    "Illegal App type:\n'%s'",
                    appTypeString);
                ERROR_ReportError(errorString);
            }

            node=MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
            if (node != NULL) {
                //update the app list
                //CellularAbstractApplicationLayerData *appCellularVar;
                appCellularVar =
                    (CellularAbstractApplicationLayerData *)
                    node->appData.appCellularAbstractVar;
                numChannelReq = 1;//used when bandwidth is used

                CellularAbstractAppInsertList(
                    node,
                    &(appCellularVar->appInfoOriginList),
                    appCellularVar->numOriginApp,
                    sourceNodeId, destNodeId,
                    appStartTime, appDuration,
                    appType, numChannelReq,
                    bandwidthReq, TRUE);
            }
#else // CELLULAR_LIB
            ERROR_ReportMissingLibrary(appStr, "Cellular");
#endif // CELLULAR_LIB
        } // end of abstrct cellular app
else if (strcmp(appStr, "PHONE-CALL") == 0)
        {
#ifdef UMTS_LIB
             char sourceString[MAX_STRING_LENGTH];
             char destString[MAX_STRING_LENGTH];
             char avgTalkTimeStr[MAX_STRING_LENGTH];
             char startTimeStr[MAX_STRING_LENGTH];
             char appDurationTimeStr[MAX_STRING_LENGTH];
             char simEndTimeStr[MAX_STRING_LENGTH];
             char errorString[MAX_STRING_LENGTH];

             NodeAddress sourceNodeId;
             NodeAddress sourceAddr;
             NodeAddress destNodeId;
             NodeAddress destAddr;
             clocktype appStartTime;
             clocktype appEndTime;
             clocktype appDuration;
             clocktype simEndTime;
             clocktype avgTalkTime;

             AppPhoneCallLayerData* appPhoneCallLayerData;

             //initilize all the node's application data info
             if (phoneCallApp == FALSE)
             {
                 Node* nextNode  = NULL;
                 nextNode=firstNode->partitionData->firstNode;
                 while (nextNode != NULL)
                 {
                     //check to see if we've initialized
                     //if not, then do so
                     appPhoneCallLayerData =
                         (AppPhoneCallLayerData *)
                             nextNode->appData.phonecallVar;

                     if (appPhoneCallLayerData == NULL)
                     {
                         //need to initialize
                         AppPhoneCallInit(nextNode, nodeInput);
                     }

                     //printf("node %d:app init ing\n",nextNode->nodeId);
                     nextNode = nextNode->nextNodeData;
                 }
                 phoneCallApp = TRUE;
             }

             // For Mobile Station nodes only
             retVal = sscanf(appInput.inputStrings[i],
                             "%*s %s %s %s %s %s",
                             sourceString,
                             destString,
                             avgTalkTimeStr,
                             startTimeStr,
                             appDurationTimeStr);

             if (retVal != 5)
             {
                 char errorString[MAX_STRING_LENGTH];
                 sprintf(
                     errorString,
                     "Incorrect PHONE-CALL setup: %s\n"
                     "'PHONE-CALL <src> <dest> <avg talk time>"
                     "<app start time> <appcall duration>'",
                     appInput.inputStrings[i]);
                 ERROR_ReportError(errorString);
             }

             IO_AppParseSourceAndDestStrings(
                 firstNode,
                 appInput.inputStrings[i],
                 sourceString,
                 &sourceNodeId,
                 &sourceAddr,
                 destString,
                 &destNodeId,
                 &destAddr);

             IO_AppForbidSameSourceAndDest(
                     appInput.inputStrings[i],
                     sourceNodeId,
                     destNodeId);

             // APP Terminating(MT) node
             if (PARTITION_NodeExists(firstNode->partitionData, destNodeId)
                 == FALSE)
             {
                sprintf(
                     errorString,
                     "%s: Node %d does not exist",
                     appInput.inputStrings[i],
                     destNodeId);
                 ERROR_ReportError(errorString);
             }

             // APP Originating(MO) node
             if (PARTITION_NodeExists(firstNode->partitionData, sourceNodeId)
                 == FALSE)
             {
                sprintf(
                    errorString,
                     "%s: Node %d does not exist",
                     appInput.inputStrings[i],
                     sourceNodeId);
                 ERROR_ReportError(errorString);
             }

             appStartTime = TIME_ConvertToClock(startTimeStr);
             appEndTime = appStartTime +
                 TIME_ConvertToClock(appDurationTimeStr);
             avgTalkTime = TIME_ConvertToClock(avgTalkTimeStr);
             simEndTime = firstNode->partitionData->maxSimClock;

 #if 0
             printf(
                 "CELLULAR_MS[%d] -> %d, avg-talk-time %s start %s, "
                 "duration %s\n",
                 node->nodeId, destNodeId, avgTalkTimeStr,
                 startTimeStr, appDurationTimeStr);
 #endif /* 0 */


             if (appStartTime <= 0 || appStartTime >= simEndTime ||
                  appEndTime <= 0 || appEndTime >= simEndTime)
             {
                 sprintf(
                     errorString,
                     "Illegal Call Time:\n'%s'"
                     "\nTotal Simulation Time = %s",
                     appInput.inputStrings[i], simEndTimeStr);
                 ERROR_ReportError(errorString);
             }

             appDuration = TIME_ConvertToClock(appDurationTimeStr);

             node=MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
             if (node != NULL) {
                 //update the app list

                 appPhoneCallLayerData =
                     (AppPhoneCallLayerData *)
                     node->appData.phonecallVar;

                 AppPhoneCallInsertList(
                     node,
                     &(appPhoneCallLayerData->appInfoOriginList),
                     appPhoneCallLayerData->numOriginApp,
                     sourceNodeId, destNodeId,
                     avgTalkTime,
                     appStartTime, appDuration,
                     TRUE);
             }
 #else // UMTS_LIB
             ERROR_ReportMissingLibrary(appStr, "UMTS");
 #endif // UMTS_LIB
         } // end of Phone Call
//StartSuperApplication
        else if (strcmp(appStr, "SUPER-APPLICATION") == 0) {
            NodeAddress clientAddr;
            NodeAddress serverAddr;
            NodeAddress clientId;
            NodeAddress serverId;
            char clientString[MAX_STRING_LENGTH];
            char serverString[MAX_STRING_LENGTH];

            BOOL wasFound;
            char simulationTimeStr[MAX_STRING_LENGTH];
            char startTimeStr[MAX_STRING_LENGTH];
            char endTimeStr[MAX_STRING_LENGTH];
            int numberOfReturnValue = 0;
            clocktype simulationTime = 0;
            double simTime = 0.0;
            clocktype sTime = 0;

            // read in the simulation time from config file
            IO_ReadString( ANY_NODEID,
                           ANY_ADDRESS,
                           nodeInput,
                           "SIMULATION-TIME",
                           &wasFound,
                           simulationTimeStr);

            // read-in the simulation-time, convert into second, and store in simTime variable.
            numberOfReturnValue = sscanf(simulationTimeStr, "%s %s", startTimeStr, endTimeStr);
            if (numberOfReturnValue == 1) {
                simulationTime = (clocktype) TIME_ConvertToClock(startTimeStr);
            } else {
                sTime = (clocktype) TIME_ConvertToClock(startTimeStr);
                simulationTime = (clocktype) TIME_ConvertToClock(endTimeStr);
                simulationTime -= sTime;
            }

            TIME_PrintClockInSecond(simulationTime, simulationTimeStr);
            simTime = atof(simulationTimeStr);

            BOOL isStartArray = FALSE;
            BOOL isUNI = FALSE;
            BOOL isRepeat = FALSE;
            BOOL isSources = FALSE;
            BOOL isDestinations = FALSE;
            BOOL isConditions = FALSE;
            BOOL isSame = FALSE;
            BOOL isLoopBack   = FALSE;

            // check if the inputStrings has enable startArray, repeat, source or denstination.
            SuperApplicationCheckInputString(&appInput,
                                             i,
                                             &isStartArray,
                                             &isUNI,
                                             &isRepeat,
                                             &isSources,
                                             &isDestinations,
                                             &isConditions);
            if (isConditions){
                SuperApplicationHandleConditionsInputString(&appInput, i);
            }
            if (isStartArray){
                SuperApplicationHandleStartTimeInputString(nodeInput, &appInput, i, isUNI);
            }
            if (isSources){
                SuperApplicationHandleSourceInputString(nodeInput, &appInput, i);
            }
            if (isDestinations){
                SuperApplicationHandleDestinationInputString(nodeInput, &appInput, i);
            }
            if (isRepeat){
                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s",
                                clientString, serverString);

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    clientString,
                    &clientId,
                    &clientAddr,
                    serverString,
                    &serverId,
                    &serverAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);
                SuperApplicationHandleRepeatInputString(node, nodeInput, &appInput, i, isUNI, simTime);
            }

            retVal = sscanf(appInput.inputStrings[i],
                            "%*s %s %s",
                            clientString, serverString);

            if (retVal != 2) {
                fprintf(stderr,
                        "Wrong SUPER-APPLICATION configuration format!\n"
                        "SUPER-APPLICATION Client Server                        \n");
                ERROR_ReportError("Illegal transition");
            }

            IO_AppParseSourceAndDestStrings(
                firstNode,
                appInput.inputStrings[i],
                clientString,
                &clientId,
                &clientAddr,
                serverString,
                &serverId,
                &serverAddr);

#ifdef ADDON_NGCNMS
            if (NODE_IsDisabled(firstNode) &&
                firstNode->nodeId != clientId && firstNode->nodeId != serverId)
            {
                continue;
            }

            //node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);
            clocktype curr = getSimTime(firstNode);
            MacCesDataUsap *usap = (MacCesDataUsap*)firstNode->macData[0]->macVar;
#endif
            if (APP_ForbidSameSourceAndDest(
                    appInput.inputStrings[i],
                    "SUPER-APPLICATION",
                    clientId,
                    serverId))
            {
                    isSame = TRUE;
            }

                if (isSame == FALSE){
                    // Handle Loopback Address
                    // TBD : must be handled by designner
                    if (NetworkIpIsLoopbackInterfaceAddress(serverAddr))
                    {
                        char errorStr[5 * MAX_STRING_LENGTH] = {0};
                        sprintf(errorStr, "SUPER-APPLICATION : Application doesn't support "
                                "Loopback Address!\n  %s\n", appInput.inputStrings[i]);

                        ERROR_ReportWarning(errorStr);
                        isLoopBack = TRUE;

                    }

                    if (isLoopBack == FALSE){
                        node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);

#ifdef ADDON_NGCNMS
                    //if SuperApplicationInit() has been already initialized
                    //and we need to re-enable a node.
                        if(firstNode->initialized &&
                          (firstNode->nodeId == clientId || firstNode->nodeId == serverId))
                        {
                            if ((curr <= firstNode->lastEnabled + usap->frameTime)
                                && firstNode->initialized ==1)
                            {
                                SuperApplicationInit(firstNode,
                                                     clientAddr,
                                                     serverAddr,
                                                     appInput.inputStrings[i],
                                                     i,
                                                     nodeInput);

                                // reset server's clientAddr so that it will find the serverPtr and
                                // continue to receive packets from the client at its new address.
                                if (firstNode->nodeId == clientId)
                                {
                                    node = MAPPING_GetNodePtrFromHash(nodeHash, serverId);

                                    if (node != NULL)
                                    {
                                        if (firstNode->disabled)
                                        {
                                            SuperApplicationResetServerClientAddr(node,
                                                                                  firstNode->oldIpAddress,
                                                                                  clientAddr);

                                        }
                                        else
                                        {
                                            int i;
                                            for (i=0; i<firstNode->numberInterfaces; i++)
                                            {
                                                if (firstNode->iface[i].disabled)
                                                {
                                                    SuperApplicationResetServerClientAddr(node,
                                                                                          firstNode->iface[i].oldIpAddress,
                                                                                          clientAddr);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                    else{
#endif
                        if (node != NULL) {
                            SuperApplicationInit(node,
                                                 clientAddr,
                                                 serverAddr,
                                                 appInput.inputStrings[i],
                                                 i,
                                                 nodeInput);
                        }
                        node = MAPPING_GetNodePtrFromHash(nodeHash, serverId);
                        if (node != NULL) {
                            SuperApplicationInit(node,
                                                 clientAddr,
                                                 serverAddr,
                                                 appInput.inputStrings[i],
                                                 i,
                                                 nodeInput);

                        }
#ifdef ADDON_NGCNMS
                    }
#endif
                    }
            }
        }
//EndSuperApplication
#ifdef MILITARY_RADIOS_LIB
        else if (strcmp(appStr, "THREADED-APP") == 0) {
            NodeAddress clientAddr;
            NodeAddress serverAddr;
            NodeAddress clientId;
            NodeAddress serverId;
            char clientString[MAX_STRING_LENGTH];
            char serverString[MAX_STRING_LENGTH];
            BOOL isDestMulticast = FALSE;
            BOOL isSimulation = FALSE;
            BOOL wasFound;
            unsigned long sessionId = 1;
            char simulationTimeStr[MAX_STRING_LENGTH];
            clocktype simulationTime = 0;
            double simTime = 0.0;
            char sessionIdStr[MAX_STRING_LENGTH];

            sprintf(sessionIdStr, " %s %ld ", "SESSION-ID",sessionId);

            // read in the simulation time from config file
            IO_ReadString( ANY_NODEID,
                           ANY_ADDRESS,
                           nodeInput,
                           "SIMULATION-TIME",
                           &wasFound,
                           simulationTimeStr);

            // read-in the simulation-time, convert into second, and store in simTime variable.
            simulationTime = (clocktype) TIME_ConvertToClock(simulationTimeStr);
            TIME_PrintClockInSecond(simulationTime, simulationTimeStr);
            simTime = atof(simulationTimeStr);

            retVal = sscanf(appInput.inputStrings[i],
                            "%*s %s %s",
                            clientString, serverString);

            if (retVal != 2) {
                printf("configuration is \n%s\n", appInput.inputStrings[i]);
                fprintf(stderr,
                        "Wrong THREADED-APP configuration format!\n"
                        "THREADED-APP Client Server                        \n");
                ERROR_ReportError("Illegal transition");
            }

            APP_CheckMulticastByParsingSourceAndDestString(
                firstNode,
                appInput.inputStrings[i],
                clientString,
                &clientId,
                &clientAddr,
                serverString,
                &serverId,
                &serverAddr,
                &isDestMulticast);

            node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);
            ThreadedAppHandleRepeatInputString(node, nodeInput, &appInput, i, simTime);


            if (!isDestMulticast &&
                APP_ForbidSameSourceAndDest(
                appInput.inputStrings[i],
                "THREADED-APP",
                clientId,
                serverId))
            {
                return;
            }

              // Handle Loopback Address
              // TBD : must be handled by designner
              if (NetworkIpIsLoopbackInterfaceAddress(serverAddr))
              {
                  char errorStr[INPUT_LENGTH] = {0};
                  sprintf(errorStr, "THREADED-APP : Application doesn't support "
                      "Loopback Address!\n  %s\n", appInput.inputStrings[i]);

                  ERROR_ReportWarning(errorStr);

                  return;
              }

              ThreadedAppDynamicInit(node, appInput.inputStrings[i], i, sessionId);
              sessionId++;
        }
#endif /* MILITARY_RADIOS_LIB */
//EndSuperApplication
//InsertPatch APP_INIT_CODE
        else
        {
            char errorString[MAX_STRING_LENGTH];
            sprintf(errorString,
                    "Unknown application type %s:\n  %s\n",
                    appStr, appInput.inputStrings[i]);
            ERROR_ReportError(errorString);
        }
    }
}

/*
 * NAME:        APP_ProcessEvent.
 * PURPOSE:     call proper protocol to process messages.
 * PARAMETERS:  node - pointer to the node,
 *              msg - pointer to the received message.
 * RETURN:      none.
 */
void APP_ProcessEvent(Node *node, Message *msg)
{
    short protocolType;
    protocolType = APP_GetProtocolType(node,msg);

    switch(protocolType)
    {
        case APP_ROUTING_BELLMANFORD:
        {
            RoutingBellmanfordLayer(node, msg);
            break;
        }
#ifdef WIRELESS_LIB
        case APP_ROUTING_FISHEYE:
        {
            RoutingFisheyeLayer(node,msg);
            break;
        }
        case APP_ROUTING_OLSR_INRIA:
        {
            RoutingOlsrInriaLayer(node, msg);
            break;
        }
        case APP_ROUTING_OLSRv2_NIIGATA:
        {
            RoutingOLSRv2_Niigata_Layer(node, msg);
            break;
        }
        case APP_NEIGHBOR_PROTOCOL:
        {
            NeighborProtocolProcessMessage(node, msg);
            break;
        }
#endif // WIRELESS_LIB
        case APP_ROUTING_STATIC:
        {
            RoutingStaticLayer(node, msg);
            break;
        }
        case APP_TELNET_CLIENT:
        {
            AppLayerTelnetClient(node, msg);
            break;
        }
        case APP_TELNET_SERVER:
        {
            AppLayerTelnetServer(node, msg);
            break;
        }
        case APP_FTP_CLIENT:
        {
            AppLayerFtpClient(node, msg);
            break;
        }
        case APP_FTP_SERVER:
        {
            AppLayerFtpServer(node, msg);
            break;
        }
        case APP_CHANSWITCH_CLIENT:
        {
            AppLayerChanswitchClient(node, msg);
            break;
        }
        case APP_CHANSWITCH_SERVER:
        {
            AppLayerChanswitchServer(node, msg);
            break;
        }
        case APP_CHANSWITCH_SINR_CLIENT:
        {
            AppLayerChanswitchSinrClient(node, msg);
            break;
        }
        case APP_CHANSWITCH_SINR_SERVER:
        {
            AppLayerChanswitchSinrServer(node, msg);
            break;
        }
        case APP_GEN_FTP_CLIENT:
        {
            AppLayerGenFtpClient(node, msg);
            break;
        }
        case APP_GEN_FTP_SERVER:
        {
            AppLayerGenFtpServer(node, msg);
            break;
        }
        case APP_CBR_CLIENT:
        {
            AppLayerCbrClient(node, msg);
            break;
        }
        case APP_CBR_SERVER:
        {
            AppLayerCbrServer(node, msg);
            break;
        }
#ifdef ADDON_BOEINGFCS
        case APP_ROUTING_HSLS:
        {
            RoutingCesHslsLayer(node,msg);
            break;
        }
#endif /* ADDON_BOEINGFCS */
        case APP_FORWARD:
        {
            AppLayerForward(node, msg);
            break;
        }
        case APP_MCBR_CLIENT:
        {
            AppLayerMCbrClient(node, msg);
            break;
        }
        case APP_MCBR_SERVER:
        {
            AppLayerMCbrServer(node, msg);
            break;
        }
        case APP_HTTP_CLIENT:
        {
            AppLayerHttpClient(node, msg);
            break;
        }
        case APP_HTTP_SERVER:
        {
            AppLayerHttpServer(node, msg);
            break;
        }
        case APP_TRAFFIC_GEN_CLIENT:
        {
            TrafficGenClientLayerRoutine(node, msg);
            break;
        }
        case APP_TRAFFIC_GEN_SERVER:
        {
            TrafficGenServerLayerRoutine(node, msg);
            break;
        }
        case APP_TRAFFIC_TRACE_CLIENT:
        {
            TrafficTraceClientLayerRoutine(node, msg);
            break;
        }
        case APP_TRAFFIC_TRACE_SERVER:
        {
            TrafficTraceServerLayerRoutine(node, msg);
            break;
        }
        case APP_MESSENGER:
        {
            MessengerLayer(node, msg);
            break;
        }
        case APP_LOOKUP_CLIENT:
        {
            AppLayerLookupClient(node, msg);
            break;
        }
        case APP_LOOKUP_SERVER:
        {
            AppLayerLookupServer(node, msg);
            break;
        }
        		case APP_VIDEO_CLIENT:
			{
				AppLayerVideoClient(node, msg);
				break;
			}
		case APP_VIDEO_SERVER:
			{
				AppLayerVideoServer(node, msg);
				break;
			}
		case APP_WBEST_CLIENT_TCP:
		case APP_WBEST_CLIENT_UDP:
			{
				AppLayerWbestClient(node,msg);
				break;
			}
		case APP_WBEST_SERVER_TCP:
		case APP_WBEST_SERVER_UDP:
			{
				AppLayerWbestServer(node,msg);
				break;
			}

        
        case APP_VBR_CLIENT:
        case APP_VBR_SERVER:
        {
            VbrProcessEvent(node, msg);
            break;
        }
        case APP_SUPERAPPLICATION_CLIENT:
        case APP_SUPERAPPLICATION_SERVER:
        {
            SuperApplicationProcessEvent(node, msg);
            break;
        }
        case APP_ROUTING_RIP:
        {
            RipProcessEvent(node,msg);
            break;
        }
        case APP_ROUTING_RIPNG:
        {
            RIPngProcessEvent(node,msg);
            break;
        }
#ifdef MILITARY_RADIOS_LIB
        case APP_THREADEDAPP_CLIENT:
        case APP_THREADEDAPP_SERVER:
        {
            ThreadedAppProcessEvent(node, msg);
            break;
        }
        case APP_DYNAMIC_THREADEDAPP:
        {
            ThreadedAppDynamicInitProcessEvent(node, msg);
            break;
        }
#endif //MILITARY_RADIOS_LIB
#ifdef ENTERPRISE_LIB
        case APP_ROUTING_HSRP:
        {
            RoutingHsrpLayer(node, msg);
            break;
        }
        case APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4:
        {
            BgpLayer(node, msg);
            break;
        }
        case APP_MPLS_LDP:
        {
            MplsLdpLayer(node, msg);
            break;
        }
        case APP_H323:
        {
            H323Layer(node, msg);
            break;
        }
        case APP_SIP:
        {
            SipProcessEvent(node, msg);
            break;
        }
        case APP_VOIP_CALL_INITIATOR:
        {
            VoipCallInitiatorHandleEvent(node, msg);
            break;
        }
        case APP_VOIP_CALL_RECEIVER:
        {
            VoipCallReceiverHandleEvent(node, msg);
            break;
        }
        case APP_H225_RAS_MULTICAST:
        case APP_H225_RAS_UNICAST:
        {
            // multicast and unicast both ras packets are handle here
            H225RasLayer(node, msg);
            break;
        }
        case APP_RTP:
        {
            RtpLayer(node, msg);
            break;
        }
        case APP_RTCP:
        {
            RtcpLayer(node, msg);
            break;
        }
#endif // ENTERPRISE_LIB

#ifdef ADDON_LINK16
        case APP_LINK_16_CBR_CLIENT:
        {
            AppLayerSpawar_Link16CbrClient(node, msg);
            break;
        }
        case APP_LINK_16_CBR_SERVER:
        {
            AppLayerSpawar_Link16CbrServer(node, msg);
            break;
        }
#endif // ADDON_LINK16

#ifdef ADDON_MGEN4
        case APP_MGEN:
        {
            MgenLayer(node, msg);
            break;
        }
#endif // ADDON_MGEN4

#ifdef ADDON_MGEN3
        case APP_MGEN3_SENDER:
        {
            AppMgenLayer(node, msg);
            break;
        }
        case APP_DREC3_RECEIVER:
        {
            AppDrecLayer(node, msg);
            break;
        }
#endif // ADDON_MGEN3

#ifdef ADDON_HELLO
        case APP_HELLO:
        {
            AppHelloProcessEvent(node,msg);
            break;
        }
#endif /* ADDON_HELLO */

#ifdef TUTORIAL_INTERFACE
        case APP_INTERFACETUTORIAL:
        {
            AppLayerInterfaceTutorial(node, msg);
            break;
        }
#endif /* TUTORIAL_INTERFACE */
#ifdef CELLULAR_LIB
        case APP_CELLULAR_ABSTRACT:
        {
            CellularAbstractAppLayer(node, msg);
            break;
        }
#endif // CELLULAR_LIB
#ifdef UMTS_LIB
        case APP_PHONE_CALL:
        {
            AppPhoneCallLayer(node, msg);
            break;
        }
#endif // UMTS_LIB
#ifdef ADDON_BOEINGFCS
        case APP_CES_QOS:
        {
            if (node->appData.useNetworkCesQos) {
                NetworkCesQosProcessEvent(node, msg);
            }
            break;
        }
#endif

#ifdef ADDON_DB
        case APP_STATSDB_AGGREGATE:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL)
            {
                ERROR_ReportWarning("Unable to extract DB infotmation for Application Aggregate\n");
                break;
            }

            // APP Aggregate event handler.
            HandleStatsDBAppAggregateInsertion(node);
            MESSAGE_Send(node, msg, db->statsAggregateTable->aggregateInterval);
            break;
        }
        case APP_STATSDB_SUMMARY:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL)
            {
                ERROR_ReportWarning("Unable to extract DB infotmation for Application Summary\n");
                break;
            }

            // Need the APP Summary event handler.
            HandleStatsDBAppSummaryInsertion(node);
            MESSAGE_Send(node, msg, db->statsSummaryTable->summaryInterval);
            break;
        }
#endif
//InsertPatch LAYER_FUNCTION

        default:
            printf("Protocol = %d\n", MESSAGE_GetProtocol(msg));
            assert(FALSE); abort();
            break;
    }//switch//
}

/*
 * NAME: APP_RunTimeStat.
 * PURPOSE:      display the run-time statistics for theapplication layer
 * PARAMETERS:  node - pointer to the node
 * RETURN:       none.
 */
void
APP_RunTimeStat(Node *node)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    int i;
    AppInfo *appList = NULL;
    NetworkRoutingProtocolType routingProtocolType;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV4)
        {
            routingProtocolType = ip->interfaceInfo[i]->routingProtocolType;
        }
        else if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV6)
        {
            routingProtocolType = ip->interfaceInfo[i]->ipv6InterfaceInfo->
                                      routingProtocolType;
        }
        else
        {
            routingProtocolType = ROUTING_PROTOCOL_NONE;
        }

        // Select application-layer routing protocol model and generate
        // run-time stat
        switch (routingProtocolType)
        {
            case ROUTING_PROTOCOL_BELLMANFORD:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }
            case ROUTING_PROTOCOL_OLSR_INRIA:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }

            case ROUTING_PROTOCOL_FISHEYE:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }
            case ROUTING_PROTOCOL_STATIC:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }
//StartRIP
            case ROUTING_PROTOCOL_RIP:
            {
                RipRunTimeStat(node, (RipData *) node->appData.routingVar);
                break;
            }
//EndRIP
//StartRIPng
            case ROUTING_PROTOCOL_RIPNG:
            {
                RIPngRunTimeStat(node, (RIPngData *) node->appData.routingVar);
                break;
            }
//EndRIPng
//InsertPatch STATS_ROUTING_FUNCTION

            default:
                break;
        }//switch//
    }


    for (appList = node->appData.appPtr;
         appList != NULL;
         appList = appList->appNext)
    {
        /*
         * Get application specific data structure and call
         * the corresponding protocol to print the stats.
         */

        switch (appList->appType)
        {
            case APP_TELNET_CLIENT:
            {
                // APP_TELNET_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_TELNET_SERVER:
            {
                // APP_TELNET_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_FTP_CLIENT:
            {
                // APP_FTP_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_FTP_SERVER:
            {
                // APP_FTP_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_GEN_FTP_CLIENT:
            {
                // APP_GEN_FTP_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_GEN_FTP_SERVER:
            {
                // APP_GEN_FTP_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_CBR_CLIENT:
            {
                // APP_CBR_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_CBR_SERVER:
            {
                // APP_CBR_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_MCBR_CLIENT:
            {
                // APP_MCBR_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_MCBR_SERVER:
            {
                // APP_MCBR_SERVER STATS UNDER CONSTRUCTION
                break;
            }

#ifdef ADDON_LINK16
            case APP_LINK_16_CBR_CLIENT:
            {
                // APP_LINK_16_CBR_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_LINK_16_CBR_SERVER:
            {
                // APP_LINK_16_CBR_SERVER STATS UNDER CONSTRUCTION
                break;
            }
#endif // ADDON_LINK16

            case APP_HTTP_CLIENT:
            {
                // APP_HTTP_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_HTTP_SERVER:
            {
                // APP_HTTP_SERVER STATS UNDER CONSTRUCTION
                break;
            }

#ifdef ADDON_HELLO
            case APP_HELLO:
            {
            break;
            }
#endif /* ADDON_HELLO */

#ifdef MILITARY_RADIOS_LIB
            case APP_THREADEDAPP_CLIENT:
            case APP_THREADEDAPP_SERVER:
            {
                ThreadedAppRunTimeStat(node, (ThreadedAppData*) appList->appDetail);
                break;
            }
#endif /* MILITARY_RADIOS_LIB */
//StartVBR
            case APP_VBR_CLIENT:
            case APP_VBR_SERVER:
            {
                VbrRunTimeStat(node, (VbrData *) appList->appDetail);
            }
                break;
//EndVBR
//StartSuperApplication
            case APP_SUPERAPPLICATION_CLIENT:
            case APP_SUPERAPPLICATION_SERVER:
            {
                SuperApplicationRunTimeStat(node, (SuperApplicationData *) appList->appDetail);
            }
                break;
//EndSuperApplication
//InsertPatch STATS_FUNCTION
            case APP_FORWARD:
                ForwardRunTimeStat(node,
                                   (AppDataForward*) appList->appDetail);
                break;
#ifdef ADDON_BOEINGFCS
            case APP_CES_CLIENT:
            case APP_CES_SERVER:
                break;
#endif /* ADDON_BOEINGFCS */

            default:
                break;
        }
    }
}

/*
 * NAME:        APP_Finalize.
 * PURPOSE:     call protocols to print statistics.
 * PARAMETERS:  node - pointer to the node,
 * RETURN:      none.
 */
void
APP_Finalize(Node *node)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    AppInfo *appList = NULL;
    AppInfo *nextApp = NULL;
    int i;
    NetworkRoutingProtocolType routingProtocolType;

#ifdef MILITARY_RADIOS_LIB
    APPLink16GatewayFinalize(node);
#endif

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV4
            || ip->interfaceInfo[i]->interfaceType == NETWORK_DUAL)
        {
            routingProtocolType = ip->interfaceInfo[i]->routingProtocolType;

            // Select application-layer routing protocol model and finalize.
            switch (routingProtocolType)
            {
                case ROUTING_PROTOCOL_BELLMANFORD:
                {
                    RoutingBellmanfordFinalize(node, i);
                    break;
                }
    #ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_FISHEYE:
                {
                    RoutingFisheyeFinalize(node);
                    break;
                }
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                    RoutingOlsrInriaFinalize(node, i);
                    break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    RoutingOLSRv2_Niigata_Finalize(node, i);
                }
    #endif // WIRELESS_LIB
                case ROUTING_PROTOCOL_STATIC:
                {
                    RoutingStaticFinalize(node);
                    break;
                }
    //StartRIP
                case ROUTING_PROTOCOL_RIP:
                {
                    RipFinalize(node, i);
                    break;
                }
    //EndRIP
    //InsertPatch FINALIZE_ROUTING_FUNCTION

                default:
                    break;
            }//switch//
        }

        if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV6
            || ip->interfaceInfo[i]->interfaceType == NETWORK_DUAL)
        {
            routingProtocolType = ip->interfaceInfo[i]->ipv6InterfaceInfo->
                                      routingProtocolType;

            // Select application-layer routing protocol model and finalize.
            switch (routingProtocolType)
            {

    #ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                    RoutingOlsrInriaFinalize(node, i);
                    break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    RoutingOLSRv2_Niigata_Finalize(node, i);
                }
    #endif // WIRELESS_LIB
                case ROUTING_PROTOCOL_STATIC:
                {
                    RoutingStaticFinalize(node);
                    break;
                }
    //StartRIPng
                case ROUTING_PROTOCOL_RIPNG:
                {
                    RIPngFinalize(node, i);
                    break;
                }
    //EndRIPng
    //InsertPatch FINALIZE_ROUTING_FUNCTION
                default:
                    break;
            }//switch//
        }
    }

#ifdef ENTERPRISE_LIB
    // Finalize BGP, if appropriate.
    if (node->appData.exteriorGatewayProtocol ==
        APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4)
    {
        BgpFinalize(node);
    }

    if (node->appData.hsrp != NULL)
    {
        for (i = 0; i < node->numberInterfaces; i++)
        {
            RoutingHsrpFinalize(node, i);
        }
    }
#endif // ENTERPRISE_LIB

    /* Select application model and finalize. */
#ifdef ADDON_BOEINGFCS
          //  case APP_CES_QOS:
          //  {
                if (node->appData.useNetworkCesQos) {
                    NetworkCesQosFinalize(node);
                }
          //  }
#endif

    for (appList = node->appData.appPtr; appList != NULL; appList = nextApp)
    {
        switch (appList->appType)
        {
            case APP_TELNET_CLIENT:
            {
                AppTelnetClientFinalize(node, appList);
                break;
            }
            case APP_TELNET_SERVER:
            {
                AppTelnetServerFinalize(node, appList);
                break;
            }
            
            			case APP_VIDEO_CLIENT:
			{
				AppVideoClientFinalize(node, appList);
				break;
			}
			case APP_VIDEO_SERVER:
			{
				AppVideoServerFinalize(node, appList);
				break;
			}	
			case APP_WBEST_CLIENT_TCP:
			case APP_WBEST_CLIENT_UDP:
			{
				AppWbestClientFinalize(node,appList);
				break;
			}
			case APP_WBEST_SERVER_TCP:
			case APP_WBEST_SERVER_UDP:
			{
				AppWbestServerFinalize(node,appList);
				break;
			}

            case APP_FTP_CLIENT:
            {
                AppFtpClientFinalize(node, appList);
                break;
            }
            case APP_FTP_SERVER:
            {
                AppFtpServerFinalize(node, appList);
                break;
            }
            case APP_CHANSWITCH_CLIENT:
            {
                AppChanswitchClientFinalize(node, appList);
                break;
            }
            case APP_CHANSWITCH_SERVER:
            {
                AppChanswitchServerFinalize(node, appList);
                break;
            }
            case APP_CHANSWITCH_SINR_CLIENT:
            {
                AppChanswitchSinrClientFinalize(node, appList);
                break;
            }
            case APP_CHANSWITCH_SINR_SERVER:
            {
                AppChanswitchSinrServerFinalize(node, appList);
                break;
            }
            case APP_GEN_FTP_CLIENT:
            {
                AppGenFtpClientFinalize(node, appList);
                break;
            }
            case APP_GEN_FTP_SERVER:
            {
                AppGenFtpServerFinalize(node, appList);
                break;
            }
            case APP_CBR_CLIENT:
            {
                AppCbrClientFinalize(node, appList);
                break;
            }
            case APP_CBR_SERVER:
            {
                AppCbrServerFinalize(node, appList);
                break;
            }
            case APP_FORWARD:
            {
                AppForwardFinalize(node, appList);
                break;
            }
#ifdef ADDON_MGEN4
            case APP_MGEN:
            {
               MgenFinalize(node);
               break;
            }
#endif // ADDON_MGEN4

#ifdef ADDON_MGEN3
            case APP_MGEN3_SENDER:
            {
                AppMgenSenderFinalize(node, appList);

                break;
            }
            case APP_DREC3_RECEIVER:
            {
                AppDrecReceiverFinalize(node, appList);
                break;
            }
#endif // ADDON_MGEN3
#ifdef TUTORIAL_INTERFACE
            case APP_INTERFACETUTORIAL:
            {
                AppInterfaceTutorialFinalize(node, appList);
                break;
            }
#endif // TUTORIAL_INTERFACE
            case APP_MESSENGER:
            {
                MessengerFinalize(node, appList);
                break;
            }
            case APP_MCBR_CLIENT:
            {
                AppMCbrClientFinalize(node, appList);
                break;
            }
            case APP_MCBR_SERVER:
            {
                AppMCbrServerFinalize(node, appList);
                break;
            }

#ifdef ADDON_LINK16
            case APP_LINK_16_CBR_CLIENT:
            {
                AppSpawar_Link16CbrClientFinalize(node, appList);
                break;
            }
            case APP_LINK_16_CBR_SERVER:
            {
                AppSpawar_Link16CbrServerFinalize(node, appList);
                break;
            }
#endif // ADDON_LINK16

            case APP_HTTP_CLIENT:
            {
                AppHttpClientFinalize(node, appList);
                break;
            }
            case APP_HTTP_SERVER:
            {
                AppHttpServerFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_GEN_CLIENT:
            {
                TrafficGenClientFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_GEN_SERVER:
            {
                TrafficGenServerFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_TRACE_CLIENT:
            {
                TrafficTraceClientFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_TRACE_SERVER:
            {
                TrafficTraceServerFinalize(node, appList);
                break;
            }
            case APP_VBR_CLIENT:
            case APP_VBR_SERVER:
            {
                VbrFinalize(node, (VbrData *) appList->appDetail);
                break;
            }

            case APP_SUPERAPPLICATION_CLIENT:
            case APP_SUPERAPPLICATION_SERVER:
            {
                SuperApplicationFinalize(node, (SuperApplicationData *) appList->appDetail);
                break;
            }

#ifdef MILITARY_RADIOS_LIB
            case APP_THREADEDAPP_CLIENT:
            case APP_THREADEDAPP_SERVER:
            {
                ThreadedAppFinalize(node, (ThreadedAppData*) appList->appDetail);
                break;
            }
            case APP_DYNAMIC_THREADEDAPP:
            {
                break;
            }
#endif /* MILITARY_RADIOS_LIB */

            case APP_LOOKUP_CLIENT:
            {
                AppLookupClientFinalize(node, appList);
                break;
            }
            case APP_LOOKUP_SERVER:
            {
                AppLookupServerFinalize(node, appList);
                break;
            }

#ifdef WIRELESS_LIB
            case APP_NEIGHBOR_PROTOCOL:
            {
                NeighborProtocolFinalize(node, appList);
                break;
            }
#endif // WIRELESS_LIB

#ifdef ENTERPRISE_LIB
            case APP_MPLS_LDP:
            {
                MplsLdpFinalize(node, appList);
                break;
            }
            case APP_VOIP_CALL_INITIATOR:
            {
                AppVoipInitiatorFinalize(node, appList);
                break;
            }
            case APP_VOIP_CALL_RECEIVER:
            {
                AppVoipReceiverFinalize(node, appList);
                break;
            }
#endif // ENTERPRISE_LIB

#ifdef ADDON_HELLO
            case APP_HELLO:
            {
                AppHelloFinalize(node);
                break;
            }
#endif /* ADDON_HELLO */
#ifdef ADDON_BOEINGFCS
            case APP_ROUTING_HSLS:
            {
                RoutingCesHslsFinalize(node);
                break;
            }
#endif
//InsertPatch FINALIZE_FUNCTION
            default:
                ERROR_ReportError("Unknown or disabled application");
                break;
        }//switch//

        nextApp = appList->appNext;
    }

#ifdef ADDON_BOEINGFCS
    MopStatsFinalize(node);
#endif
#ifdef ENTERPRISE_LIB
    if (((RtpData*)(node->appData.rtpData)) &&
        ((RtpData*)(node->appData.rtpData))->rtpStats)
    {
        RtpFinalize(node);
        RtcpFinalize(node);
    }

    if (node->appData.multimedia && node->appData.multimedia->sigPtr)
    {
        if (node->appData.multimedia->sigType == SIG_TYPE_H323)
        {
            H323Finalize(node);
        }
        else if (node->appData.multimedia->sigType == SIG_TYPE_SIP)
        {
            SipFinalize(node);
        }
    }
#endif // ENTERPRISE_LIB

#ifdef MILITARY_RADIOS_LIB
    if (node->appData.triggerAppData != NULL)
    {
        ThreadedAppTriggerFinalize(node);
    }
#endif /* MILITARY_RADIOS_LIB */

#ifdef CELLULAR_LIB
    if (node->appData.appCellularAbstractVar)
    {
        CellularAbstractAppFinalize(node);
    }
#endif // CELLULAR_LIB
#ifdef UMTS_LIB
    if (node->appData.phonecallVar)
    {
        AppPhoneCallFinalize(node);
    }
#endif // UMTS_LIB
}

#ifdef ADDON_NGCNMS
//-----------------------------------------------------------------------------
// FUNCTION     APP_Destructor()
// PURPOSE      Free The Application Data structure.
//
// PARAMETERS   Node *node       : Pointer to node
//-----------------------------------------------------------------------------
static
void
APP_Destructor(Node *node)
{
    if (node->appData.multimedia != NULL)
        MEM_free(node->appData.multimedia);
    if (node->appData.portTable != NULL)
        MEM_free(node->appData.portTable);
    if (node->appData.triggerAppData != NULL)
        MEM_free(node->appData.triggerAppData);
    if (node->appData.clientPtrList != NULL)
        delete node->appData.clientPtrList;

    // only remove appData if node is getting reset.
    if (node->disabled)
    {
        if (node->appData.appPtr != NULL)
        {
            AppInfo *appList = node->appData.appPtr;
            AppInfo *appListNext = NULL;

            for (; appList != NULL; appList = appListNext)
            {
                appListNext = appList->appNext;
                MEM_free(appList);
            }

            node->appData.appPtr = NULL;

        }
    }

    // Dynamic Hierarchy
    char path[MAX_STRING_LENGTH];
    D_Hierarchy *h = &node->partitionData->dynamicHierarchy;

    if (h->IsEnabled())
    {
        sprintf(path, "node/%d/qos", node->nodeId);

        h->RemoveLevel(path);
    }

    //Free mop_stats
    MopStatsDestructor(node);

}

//-----------------------------------------------------------------------------
// FUNCTION     APP_Reset()
// PURPOSE      Reset and Re-initialize the Application protocols and/or
//              transport layer.
//
// PARAMETERS   Node *node       : Pointer to node
//              nodeInput: structure containing contents of input file
//-----------------------------------------------------------------------------
void
APP_Reset(Node *node, const NodeInput *nodeInput)
{
    struct_app_reset_function* function;
    function = node->appData.resetFunctionList->first;

    //Function Destructor Ptrs
    while(function != NULL)
    {
        (function->setFuncPtr)(node, nodeInput);
        function = function->next;
    }

    // These functions are handled differently because it
    // is unclear how to only call them once using the
    // function pointer method.

    AppCbrReset(node);
    AppMCbrReset(node);

    //destructor for layer
    APP_Destructor(node);

    //init function for layer
    APP_Initialize(node, nodeInput);

    // application doesnt get wiped out on an interface restart.
    if (node->disabled) // || node->numberInterfaces == 1)
    {
        //reason for setting initialized to 1 and then to 0
        // is so we dont re-initialize the same node multiple times.
        node->initialized = 1;
        APP_InitializeApplications(node, nodeInput);
        node->initialized = 0;
    }
    else
    {
        // reset source IP address to the new one so that
        // packets will continue to be routed.
        APP_ReInitializeApplications(node, nodeInput);
    }

}

//-----------------------------------------------------------------------------
// FUNCTION     APP_AddResetFunctionList()
// PURPOSE      Add which protocols in the application layer to be reset to a
//              fuction list pointer.
//
// PARAMETERS   Node *node       : Pointer to node
//              voind *param: pointer to the protocols reset function
//-----------------------------------------------------------------------------
void
APP_AddResetFunctionList(Node* node, void *param)
{
    struct_app_reset_function* function;

    function = new struct_app_reset_function;
    function->setFuncPtr = (AppSetFunctionPtr)param;
    function->next = NULL;

    if (node->appData.resetFunctionList->first == NULL)
    {
        node->appData.resetFunctionList->last = function;
        node->appData.resetFunctionList->first =
            node->appData.resetFunctionList->last;
    }
    else
    {
        node->appData.resetFunctionList->last->next = function;
        node->appData.resetFunctionList->last =
            node->appData.resetFunctionList->last->next;
    }
}
#endif

