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
 * This file contains data structure used by the chanswitch application and
 * prototypes of functions defined in chanswitch.c.
 */

#ifndef CHANSWITCH_APP_H
#define CHANSWITCH_APP_H

#include "types.h"

typedef
struct struct_app_chanswitch_client_str
{
    int         connectionId;
    Address     localAddr;
    Address     remoteAddr;
    clocktype   sessionStart;
    clocktype   sessionFinish;
    clocktype   lastTime;
    BOOL        sessionIsClosed;
    int         itemsToSend;
    int         itemSizeLeft;
    Int64       numBytesSent;
    Int64       numBytesRecvd;
    Int32       bytesRecvdDuringThePeriod;
    clocktype   lastItemSent;
    int         uniqueId;
    RandomSeed  seed;
//DERIUS
    char        cmd[MAX_STRING_LENGTH];

    
}
AppDataChanswitchClient;

typedef
struct struct_app_chanswitch_server_str
{
    int         connectionId;
    Address     localAddr;
    Address     remoteAddr;
    clocktype   sessionStart;
    clocktype   sessionFinish;
    clocktype   lastTime;
    BOOL        sessionIsClosed;
    Int64       numBytesSent;
    Int64       numBytesRecvd;
    Int32       bytesRecvdDuringThePeriod;
    clocktype   lastItemSent;
    RandomSeed  seed;
//
    BOOL        isLoggedIn;
    int         portForDataConn;
}
AppDataChanswitchServer;

/*
 * NAME:        AppLayerChanswitchClient.
 * PURPOSE:     Models the behaviour of Chanswitch Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchClient(Node *nodePtr, Message *msg);

/*
 * NAME:        AppChanswitchClientInit.
 * PURPOSE:     Initialize a Chanswitch session.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              serverAddr - address of the server,
 *              itemsToSend - number of items to send,
 *              waitTime - time until the session starts.
 * RETURN:      none.
 */
void
AppChanswitchClientInit(
    Node *nodePtr,
    Address clientAddr,
    Address serverAddr,
    int itemsToSend,
    clocktype waitTime);

/*
 * NAME:        AppChanswitchClientPrintStats.
 * PURPOSE:     Prints statistics of a Chanswitch session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              clientPtr - pointer to the chanswitch client data structure.
 * RETURN:      none.
 */
void
AppChanswitchClientPrintStats(Node *nodePtr, AppDataChanswitchClient *clientPtr);

/*
 * NAME:        AppChanswitchClientFinalize.
 * PURPOSE:     Collect statistics of a Chanswitch session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchClientFinalize(Node *nodePtr, AppInfo* appInfo);

/*
 * NAME:        AppChanswitchClientGetChanswitchClient.
 * PURPOSE:     search for a chanswitch client data structure.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              connId - connection ID of the chanswitch client.
 * RETURN:      the pointer to the chanswitch client data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchClient *
AppChanswitchClientGetChanswitchClient(Node *nodePtr, int connId);

/*
 * NAME:        AppChanswitchClientUpdateChanswitchClient.
 * PURPOSE:     update existing chanswitch client data structure by including
 *              connection id.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchClient *
AppChanswitchClientUpdateChanswitchClient(
    Node *nodePtr,
    TransportToAppOpenResult *openResult);

/*
 * NAME:        AppChanswitchClientNewChanswitchClient.
 * PURPOSE:     create a new chanswitch client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              clientAddr - address of this client.
 *              serverAddr - chanswitch server to this client.
 *              itemsToSend - number of chanswitch items to send in simulation.
 * RETRUN:      the pointer to the created chanswitch client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchClient *
AppChanswitchClientNewChanswitchClient(
    Node *nodePtr,
    Address clientAddr,
    Address serverAddr,
    int itemsToSend);

/*
 * NAME:        AppChanswitchClientSendNextItem.
 * PURPOSE:     Send the next item.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              clientPtr - pointer to the chanswitch client data structure.
 * RETRUN:      none.
 */
void
AppChanswitchClientSendNextItem(Node *nodePtr, AppDataChanswitchClient *clientPtr);

/*
 * NAME:        AppChanswitchClientSendNextPacket.
 * PURPOSE:     Send the remaining data.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              clientPtr - pointer to the chanswitch client data structure.
 * RETRUN:      none.
 */
void
AppChanswitchClientSendNextPacket(Node *nodePtr, AppDataChanswitchClient *clientPtr);

/*
 * NAME:        AppChanswitchClientItemsToSend.
 * PURPOSE:     call tcplib function chanswitch_nitems() to get the
 *              number of items to send in an chanswitch session.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETRUN:      amount of items to send.
 */
int
AppChanswitchClientItemsToSend(AppDataChanswitchClient *clientPtr);

/*
 * NAME:        AppChanswitchClientItemSize.
 * PURPOSE:     call tcplib function chanswitch_itemsize() to get the size
 *              of each item.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETRUN:      size of an item.
 */
int
AppChanswitchClientItemSize(AppDataChanswitchClient *clientPtr);

/*
 * NAME:        AppChanswitchServerCtrlPktSize.
 * PURPOSE:     call tcplib function chanswitch_ctlsize().
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETRUN:      chanswitch control packet size.
 */
int
AppChanswitchClientCtrlPktSize(AppDataChanswitchClient *clientPtr);

/*
 * NAME:        AppLayerChanswitchServer.
 * PURPOSE:     Models the behaviour of Chanswitch Server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchServer(Node *nodePtr, Message *msg);

/*
 * NAME:        AppChanswitchServerInit.
 * PURPOSE:     listen on Chanswitch server port.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETURN:      none.
 */
void
AppChanswitchServerInit(Node *nodePtr, Address serverAddr);

/*
 * NAME:        AppChanswitchServerPrintStats.
 * PURPOSE:     Prints statistics of a Chanswitch session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              serverPtr - pointer to the chanswitch server data structure.
 * RETURN:      none.
 */
void
AppChanswitchServerPrintStats(Node *nodePtr, AppDataChanswitchServer *serverPtr);


/*
 * NAME:        AppChanswitchServerFinalize.
 * PURPOSE:     Collect statistics of a Chanswitch session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchServerFinalize(Node *nodePtr, AppInfo* appInfo);

/*
 * NAME:        AppChanswitchServerGetChanswitchServer.
 * PURPOSE:     search for a chanswitch server data structure.
 * PARAMETERS:  appList - link list of applications,
 *              connId - connection ID of the chanswitch server.
 * RETURN:      the pointer to the chanswitch server data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchServer *
AppChanswitchServerGetChanswitchServer(Node *nodePtr, int connId);

/*
 * NAME:        AppChanswitchServerNewChanswitchServer.
 * PURPOSE:     create a new chanswitch server data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch server data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchServer *
AppChanswitchServerNewChanswitchServer(Node *nodePtr,
                         TransportToAppOpenResult *openResult);

/*
 * NAME:        AppChanswitchServerSendCtrlPkt.
 * PURPOSE:     call AppChanswitchCtrlPktSize() to get the response packet
 *              size, and send the packet.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETRUN:      none.
 */
void
AppChanswitchServerSendCtrlPkt(Node *nodePtr, AppDataChanswitchServer *serverPtr);

/*
 * NAME:        AppChanswitchServerCtrlPktSize.
 * PURPOSE:     call tcplib function chanswitch_ctlsize().
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETRUN:      chanswitch control packet size.
 */
int
AppChanswitchServerCtrlPktSize(AppDataChanswitchServer *serverPtr);

#endif /* _CHANSWITCH_APP_H_ */
