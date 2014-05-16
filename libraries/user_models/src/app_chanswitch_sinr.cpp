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

#ifdef DEBUG_CHANSWITCH_SINR
            printf("CHANSWITCH_SINR Client: Node %d comparing uniqueId "
                   "%d with %d\n",
                   node->nodeId,
                   tmpChanswitchSinrClient->uniqueId,
                   openResult->uniqueId);
#endif /* DEBUG_CHANSWITCH_SINR */

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
    chanswitch_sinrClient->state = TX_IDLE;
    chanswitch_sinrClient->got_RX_nodelist = FALSE;
    chanswitch_sinrClient->signalStrengthAtRx = 0.0;
    chanswitch_sinrClient->numChannels = 1;
    chanswitch_sinrClient->currentChannel = 0;
    chanswitch_sinrClient->nextChannel = 0;

#ifdef DEBUG_CHANSWITCH_SINR
    char addrStr[MAX_STRING_LENGTH];
    printf("CHANSWITCH_SINR Client: Node %d updating chanswitch_sinr client structure\n",
            node->nodeId);
    printf("    connectionId = %d\n", chanswitch_sinrClient->connectionId);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->localAddr, addrStr);
    printf("    localAddr = %s\n", addrStr);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->remoteAddr, addrStr);
    printf("    remoteAddr = %s\n", addrStr);
    // printf("    itemsToSend = %d\n", chanswitch_sinrClient->itemsToSend);
#endif /* DEBUG_CHANSWITCH_SINR */

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
                         double hnThreshold,
                         double csThreshold,
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
    chanswitch_sinrClient->state = TX_IDLE;
    chanswitch_sinrClient->got_RX_nodelist = FALSE;
    chanswitch_sinrClient->signalStrengthAtRx = 0.0;
    chanswitch_sinrClient->numChannels = 1;
    chanswitch_sinrClient->currentChannel = 0;
    chanswitch_sinrClient->nextChannel = 0;
    chanswitch_sinrClient->initBackoff = FALSE;
    chanswitch_sinrClient->initial = FALSE;
    chanswitch_sinrClient->hnThreshold = hnThreshold;
    chanswitch_sinrClient->csThreshold = csThreshold;
    chanswitch_sinrClient->changeBackoffTime = changeBackoffTime;



#ifdef DEBUG_CHANSWITCH_SINR
    char addrStr[MAX_STRING_LENGTH];
    printf("CHANSWITCH_SINR Client: Node %d creating new chanswitch_sinr "
           "client structure\n", node->nodeId);
    printf("    connectionId = %d\n", chanswitch_sinrClient->connectionId);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->localAddr, addrStr);
    printf("    localAddr = %s\n", addrStr);
    IO_ConvertIpAddressToString(&chanswitch_sinrClient->remoteAddr, addrStr);
    printf("    remoteAddr = %s\n", addrStr);
    printf("    hnThreshold = %f\n",hnThreshold);
    printf("    csThreshold = %f\n",csThreshold);

    // printf("    itemsToSend = %d\n", chanswitch_sinrClient->itemsToSend);
#endif /* DEBUG_CHANSWITCH_SINR */

    #ifdef DEBUG_CHANSWITCH_SINR_OUTPUT_FILE
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
    chanswitch_sinrServer->lastItemSent = 0;
    chanswitch_sinrServer->state = RX_IDLE;
    chanswitch_sinrServer->numChannels = 1;
    chanswitch_sinrServer->currentChannel = 0;
    chanswitch_sinrServer->nextChannel = 0;

    RANDOM_SetSeed(chanswitch_sinrServer->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_CHANSWITCH_SINR_SERVER,
                   chanswitch_sinrServer->connectionId);

    APP_RegisterNewApp(node, APP_CHANSWITCH_SINR_SERVER, chanswitch_sinrServer);

    #ifdef DEBUG_CHANSWITCH_SINR_OUTPUT_FILE
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
 * NAME:        AppChanswitchSinrClientSendProbeInit.
 * PURPOSE:     Send the "probe init" packet.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 *              initial - is this the first channel switch?
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendProbeInit(Node *node, AppDataChanswitchSinrClient *clientPtr){

    int options = 0;
    if(clientPtr->initial == FALSE){
        options = 1; //not the first chanswitch_sinr
    }

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_SINR_PROBE_PKT_SIZE); //4
    memset(payload,PROBE_PKT,1);
    memset(payload+1,0xfe,2); //dummy data for chanswitch_sinr mask
    memset(payload+3,options,1); //options byte only specifies initial chanswitch_sinr

    if (!clientPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                clientPtr->connectionId,
                payload,
                CHANSWITCH_SINR_PROBE_PKT_SIZE,
                TRACE_APP_CHANSWITCH_SINR);

    }
     MEM_free(payload);

}

/*
 * NAME:        AppChanswitchSinrClientSendChangeInit.
 * PURPOSE:     Send the "change init" packet.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrClientSendChangeInit(Node *node, AppDataChanswitchSinrClient *clientPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_SINR_CHANGE_PKT_SIZE); //2
    memset(payload,CHANGE_PKT,1);
    memcpy(payload+1,&(clientPtr->nextChannel),1); //first byte of int

    if (!clientPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                clientPtr->connectionId,
                payload,
                CHANSWITCH_SINR_CHANGE_PKT_SIZE,
                TRACE_APP_CHANSWITCH_SINR);

    }
     MEM_free(payload);

}


/*
 * NAME:            
 * PURPOSE:     Start scanning channels - either client or server.
 * PARAMETERS:  node - pointer to the node
 * connectionId - identifier of the client/server connection
 * RETURN:      none.
 */
