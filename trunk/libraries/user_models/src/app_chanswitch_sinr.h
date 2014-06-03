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

#define CHANSWITCH_SINR_SCAN_PKT_SIZE 3 // id (1) : channel mask (2)
#define CHANSWITCH_SINR_CHANGE_PKT_SIZE 2 //id (1) : new channel (1)

#define RX_CHANGE_WFACK_TIMEOUT    (200 * MILLI_SECOND)
#define RX_VERIFY_WFACK_TIMEOUT    (200 * MILLI_SECOND)
#define TX_CHANGE_ACK_DELAY        (5 * MILLI_SECOND)

 //id (1) :: new channel (1)

 //tx (client) states
 enum {
    TX_S_IDLE = 0,
    TX_SCAN_INIT,
    TX_CHANGE

 };

 //rx (server) states
 enum{
    RX_S_IDLE = 0,
    RX_SCANNING,
    RX_CHANGE_WFACK,
    RX_VERIFY_WFACK
 };

//pkts sent by chanswitch app
 enum {
    TX_SCAN_PKT = 1,
    RX_CHANGE_PKT,
    TX_CHANGE_ACK,
    TX_VERIFY_ACK
 };

typedef
struct struct_app_chanswitch_sinr_client_str
{
    int             connectionId;
    Address         localAddr;
    Address         remoteAddr;
    clocktype       sessionStart;
    clocktype       sessionFinish;
    clocktype       lastTime;
    BOOL            sessionIsClosed;
    int             itemsToSend;
    int             itemSizeLeft;
    Int64           numBytesSent;
    Int64           numBytesRecvd;
    Int32           bytesRecvdDuringThePeriod;
    clocktype       lastItemSent;
    int             uniqueId;
    RandomSeed      seed;

//DERIUS
    char            cmd[MAX_STRING_LENGTH];
//CHANSWITCH additions
    int state;
    Mac802Address   myAddr; 
    int             numChannels;
    int             currentChannel;
    D_BOOL*         channelSwitch; //channels that can be switched to  
    double          noise_mW;      //thermal noise is the same on every channel in QualNet   
    BOOL            initBackoff;  //backoff to prevent too many channel switches
    int             nextChannel;
    clocktype       changeBackoffTime;

    
} AppDataChanswitchSinrClient;

typedef
struct struct_app_chanswitch_sinr_server_str
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
    //CHANSWITCH additions
    int             state;
    int             currentChannel;
    D_BOOL*         channelSwitch; //channels that can be switched to
    double*         avg_intnoise_dB; //arrays for worst/average noise on each channel
    double*         worst_intnoise_dB;
    double          noise_mW;
    double          txRss;    //RSS of TX node
    int             nextChannel;
} AppDataChanswitchSinrServer;

/*
 * NAME:        AppChanswitchSinrClientSendScanInit.
 * PURPOSE:     Send the "scan init" packet.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendScanInit(Node *node, AppDataChanswitchSinrClient *clientPtr);



/*
 * NAME:        AppChanswitchSinrClientSendChangeAck.
 * PURPOSE:     Send the ack indicating client got change request
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendChangeAck(Node *node, AppDataChanswitchSinrClient *clientPtr);

/*
 * NAME:        AppChanswitchSinrClientSendVerifyAck.
 * PURPOSE:     Send the ack indicating client changed to the new channel
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendVerifyAck(Node *node, AppDataChanswitchSinrClient *clientPtr);

/*
 * NAME:        AppChanswitchSinrServerScanChannels.
 * PURPOSE:     Initiate SINR scan of channels at RX node.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerScanChannels(Node *node, AppDataChanswitchSinrServer *serverPtr);


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
    clocktype changeBackoffTime);

/*
 * NAME:        AppChanswitchSinrServerEvaulateChannels
 * PURPOSE:     Evaulate the node list and select the next channel.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server
 *              
 * RETURN:      none.
 */
void
AppChanswitchSinrServerEvaluateChannels(Node *node, AppDataChanswitchSinrServer *serverPtr);


/*
 * NAME:        AppChanswitchSinrServerSendChangePkt
 * PURPOSE:     Send the change packet to the TX node.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server
 *              
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendChangePkt(Node *node, AppDataChanswitchSinrServer *serverPtr);


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






#endif /* _CHANSWITCH_SINR_APP_H_ */
