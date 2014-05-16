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
 * This file contains data structure used by the chanswitch_sinr application and
 * prototypes of functions defined in chanswitch_sinr.c.
 */

#ifndef CHANSWITCH_SINR_APP_H
#define CHANSWITCH_SINR_APP_H

#include "types.h"

#define CHANSWITCH_SINR_PROBE_PKT_SIZE           4
 //id (1) : channel mask (2) : flags (1)
#define CHANSWITCH_SINR_CHANGE_PKT_SIZE          2
 //id (1) : new channel (1)
#define CHANSWITCH_SINR_ACK_SIZE                 1
#define CHANSWITCH_SINR_LIST_HEADER_SIZE         9
 // id (1) : rx mac addr (6) : total station count (2) 
#define CHANSWITCH_SINR_LIST_ENTRY_SIZE          16   
//  channel (1) :  mac addr (6) : signal strength (8) : isAp (1)

#define TX_PROBE_WFACK_TIMEOUT     (100 * MILLI_SECOND)
#define TX_CHANGE_WFACK_TIMEOUT    (200 * MILLI_SECOND)
#define TX_VERIFY_WFACK_TIMEOUT    (200 * MILLI_SECOND)
#define RX_PROBE_ACK_DELAY         (5 * MILLI_SECOND)
#define RX_CHANGE_ACK_DELAY        (5 * MILLI_SECOND) 
#define SINR_MIN_DB                20.0             //SINR threshold for hidden node in dB (default)
#define CS_MIN_DBM                 -69.0            //energy threshold for carrier sense node in dBm (default)
#define CHANGE_BACKOFF             (1 * SECOND)
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
    TX_VERIFY_WFACK

 };

 //rx (server) states
 enum{
    RX_IDLE = 0,
    RX_PROBE_ACK,
    RX_PROBING,
    RX_CHANGE_ACK,
    RX_VERIFY_ACK
 };

//pkts sent by chanswitch_sinr app
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
struct struct_app_chanswitch_sinr_client_str
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
    double                  noise_mW; //thermal noise is the same on every channel in QualNet      
    double                  hnThreshold; //threshold for strong hidden node in dB (relative interference from HN)
    double                  csThreshold; //threshold for strong cs node in dBm 
    clocktype               changeBackoffTime; //minimum delay between channel scan/change (default 1s)
    BOOL                    initBackoff; 
    BOOL                    initial; //is this the "initial" chanswitch_sinr or a "mid-stream" chanswitch_sinr?
    Mac802Address           rxAddr; 
    
}AppDataChanswitchSinrClient;

typedef
struct struct_app_chanswitch_sinr_server_str
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
}AppDataChanswitchSinrServer;

