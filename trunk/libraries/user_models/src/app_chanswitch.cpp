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
 * function, and finalize function used by the chanswitch application.
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
#include "app_chanswitch.h"

 #define DEBUG 1

/*
 * Static Functions
 */

/*
 * NAME:        AppChanswitchClientGetChanswitchClient.
 * PURPOSE:     search for a chanswitch client data structure.
 * PARAMETERS:  node - pointer to the node.
 *              connId - connection ID of the chanswitch client.
 * RETURN:      the pointer to the chanswitch client data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchClient *
AppChanswitchClientGetChanswitchClient(Node *node, int connId)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataChanswitchClient *chanswitchClient;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_CHANSWITCH_CLIENT)
        {
            chanswitchClient = (AppDataChanswitchClient *) appList->appDetail;

            if (chanswitchClient->connectionId == connId)
            {
                return chanswitchClient;
            }
        }
    }

    return NULL;
}

/*
 * NAME:        AppChanswitchClientUpdateChanswitchClient.
 * PURPOSE:     update existing chanswitch client data structure by including
 *              connection id.
 * PARAMETERS:  node - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchClient *
AppChanswitchClientUpdateChanswitchClient(Node *node,
                            TransportToAppOpenResult *openResult)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataChanswitchClient *tmpChanswitchClient = NULL;
    AppDataChanswitchClient *chanswitchClient = NULL;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_CHANSWITCH_CLIENT)
        {
            tmpChanswitchClient = (AppDataChanswitchClient *) appList->appDetail;

#ifdef DEBUG
            printf("CHANSWITCH Client: Node %d comparing uniqueId "
                   "%d with %d\n",
                   node->nodeId,
                   tmpChanswitchClient->uniqueId,
                   openResult->uniqueId);
#endif /* DEBUG */

            if (tmpChanswitchClient->uniqueId == openResult->uniqueId)
            {
                chanswitchClient = tmpChanswitchClient;
                break;
            }
        }
    }

    if (chanswitchClient == NULL)
    {
        assert(FALSE);
    }

    /*
     * fill in connection id, etc.
     */
    chanswitchClient->connectionId = openResult->connectionId;
    chanswitchClient->localAddr = openResult->localAddr;
    chanswitchClient->remoteAddr = openResult->remoteAddr;
    chanswitchClient->sessionStart = getSimTime(node);
    chanswitchClient->sessionFinish = getSimTime(node);
    chanswitchClient->lastTime = 0;
    chanswitchClient->sessionIsClosed = FALSE;
    chanswitchClient->bytesRecvdDuringThePeriod = 0;
    chanswitchClient->state = TX_IDLE;
#ifdef DEBUG
    char addrStr[MAX_STRING_LENGTH];
    printf("CHANSWITCH Client: Node %d updating chanswitch client structure\n",
            node->nodeId);
    printf("    connectionId = %d\n", chanswitchClient->connectionId);
    IO_ConvertIpAddressToString(&chanswitchClient->localAddr, addrStr);
    printf("    localAddr = %s\n", addrStr);
    IO_ConvertIpAddressToString(&chanswitchClient->remoteAddr, addrStr);
    printf("    remoteAddr = %s\n", addrStr);
    // printf("    itemsToSend = %d\n", chanswitchClient->itemsToSend);
#endif /* DEBUG */

    return chanswitchClient;
}

/*
 * NAME:        AppChanswitchClientNewChanswitchClient.
 * PURPOSE:     create a new chanswitch client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node.
 *              clientAddr - address of this client.
 *              serverAddr - chanswitch server to this client.
 *              itemsToSend - number of chanswitch items to send in simulation.
 * RETRUN:      the pointer to the created chanswitch client data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchClient *
AppChanswitchClientNewChanswitchClient(Node *node,
                         Address clientAddr,
                         Address serverAddr)
{
    AppDataChanswitchClient *chanswitchClient;

    chanswitchClient = (AppDataChanswitchClient *)
                MEM_malloc(sizeof(AppDataChanswitchClient));

    memset(chanswitchClient, 0, sizeof(AppDataChanswitchClient));

    /*
     * fill in connection id, etc.
     */
    chanswitchClient->connectionId = -1;
    chanswitchClient->uniqueId = node->appData.uniqueId++;

    chanswitchClient->localAddr = clientAddr;
    chanswitchClient->remoteAddr = serverAddr;

    // Make unique seed.
    RANDOM_SetSeed(chanswitchClient->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_CHANSWITCH_CLIENT,
                   chanswitchClient->uniqueId);


    chanswitchClient->itemsToSend = 100;
    chanswitchClient->itemSizeLeft = 0;
    chanswitchClient->numBytesSent = 0;
    chanswitchClient->numBytesRecvd = 0;
    chanswitchClient->lastItemSent = 0;
    chanswitchClient->state = TX_IDLE;

