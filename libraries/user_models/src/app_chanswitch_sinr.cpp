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
 * function, and finalize function used by the chanswitch_sinr application.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "api.h"
#include "transport_tcp.h"
#include "tcpapps.h"
#include "app_util.h"
#include "app_chanswitch_sinr.h"
#include "app_chanswitch.h"

 #define DEBUG_CHANSWITCH_SINR 1

/*
 * Static Functions
 */

/*
 * NAME:        AppChanswitchSinrClientGetChanswitchSinrClient.
 * PURPOSE:     search for a chanswitch_sinr client data structure.
 * PARAMETERS:  node - pointer to the node.
 *              connId - connection ID of the chanswitch_sinr client.
 * RETURN:      the pointer to the chanswitch_sinr client data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchSinrClient *
AppChanswitchSinrClientGetChanswitchSinrClient(Node *node, int connId)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataChanswitchSinrClient *chanswitch_sinrClient;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_CHANSWITCH_SINR_CLIENT)
        {
            chanswitch_sinrClient = (AppDataChanswitchSinrClient *) appList->appDetail;

            if (chanswitch_sinrClient->connectionId == connId)
            {
                return chanswitch_sinrClient;
            }
        }
    }

    return NULL;
}

/*
 * NAME:        AppChanswitchSinrClientUpdateChanswitchSinrClient.
 * PURPOSE:     update existing chanswitch_sinr client data structure by including
 *              connection id.
 * PARAMETERS:  node - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch_sinr client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchSinrClient *
AppChanswitchSinrClientUpdateChanswitchSinrClient(Node *node,
                            TransportToAppOpenResult *openResult)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataChanswitchSinrClient *tmpChanswitchSinrClient = NULL;
    AppDataChanswitchSinrClient *chanswitch_sinrClient = NULL;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_CHANSWITCH_SINR_CLIENT)
        {
            tmpChanswitchSinrClient = (AppDataChanswitchSinrClient *) appList->appDetail;

#ifdef DEBUG
            printf("CHANSWITCH_SINR Client: Node %d comparing uniqueId "
                   "%d with %d\n",
                   node->nodeId,
                   tmpChanswitchSinrClient->uniqueId,
                   openResult->uniqueId);
#endif /* DEBUG */

            if (tmpChanswitchSinrClient->uniqueId == openResult->uniqueId)
            {
                chanswitch_sinrClient = tmpChanswitchSinrClient;
                break;
            }
        }
    }

    if (chanswitch_sinrClient == NULL)
    {
        assert(FALSE);
    }

    /*
     * fill in connection id, etc.
     */
    chanswitch_sinrClient->connectionId = openResult->connectionId;
    chanswitch_sinrClient->localAddr = openResult->localAddr;
    chanswitch_sinrClient->remoteAddr = openResult->remoteAddr;
    chanswitch_sinrClient->sessionStart = getSimTime(node);
    chanswitch_sinrClient->sessionFinish = getSimTime(node);
    chanswitch_sinrClient->lastTime = 0;
    chanswitch_sinrClient->sessionIsClosed = FALSE;
    chanswitch_sinrClient->bytesRecvdDuringThePeriod = 0;
    chanswitch_sinrClient->state = TX_S_IDLE;
#ifdef DEBUG
    char addrStr[MAX_STRING_LENGTH];
    printf("CHANSWITCH_SINR Client: Node %d updating chanswitch_sinr client structure\n",
            node->nodeId);
    printf("    connectionId = %d\n", chanswitch_sinrClient->connectionId);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->localAddr, addrStr);
    printf("    localAddr = %s\n", addrStr);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->remoteAddr, addrStr);
    printf("    remoteAddr = %s\n", addrStr);
    printf("    itemsToSend = %d\n", chanswitch_sinrClient->itemsToSend);
#endif /* DEBUG */

    return chanswitch_sinrClient;
}

/*
 * NAME:        AppChanswitchSinrClientNewChanswitchSinrClient.
 * PURPOSE:     create a new chanswitch_sinr client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node.
 *              clientAddr - address of this client.
 *              serverAddr - chanswitch_sinr server to this client.
 *              itemsToSend - number of chanswitch_sinr items to send in simulation.
 * RETRUN:      the pointer to the created chanswitch_sinr client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchSinrClient *
AppChanswitchSinrClientNewChanswitchSinrClient(Node *node,
                         Address clientAddr,
                         Address serverAddr,
                         clocktype changeBackoffTime)
{
    AppDataChanswitchSinrClient *chanswitch_sinrClient;

    chanswitch_sinrClient = (AppDataChanswitchSinrClient *)
                MEM_malloc(sizeof(AppDataChanswitchSinrClient));

    memset(chanswitch_sinrClient, 0, sizeof(AppDataChanswitchSinrClient));

    /*
     * fill in connection id, etc.
     */
    chanswitch_sinrClient->connectionId = -1;
    chanswitch_sinrClient->uniqueId = node->appData.uniqueId++;

    chanswitch_sinrClient->localAddr = clientAddr;
    chanswitch_sinrClient->remoteAddr = serverAddr;

    // Make unique seed.
    RANDOM_SetSeed(chanswitch_sinrClient->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_CHANSWITCH_SINR_CLIENT,
                   chanswitch_sinrClient->uniqueId);


    chanswitch_sinrClient->itemsToSend = 100;
    chanswitch_sinrClient->itemSizeLeft = 0;
    chanswitch_sinrClient->numBytesSent = 0;
    chanswitch_sinrClient->numBytesRecvd = 0;
    chanswitch_sinrClient->lastItemSent = 0;
    chanswitch_sinrClient->state = TX_S_IDLE;
    chanswitch_sinrClient->initBackoff = FALSE;
    chanswitch_sinrClient->numChannels = 1;
    chanswitch_sinrClient->currentChannel = 0;
    chanswitch_sinrClient->changeBackoffTime = changeBackoffTime; //1 second default