/*
 * NAME:        AppLayerChanswitchSinrClient.
 * PURPOSE:     Models the behaviour of ChanswitchSinr Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchSinrClient(Node *nodePtr, Message *msg);

/*
 * NAME:        AppChanswitchSinrClientInit.
 * PURPOSE:     Initialize a ChanswitchSinr session.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              serverAddr - address of the server,
 *              itemsToSend - number of items to send,
 *              waitTime - time until the session starts.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientInit(
    Node *nodePtr,
    Address clientAddr,
    Address serverAddr,
    clocktype waitTime,
    double hnThreshold,
    double csThreshold,
    clocktype changeBackoffTime);

/*
 * NAME:        AppChanswitchSinrClientPrintStats.
 * PURPOSE:     Prints statistics of a ChanswitchSinr session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              clientPtr - pointer to the chanswitch_sinr client data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientPrintStats(Node *nodePtr, AppDataChanswitchSinrClient *clientPtr);

/*
 * NAME:        AppChanswitchSinrClientFinalize.
 * PURPOSE:     Collect statistics of a ChanswitchSinr session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientFinalize(Node *nodePtr, AppInfo* appInfo);

/*
 * NAME:        AppChanswitchSinrClientGetChanswitchSinrClient.
 * PURPOSE:     search for a chanswitch_sinr client data structure.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              connId - connection ID of the chanswitch_sinr client.
 * RETURN:      the pointer to the chanswitch_sinr client data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchSinrClient *
AppChanswitchSinrClientGetChanswitchSinrClient(Node *nodePtr, int connId);

/*
 * NAME:        AppChanswitchSinrClientUpdateChanswitchSinrClient.
 * PURPOSE:     update existing chanswitch_sinr client data structure by including
 *              connection id.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch_sinr client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchSinrClient *
AppChanswitchSinrClientUpdateChanswitchSinrClient(
    Node *nodePtr,
    TransportToAppOpenResult *openResult);

/*
 * NAME:        AppChanswitchSinrClientNewChanswitchSinrClient.
 * PURPOSE:     create a new chanswitch_sinr client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              clientAddr - address of this client.
 *              serverAddr - chanswitch_sinr server to this client.
 *              itemsToSend - number of chanswitch_sinr items to send in simulation.
 * RETRUN:      the pointer to the created chanswitch_sinr client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchSinrClient *
AppChanswitchSinrClientNewChanswitchSinrClient(
    Node *nodePtr,
    Address clientAddr,
    Address serverAddr,
    double hnThreshold,
    double csThreshold,
    clocktype changeBackoffTime);


/*
 * NAME:        AppChanswitchSinrClientItemsToSend.
 * PURPOSE:     call tcplib function chanswitch_sinr_nitems() to get the
 *              number of items to send in an chanswitch_sinr session.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETRUN:      amount of items to send.
 */
int
AppChanswitchSinrClientItemsToSend(AppDataChanswitchSinrClient *clientPtr);

/*
 * NAME:        AppChanswitchSinrClientItemSize.
 * PURPOSE:     call tcplib function chanswitch_sinr_itemsize() to get the size
 *              of each item.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETRUN:      size of an item.
 */
int
AppChanswitchSinrClientItemSize(AppDataChanswitchSinrClient *clientPtr);


/*
 * NAME:        AppLayerChanswitchSinrServer.
 * PURPOSE:     Models the behaviour of ChanswitchSinr Server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchSinrServer(Node *nodePtr, Message *msg);

/*
 * NAME:        AppChanswitchSinrServerInit.
 * PURPOSE:     listen on ChanswitchSinr server port.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerInit(Node *nodePtr, Address serverAddr);

/*
 * NAME:        AppChanswitchSinrServerPrintStats.
 * PURPOSE:     Prints statistics of a ChanswitchSinr session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              serverPtr - pointer to the chanswitch_sinr server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerPrintStats(Node *nodePtr, AppDataChanswitchSinrServer *serverPtr);


/*
 * NAME:        AppChanswitchSinrServerFinalize.
 * PURPOSE:     Collect statistics of a ChanswitchSinr session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerFinalize(Node *nodePtr, AppInfo* appInfo);

/*
 * NAME:        AppChanswitchSinrServerGetChanswitchSinrServer.
 * PURPOSE:     search for a chanswitch_sinr server data structure.
 * PARAMETERS:  appList - link list of applications,
 *              connId - connection ID of the chanswitch_sinr server.
 * RETURN:      the pointer to the chanswitch_sinr server data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchSinrServer *
AppChanswitchSinrServerGetChanswitchSinrServer(Node *nodePtr, int connId);

/*
 * NAME:        AppChanswitchSinrServerNewChanswitchSinrServer.
 * PURPOSE:     create a new chanswitch_sinr server data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch_sinr server data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchSinrServer *
AppChanswitchSinrServerNewChanswitchSinrServer(Node *nodePtr,
                         TransportToAppOpenResult *openResult);


/*
 * NAME:        AppChanswitchSinrClientSendProbeInit.
 * PURPOSE:     Send the chanswitch_sinr request pkt.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the server data structure.
 *              initial - is this the first channel switch?
 *              myAddr - MAC address of this TX node 
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendProbeInit(Node *node, AppDataChanswitchSinrClient *clientPtr);

/*
 * NAME:        AppChanswitchSinrServerSendProbeAck.
 * PURPOSE:     Send the ack indicating server got probe request
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 *              initial - is this the first channel switch?
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendProbeAck(Node *node, AppDataChanswitchSinrServer *serverPtr);

/*
 * NAME:        AppChanswitchSinrServerSendChangeAck.
 * PURPOSE:     Send the ack indicating server got change request
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendChangeAck(Node *node, AppDataChanswitchSinrServer *serverPtr);

/*
 * NAME:        AppChanswitchSinrServerSendVerifyAck.
 * PURPOSE:     Send the ack indicating server changed to the new channel
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendVerifyAck(Node *node, AppDataChanswitchSinrServer *serverPtr);


/*
 * NAME:        AppChanswitchSinrClientSendChangeInit.
 * PURPOSE:     Send the "change init" packet.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendChangeInit(Node *node, AppDataChanswitchSinrClient *clientPtr);

/*
 * NAME:        AppChanswitchSinrStartProbing.
 * PURPOSE:     Start scanning channels - either client or server.
 * PARAMETERS:  node - pointer to the node
 *              appType:     CHANSWITCH_SINR_TX_CLIENT or CHANSWITCH_SINR_RX_SERVER
 * RETURN:      none.
 */