#ifdef DEBUG
    char addrStr[MAX_STRING_LENGTH];
    printf("CHANSWITCH Client: Node %d creating new chanswitch "
           "client structure\n", node->nodeId);
    printf("    connectionId = %d\n", chanswitchClient->connectionId);
    IO_ConvertIpAddressToString(&chanswitchClient->localAddr, addrStr);
    printf("    localAddr = %s\n", addrStr);
    IO_ConvertIpAddressToString(&chanswitchClient->remoteAddr, addrStr);
    printf("    remoteAddr = %s\n", addrStr);
    // printf("    itemsToSend = %d\n", chanswitchClient->itemsToSend);
#endif /* DEBUG */

    #ifdef DEBUG_OUTPUT_FILE
    {
        char fileName[MAX_STRING_LENGTH];
        FILE *fp;
        char dataBuf[MAX_STRING_LENGTH];

        sprintf(fileName, "CHANSWITCH_Throughput_%d", node->nodeId);

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

    APP_RegisterNewApp(node, APP_CHANSWITCH_CLIENT, chanswitchClient);


    return chanswitchClient;
}



/*
 * NAME:        AppChanswitchClientItemsToSend.
 * PURPOSE:     call tcplib function ftp_nitems() to get the
 *              number of items to send in an chanswitch session.
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      amount of items to send.
 */
int
AppChanswitchClientItemsToSend(AppDataChanswitchClient *clientPtr)
{
    int items;

    items = ftp_nitems(clientPtr->seed);

#ifdef DEBUG
    printf("CHANSWITCH nitems = %d\n", items);
#endif /* DEBUG */

    return items;
}

/*
 * NAME:        AppChanswitchClientItemSize.
 * PURPOSE:     call tcplib function ftp_itemsize() to get the size
 *              of each item.
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      size of an item.
 */
int
AppChanswitchClientItemSize(AppDataChanswitchClient *clientPtr)
{
    int size;

    size = ftp_itemsize(clientPtr->seed);

#ifdef DEBUG
    printf("CHANSWITCH item size = %d\n", size);
#endif /* DEBUG */

    return size;
}

/*
 * NAME:        AppChanswitchClientCtrlPktSize.
 * PURPOSE:     call tcplib function ftp_ctlsize().
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      chanswitch control packet size.
 */
int
AppChanswitchClientCtrlPktSize(AppDataChanswitchClient *clientPtr)
{
    int ctrlPktSize;
    ctrlPktSize = ftp_ctlsize(clientPtr->seed);

#ifdef DEBUG
    printf("CHANSWITCH: Node %d ftp control pktsize = %d\n",
           ctrlPktSize);
#endif /* DEBUG */

    return (ctrlPktSize);
}

/*
 * NAME:        AppChanswitchServerGetChanswitchServer.
 * PURPOSE:     search for a chanswitch server data structure.
 * PARAMETERS:  appList - link list of applications,
 *              connId - connection ID of the chanswitch server.
 * RETURN:      the pointer to the chanswitch server data structure,
 *              NULL if nothing found.
 */
AppDataChanswitchServer *
AppChanswitchServerGetChanswitchServer(Node *node, int connId)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataChanswitchServer *chanswitchServer;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_CHANSWITCH_SERVER)
        {
            chanswitchServer = (AppDataChanswitchServer *) appList->appDetail;

            if (chanswitchServer->connectionId == connId)
            {
                return chanswitchServer;
            }
        }
    }

    return NULL;
}


/*
 * NAME:        AppChanswitchServerNewChanswitchServer.
 * PURPOSE:     create a new chanswitch server data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node.
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created chanswitch server data structure,
 *              NULL if no data structure allocated.
 */
