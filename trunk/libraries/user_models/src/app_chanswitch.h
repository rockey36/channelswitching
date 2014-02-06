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

#define CHANSWITCH_PROBE_PKT_SIZE           6
 //id (1) : channel mask (2) : flags (1)
#define CHANSWITCH_ACK_SIZE                 1
#define CHANSWITCH_LIST_HEADER_SIZE         9
 // id (1) : rx mac addr (6) : total station count (2) 
#define CHANSWITCH_LIST_ENTRY_SIZE          16   
//  channel (1) :  mac addr (6) : signal strength (8) : isAp (1)

#define TX_PROBE_WFACK_TIMEOUT     (50 * MILLI_SECOND)
#define TX_CHANGE_WFACK_TIMEOUT    (50 * MILLI_SECOND)
#define TX_VERIFY_WFACK_TIMEOUT    (50 * MILLI_SECOND)
#define RX_PROBE_ACK_DELAY         (5 * MILLI_SECOND)
#define RX_CHANGE_ACK_DELAY        (5 * MILLI_SECOND) 
#define SINR_MIN_DB                20.0             //SINR threshold for hidden node in dB (default)
#define CS_MIN_DBM                 -69.0            //energy threshold for carrier sense node in dBm (default)
#define NUM_CHANNELS               14               //hardcode because C++ is gross. should not be using more than this in our simulations  

//tx (client) states
 enum {
    TX_IDLE = 0,
    TX_PROBE_INIT,
    TX_PROBE_WFACK,
    TX_PROBING,
    TX_PROBE_WFRX,
    TX_CHANGE_INIT,
    TX_CHANGE_WFACK,
    TX_VERIFY_INIT,
    TX_VERIFY_WFACK

 };

 //rx (server) states
 enum{
    RX_IDLE = 0,
    RX_PROBE_ACK,
    RX_PROBING,
    RX_CHANGE_ACK,
    RX_WF_VERIFY
 };

//pkts sent by chanswitch app
 enum {
    PROBE_PKT   = 1,
    PROBE_ACK   = 2,
    CHANGE_PKT  = 3,
    CHANGE_ACK  = 4,
    VERIFY_PKT  = 5,
    VERIFY_ACK  = 6,
    NODE_LIST   = 7
 };

typedef
struct struct_app_chanswitch_client_str
{
    int                     connectionId;
    Address                 localAddr;
    Address                 remoteAddr;
    clocktype               sessionStart;
    clocktype               sessionFinish;
    clocktype               lastTime;
    BOOL                    sessionIsClosed;
    int                     itemsToSend;
    int                     itemSizeLeft;
    Int64                   numBytesSent;
    Int64                   numBytesRecvd;
    Int32                   bytesRecvdDuringThePeriod;
    clocktype               lastItemSent;
    int                     uniqueId;
    RandomSeed              seed;
//DERIUS    
    char                    cmd[MAX_STRING_LENGTH];
    int                     state;
    BOOL                    got_RX_nodelist;
    Mac802Address           myAddr; 
    DOT11_VisibleNodeInfo*  txNodeList;
    DOT11_VisibleNodeInfo*  rxNodeList;
    double                  signalStrengthAtRx;
    int                     numChannels;
    int                     currentChannel;
    int                     nextChannel;
    D_BOOL*                 channelSwitch; //channels that can be switched to                
    
}AppDataChanswitchClient;

typedef
struct struct_app_chanswitch_server_str
{
    int             connectionId;
    Address         localAddr;
    Address         remoteAddr;
    clocktype       sessionStart;
    clocktype       sessionFinish;
    clocktype       lastTime;
    BOOL            sessionIsClosed;
    Int64           numBytesSent;
    Int64           numBytesRecvd;
    Int32           bytesRecvdDuringThePeriod;
    clocktype       lastItemSent;
    RandomSeed      seed;
//
    BOOL            isLoggedIn;
    int             portForDataConn;
    int             state;
    Mac802Address   myAddr;
    int             numChannels;
    int             currentChannel;
    int             nextChannel;
}AppDataChanswitchServer;

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
    Address serverAddr);


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
 * NAME:        AppChanswitchClientSendProbeInit.
 * PURPOSE:     Send the chanswitch request pkt.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the server data structure.
 *              initial - is this the first channel switch?
 *              myAddr - MAC address of this TX node 
 * RETURN:      none.
 */
void
AppChanswitchClientSendProbeInit(Node *node, AppDataChanswitchClient *clientPtr, BOOL initial);

/*
 * NAME:        AppChanswitchServerSendProbeAck.
 * PURPOSE:     Send the ack indicating server got probe request
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 *              initial - is this the first channel switch?
 * RETURN:      none.
 */
void
AppChanswitchServerSendProbeAck(Node *node, AppDataChanswitchServer *serverPtr);

/*
 * NAME:        AppChanswitchStartProbing.
 * PURPOSE:     Start scanning channels - either client or server.
 * PARAMETERS:  node - pointer to the node
 *              appType:     CHANSWITCH_TX_CLIENT or CHANSWITCH_RX_SERVER
 * RETURN:      none.
 */
void
AppChanswitchStartProbing(Node *node, int appType);

/*
 * NAME:        AppChanswitchServerSendVisibleNodeList.
 * PURPOSE:     Send the list of visible nodes to the TX.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchServerSendVisibleNodeList(Node *node, 
                                        AppDataChanswitchServer *serverPtr, 
                                        DOT11_VisibleNodeInfo* nodeInfo,
                                        int nodeCount);

/*
 * NAME:        AppChanswitchGetMyMacAddr.
 * PURPOSE:     Ask MAC for my MAC address
 * PARAMETERS:  node - pointer to the node,
 *              connectionId - identifier of the client/server connection
 *              appType - is this app TX or RX
 * RETURN:      none.
 */
void
AppChanswitchGetMyMacAddr(Node *node, int connectionId, int appType);

/*
 * NAME:        AppChanswitchClientParseRXNodeList.
 * PURPOSE:     Parse the node list packet from RX.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client
 *              packet - raw packet from RX
 * RETURN:      none.
 */
void
AppChanswitchClientParseRxNodeList(Node *node, AppDataChanswitchClient *clientPtr, char *packet);

/*
 * NAME:        AppChanswitchClientEvaulateChannels
 * PURPOSE:     Evaulate the node list and select the next channel.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client
 *              
 * RETURN:      the channel to switch to.
 */
int
AppChanswitchClientEvaluateChannels(Node *node,AppDataChanswitchClient *clientPtr);



#endif /* _CHANSWITCH_APP_H_ */