#ifdef DEBUG
    char addrStr[MAX_STRING_LENGTH];
    printf("CHANSWITCH_SINR Client: Node %d creating new chanswitch_sinr "
           "client structure\n", node->nodeId);
    printf("    connectionId = %d\n", chanswitch_sinrClient->connectionId);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->localAddr, addrStr);
    printf("    localAddr = %s\n", addrStr);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->remoteAddr, addrStr);
    printf("    remoteAddr = %s\n", addrStr);
    printf("    itemsToSend = %d\n", chanswitch_sinrClient->itemsToSend);
#endif /* DEBUG */

    #ifdef DEBUG_OUTPUT_FILE
    {
        char fileName[MAX_STRING_LENGTH];
        FILE *fp;
        char dataBuf[MAX_STRING_LENGTH];

        sprintf(fileName, "CHANSWITCH_SINR_Throughput_%d", node->nodeId);

        /*
         * Open a data file to accumulate different statistics
         * informations of this node.
         */
        fp = fopen(fileName, "w");
        if (fp)
        {
            sprintf(dataBuf, "%s\n", "Simclock   Throughput(in Kbps) ");
            fwrite(dataBuf, strlen(dataBuf), 1, fp);
            fclose(fp);
        }
    }
    #endif

    APP_RegisterNewApp(node, APP_CHANSWITCH_SINR_CLIENT, chanswitch_sinrClient);


    return chanswitch_sinrClient;
}




/*
 * NAME:        AppChanswitchSinrServerGetChanswitchSinrServer.
 * PURPOSE:     search for a chanswitch_sinr server data structure.
 * PARAMETERS:  appList - link list of applications,
 *              connId - connection ID of the chanswitch_sinr server.
 * RETURN:      the pointer to the chanswitch_sinr server data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchSinrServer *
AppChanswitchSinrServerGetChanswitchSinrServer(Node *node, int connId)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataChanswitchSinrServer *chanswitch_sinrServer;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_CHANSWITCH_SINR_SERVER)
        {
            chanswitch_sinrServer = (AppDataChanswitchSinrServer *) appList->appDetail;

            if (chanswitch_sinrServer->connectionId == connId)
            {
                return chanswitch_sinrServer;
            }
        }
    }

    return NULL;
}


/*
 * NAME:        AppChanswitchSinrServerNewChanswitchSinrServer.
 * PURPOSE:     create a new chanswitch_sinr server data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch_sinr server data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchSinrServer *
AppChanswitchSinrServerNewChanswitchSinrServer(Node *node,
                         TransportToAppOpenResult *openResult)
{
    AppDataChanswitchSinrServer *chanswitch_sinrServer;

    chanswitch_sinrServer = (AppDataChanswitchSinrServer *)
                MEM_malloc(sizeof(AppDataChanswitchSinrServer));

    /*
     * fill in connection id, etc.
     */
    chanswitch_sinrServer->connectionId = openResult->connectionId;
    chanswitch_sinrServer->localAddr = openResult->localAddr;
    chanswitch_sinrServer->remoteAddr = openResult->remoteAddr;
    chanswitch_sinrServer->sessionStart = getSimTime(node);
    chanswitch_sinrServer->sessionFinish = getSimTime(node);
    chanswitch_sinrServer->lastTime = 0;
    chanswitch_sinrServer->sessionIsClosed = FALSE;
    chanswitch_sinrServer->numBytesSent = 0;
    chanswitch_sinrServer->numBytesRecvd = 0;
    chanswitch_sinrServer->bytesRecvdDuringThePeriod = 0;
    chanswitch_sinrServer->state = RX_S_IDLE;
    chanswitch_sinrServer->lastItemSent = 0;

    RANDOM_SetSeed(chanswitch_sinrServer->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_CHANSWITCH_SINR_SERVER,
                   chanswitch_sinrServer->connectionId);

    APP_RegisterNewApp(node, APP_CHANSWITCH_SINR_SERVER, chanswitch_sinrServer);

    #ifdef DEBUG_OUTPUT_FILE
    {
        char fileName[MAX_STRING_LENGTH];
        FILE *fp;
        char dataBuf[MAX_STRING_LENGTH];

        sprintf(fileName, "CHANSWITCH_SINR_Throughput_%d", node->nodeId);

        /*
         * Open a data file to accumulate different statistics
         * informations of this node.
         */
        fp = fopen(fileName, "w");
        if (fp)
        {
            sprintf(dataBuf, "%s\n", "Simclock   Throughput(in Kbps) ");
            fwrite(dataBuf, strlen(dataBuf), 1, fp);
            fclose(fp);
        }
    }
    #endif

    return chanswitch_sinrServer;
}