AppDataChanswitchServer *
AppChanswitchServerNewChanswitchServer(Node *node,
                         TransportToAppOpenResult *openResult)
{
    AppDataChanswitchServer *chanswitchServer;

    chanswitchServer = (AppDataChanswitchServer *)
                MEM_malloc(sizeof(AppDataChanswitchServer));

    /*
     * fill in connection id, etc.
     */
    chanswitchServer->connectionId = openResult->connectionId;
    chanswitchServer->localAddr = openResult->localAddr;
    chanswitchServer->remoteAddr = openResult->remoteAddr;
    chanswitchServer->sessionStart = getSimTime(node);
    chanswitchServer->sessionFinish = getSimTime(node);
    chanswitchServer->lastTime = 0;
    chanswitchServer->sessionIsClosed = FALSE;
    chanswitchServer->numBytesSent = 0;
    chanswitchServer->numBytesRecvd = 0;
    chanswitchServer->bytesRecvdDuringThePeriod = 0;
    chanswitchServer->lastItemSent = 0;
    chanswitchServer->state = RX_IDLE;

    RANDOM_SetSeed(chanswitchServer->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_CHANSWITCH_SERVER,
                   chanswitchServer->connectionId);

    APP_RegisterNewApp(node, APP_CHANSWITCH_SERVER, chanswitchServer);

    #ifdef DEBUG_OUTPUT_FILE
    {
        char fileName[MAX_STRING_LENGTH];
        FILE *fp;
        char dataBuf[MAX_STRING_LENGTH];

        sprintf(fileName, "CHANSWITCH_Throughput_%d", node->nodeId);

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

    return chanswitchServer;
}


/*
 * NAME:        AppChanswitchClientSendProbeInit.
 * PURPOSE:     Send the "probe init" packet.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 *              initial - is this the first channel switch?
 * RETURN:      none.
 */
void
AppChanswitchClientSendProbeInit(Node *node, AppDataChanswitchClient *clientPtr, BOOL initial){

    int options = 0;
    if(!initial){
        options = 1; //not the first chanswitch
    }

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_PROBE_PKT_SIZE); //4
    memset(payload,PROBE_PKT,1);
    memset(payload+1,0xfe,2); //dummy data for chanswitch mask
    memset(payload+3,options,1); //options byte only specifies initial chanswitch
    if (!clientPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                clientPtr->connectionId,
                payload,
                CHANSWITCH_PROBE_PKT_SIZE,
                TRACE_APP_CHANSWITCH);

    }
     MEM_free(payload);

}

/*
 * NAME:        AppChanswitchStartProbing.
 * PURPOSE:     Start scanning channels - either client or server.
 * PARAMETERS:  node - pointer to the node
 * RETURN:      none.
 */
void
AppChanswitchStartProbing(Node *node){
    Message *macMsg;
    macMsg = MESSAGE_Alloc(node, 
        MAC_LAYER,
        MAC_PROTOCOL_DOT11,
        MSG_MAC_FromAppChanswitchRequest);
    MESSAGE_Send(node, macMsg, 0);
}

/*
 * NAME:        AppChanswitchServerSendProbeAck.
 * PURPOSE:     Send the ack indicating server got probe request
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 *              initial - is this the first channel switch?
 * RETURN:      none.
 */
void
AppChanswitchServerSendProbeAck(Node *node, AppDataChanswitchServer *serverPtr){

    char *payload;

    payload = (char *)MEM_malloc(CHANSWITCH_ACK_SIZE); //1
    memset(payload,PROBE_ACK,1);
    
    if (!serverPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                serverPtr->connectionId,
                payload,
                CHANSWITCH_ACK_SIZE,
                TRACE_APP_CHANSWITCH);
    }
     MEM_free(payload);
}


/*
 * Public Functions
 */

/*
 * NAME:        AppLayerChanswitchClient.
 * PURPOSE:     Models the behaviour of Chanswitch Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchClient(Node *node, Message *msg)
{
    char buf[MAX_STRING_LENGTH];
    AppDataChanswitchClient *clientPtr;

    ctoa(getSimTime(node), buf);

    switch (msg->eventType)
    {
        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult *openResult;

            openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: CHANSWITCH Client node %u got OpenResult\n", buf, node->nodeId);
#endif /* DEBUG */

            assert(openResult->type == TCP_CONN_ACTIVE_OPEN);

            if (openResult->connectionId < 0)
            {
#ifdef DEBUG
                printf("%s: CHANSWITCH Client node %u connection failed!\n",
                       buf, node->nodeId);
#endif /* DEBUG */

                node->appData.numAppTcpFailure ++;
            }
            else
            {
                AppDataChanswitchClient *clientPtr;

                clientPtr = AppChanswitchClientUpdateChanswitchClient(node, openResult);

                assert(clientPtr != NULL);

               //Send the 'start scanning' pkt to server.
                clientPtr->state = TX_PROBE_INIT;
                AppChanswitchClientSendProbeInit(node, clientPtr, TRUE);
                clientPtr->state = TX_PROBE_WFACK;

            }

            break;
        }
        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *dataSent;
            char *packet;

            dataSent = (TransportToAppDataSent *) MESSAGE_ReturnInfo(msg);