void
AppChanswitchSinrStartProbing(Node *node, int connectionId, int appType){
    Message *macMsg;
    macMsg = MESSAGE_Alloc(node, 
    MAC_LAYER,
    MAC_PROTOCOL_DOT11,
    MSG_MAC_FromAppChanswitchSinrRequest);

    AppToMacStartProbe* info = (AppToMacStartProbe*)
        MESSAGE_InfoAlloc(
            node,
            macMsg,
            sizeof(AppToMacStartProbe));
    ERROR_Assert(info, "cannot allocate enough space for needed info");
    info->connectionId = connectionId;
    info->appType = appType;

    MESSAGE_Send(node, macMsg, 0);
}

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
AppChanswitchSinrChangeChannels(Node *node, int connectionId, int appType, int oldChannel, int newChannel){
    Message *macMsg;
    macMsg = MESSAGE_Alloc(node, 
    MAC_LAYER,
    MAC_PROTOCOL_DOT11,
    MSG_MAC_FromAppChangeChannelRequest);

    AppToMacChannelChange* info = (AppToMacChannelChange*)
        MESSAGE_InfoAlloc(
            node,
            macMsg,
            sizeof(AppToMacChannelChange));
    ERROR_Assert(info, "cannot allocate enough space for needed info");
    info->connectionId = connectionId;
    info->appType = appType;
    info->oldChannel = oldChannel;
    info->newChannel = newChannel;

    MESSAGE_Send(node, macMsg, 0);

}

/*
 * NAME:        AppChanswitchSinrGetMyMacAddr.
 * PURPOSE:     Ask MAC for my MAC address and other MAC-layer info.
 *              After this is finished, ProbeInit will be called.
 * PARAMETERS:  node - pointer to the node,
 *              connectionId - identifier of the client/server connection
 *              appType - is this app TX or RX
 * RETURN:      none.
 */
void
AppChanswitchSinrGetMyMacAddr(Node *node, int connectionId, int appType, BOOL initial){
    Message *macMsg;
    macMsg = MESSAGE_Alloc(node, 
    MAC_LAYER,
    MAC_PROTOCOL_DOT11,
    MSG_MAC_FromAppMACAddressRequest);

    AppToMacAddrRequest* info = (AppToMacAddrRequest*)
        MESSAGE_InfoAlloc(
            node,
            macMsg,
            sizeof(AppToMacAddrRequest));
    ERROR_Assert(info, "cannot allocate enough space for needed info");
    info->connectionId = connectionId;
    info->appType = appType;
    info->initial = initial;

    MESSAGE_Send(node, macMsg, 0);
}



/*
 * NAME:        AppChanswitchSinrServerSendProbeAck.
 * PURPOSE:     Send the ack indicating server got probe request
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendProbeAck(Node *node, AppDataChanswitchSinrServer *serverPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_SINR_ACK_SIZE); //1
    memset(payload,PROBE_ACK,1);
    
    if (!serverPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                serverPtr->connectionId,
                payload,
                CHANSWITCH_SINR_ACK_SIZE,
                TRACE_APP_CHANSWITCH_SINR);
    }
     MEM_free(payload);
}

/*
 * NAME:        AppChanswitchSinrServerSendChangeAck.
 * PURPOSE:     Send the ack indicating server got change request
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendChangeAck(Node *node, AppDataChanswitchSinrServer *serverPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_SINR_ACK_SIZE); //1
    memset(payload,CHANGE_ACK,1);
    
    if (!serverPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                serverPtr->connectionId,
                payload,
                CHANSWITCH_SINR_ACK_SIZE,
                TRACE_APP_CHANSWITCH_SINR);
    }
     MEM_free(payload);
}

/*
 * NAME:        AppChanswitchSinrServerSendVerifyAck.
 * PURPOSE:     Send the ack indicating server changed to the new channel
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETURN:      none.
 */
void
AppChanswitchSinrServerSendVerifyAck(Node *node, AppDataChanswitchSinrServer *serverPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_SINR_ACK_SIZE); //1
    memset(payload,VERIFY_ACK,1);
    
    if (!serverPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                serverPtr->connectionId,
                payload,
                CHANSWITCH_SINR_ACK_SIZE,
                TRACE_APP_CHANSWITCH_SINR);
    }
     MEM_free(payload);
}


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
                                        int nodeCount)
{
    char *payload;
    //11 + 24 * nodeCount
    int length = CHANSWITCH_SINR_LIST_HEADER_SIZE + CHANSWITCH_SINR_LIST_ENTRY_SIZE * nodeCount; 
    payload = (char *)MEM_malloc(length);
    memset(payload, NODE_LIST, 1);
    // ERROR_Assert(serverPtr->myAddr != NULL, "RX needs to know its MAC address to send the NodeList pkt. \n");
    memcpy(payload+1, &(serverPtr->myAddr), 6); //rx MAC address
    memset(payload+7, (nodeCount % 256),1); //low byte of node count
    memset(payload+8, (nodeCount / 256),1); //high byte of node count
    int count = 0;

    while(nodeInfo != NULL){
    // printf("(APP RX) channel %d, signal strength %f dBm, isAP %d, bss %d \n",
    //     nodeInfo->channelId, nodeInfo->signalStrength, nodeInfo->isAP, nodeInfo->bssAddr);
        char *offset = payload + CHANSWITCH_SINR_LIST_HEADER_SIZE + count * CHANSWITCH_SINR_LIST_ENTRY_SIZE;
        memcpy(offset, &(nodeInfo->channelId), 1);
        memcpy(offset+1, &(nodeInfo->bssAddr),6);
        memcpy(offset+7, &(nodeInfo->signalStrength),8);
        memcpy(offset+15, &(nodeInfo->isAP),1);
        nodeInfo = nodeInfo->next;
        count++;

    }
    printf("\n \n");

    if (!serverPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                serverPtr->connectionId,
                payload,
                length,
                TRACE_APP_CHANSWITCH_SINR);
    }
     MEM_free(payload);

}

/*
 * NAME:        AppChanswitchSinrClientParseRXNodeList.
 * PURPOSE:     Parse the node list packet from RX.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client
 *              packet - raw packet from RX
 * RETURN:      none.
 */