/*
 * NAME:        AppChanswitchSinrClientSendScanInit.
 * PURPOSE:     Send the "scan init" packet.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendScanInit(Node *node, AppDataChanswitchSinrClient *clientPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_SINR_SCAN_PKT_SIZE); //3
    memset(payload,TX_SCAN_PKT,1);
    memset(payload+1,0xfe,2); //dummy data for chanswitch mask

    if (!clientPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                clientPtr->connectionId,
                payload,
                CHANSWITCH_SINR_SCAN_PKT_SIZE,
                TRACE_APP_CHANSWITCH_SINR);

    }
     MEM_free(payload);

}

/*
 * NAME:        AppChanswitchSinrClientSendChangeAck.
 * PURPOSE:     Send the ack indicating client got change request
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendChangeAck(Node *node, AppDataChanswitchSinrClient *clientPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_ACK_SIZE); //1
    memset(payload,TX_CHANGE_ACK,1);
    
    if (!clientPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                clientPtr->connectionId,
                payload,
                CHANSWITCH_ACK_SIZE,
                TRACE_APP_CHANSWITCH_SINR);
    }
     MEM_free(payload);
}

/*
 * NAME:        AppChanswitchSinrClientSendVerifyAck.
 * PURPOSE:     Send the ack indicating client got verify request
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendVerifyAck(Node *node, AppDataChanswitchSinrClient *clientPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_ACK_SIZE); //1
    memset(payload,TX_VERIFY_ACK,1);
    
    if (!clientPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                clientPtr->connectionId,
                payload,
                CHANSWITCH_ACK_SIZE,
                TRACE_APP_CHANSWITCH_SINR);
    }
     MEM_free(payload);
}



/*
 * Public Functions
 */

/*
 * NAME:        AppLayerChanswitchSinrClient.
 * PURPOSE:     Models the behaviour of ChanswitchSinr Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchSinrClient(Node *node, Message *msg)
{
    char buf[MAX_STRING_LENGTH];
    AppDataChanswitchSinrClient *clientPtr;

    ctoa(getSimTime(node), buf);

    switch (msg->eventType)
    {
        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult *openResult;

            openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: CHANSWITCH_SINR Client node %u got OpenResult\n", buf, node->nodeId);
#endif /* DEBUG */

            assert(openResult->type == TCP_CONN_ACTIVE_OPEN);

            if (openResult->connectionId < 0)
            {
#ifdef DEBUG
                printf("%s: CHANSWITCH_SINR Client node %u connection failed!\n",
                       buf, node->nodeId);
#endif /* DEBUG */

                node->appData.numAppTcpFailure ++;
            }
            else
            {
                AppDataChanswitchSinrClient *clientPtr;

                clientPtr = AppChanswitchSinrClientUpdateChanswitchSinrClient(node, openResult);

                assert(clientPtr != NULL);

                clientPtr->state = TX_SCAN_INIT;

                //get the needed info from MAC layer. initial scan will start after if enabled
                AppChanswitchGetMyMacAddr(node,openResult->connectionId, CHANSWITCH_SINR_TX_CLIENT, TRUE);

                // //start channel reselection backoff timer
                clientPtr->initBackoff = TRUE;
                Message *initTimeout;
                initTimeout = MESSAGE_Alloc(node, 
                    APP_LAYER,
                    APP_CHANSWITCH_SINR_CLIENT,
                    MSG_APP_TxChannelSelectionTimeout);

                AppChanswitchTimeout* initInfo = (AppChanswitchTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        initTimeout,
                        sizeof(AppChanswitchTimeout));
                ERROR_Assert(initInfo, "cannot allocate enough space for needed info");
                initInfo->connectionId = clientPtr->connectionId;

                MESSAGE_Send(node, initTimeout, clientPtr->changeBackoffTime);

            }

            break;
        }
        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *dataSent;

            dataSent = (TransportToAppDataSent *) MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: CHANSWITCH_SINR Client node %u sent data %d\n",
                   buf, node->nodeId, dataSent->length);
#endif /* DEBUG */

            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                                 dataSent->connectionId);

            assert(clientPtr != NULL);

            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            TransportToAppDataReceived *dataRecvd;
            char *packet;

            dataRecvd = (TransportToAppDataReceived *)
                         MESSAGE_ReturnInfo(msg);
            packet = MESSAGE_ReturnPacket(msg);

#ifdef DEBUG
            printf("%s: CHANSWITCH_SINR Client node %u received data %d\n",
                   buf, node->nodeId, msg->packetSize);