void
AppChanswitchSinrStartProbing(Node *node, int appType);

/*
 * NAME:            
 * PURPOSE:     Change to the next channel - either client or server.
 * PARAMETERS:  node - pointer to the node
 * connectionId - identifier of the client/server connection
 * appType      - is this app TX or RX
 * newChannel   - the new channel to switch to
 * RETURN:      none.
 */
void 
AppChanswitchSinrChangeChannels(Node *node, int connectionId, int appType, int oldChannel, int newChannel);

/*
 * NAME:        AppChanswitchSinrServerSendVisibleNodeList.
 * PURPOSE:     Send the list of visible nodes to the TX.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendVisibleNodeList(Node *node, 
                                        AppDataChanswitchSinrServer *serverPtr, 
                                        DOT11_VisibleNodeInfo* nodeInfo,
                                        int nodeCount);

/*
 * NAME:        AppChanswitchSinrGetMyMacAddr.
 * PURPOSE:     Ask MAC for my MAC address (and start the probe after getting it.)
 * PARAMETERS:  node - pointer to the node,
 *              connectionId - identifier of the client/server connection
 *              appType - is this app TX or RX
 *              initial - true if first chanswitch_sinr, false otherwise
 * RETURN:      none.
 */
void
AppChanswitchSinrGetMyMacAddr(Node *node, int connectionId, int appType, BOOL initial);

/*
 * NAME:        AppChanswitchSinrClientParseRXNodeList.
 * PURPOSE:     Parse the node list packet from RX.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client
 *              packet - raw packet from RX
 * RETURN:      none.
 */
void
AppChanswitchSinrClientParseRxNodeList(Node *node, AppDataChanswitchSinrClient *clientPtr, char *packet);

/*
 * NAME:        AppChanswitchSinrClientEvaulateChannels
 * PURPOSE:     Evaulate the node list and select the next channel.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client
 *              
 * RETURN:      the channel to switch to.
 */
int
AppChanswitchSinrClientEvaluateChannels(Node *node,AppDataChanswitchSinrClient *clientPtr);

/*
 * NAME:        ClearVisibleNodeList
 * PURPOSE:     Clear (deallocate) this node list in preparation for the next scan.
 * PARAMETERS:  nodelist - pointer to the visible node list
 * RETURN:      none.
 */
void
ClearVisibleNodeList(DOT11_VisibleNodeInfo* nodeList);

/*
 * NAME:        AppChanswitchSinrClientChangeInit.
 *              Perform required functions upon reaching TX_CHANGE_INIT state.
 *              (Evaluate channels, determine if channel change is needed and start ACK timeout.)
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void 
AppChanswitchSinrClientChangeInit(Node *node,AppDataChanswitchSinrClient *clientPtr);





#endif /* _CHANSWITCH_SINR_APP_H_ */