void 
AppChanswitchSinrClientParseRxNodeList(Node *node, AppDataChanswitchSinrClient *clientPtr, char *packet){
    int nodeCount = 0;

    memcpy(&(clientPtr->rxAddr), &packet[1], 6);
    memcpy(&nodeCount, &packet[7], 2);
    printf("Node count from RX: %d \n", nodeCount);

    DOT11_VisibleNodeInfo* nodeList = clientPtr->rxNodeList;
    int channelId;
    Mac802Address bssAddr;
    double signalStrength;
    int isAP = 0;
    int i = 0;

    while(i < nodeCount){
        char *offset = packet + CHANSWITCH_SINR_LIST_HEADER_SIZE + i * CHANSWITCH_SINR_LIST_ENTRY_SIZE;
        memcpy(&channelId,offset,1);
        memcpy(&bssAddr,offset+1,6);
        memcpy(&signalStrength,offset+7,8);
        memcpy(&isAP,offset+15,1);
        //if this is my (tx) node, save my signal strength
        if(bssAddr == clientPtr->myAddr){
            clientPtr->signalStrengthAtRx = signalStrength;
            printf("AppChanswitchSinrClientParseRxNodeList at node %d: TX signal strength is %f dBm at RX \n",
                node->nodeId, signalStrength);
        }
        //otherwise add to the list
        else{
            DOT11_VisibleNodeInfo* nodeInfo = (DOT11_VisibleNodeInfo*) MEM_malloc(sizeof(DOT11_VisibleNodeInfo));
            ERROR_Assert(nodeInfo != NULL, "MAC 802.11: Out of memory!");
            memset((char*) nodeInfo, 0, sizeof(DOT11_VisibleNodeInfo)); 
            nodeInfo->channelId = channelId;
            nodeInfo->bssAddr = bssAddr;
            nodeInfo->signalStrength = signalStrength;
            nodeInfo->isAP = isAP;
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("AppChanswitchSinrClientParseRxNodeList at node %d: channel %d, signal strength %f dBm, isAP %d, bss %d \n",
                    node->nodeId,nodeInfo->channelId, nodeInfo->signalStrength, nodeInfo->isAP, nodeInfo->bssAddr);
            #endif
            // add visible node in the list
            nodeInfo->next = clientPtr->rxNodeList;
            clientPtr->rxNodeList = nodeInfo;
        }
        i++;
    }

}

/*
 * NAME:        AppChanswitchSinrClientEvaulateChannels
 * PURPOSE:     Evaulate the node list and select the next channel.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the client
 *              
 * RETURN:      the channel to switch to.
 */
int
AppChanswitchSinrClientEvaluateChannels(Node *node,AppDataChanswitchSinrClient *clientPtr){

    DOT11_VisibleNodeInfo* rxNode = clientPtr->rxNodeList;
    DOT11_VisibleNodeInfo* txNode = clientPtr->txNodeList;

    int i = 0;
    printf("List of available channels: ");
    while(i<clientPtr->numChannels){
        if(clientPtr->channelSwitch[i]){
            printf("%d ", i);
        }
        i++;

    }
    printf("\n");

    //constant NUM_CHANNELS since C++ cannot dynamically allocate arrays :/
    int hiddenNodeCount[NUM_CHANNELS] = { 0 }; //HN count per channel
    int csNodeCount[NUM_CHANNELS] = { 0 }; //CS count per channel
    BOOL isHN;
    double sinr = 0.0;

    //look for HN
    while(rxNode != NULL){
        isHN = TRUE;
        //hidden if RX sees it and TX doesn't, and the signal strength isn't strong enough for TX to negotiate
        while(txNode != NULL){
            if((rxNode->bssAddr == txNode->bssAddr) && (txNode->signalStrength >= clientPtr->csThreshold)){
            // if(rxNode->bssAddr == txNode->bssAddr){ 
                isHN = FALSE;
            }
            txNode = txNode->next;
        }

        //verify signal strength
        sinr = NON_DB(clientPtr->signalStrengthAtRx) / (NON_DB(rxNode->signalStrength) + clientPtr->noise_mW) ; 

        if(isHN && (sinr > clientPtr->hnThreshold)){ //20 dB default
            #ifdef DEBUG_CHANSWITCH_SINR
            printf("Weak hidden node %02x:%02x:%02x:%02x:%02x:%02x on channel %d (sinr at RX is %f dBm when transmitting) \n",
                    rxNode->bssAddr.byte[5], 
                    rxNode->bssAddr.byte[4], 
                    rxNode->bssAddr.byte[3],
                    rxNode->bssAddr.byte[2],
                    rxNode->bssAddr.byte[1],
                    rxNode->bssAddr.byte[0], 
                    rxNode->channelId,
                    sinr);
            #endif
            isHN = FALSE;
        }

        if(isHN){
            printf("Hidden node %02x:%02x:%02x:%02x:%02x:%02x on channel %d (sinr at RX = %f dBm when transmitting)  \n",
                    rxNode->bssAddr.byte[5], 
                    rxNode->bssAddr.byte[4], 
                    rxNode->bssAddr.byte[3],
                    rxNode->bssAddr.byte[2],
                    rxNode->bssAddr.byte[1],
                    rxNode->bssAddr.byte[0],
                    rxNode->channelId,
                    sinr);
            hiddenNodeCount[rxNode->channelId]++;

        }
                txNode = clientPtr->txNodeList;
                rxNode = rxNode->next;
    }

    txNode = clientPtr->txNodeList;
    rxNode = clientPtr->rxNodeList;
    BOOL isCS;

    //count carrier sensing nodes at TX
    while(txNode != NULL){
        isCS = TRUE;

        if(txNode->bssAddr == clientPtr->rxAddr){
            #ifdef DEBUG_CHANSWITCH_SINR
            printf("RX node %02x:%02x:%02x:%02x:%02x:%02x on channel %d (signal strength %f)\n",
                    txNode->bssAddr.byte[5], 
                    txNode->bssAddr.byte[4], 
                    txNode->bssAddr.byte[3],
                    txNode->bssAddr.byte[2],
                    txNode->bssAddr.byte[1],
                    txNode->bssAddr.byte[0], 
                    txNode->channelId,
                    txNode->signalStrength);
            #endif
            isCS = FALSE;
        }
        else if(txNode->signalStrength < clientPtr->csThreshold){ //-69.0 dBm default
            #ifdef DEBUG_CHANSWITCH_SINR
            printf("Weak CS node %02x:%02x:%02x:%02x:%02x:%02x on channel %d (signal strength %f)\n",
                    txNode->bssAddr.byte[5], 
                    txNode->bssAddr.byte[4], 
                    txNode->bssAddr.byte[3],
                    txNode->bssAddr.byte[2],
                    txNode->bssAddr.byte[1],
                    txNode->bssAddr.byte[0], 
                    txNode->channelId,
                    txNode->signalStrength);
            #endif
            isCS = FALSE;
        }
        if(isCS){
            printf("Carrier sensing node %02x:%02x:%02x:%02x:%02x:%02x on channel %d (signal strength %f) \n",
                    txNode->bssAddr.byte[5], 
                    txNode->bssAddr.byte[4], 
                    txNode->bssAddr.byte[3],
                    txNode->bssAddr.byte[2],
                    txNode->bssAddr.byte[1],
                    txNode->bssAddr.byte[0], 
                    txNode->channelId,
                    txNode->signalStrength);
            csNodeCount[txNode->channelId]++;
        }
        txNode = txNode->next;
    }

    i = 0;
    printf("Channel stats: \n");
    while(i<NUM_CHANNELS){
        if(clientPtr->channelSwitch[i]){
            printf("Channel %d: %d HN, %d CS nodes \n", i, hiddenNodeCount[i], csNodeCount[i]);
        }
        i++;
    }


    //set the next channel and previous channel
    BOOL tied = FALSE;
    //first check for hidden nodes
    i=0;
    int bestChannel = -1;
    int lowest = -1;
    while(i<NUM_CHANNELS){
        if(clientPtr->channelSwitch[i]){
            if(bestChannel < 0){
                bestChannel = i;
                lowest = hiddenNodeCount[i];
            }
            else if (hiddenNodeCount[i] < lowest){
                tied = FALSE;
                bestChannel = i;
                lowest = hiddenNodeCount[i];
            }
            else if (hiddenNodeCount[i] == lowest){
                tied = TRUE;
            }
        }
        i++;
    }

    ERROR_Assert((lowest > -1 && bestChannel > -1), "error counting HN on channel \n");

    if(!tied){
        printf("The best channel is channel %d with %d hidden nodes. \n", bestChannel, lowest);
        
    }

    //multiple channels were tied for # of HN = check for CS nodes
    else{
        printf("Multiple channels had %d hidden nodes. Now checking carrier sense nodes. \n", lowest);
        BOOL tied = FALSE;
        bestChannel = -1;
        lowest = -1;
        i=0;

        while(i<NUM_CHANNELS){
            if(clientPtr->channelSwitch[i]){
                if(bestChannel < 0){
                    bestChannel = i;
                    lowest = csNodeCount[i];
                }
                else if (csNodeCount[i] < lowest){
                    tied = FALSE;
                    bestChannel = i;
                    lowest = csNodeCount[i];
                }
                else if (csNodeCount[i] == lowest){
                    tied = TRUE;
                }
            }
            i++;
        }
        ERROR_Assert((lowest > -1 && bestChannel > -1), "error counting CS on channel \n");

        if(!tied){
            printf("The best channel is channel %d with %d carrier sense nodes. \n", bestChannel, lowest);
            }
        else{
            printf("Multiple channels were tied for fewest hidden nodes and fewest carrier sense nodes. Stay on the same channel. \n");
            //in QualNet background noise is the same on each channel, so stay on the same channel instead of tiebreaking
            bestChannel = clientPtr->currentChannel;
        }
        
    }
    //clear both node lists
    ClearVisibleNodeList(clientPtr->rxNodeList);
    clientPtr->rxNodeList = NULL;
    // ClearVisibleNodeList(clientPtr->txNodeList);
    clientPtr->txNodeList = NULL;

  
return bestChannel;
}