#endif /* DEBUG */

            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                                 dataRecvd->connectionId);

            assert(clientPtr != NULL);

            switch(clientPtr->state){
                case TX_SCAN_INIT: {
                    if(packet[0] == RX_CHANGE_PKT){
                    printf("CHANSWITCH_SINR Client: Node %ld at %s got RX_CHANGE_PKT while in TX_SCAN_INIT state.\n",
                            node->nodeId, buf);
                    //set the new channel
                    int nextChannel = 0;
                    memcpy(&nextChannel,packet+1,1);
                    clientPtr->nextChannel = nextChannel;
                    //send the TX_CHANGE_ACK to RX
                    AppChanswitchSinrClientSendChangeAck(node, clientPtr);
                    //delay
                    Message *changeMsg;
                    changeMsg = MESSAGE_Alloc(node, 
                        APP_LAYER,
                        APP_CHANSWITCH_SINR_CLIENT,
                        MSG_APP_TxChangeAckDelay);
                    AppChanswitchTimeout* info = (AppChanswitchTimeout*)
                        MESSAGE_InfoAlloc(
                            node,
                            changeMsg,
                            sizeof(AppChanswitchTimeout));
                    ERROR_Assert(info, "cannot allocate enough space for needed info");
                    info->connectionId = dataRecvd->connectionId;

                    MESSAGE_Send(node, changeMsg, TX_CHANGE_ACK_DELAY);
                    }
                    else {
                        ERROR_ReportWarning("CHANSWITCH_SINR Client: received unknown packet from RX \n");
                        
                    }
                    break;
                }
            }

            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult *closeResult;

            closeResult = (TransportToAppCloseResult *)
                          MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: CHANSWITCH_SINR Client node %u got close result\n",
                   buf, node->nodeId);
#endif /* DEBUG */

            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                                 closeResult->connectionId);

            assert(clientPtr != NULL);

            if (clientPtr->sessionIsClosed == FALSE)
            {
                clientPtr->sessionIsClosed = TRUE;
                clientPtr->sessionFinish = getSimTime(node);
            }

            break;
        }

        case MSG_APP_FromMac_MACAddressRequest: {
            MacToAppAddrRequest* addrRequest;
            addrRequest = (MacToAppAddrRequest*) MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node, addrRequest->connectionId);
            clientPtr->myAddr = addrRequest->myAddr; //save my address
            clientPtr->currentChannel = addrRequest->currentChannel;
            clientPtr->numChannels = addrRequest->numChannels;
            clientPtr->noise_mW = addrRequest->noise_mW;
            clientPtr->channelSwitch = addrRequest->channelSwitch;

            printf("%s: CHANSWITCH_SINR Client node %u got MSG_APP_FromMac_MACAddressRequest\n",
                   buf, node->nodeId);

            if(!(addrRequest->initial) || (addrRequest->initial && addrRequest->asdcsInit))
            {
                printf("Initial scan enabled - sending SCAN_PKT to RX \n");  
                AppChanswitchSinrClientSendScanInit(node, clientPtr);
                clientPtr->state = TX_SCAN_INIT;

            }
            else {
                clientPtr->state = TX_S_IDLE;
            }
            break;
        }

        //Prevents multiple channel reselections from being initiated simultaneously
        case MSG_APP_TxChannelSelectionTimeout:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Client node %u got channel selection timeout\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH */
            AppChanswitchTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                            timeoutInfo->connectionId);
            clientPtr->initBackoff = FALSE;
            break;
        }

        case MSG_APP_TxChangeAckDelay: {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH Client node %u change ACK delay timer expired\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
                            AppChanswitchTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                            timeoutInfo->connectionId);
            //change channels
            AppChanswitchChangeChannels(
                node, 
                clientPtr->connectionId, 
                CHANSWITCH_SINR_TX_CLIENT, 
                clientPtr->currentChannel,
                clientPtr->nextChannel);
            //send TX_VERIFY_ACK
            AppChanswitchSinrClientSendVerifyAck(node, clientPtr);
            break;
        }

        case MSG_APP_InitiateChannelScanRequest: {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH SINR Client node %u received a request to initiate a channel scan \n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            AppInitScanRequest* initRequest = (AppInitScanRequest*) MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node, initRequest->connectionId);

            //start the scan if timer isn't expired
            if(clientPtr->state == TX_S_IDLE && clientPtr->initBackoff == FALSE){
                printf("Attempting mid-stream channel switch on node %u \n", node->nodeId);
                //start backoff timer to prevent multiple requests
                clientPtr->state = TX_SCAN_INIT;
                clientPtr->initBackoff = TRUE;
                Message *initTimeout;
                initTimeout = MESSAGE_Alloc(node, 
                    APP_LAYER,
                    APP_CHANSWITCH_SINR_CLIENT,
                    MSG_APP_TxChannelSelectionTimeout);

                AppChanswitchTimeout* initInfo = (AppChanswitchTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        initTimeout,
                        sizeof(AppChanswitchTimeout));
                ERROR_Assert(initInfo, "cannot allocate enough space for needed info");
                initInfo->connectionId = clientPtr->connectionId;
                MESSAGE_Send(node, initTimeout, clientPtr->changeBackoffTime);

                //Get my mac address from MAC layer (probe will start after)
                AppChanswitchGetMyMacAddr(node,clientPtr->connectionId, CHANSWITCH_SINR_TX_CLIENT, FALSE);
            }
            break;
        }
        default:
            ctoa(getSimTime(node), buf);
            printf("Time %s: CHANSWITCH_SINR Client node %u received message of unknown"
                   " type %d.\n", buf, node->nodeId, msg->eventType);
            assert(FALSE);
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        AppChanswitchSinrClientInit.
 * PURPOSE:     Initialize a ChanswitchSinr session.
 * PARAMETERS:  node - pointer to the node,
 *              serverAddr - address of the server,
 *              itemsToSend - number of items to send,
 *              waitTime - time until the session starts.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientInit(
    Node *node,
    Address clientAddr,
    Address serverAddr,
    clocktype waitTime,
    clocktype changeBackoffTime)
{
    AppDataChanswitchSinrClient *clientPtr;


    clientPtr = AppChanswitchSinrClientNewChanswitchSinrClient(node,
                                         clientAddr,
                                         serverAddr,
                                         changeBackoffTime);

    if (clientPtr == NULL)
    {
        printf("CHANSWITCH_SINR Client: Node %d cannot allocate "
               "new chanswitch_sinr client\n", node->nodeId);

        assert(FALSE);
    }

    APP_TcpOpenConnectionWithPriority(
        node,
        APP_CHANSWITCH_SINR_CLIENT,
        clientAddr,
        node->appData.nextPortNum++,
        serverAddr,
        (short) APP_CHANSWITCH_SINR_SERVER,
        clientPtr->uniqueId,
        waitTime,
        APP_DEFAULT_TOS);
}