#ifdef DEBUG
            printf("%s: CHANSWITCH Client node %u sent data %d\n",
                   buf, node->nodeId, dataSent->length);
#endif /* DEBUG */

            clientPtr = AppChanswitchClientGetChanswitchClient(node,
                                                 dataSent->connectionId);

            assert(clientPtr != NULL);

            clientPtr->numBytesSent += (clocktype) dataSent->length;
            clientPtr->lastItemSent = getSimTime(node);


            /* Instant throughput is measured here after each 1 second */
            #ifdef DEBUG_OUTPUT_FILE
            {
                char fileName[MAX_STRING_LENGTH];
                FILE *fp;
                clocktype currentTime;

                clientPtr->bytesRecvdDuringThePeriod += dataSent->length;

                sprintf(fileName, "CHANSWITCH_Throughput_%d", node->nodeId);

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

#ifdef DEBUG
            printf("%s: CHANSWITCH Client node %u received data %d\n",
                   buf, node->nodeId, msg->packetSize);
#endif /* DEBUG */

            clientPtr = AppChanswitchClientGetChanswitchClient(node,
                                                 dataRecvd->connectionId);

            assert(clientPtr != NULL);

            clientPtr->numBytesRecvd += (clocktype) msg->packetSize;

            switch(clientPtr->state){
                case TX_PROBE_WFACK:{
                    if(packet[0] == PROBE_ACK){
                       #ifdef DEBUG
                        printf("CHANSWITCH Client: Node %ld at %s got PROBE_ACK\n",
                            node->nodeId, buf);
                       #endif 
                        clientPtr->state = TX_PROBING;
                        //start the probe
                        AppChanswitchStartProbing(node);
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
            printf("%s: CHANSWITCH Client node %u got close result\n",
                   buf, node->nodeId);
#endif /* DEBUG */

            clientPtr = AppChanswitchClientGetChanswitchClient(node,
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
        #ifdef DEBUG
            printf("%s: CHANSWITCH Client node %u got probe ACK timeout\n",
                   buf, node->nodeId);
        #endif /* DEBUG */
        break;
    }

        case MSG_APP_TxChangeWfAckTimeout:
    {
        #ifdef DEBUG
            printf("%s: CHANSWITCH Client node %u got change ACK timeout\n",
                   buf, node->nodeId);
        #endif /* DEBUG */
        break;
    }

    case MSG_APP_TxVerifyWfAckTimeout:
    {
        #ifdef DEBUG
            printf("%s: CHANSWITCH Client node %u got verify ACK timeout\n",
                   buf, node->nodeId);
        #endif /* DEBUG */
        break;
    }

        default:
            ctoa(getSimTime(node), buf);
            printf("Time %s: CHANSWITCH Client node %u received message of unknown"
                   " type %d.\n", buf, node->nodeId, msg->eventType);
            assert(FALSE);
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        AppChanswitchClientInit.
 * PURPOSE:     Initialize a Chanswitch session.
 * PARAMETERS:  node - pointer to the node,
 *              serverAddr - address of the server,
 *              itemsToSend - number of items to send,
 *              waitTime - time until the session starts.
 * RETURN:      none.
 */
void
AppChanswitchClientInit(
    Node *node,
    Address clientAddr,
    Address serverAddr,
    clocktype waitTime)
{
    AppDataChanswitchClient *clientPtr;

    /* Check to make sure the number of chanswitch items is a correct value. */

    // if (itemsToSend < 0)
    // {
    //     printf("CHANSWITCH Client: Node %d items to send needs to be >= 0\n",
    //            node->nodeId);

    //     exit(0);
    // }

    clientPtr = AppChanswitchClientNewChanswitchClient(node,
                                         clientAddr,
                                         serverAddr);

    if (clientPtr == NULL)
    {
        printf("CHANSWITCH Client: Node %d cannot allocate "
               "new chanswitch client\n", node->nodeId);

        assert(FALSE);
    }

    APP_TcpOpenConnectionWithPriority(
        node,
        APP_CHANSWITCH_CLIENT,
        clientAddr,
        node->appData.nextPortNum++,
        serverAddr,
        (short) APP_CHANSWITCH_SERVER,
        clientPtr->uniqueId,
        waitTime,
        APP_DEFAULT_TOS);
}

/*
 * NAME:        AppChanswitchClientPrintStats.
 * PURPOSE:     Prints statistics of a Chanswitch session.
 * PARAMETERS:  node - pointer to the node.
 *              clientPtr - pointer to the chanswitch client data structure.
 * RETURN:      none.
 */
void
AppChanswitchClientPrintStats(Node *node, AppDataChanswitchClient *clientPtr)
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
        "CHANSWITCH Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "First Packet Sent at (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Last Packet Sent at (s) = %s", finishStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Session Status = %s", statusStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    ctoa(clientPtr->numBytesSent, numBytesSentStr);
    sprintf(buf, "Total Bytes Sent = %s", numBytesSentStr);

    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    ctoa(clientPtr->numBytesRecvd, numBytesRecvdStr);
    sprintf(buf, "Total Bytes Received = %s", numBytesRecvdStr);

    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);
}

/*
 * NAME:        AppChanswitchClientFinalize.
 * PURPOSE:     Collect statistics of a Chanswitch session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchClientFinalize(Node *node, AppInfo* appInfo)
{
    AppDataChanswitchClient *clientPtr = (AppDataChanswitchClient*)appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        AppChanswitchClientPrintStats(node, clientPtr);
    }
}

/*
 * NAME:        AppLayerChanswitchServer.
 * PURPOSE:     Models the behaviour of Chanswitch Server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerChanswitchServer(Node *node, Message *msg)
{
    char buf[MAX_STRING_LENGTH];
    AppDataChanswitchServer *serverPtr;

    ctoa(getSimTime(node), buf);

    switch (msg->eventType)
    {
        case MSG_APP_FromTransListenResult:
        {
            TransportToAppListenResult *listenResult;

            listenResult = (TransportToAppListenResult *)
                           MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("CHANSWITCH Server: Node %ld at %s got listenResult\n",
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
            printf("CHANSWITCH Server: Node %ld at %s got OpenResult\n",
                   node->nodeId, buf);
#endif /* DEBUG */

            assert(openResult->type == TCP_CONN_PASSIVE_OPEN);

            if (openResult->connectionId < 0)
            {
                node->appData.numAppTcpFailure++;
            }
            else
            {
                AppDataChanswitchServer *serverPtr;
                serverPtr = AppChanswitchServerNewChanswitchServer(node, openResult);
                assert(serverPtr != NULL);


            }

            break;
        }

        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *dataSent;

            dataSent = (TransportToAppDataSent *) MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("CHANSWITCH Server Node %ld at %s sent data %ld\n",
                   node->nodeId, buf, dataSent->length);
#endif /* DEBUG */

            serverPtr = AppChanswitchServerGetChanswitchServer(node,
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
            printf("CHANSWITCH Server: Node %ld at %s received data size %d\n",
                   node->nodeId, buf, MESSAGE_ReturnPacketSize(msg));
#endif /* DEBUG */

            serverPtr = AppChanswitchServerGetChanswitchServer(node,
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


            switch(serverPtr->state){
                case RX_IDLE:
                {
                    if(packet[0] == PROBE_PKT){
                       #ifdef DEBUG
                        printf("CHANSWITCH Server: Node %ld at %s got PROBE_PKT\n",
                            node->nodeId, buf);
                       #endif 
                        serverPtr->state = RX_PROBE_ACK;
                        //send PROBE_ACK to TX
                        AppChanswitchServerSendProbeAck(node, serverPtr);
                        //start the delay timer
                        Message *probeMsg;
                        probeMsg = MESSAGE_Alloc(node, 
                            APP_LAYER,
                            APP_CHANSWITCH_SERVER,
                            MSG_APP_RxProbeAckDelay);
                        MESSAGE_Send(node, probeMsg, RX_PROBE_ACK_DELAY);
                        }
                        else if (packet[0] == CHANGE_PKT){
                        #ifdef DEBUG
                            printf("CHANSWITCH Server: Node %ld at %s got CHANGE_PKT\n",
                                node->nodeId, buf);
                        #endif 
                        }
                        else if (packet[0] == VERIFY_PKT){
                        #ifdef DEBUG
                            printf("CHANSWITCH Server: Node %ld at %s got VERIFY_PKT\n",
                                node->nodeId, buf);
                        #endif 
                        }
                        else{
                            ERROR_Assert(FALSE, "CHANSWITCH Server: Received unknown pkt type \n");
                        }

                    break;

                }

            }




			/*
			 * Test if the received data contains the last byte
			 * of an item.  If so, send a response packet back.
			 * If the data contains a 'c', close the connection.
			 */
			// if (packet[msg->packetSize - 1] == 'd')
			// {
			// 	/* Do nothing since item is not completely received yet. */
			// }
			// else if (packet[msg->packetSize - 1] == 'e')
			// {
			// 	/* Item completely received, now send control info. */

			// 	AppChanswitchServerSendCtrlPkt(node, serverPtr);
			// }
			// else if (packet[msg->packetSize - 1] == 'c')
			// {
			// 	/*
			// 	 * Client wants to close the session, so server also
			// 	 * initiates a close.
			// 	 */
			// 	APP_TcpCloseConnection(
			// 		node,
			// 		serverPtr->connectionId);

			// 	serverPtr->sessionFinish = getSimTime(node);
			// 	serverPtr->sessionIsClosed = TRUE;
			// }
			// else
			// {
			//    assert(FALSE);
			// }

            break;
        }

        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult *closeResult;

            closeResult = (TransportToAppCloseResult *)
                          MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("CHANSWITCH Server: Node %ld at %s got close result\n",
                   node->nodeId, buf);
#endif /* DEBUG */

            serverPtr = AppChanswitchServerGetChanswitchServer(node,
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
            #ifdef DEBUG
                printf("%s: CHANSWITCH Server node %u ACK delay timer expired\n",
                       buf, node->nodeId);
            #endif /* DEBUG */
            AppChanswitchStartProbing(node);
            break;
        }

        case MSG_APP_RxChangeAckDelay:
        {
            #ifdef DEBUG
                printf("%s: CHANSWITCH Server node %u ACK change timer expired\n",
                       buf, node->nodeId);
            #endif /* DEBUG */
            break;
        }

        default:
            ctoa(getSimTime(node), buf);
            printf("CHANSWITCH Server: Node %u at time %s received "
                   "message of unknown type"
                   " %d.\n", node->nodeId, buf, msg->eventType);
            assert(FALSE);
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        AppChanswitchServerInit.
 * PURPOSE:     listen on Chanswitch server port.
 * PARAMETERS:  node - pointer to the node.
 * RETURN:      none.
 */
void
AppChanswitchServerInit(Node *node, Address serverAddr)
{
    APP_TcpServerListen(
        node,
        APP_CHANSWITCH_SERVER,
        serverAddr,
        (short) APP_CHANSWITCH_SERVER);
}

/*
 * NAME:        AppChanswitchServerPrintStats.
 * PURPOSE:     Prints statistics of a Chanswitch session.
 * PARAMETERS:  node - pointer to the node.
 *              serverPtr - pointer to the chanswitch server data structure.
 * RETURN:      none.
 */
void
AppChanswitchServerPrintStats(Node *node, AppDataChanswitchServer *serverPtr)
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
        "CHANSWITCH Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "First Packet Sent at (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Last Packet Sent at (s) = %s", finishStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Session Status = %s", statusStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    ctoa(serverPtr->numBytesSent, numBytesSentStr);
    sprintf(buf, "Total Bytes Sent = %s", numBytesSentStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    ctoa(serverPtr->numBytesRecvd, numBytesRecvdStr);
    sprintf(buf, "Total Bytes Received = %s", numBytesRecvdStr);

    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(
        node,
        "Application",
        "CHANSWITCH Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);
}

/*
 * NAME:        AppChanswitchServerFinalize.
 * PURPOSE:     Collect statistics of a Chanswitch session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppChanswitchServerFinalize(Node *node, AppInfo* appInfo)
{
    AppDataChanswitchServer *serverPtr = (AppDataChanswitchServer*)appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        AppChanswitchServerPrintStats(node, serverPtr);
    }
}