/*
 * NAME:        AppChanswitchSinrClientChangeInit.
 *              Perform required functions upon reaching TX_CHANGE_INIT state.
 *              (Evaluate channels, determine if channel change is needed and start ACK timeout.)
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void AppChanswitchSinrClientChangeInit(Node *node,AppDataChanswitchSinrClient *clientPtr){
    clientPtr->nextChannel = AppChanswitchSinrClientEvaluateChannels(node,clientPtr);          
    if(clientPtr->nextChannel == clientPtr->currentChannel){
        printf("best channel is the same as the current channel, do nothing \n");
        clientPtr->state = TX_IDLE;
    }
    else{
        printf("best channel is channel %d, sending change pkt to RX \n", clientPtr->nextChannel);
        AppChanswitchSinrClientSendChangeInit(node, clientPtr);
        //start change ACK timeout timer
        Message *timeout;

        timeout = MESSAGE_Alloc(node, 
            APP_LAYER,
            APP_CHANSWITCH_SINR_CLIENT,
            MSG_APP_TxChangeWfAckTimeout);

        AppChanswitchSinrTimeout* info = (AppChanswitchSinrTimeout*)
        MESSAGE_InfoAlloc(
                node,
                timeout,
                sizeof(AppChanswitchSinrTimeout));
        ERROR_Assert(info, "cannot allocate enough space for needed info");
        info->connectionId = clientPtr->connectionId;
        MESSAGE_Send(node, timeout, TX_CHANGE_WFACK_TIMEOUT);
    }
    clientPtr->state = TX_CHANGE_WFACK;

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

            #ifdef DEBUG_CHANSWITCH_SINR
            printf("%s: CHANSWITCH_SINR Client node %u got OpenResult\n", buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */

            assert(openResult->type == TCP_CONN_ACTIVE_OPEN);

            if (openResult->connectionId < 0)
            {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Client node %u connection failed!\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */

                node->appData.numAppTcpFailure ++;
            }
            else
            {
                AppDataChanswitchSinrClient *clientPtr;

                clientPtr = AppChanswitchSinrClientUpdateChanswitchSinrClient(node, openResult);

                assert(clientPtr != NULL);

                clientPtr->state = TX_PROBE_INIT;
                //get MAC addr from MAC layer (probe will immediately start thereafter unless disabled)
                AppChanswitchSinrGetMyMacAddr(node,openResult->connectionId, CHANSWITCH_SINR_TX_CLIENT, TRUE);

                //start channel reselection backoff timer
                clientPtr->initBackoff = TRUE;
                Message *initTimeout;
                initTimeout = MESSAGE_Alloc(node, 
                    APP_LAYER,
                    APP_CHANSWITCH_SINR_CLIENT,
                    MSG_APP_TxChannelSelectionTimeout);

                AppChanswitchSinrTimeout* initInfo = (AppChanswitchSinrTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        initTimeout,
                        sizeof(AppChanswitchSinrTimeout));
                ERROR_Assert(initInfo, "cannot allocate enough space for needed info");
                initInfo->connectionId = clientPtr->connectionId;

                MESSAGE_Send(node, initTimeout, clientPtr->changeBackoffTime);

            }

            break;
        }

        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *dataSent;
            char *packet;

            dataSent = (TransportToAppDataSent *) MESSAGE_ReturnInfo(msg);

            #ifdef DEBUG_CHANSWITCH_SINR
            printf("%s: CHANSWITCH_SINR Client node %u sent data %d\n",
                   buf, node->nodeId, dataSent->length);
            #endif /* DEBUG_CHANSWITCH_SINR */

            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                                 dataSent->connectionId);

            assert(clientPtr != NULL);

            clientPtr->numBytesSent += (clocktype) dataSent->length;
            clientPtr->lastItemSent = getSimTime(node);


            /* Instant throughput is measured here after each 1 second */
            #ifdef DEBUG_CHANSWITCH_SINR_OUTPUT_FILE
            {
                char fileName[MAX_STRING_LENGTH];
                FILE *fp;
                clocktype currentTime;

                clientPtr->bytesRecvdDuringThePeriod += dataSent->length;

                sprintf(fileName, "CHANSWITCH_SINR_Throughput_%d", node->nodeId);

                fp = fopen(fileName, "a");
                if (fp)
                {
                    currentTime = getSimTime(node);

                    if ((int)((currentTime -
                        clientPtr->lastTime)/SECOND) >= 1)
                    {
                        /* get throughput within this time window */
                        int throughput = (int)
                            ((clientPtr->bytesRecvdDuringThePeriod
                                * 8.0 * SECOND)
                            / (getSimTime(node) - clientPtr->lastTime));

                        fprintf(fp, "%d\t\t%d\n", (int)(currentTime/SECOND),
                                (throughput/1000));

                        clientPtr->lastTime = currentTime;
                        clientPtr->bytesRecvdDuringThePeriod = 0;
                    }
                    fclose(fp);
                }
            }
            #endif

            //close the session if requested by the previous pkt
            if (clientPtr->sessionIsClosed == TRUE)
            {
                APP_TcpCloseConnection(
                    node,
                    clientPtr->connectionId);
            }

            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            TransportToAppDataReceived *dataRecvd;
            char *packet;

            dataRecvd = (TransportToAppDataReceived *)
                         MESSAGE_ReturnInfo(msg);

            packet = MESSAGE_ReturnPacket(msg);

            #ifdef DEBUG_CHANSWITCH_SINR
            printf("%s: CHANSWITCH_SINR Client node %u received data %d\n",
                   buf, node->nodeId, msg->packetSize);
            #endif /* DEBUG_CHANSWITCH_SINR */

            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                                 dataRecvd->connectionId);

            assert(clientPtr != NULL);

            clientPtr->numBytesRecvd += (clocktype) msg->packetSize;

            switch(clientPtr->state){
                case TX_IDLE: {
                if(packet[0] == PROBE_ACK){
                       #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got PROBE_ACK while in TX_IDLE state.\n",
                            node->nodeId, buf);
                       #endif 
                    }
                
                else if(packet[0] == CHANGE_ACK){
                       #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got CHANGE_ACK while in TX_IDLE state.\n",
                            node->nodeId, buf);
                       #endif 
                    }
                
                else if(packet[0] == VERIFY_ACK){
                       #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got VERIFY_ACK while in TX_IDLE state.\n",
                            node->nodeId, buf);
                       #endif                   
                    }
                }
                case TX_PROBE_WFACK:{
                    if(packet[0] == PROBE_ACK){
                       #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got PROBE_ACK\n",
                            node->nodeId, buf);
                       #endif 
                        clientPtr->state = TX_PROBING;
                        clientPtr->got_RX_nodelist = FALSE;
                        //start the probe
                        AppChanswitchSinrStartProbing(node, dataRecvd->connectionId,
                            CHANSWITCH_SINR_TX_CLIENT);
                    }
                    break;
                }
                case TX_PROBING: { 
                    if(packet[0] == NODE_LIST){ //got node list before TX finished its probe
                        #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got NODE_LIST while in TX_PROBING state.\n",
                                node->nodeId, buf);
                        #endif
                        clientPtr->got_RX_nodelist = TRUE;
                        AppChanswitchSinrClientParseRxNodeList(node,clientPtr,packet);
                    }
                    if(packet[0] == PROBE_ACK){
                        #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got PROBE_ACK while in TX_PROBING state.\n",
                                node->nodeId, buf);
                        #endif
                    }
                    break;
                }
                case TX_PROBE_WFRX: {
                    if(packet[0] == PROBE_ACK){
                        #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got PROBE_ACK while in TX_PROBE_WFRX state.\n",
                            node->nodeId, buf);
                        #endif
                    }
                    if(packet[0] == NODE_LIST){ //got node list after TX finished probing
                        #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got NODE_LIST while in TX_PROBE_WFRX state.\n",
                            node->nodeId, buf);
                        #endif
                        clientPtr->got_RX_nodelist = TRUE;
                        //save RX's node list
                        AppChanswitchSinrClientParseRxNodeList(node,clientPtr,packet);
                        clientPtr->state = TX_CHANGE_INIT;
                        //evaluate channels and start ACK timeout
                        AppChanswitchSinrClientChangeInit(node,clientPtr);
                    }
                    break;
                }
                case TX_CHANGE_WFACK:{
                    if(packet[0] == CHANGE_ACK){
                       #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got CHANGE_ACK while in TX_CHANGE_WFACK state.\n",
                            node->nodeId, buf);
                       #endif 
                        AppChanswitchSinrChangeChannels(node, 
                                                    clientPtr->connectionId, 
                                                    CHANSWITCH_SINR_TX_CLIENT, 
                                                    clientPtr->currentChannel,
                                                    clientPtr->nextChannel);
                        clientPtr->currentChannel = clientPtr->nextChannel;
                        printf("Switch to new channel %d completed on TX and RX nodes. \n",clientPtr->currentChannel);
                        clientPtr->state = TX_IDLE;
                    }
                    break;
                }
                case TX_VERIFY_WFACK:{
                    if(packet[0] == VERIFY_ACK){
                        #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Client: Node %ld at %s got VERIFY_ACK in while in TX_VERIFY_WFACK state.\n",
                            node->nodeId, buf);
                        #endif
                        clientPtr->currentChannel = clientPtr->nextChannel;
                        clientPtr->state = TX_IDLE;
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

            #ifdef DEBUG_CHANSWITCH_SINR
            printf("%s: CHANSWITCH_SINR Client node %u got close result\n",
                   buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */

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
        case MSG_APP_TxProbeWfAckTimeout:
        {

            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Client node %u got probe ACK timeout (enabled)\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */

            AppChanswitchSinrTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchSinrTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                            timeoutInfo->connectionId);

            if(clientPtr->state == TX_PROBE_WFACK){
                clientPtr->state = TX_PROBING;
                //start the probe
                clientPtr->got_RX_nodelist = FALSE;
                AppChanswitchSinrStartProbing(node, timeoutInfo->connectionId,
                    CHANSWITCH_SINR_TX_CLIENT);
            }
            break;  
        }

        case MSG_APP_TxChangeWfAckTimeout:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Client node %u got change ACK timeout\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */     
            AppChanswitchSinrTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchSinrTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                            timeoutInfo->connectionId);
            if(clientPtr->state == TX_CHANGE_WFACK){
                //change channels, start timer, wait for verify pkt
                clientPtr->state = TX_VERIFY_WFACK;
                AppChanswitchSinrChangeChannels(
                        node, 
                        clientPtr->connectionId, 
                        CHANSWITCH_SINR_TX_CLIENT, 
                        clientPtr->currentChannel,
                        clientPtr->nextChannel);

                Message *timeout;
                timeout = MESSAGE_Alloc(node, 
                                        APP_LAYER,
                                        APP_CHANSWITCH_SINR_CLIENT,
                                        MSG_APP_TxVerifyWfAckTimeout);
                AppChanswitchSinrTimeout* info = (AppChanswitchSinrTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        timeout,
                        sizeof(AppChanswitchSinrTimeout));
                ERROR_Assert(info, "cannot allocate enough space for needed info");
                info->connectionId = clientPtr->connectionId;
                MESSAGE_Send(node, timeout, TX_VERIFY_WFACK_TIMEOUT);
                
            }
            break;
        }

        case MSG_APP_TxVerifyWfAckTimeout:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Client node %u got verify ACK timeout\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            AppChanswitchSinrTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchSinrTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                            timeoutInfo->connectionId);
            //TODO: return to the previous channel and wait 
            if(clientPtr->state = TX_VERIFY_WFACK){
                clientPtr->state = TX_CHANGE_WFACK;
                AppChanswitchSinrChangeChannels(
                        node, 
                        clientPtr->connectionId, 
                        CHANSWITCH_SINR_TX_CLIENT, 
                        clientPtr->nextChannel,
                        clientPtr->currentChannel);

                Message *timeout;
                timeout = MESSAGE_Alloc(node, 
                                        APP_LAYER,
                                        APP_CHANSWITCH_SINR_CLIENT,
                                        MSG_APP_TxChangeWfAckTimeout);
                AppChanswitchSinrTimeout* info = (AppChanswitchSinrTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        timeout,
                        sizeof(AppChanswitchSinrTimeout));
                ERROR_Assert(info, "cannot allocate enough space for needed info");
                info->connectionId = clientPtr->connectionId;
                MESSAGE_Send(node, timeout, TX_CHANGE_WFACK_TIMEOUT);

            }
            break;

        }

        //Prevents multiple channel reselections from being initiated simultaneously
        case MSG_APP_TxChannelSelectionTimeout:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Client node %u got channel selection timeout\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            AppChanswitchSinrTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchSinrTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                            timeoutInfo->connectionId);
            clientPtr->initBackoff = FALSE;
            break;
        }

        case MSG_APP_FromMacTxScanFinished:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Client node %u channel scan finished\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            MacToAppScanComplete* scanComplete;
            scanComplete = (MacToAppScanComplete*) MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                            scanComplete->connectionId);

            clientPtr->txNodeList = scanComplete->nodeInfo;
            if(clientPtr->state == TX_PROBING){

                if(!clientPtr->got_RX_nodelist){ //did not get nodelist yet
                 //return to original channel and wait
                AppChanswitchSinrChangeChannels(
                        node, 
                        clientPtr->connectionId, 
                        CHANSWITCH_SINR_TX_CLIENT, 
                        clientPtr->currentChannel,
                        clientPtr->currentChannel);
                    clientPtr->state = TX_PROBE_WFRX;

                }
                else { //got nodelist from RX already
                    clientPtr->state = TX_CHANGE_INIT;
                    //evaluate channels and start ACK timeout
                    AppChanswitchSinrClientChangeInit(node,clientPtr);
                }
            }

            else {
                ERROR_ReportWarning("TX scan at node finished in unknown state. \n");
            }

            break;
        }

        case MSG_APP_FromMac_MACAddressRequest:
        {
            MacToAppAddrRequest* addrRequest;
            addrRequest = (MacToAppAddrRequest*) MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node, addrRequest->connectionId);
            clientPtr->myAddr = addrRequest->myAddr; //save my address
            clientPtr->currentChannel = addrRequest->currentChannel;
            clientPtr->numChannels = addrRequest->numChannels;
            clientPtr->noise_mW = addrRequest->noise_mW;
            clientPtr->channelSwitch = addrRequest->channelSwitch;

            #ifdef DEBUG_CHANSWITCH_SINR
                printf("TX mac address: %02x:%02x:%02x:%02x:%02x:%02x, on channel %d of %d channels \n", 
                    clientPtr->myAddr.byte[5], 
                    clientPtr->myAddr.byte[4], 
                    clientPtr->myAddr.byte[3],
                    clientPtr->myAddr.byte[2],
                    clientPtr->myAddr.byte[1],
                    clientPtr->myAddr.byte[0],
                    clientPtr->currentChannel,
                    clientPtr->numChannels);
            #endif

            // //start probe ACK timeout timer (don't do first scan if disabled)
            if(!(addrRequest->initial) || (addrRequest->initial && addrRequest->asdcsInit))
            {
                //start probe init
                if(addrRequest->initial == TRUE){
                    clientPtr->initial = TRUE;
                    AppChanswitchSinrClientSendProbeInit(node, clientPtr);
                }
                else{
                    clientPtr->initial = FALSE;
                    AppChanswitchSinrClientSendProbeInit(node, clientPtr);
                }
                
                Message *timeout;

                timeout = MESSAGE_Alloc(node, 
                    APP_LAYER,
                    APP_CHANSWITCH_SINR_CLIENT,
                    MSG_APP_TxProbeWfAckTimeout);

                AppChanswitchSinrTimeout* info = (AppChanswitchSinrTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        timeout,
                        sizeof(AppChanswitchSinrTimeout));
                ERROR_Assert(info, "cannot allocate enough space for needed info");
                info->connectionId = clientPtr->connectionId;

                MESSAGE_Send(node, timeout, TX_PROBE_WFACK_TIMEOUT);
                clientPtr->state = TX_PROBE_WFACK;
             }
            else {
                clientPtr->state = TX_IDLE;
            }

            break;
        }

        case MSG_APP_InitiateChannelScanRequest: {
            // #ifdef DEBUG_CHANSWITCH_SINR
            //     printf("%s: CHANSWITCH_SINR Client node %u received a request to initiate a channel scan \n",
            //            buf, node->nodeId);
            // #endif /* DEBUG_CHANSWITCH_SINR */
            AppInitScanRequest* initRequest = (AppInitScanRequest*) MESSAGE_ReturnInfo(msg);
            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node, initRequest->connectionId);

            //start the scan if timer isn't expired
            if(clientPtr->state == TX_IDLE && clientPtr->initBackoff == FALSE){
                printf("Attempting mid-stream channel switch on node %u \n", node->nodeId);
                //start backoff timer to prevent multiple requests
                clientPtr->state = TX_PROBE_INIT;
                clientPtr->initBackoff = TRUE;
                Message *initTimeout;
                initTimeout = MESSAGE_Alloc(node, 
                    APP_LAYER,
                    APP_CHANSWITCH_SINR_CLIENT,
                    MSG_APP_TxChannelSelectionTimeout);

                AppChanswitchSinrTimeout* initInfo = (AppChanswitchSinrTimeout*)
                MESSAGE_InfoAlloc(
                        node,
                        initTimeout,
                        sizeof(AppChanswitchSinrTimeout));
                ERROR_Assert(initInfo, "cannot allocate enough space for needed info");
                initInfo->connectionId = clientPtr->connectionId;
                MESSAGE_Send(node, initTimeout, clientPtr->changeBackoffTime);

                //Get my mac address from MAC layer (probe will start after)
                AppChanswitchSinrGetMyMacAddr(node,clientPtr->connectionId, CHANSWITCH_SINR_TX_CLIENT, FALSE);
            }
            break;
        }

        default: {
            ctoa(getSimTime(node), buf);
            printf("Time %s: CHANSWITCH_SINR Client node %u received message of unknown"
                   " type %d.\n", buf, node->nodeId, msg->eventType);
            assert(FALSE);
        }  
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        ClearVisibleNodeList
 * PURPOSE:     Clear (deallocate) this node list in preparation for the next scan.
 * PARAMETERS:  nodelist - pointer to the visible node list
 * RETURN:      none.
 */