/*
 * NAME:        AppChanswitchSinrClientPrintStats.
 * PURPOSE:     Prints statistics of a ChanswitchSinr session.
 * PARAMETERS:  node - pointer to the node.
 *              clientPtr - pointer to the chanswitch_sinr client data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientPrintStats(Node *node, AppDataChanswitchSinrClient *clientPtr)
{
    clocktype throughput;
    char addrStr[MAX_STRING_LENGTH];
    char startStr[MAX_STRING_LENGTH];
    char finishStr[MAX_STRING_LENGTH];
    char statusStr[MAX_STRING_LENGTH];
    char numBytesSentStr[MAX_STRING_LENGTH];
    char numBytesRecvdStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char throughputStr[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(clientPtr->sessionStart, startStr);
    TIME_PrintClockInSecond(clientPtr->lastItemSent, finishStr);

    if (clientPtr->connectionId < 0)
    {
        sprintf(statusStr, "Failed");
    }
    else if (clientPtr->sessionIsClosed == FALSE)
    {
        clientPtr->sessionFinish = getSimTime(node);
        sprintf(statusStr, "Not closed");
    }
    else
    {
        sprintf(statusStr, "Closed");
    }

    if (clientPtr->sessionFinish <= clientPtr->sessionStart)
    {
        throughput = 0;
    }
    else
    {
        throughput =
            (clocktype) ((clientPtr->numBytesSent * 8.0 * SECOND)
            / (clientPtr->sessionFinish - clientPtr->sessionStart));
    }

    ctoa(throughput, throughputStr);

    IO_ConvertIpAddressToString(&clientPtr->remoteAddr, addrStr);
    sprintf(buf, "Server address = %s", addrStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "First Packet Sent at (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Last Packet Sent at (s) = %s", finishStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Session Status = %s", statusStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    ctoa(clientPtr->numBytesSent, numBytesSentStr);
    sprintf(buf, "Total Bytes Sent = %s", numBytesSentStr);

    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    ctoa(clientPtr->numBytesRecvd, numBytesRecvdStr);
    sprintf(buf, "Total Bytes Received = %s", numBytesRecvdStr);

    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);
}

/*
 * NAME:        AppChanswitchSinrClientFinalize.
 * PURPOSE:     Collect statistics of a ChanswitchSinr session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientFinalize(Node *node, AppInfo* appInfo)
{
    AppDataChanswitchSinrClient *clientPtr = (AppDataChanswitchSinrClient*)appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        AppChanswitchSinrClientPrintStats(node, clientPtr);
    }
}

/*
 * NAME:        AppChanswitchSinrServerScanChannels.
 * PURPOSE:     Initiate SINR scan of channels at RX node.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerScanChannels(Node *node, AppDataChanswitchSinrServer *serverPtr){
    // printf("AppChanswitchSinrServerScanChannels \n");
    Message *macMsg;
    macMsg = MESSAGE_Alloc(node, 
    MAC_LAYER,
    MAC_PROTOCOL_DOT11,
    MSG_MAC_FromAppInitiateSinrScanRequest);

    AppToMacStartProbe* info = (AppToMacStartProbe*)
        MESSAGE_InfoAlloc(
            node,
            macMsg,
            sizeof(AppToMacStartProbe));
    ERROR_Assert(info, "cannot allocate enough space for needed info");
    info->connectionId = serverPtr->connectionId;
    info->appType = APP_CHANSWITCH_SINR_SERVER;
    info->initial = FALSE; //unused

    MESSAGE_Send(node, macMsg, 0);
}

/*
 * NAME:        AppChanswitchSinrServerEvaulateChannels
 * PURPOSE:     Evaulate the node list and select the next channel.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the client
 *              
 * RETURN:      the channel to switch to.
 */
void
AppChanswitchSinrServerEvaluateChannels(Node *node, AppDataChanswitchSinrServer *serverPtr){

    int newChannel;
    int lowest_int_channel = serverPtr->currentChannel;
    int highest_int_channel = serverPtr->currentChannel;
    double lowest_int = 0.0;
    double highest_int = -100.0;
    double this_channel_int = -90.0;
    int numChannels = PROP_NumberChannels(node);

    for (int i = 1; i < numChannels+1; i++) {     
        if (serverPtr->channelSwitch[newChannel]) {
            this_channel_int = serverPtr->avg_intnoise_dB[newChannel];
            double sinr = serverPtr->txRss - this_channel_int ; 
            printf("Channel %d: Interference %f, SINR %f \n", newChannel, this_channel_int, IN_DB(sinr));
            if(this_channel_int < lowest_int){
                lowest_int = this_channel_int;
                lowest_int_channel = newChannel;
            }
            if(this_channel_int > highest_int){
                highest_int = this_channel_int;
                highest_int_channel = newChannel;
            }
        }
        newChannel = (i + serverPtr->currentChannel) % numChannels; 
    }
    double worstsinr = serverPtr->txRss - highest_int;
    double bestsinr = serverPtr->txRss - lowest_int;
    printf("AppChanswitchSinrServerEvaluateChannels: worst SINR is %f dB on channel %d, best is %f dB on channel %d for node %d \n", 
        worstsinr,highest_int_channel, bestsinr, lowest_int_channel, node->nodeId);

    printf("AppChanswitchSinrServerEvaluateChannels: Selecting channel %d on node %d...\n",lowest_int_channel,node->nodeId);
    serverPtr->nextChannel = newChannel;
}

/*
 * NAME:        AppChanswitchSinrServerSendChangePkt
 * PURPOSE:     Send the change packet to the TX node.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server
 *              
 * RETURN:      none.
 */
void AppChanswitchSinrServerSendChangePkt(Node *node, AppDataChanswitchSinrServer *serverPtr){
        printf("best channel is channel %d, sending change pkt to TX \n", serverPtr->nextChannel);

        //first send the packet.
        char *payload;
        payload = (char *)MEM_malloc(CHANSWITCH_SINR_CHANGE_PKT_SIZE); //2
        memset(payload,RX_CHANGE_PKT,1);
        memcpy(payload+1,&(serverPtr->nextChannel),1); //first byte of int

        if (!serverPtr->sessionIsClosed)
        {
            APP_TcpSendData(
                    node,
                    serverPtr->connectionId,
                    payload,
                    CHANSWITCH_SINR_CHANGE_PKT_SIZE,
                    TRACE_APP_CHANSWITCH_SINR);

        }
         MEM_free(payload);

}


/*
 * NAME:        AppLayerChanswitchSinrServer.
 * PURPOSE:     Models the behaviour of ChanswitchSinr Server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchSinrServer(Node *node, Message *msg)
{
    char buf[MAX_STRING_LENGTH];
    AppDataChanswitchSinrServer *serverPtr;

    ctoa(getSimTime(node), buf);

    switch (msg->eventType)
    {
        case MSG_APP_FromTransListenResult:
        {
            TransportToAppListenResult *listenResult;

            listenResult = (TransportToAppListenResult *)
                           MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("CHANSWITCH_SINR Server: Node %ld at %s got listenResult\n",
                   node->nodeId, buf);
#endif /* DEBUG */

            if (listenResult->connectionId == -1)
            {
                node->appData.numAppTcpFailure++;
            }

            break;
        }

        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult *openResult;
            openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("CHANSWITCH_SINR Server: Node %ld at %s got OpenResult\n",
                   node->nodeId, buf);
#endif /* DEBUG */

            assert(openResult->type == TCP_CONN_PASSIVE_OPEN);

            if (openResult->connectionId < 0)
            {
                node->appData.numAppTcpFailure++;
            }
            else
            {
                AppDataChanswitchSinrServer *serverPtr;
                serverPtr = AppChanswitchSinrServerNewChanswitchSinrServer(node, openResult);
                assert(serverPtr != NULL);
            }

            break;
        }

        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *dataSent;

            dataSent = (TransportToAppDataSent *) MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("CHANSWITCH_SINR Server Node %ld at %s sent data %ld\n",
                   node->nodeId, buf, dataSent->length);