void
ClearVisibleNodeList(DOT11_VisibleNodeInfo* nodeList){
    DOT11_VisibleNodeInfo *current = nodeList;
    DOT11_VisibleNodeInfo *next;
    while(current != NULL){
        next = current->next;
        free (current);
        current = next;
    }
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
    double hnThreshold,
    double csThreshold,
    clocktype changeBackoffTime)
{
    AppDataChanswitchSinrClient *clientPtr;

    clientPtr = AppChanswitchSinrClientNewChanswitchSinrClient(node,
                                         clientAddr,
                                         serverAddr,
                                         hnThreshold,
                                         csThreshold,
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

#ifdef DEBUG_CHANSWITCH_SINR
            printf("CHANSWITCH_SINR Server: Node %ld at %s got listenResult\n",
                   node->nodeId, buf);
#endif /* DEBUG_CHANSWITCH_SINR */

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

#ifdef DEBUG_CHANSWITCH_SINR
            printf("CHANSWITCH_SINR Server: Node %ld at %s got OpenResult\n",
                   node->nodeId, buf);
#endif /* DEBUG_CHANSWITCH_SINR */

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

#ifdef DEBUG_CHANSWITCH_SINR
            printf("CHANSWITCH_SINR Server Node %ld at %s sent data %ld\n",
                   node->nodeId, buf, dataSent->length);
#endif /* DEBUG_CHANSWITCH_SINR */

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

#ifdef DEBUG_CHANSWITCH_SINR
            printf("CHANSWITCH_SINR Server: Node %ld at %s received data size %d\n",
                   node->nodeId, buf, MESSAGE_ReturnPacketSize(msg));
#endif /* DEBUG_CHANSWITCH_SINR */

            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                                 dataRecvd->connectionId);

            //assert(serverPtr != NULL);
            if(serverPtr == NULL)
            {
				// server pointer can be NULL due to the real-world apps
                break;
					
            }
            assert(serverPtr->sessionIsClosed == FALSE);

            serverPtr->numBytesRecvd +=
                    (clocktype) MESSAGE_ReturnPacketSize(msg);

            // printf("server is in state %d meow \n", serverPtr->state);
            switch(serverPtr->state){
                case RX_IDLE:
                {
                    if(packet[0] == PROBE_PKT){
                       #ifdef DEBUG_CHANSWITCH_SINR
                        printf("CHANSWITCH_SINR Server: Node %ld at %s got PROBE_PKT\n",
                            node->nodeId, buf);
                       #endif 
                        serverPtr->state = RX_PROBE_ACK;
                        //send PROBE_ACK to TX
                        AppChanswitchSinrServerSendProbeAck(node, serverPtr);
                        //start the delay timer
                        Message *probeMsg;
                        probeMsg = MESSAGE_Alloc(node, 
                            APP_LAYER,
                            APP_CHANSWITCH_SINR_SERVER,
                            MSG_APP_RxProbeAckDelay);
                        AppChanswitchSinrTimeout* info = (AppChanswitchSinrTimeout*)
                            MESSAGE_InfoAlloc(
                                node,
                                probeMsg,
                                sizeof(AppChanswitchSinrTimeout));
                        ERROR_Assert(info, "cannot allocate enough space for needed info");
                        info->connectionId = dataRecvd->connectionId;

                        MESSAGE_Send(node, probeMsg, RX_PROBE_ACK_DELAY);
                        }

                        else if (packet[0] == CHANGE_PKT){
                        #ifdef DEBUG_CHANSWITCH_SINR
                            printf("CHANSWITCH_SINR Server: Node %ld at %s got CHANGE_PKT\n",
                                node->nodeId, buf);
                        #endif
                        int nextChannel = 0;
                        memcpy(&nextChannel,packet+1,1);
                        serverPtr->nextChannel = nextChannel;
                        serverPtr->state = RX_CHANGE_ACK;
                        //send CHANGE_ACK to TX
                        AppChanswitchSinrServerSendChangeAck(node, serverPtr);
                        //start the delay timer
                        Message *changeMsg;
                        changeMsg = MESSAGE_Alloc(node, 
                            APP_LAYER,
                            APP_CHANSWITCH_SINR_SERVER,
                            MSG_APP_RxChangeAckDelay);
                        AppChanswitchSinrTimeout* info = (AppChanswitchSinrTimeout*)
                            MESSAGE_InfoAlloc(
                                node,
                                changeMsg,
                                sizeof(AppChanswitchSinrTimeout));
                        ERROR_Assert(info, "cannot allocate enough space for needed info");
                        info->connectionId = dataRecvd->connectionId;

                        MESSAGE_Send(node, changeMsg, RX_CHANGE_ACK_DELAY);

                        }
                        else{
                            ERROR_Assert(FALSE, "CHANSWITCH_SINR Server: Received unknown pkt type \n");
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

#ifdef DEBUG_CHANSWITCH_SINR
            printf("CHANSWITCH_SINR Server: Node %ld at %s got close result\n",
                   node->nodeId, buf);
#endif /* DEBUG_CHANSWITCH_SINR */

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

        case MSG_APP_RxProbeAckDelay:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Server node %u ACK delay timer expired\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            AppChanswitchSinrTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchSinrTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                            timeoutInfo->connectionId);
            AppChanswitchSinrGetMyMacAddr(node,serverPtr->connectionId,CHANSWITCH_SINR_RX_SERVER, FALSE); //bool doesn't matter
            break;
        }

        case MSG_APP_RxChangeAckDelay:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Server node %u ACK change timer expired\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            AppChanswitchSinrTimeout* timeoutInfo;
            timeoutInfo = (AppChanswitchSinrTimeout*) 
                            MESSAGE_ReturnInfo(msg);
            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                            timeoutInfo->connectionId);
            //change to the new channel
            AppChanswitchSinrChangeChannels(node, 
                                        serverPtr->connectionId, 
                                        CHANSWITCH_SINR_RX_SERVER, 
                                        serverPtr->currentChannel,
                                        serverPtr->nextChannel);
            //send verify ACK
            serverPtr->state = RX_VERIFY_ACK;
            AppChanswitchSinrServerSendVerifyAck(node,serverPtr);
            serverPtr->state = RX_IDLE;
            break;
        }

        case MSG_APP_FromMacRxScanFinished:
        {
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("%s: CHANSWITCH_SINR Server node %u channel scan finished\n",
                       buf, node->nodeId);
            #endif /* DEBUG_CHANSWITCH_SINR */
            //send the packet to TX
            MacToAppScanComplete* scanComplete;
            scanComplete = (MacToAppScanComplete*) MESSAGE_ReturnInfo(msg);
            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node,
                                            scanComplete->connectionId);
            //return to original channel
            AppChanswitchSinrChangeChannels(node, 
                            serverPtr->connectionId, 
                            CHANSWITCH_SINR_RX_SERVER, 
                            serverPtr->currentChannel,
                            serverPtr->currentChannel);
            AppChanswitchSinrServerSendVisibleNodeList(node, 
                                                    serverPtr, 
                                                    scanComplete->nodeInfo, 
                                                    scanComplete->nodeCount);
            serverPtr->state = RX_IDLE;
            break;
        }

        case MSG_APP_FromMac_MACAddressRequest:
        {
            MacToAppAddrRequest* addrRequest;
            addrRequest = (MacToAppAddrRequest*) MESSAGE_ReturnInfo(msg);
            serverPtr = AppChanswitchSinrServerGetChanswitchSinrServer(node, addrRequest->connectionId);
            serverPtr->myAddr = addrRequest->myAddr; //save my address
            serverPtr->currentChannel = addrRequest->currentChannel;
            serverPtr->numChannels = addrRequest->numChannels;
            #ifdef DEBUG_CHANSWITCH_SINR
                printf("RX mac address: %02x:%02x:%02x:%02x:%02x:%02x on channel %d of %d channels \n", 
                    serverPtr->myAddr.byte[5], 
                    serverPtr->myAddr.byte[4], 
                    serverPtr->myAddr.byte[3],
                    serverPtr->myAddr.byte[2],
                    serverPtr->myAddr.byte[1],
                    serverPtr->myAddr.byte[0],
                    serverPtr->currentChannel,
                    serverPtr->numChannels);
            #endif
            AppChanswitchSinrStartProbing(node, addrRequest->connectionId,
                                        CHANSWITCH_SINR_RX_SERVER);
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