#endif /* DEBUG */

            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                     dataSent->connectionId);

            assert(serverPtr != NULL);

            serverPtr->numBytesSent += (clocktype) dataSent->length;
            serverPtr->lastItemSent = getSimTime(node);

            break;
        }

        case MSG_APP_FromTransDataReceived:
        {
            TransportToAppDataReceived *dataRecvd;
            char *packet;

            dataRecvd = (TransportToAppDataReceived *)
                            MESSAGE_ReturnInfo(msg);

            packet = MESSAGE_ReturnPacket(msg);

#ifdef DEBUG
            printf("CHANSWITCH_SINR Server: Node %ld at %s received data size %d\n",
                   node->nodeId, buf, MESSAGE_ReturnPacketSize(msg));
#endif /* DEBUG */

            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                                 dataRecvd->connectionId);

            //assert(serverPtr != NULL);
            if(serverPtr == NULL)
            {
                // server pointer can be NULL due to the real-world apps
                break;
                    
            }
            assert(serverPtr->sessionIsClosed == FALSE);


            // printf("server is in state %d meow \n", serverPtr->state);
            switch(serverPtr->state){
                case RX_S_IDLE:{
                    if(packet[0] == TX_SCAN_PKT){
                        serverPtr->state = RX_SCANNING;
                        AppChanswitchSinrServerScanChannels(node, serverPtr);
                    }
                break;
                }
                case RX_CHANGE_WFACK:{
                    if(packet[0] == TX_CHANGE_ACK){
                        #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH Server: Node %ld at %s got TX_CHANGE_ACK while in RX_CHANGE_WFACK state.\n",
                            node->nodeId, buf);
                       #endif 
                       //change to the new channel 
                        AppChanswitchChangeChannels(node, 
                                serverPtr->connectionId, 
                                CHANSWITCH_SINR_RX_SERVER, 
                                serverPtr->currentChannel,
                                serverPtr->nextChannel);
                        serverPtr->currentChannel = serverPtr->nextChannel;
                        printf("Switch to new channel %d completed on TX and RX nodes. \n",serverPtr->currentChannel);
                        serverPtr->state = RX_S_IDLE;

                    }
                    break;
                }
                case RX_VERIFY_WFACK:{
                    if(packet[0] == TX_VERIFY_ACK){
                        #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH Server: Node %ld at %s got TX_VERIFY_ACK while in RX_VERIFY_WFACK state.\n",
                            node->nodeId, buf);
                       #endif 
                        printf("Switch to new channel %d completed on TX and RX nodes. \n",serverPtr->currentChannel);
                        serverPtr->state = RX_S_IDLE;

                    }
                    break;
                }

            }
            break;
        }

        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult *closeResult;

            closeResult = (TransportToAppCloseResult *)
                          MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("CHANSWITCH_SINR Server: Node %ld at %s got close result\n",
                   node->nodeId, buf);
#endif /* DEBUG */

            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                                 closeResult->connectionId);
            assert(serverPtr != NULL);

            if (serverPtr->sessionIsClosed == FALSE)
            {
                serverPtr->sessionIsClosed = TRUE;
                serverPtr->sessionFinish = getSimTime(node);
            }

            break;
        }

        case MSG_APP_FromMacRxScanFinished: {
            MacToAppSinrScanComplete* scanComplete;
            scanComplete = (MacToAppSinrScanComplete*) MESSAGE_ReturnInfo(msg);
            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                                 scanComplete->connectionId);

            switch(serverPtr->state){
                case RX_SCANNING:{
                    printf("MSG_APP_FromMacRxScanFinished: TX RSS = %f \n", scanComplete->txRss);
                    serverPtr->currentChannel = scanComplete->currentChannel;
                    serverPtr->avg_intnoise_dB = scanComplete->avg_intnoise_dB;
                    serverPtr->worst_intnoise_dB = scanComplete->worst_intnoise_dB;
                    serverPtr->txRss = scanComplete->txRss;
                    serverPtr->channelSwitch = scanComplete->channelSwitch;
                    serverPtr->state = RX_CHANGE_WFACK;
                    AppChanswitchSinrServerEvaluateChannels(node, serverPtr);
                    AppChanswitchSinrServerSendChangePkt(node, serverPtr);

                    //start change ACK timeout timer
                    Message *timeout;

                    timeout = MESSAGE_Alloc(node, 
                        APP_LAYER,
                        APP_CHANSWITCH_SINR_SERVER,
                        MSG_APP_RxChangeWfAckTimeout);

                    AppChanswitchTimeout* info = (AppChanswitchTimeout*)
                    MESSAGE_InfoAlloc(
                            node,
                            timeout,
                            sizeof(AppChanswitchTimeout));
                    ERROR_Assert(info, "cannot allocate enough space for needed info");
                    info->connectionId = serverPtr->connectionId;
                    MESSAGE_Send(node, timeout, RX_CHANGE_WFACK_TIMEOUT);
                
                    serverPtr->state = RX_CHANGE_WFACK;


                    break;  
                }
                default: {
                    ERROR_ReportWarning("SINR scan finished with RX node in invalid state. \n");
                }

            }

            break;
        }
        case MSG_APP_RxChangeWfAckTimeout: {
            #ifdef DEBUG_CHANSWITCH_SINR
            printf("CHANSWITCH_SINR Server: got change ACK timeout \n");
            #endif
            AppChanswitchTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                            timeoutInfo->connectionId);
            //switch to new channel, wait for VERIFY_ACK
            if(serverPtr->state == RX_CHANGE_WFACK){
                //change channels, start timer, wait for verify pkt
                serverPtr->state = RX_VERIFY_WFACK;
                AppChanswitchChangeChannels(
                        node, 
                        serverPtr->connectionId, 
                        CHANSWITCH_SINR_RX_SERVER, 
                        serverPtr->currentChannel,
                        serverPtr->nextChannel);

                Message *timeout;
                timeout = MESSAGE_Alloc(node, 
                                        APP_LAYER,
                                        CHANSWITCH_SINR_RX_SERVER,
                                        MSG_APP_RxVerifyWfAckTimeout);
                AppChanswitchTimeout* info = (AppChanswitchTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        timeout,
                        sizeof(AppChanswitchTimeout));
                ERROR_Assert(info, "cannot allocate enough space for needed info");
                info->connectionId = serverPtr->connectionId;
                MESSAGE_Send(node, timeout, RX_VERIFY_WFACK_TIMEOUT);
                
            }
            break;
        }

        case MSG_APP_RxVerifyWfAckTimeout:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Server node %u got verify ACK timeout\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            AppChanswitchTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                            timeoutInfo->connectionId);
            //return to the previous channel and wait 
            if(serverPtr->state = RX_VERIFY_WFACK){
                serverPtr->state = RX_CHANGE_WFACK;
                AppChanswitchChangeChannels(
                        node, 
                        serverPtr->connectionId, 
                        CHANSWITCH_SINR_RX_SERVER, 
                        serverPtr->nextChannel,
                        serverPtr->currentChannel);

                Message *timeout;
                timeout = MESSAGE_Alloc(node, 
                                        APP_LAYER,
                                        APP_CHANSWITCH_SINR_SERVER,
                                        MSG_APP_RxChangeWfAckTimeout);
                AppChanswitchTimeout* info = (AppChanswitchTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        timeout,
                        sizeof(AppChanswitchTimeout));
                ERROR_Assert(info, "cannot allocate enough space for needed info");
                info->connectionId = serverPtr->connectionId;
                MESSAGE_Send(node, timeout, RX_CHANGE_WFACK_TIMEOUT);

            }
            break;

        }

        default:
            ctoa(getSimTime(node), buf);
            printf("CHANSWITCH_SINR Server: Node %u at time %s received "
                   "message of unknown type"
                   " %d.\n", node->nodeId, buf, msg->eventType);
            assert(FALSE);
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        AppChanswitchSinrServerInit.
 * PURPOSE:     listen on ChanswitchSinr server port.
 * PARAMETERS:  node - pointer to the node.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerInit(Node *node, Address serverAddr)
{
    APP_TcpServerListen(
        node,
        APP_CHANSWITCH_SINR_SERVER,
        serverAddr,
        (short) APP_CHANSWITCH_SINR_SERVER);
}

/*
 * NAME:        AppChanswitchSinrServerPrintStats.
 * PURPOSE:     Prints statistics of a ChanswitchSinr session.
 * PARAMETERS:  node - pointer to the node.
 *              serverPtr - pointer to the chanswitch_sinr server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerPrintStats(Node *node, AppDataChanswitchSinrServer *serverPtr)
{
    clocktype throughput;
    char addrStr[MAX_STRING_LENGTH];
    char startStr[MAX_STRING_LENGTH];
    char finishStr[MAX_STRING_LENGTH];
    char statusStr[MAX_STRING_LENGTH];
    char numBytesSentStr[MAX_STRING_LENGTH];
    char numBytesRecvdStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char throughputStr[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(serverPtr->sessionStart, startStr);
    TIME_PrintClockInSecond(serverPtr->lastItemSent, finishStr);

    if (serverPtr->connectionId < 0)
    {
        sprintf(statusStr, "Failed");
    }
    else if (serverPtr->sessionIsClosed == FALSE)
    {
        serverPtr->sessionFinish = getSimTime(node);
        sprintf(statusStr, "Not closed");
    }
    else
    {
        sprintf(statusStr, "Closed");
    }

    if (serverPtr->sessionFinish <= serverPtr->sessionStart)
    {
        throughput = 0;
    }
    else
    {
        throughput =
            (clocktype) ((serverPtr->numBytesRecvd * 8.0 * SECOND)
            / (serverPtr->sessionFinish - serverPtr->sessionStart));
    }

    ctoa(throughput, throughputStr);

    IO_ConvertIpAddressToString(&serverPtr->remoteAddr, addrStr);
    sprintf(buf, "Client Address = %s", addrStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "First Packet Sent at (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Last Packet Sent at (s) = %s", finishStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Session Status = %s", statusStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    ctoa(serverPtr->numBytesSent, numBytesSentStr);
    sprintf(buf, "Total Bytes Sent = %s", numBytesSentStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    ctoa(serverPtr->numBytesRecvd, numBytesRecvdStr);
    sprintf(buf, "Total Bytes Received = %s", numBytesRecvdStr);

    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH_SINR Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);
}

/*
 * NAME:        AppChanswitchSinrServerFinalize.
 * PURPOSE:     Collect statistics of a ChanswitchSinr session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerFinalize(Node *node, AppInfo* appInfo)
{
    AppDataChanswitchSinrServer *serverPtr = (AppDataChanswitchSinrServer*)appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        AppChanswitchSinrServerPrintStats(node, serverPtr);
    }
}

