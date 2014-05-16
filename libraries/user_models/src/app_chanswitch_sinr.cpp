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
                         int itemsToSend)
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

    if (itemsToSend > 0)
    {
        chanswitch_sinrClient->itemsToSend = itemsToSend;
    }
    else
    {
        chanswitch_sinrClient->itemsToSend = AppChanswitchSinrClientItemsToSend(chanswitch_sinrClient);
    }

    chanswitch_sinrClient->itemSizeLeft = 0;
    chanswitch_sinrClient->numBytesSent = 0;
    chanswitch_sinrClient->numBytesRecvd = 0;
    chanswitch_sinrClient->lastItemSent = 0;

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
 * NAME:        AppChanswitchSinrClientSendNextItem.
 * PURPOSE:     Send the next item.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the chanswitch_sinr client data structure.
 * RETRUN:      none.
 */
void
AppChanswitchSinrClientSendNextItem(Node *node, AppDataChanswitchSinrClient *clientPtr)
{
    assert(clientPtr->itemSizeLeft == 0);

    if (clientPtr->itemsToSend > 0)
    {
        clientPtr->itemSizeLeft = AppChanswitchSinrClientItemSize(clientPtr);
        clientPtr->itemSizeLeft += AppChanswitchSinrClientCtrlPktSize(clientPtr);

        AppChanswitchSinrClientSendNextPacket(node, clientPtr);
        clientPtr->itemsToSend --;
    }
    else
    {
        if (!clientPtr->sessionIsClosed)
        {
            APP_TcpSendData(
                node,
                clientPtr->connectionId,
                (char *)"c",
                1,
                TRACE_APP_CHANSWITCH_SINR);
        }

        clientPtr->sessionIsClosed = TRUE;
        clientPtr->sessionFinish = getSimTime(node);

    }
}

/*
 * NAME:        AppChanswitchSinrClientSendNextPacket.
 * PURPOSE:     Send the remaining data.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the chanswitch_sinr client data structure.
 * RETRUN:      none.
 */
void
AppChanswitchSinrClientSendNextPacket(Node *node, AppDataChanswitchSinrClient *clientPtr)
{
    int itemSize;
    char *payload;

    /* Break packet down of larger than APP_MAX_DATA_SIZE. */
    if (clientPtr->itemSizeLeft > APP_MAX_DATA_SIZE)
    {
        itemSize = APP_MAX_DATA_SIZE;
        clientPtr->itemSizeLeft -= APP_MAX_DATA_SIZE;
        payload = (char *)MEM_malloc(itemSize);
        memset(payload,'d',itemSize);
    }
    else
    {
        itemSize = clientPtr->itemSizeLeft;
        payload = (char *)MEM_malloc(itemSize);
        memset(payload,'d',itemSize);

        /* Mark the end of the packet. */

         *(payload + itemSize - 1) = 'e';
          clientPtr->itemSizeLeft = 0;
    }

    if (!clientPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                clientPtr->connectionId,
                payload,
                itemSize,
                TRACE_APP_CHANSWITCH_SINR);
    }
    MEM_free(payload);
}

/*
 * NAME:        AppChanswitchSinrClientItemsToSend.
 * PURPOSE:     call tcplib function ftp_nitems() to get the
 *              number of items to send in an chanswitch_sinr session.
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      amount of items to send.
 */
int
AppChanswitchSinrClientItemsToSend(AppDataChanswitchSinrClient *clientPtr)
{
    int items;

    items = ftp_nitems(clientPtr->seed);

#ifdef DEBUG
    printf("CHANSWITCH_SINR nitems = %d\n", items);
#endif /* DEBUG */

    return items;
}

/*
 * NAME:        AppChanswitchSinrClientItemSize.
 * PURPOSE:     call tcplib function ftp_itemsize() to get the size
 *              of each item.
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      size of an item.
 */
int
AppChanswitchSinrClientItemSize(AppDataChanswitchSinrClient *clientPtr)
{
    int size;

    size = ftp_itemsize(clientPtr->seed);

#ifdef DEBUG
    printf("CHANSWITCH_SINR item size = %d\n", size);
#endif /* DEBUG */

    return size;
}

/*
 * NAME:        AppChanswitchSinrClientCtrlPktSize.
 * PURPOSE:     call tcplib function ftp_ctlsize().
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      chanswitch_sinr control packet size.
 */
int
AppChanswitchSinrClientCtrlPktSize(AppDataChanswitchSinrClient *clientPtr)
{
    int ctrlPktSize;
    ctrlPktSize = ftp_ctlsize(clientPtr->seed);

#ifdef DEBUG
    printf("CHANSWITCH_SINR: Node %d chanswitch_sinr control pktsize = %d\n",
           ctrlPktSize);
#endif /* DEBUG */

    return (ctrlPktSize);
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
 * NAME:        AppChanswitchSinrServerSendCtrlPkt.
 * PURPOSE:     call AppChanswitchSinrCtrlPktSize() to get the response packet
 *              size, and send the packet.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETRUN:      none.
 */
void
AppChanswitchSinrServerSendCtrlPkt(Node *node, AppDataChanswitchSinrServer *serverPtr)
{
    int pktSize;
    char *payload;

    pktSize = AppChanswitchSinrServerCtrlPktSize(serverPtr);

    if (pktSize > APP_MAX_DATA_SIZE)
    {
        /*
         * Control packet size is larger than APP_MAX_DATA_SIZE,
         * set it to APP_MAX_DATA_SIZE. This should be rare.
         */
        pktSize = APP_MAX_DATA_SIZE;
    }
    payload = (char *)MEM_malloc(pktSize);
    memset(payload,'d',pktSize);
    if (!serverPtr->sessionIsClosed)
    {
        APP_TcpSendData(
                node,
                serverPtr->connectionId,
                payload,
                pktSize,
                TRACE_APP_CHANSWITCH_SINR);
    }
     MEM_free(payload);
}

/*
 * NAME:        AppChanswitchSinrServerCtrlPktSize.
 * PURPOSE:     call tcplib function ftp_ctlsize().
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      chanswitch_sinr control packet size.
 */
int
AppChanswitchSinrServerCtrlPktSize(AppDataChanswitchSinrServer *serverPtr)
{
    int ctrlPktSize;
    ctrlPktSize = ftp_ctlsize(serverPtr->seed);

#ifdef DEBUG
    printf("CHANSWITCH_SINR: Node chanswitch_sinr control pktsize = %d\n",
           ctrlPktSize);
#endif /* DEBUG */

    return (ctrlPktSize);
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

                AppChanswitchSinrClientSendNextItem(node, clientPtr);
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

            clientPtr->numBytesSent += (clocktype) dataSent->length;
            clientPtr->lastItemSent = getSimTime(node);

            /* Instant throughput is measured here after each 1 second */
            #ifdef DEBUG_OUTPUT_FILE
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

            if (clientPtr->itemSizeLeft > 0)
            {
                AppChanswitchSinrClientSendNextPacket(node, clientPtr);
            }
            else if (clientPtr->itemsToSend == 0
                     && clientPtr->sessionIsClosed == TRUE)
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

            dataRecvd = (TransportToAppDataReceived *)
                         MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: CHANSWITCH_SINR Client node %u received data %d\n",
                   buf, node->nodeId, msg->packetSize);
#endif /* DEBUG */

            clientPtr = AppChanswitchSinrClientGetChanswitchSinrClient(node,
                                                 dataRecvd->connectionId);

            assert(clientPtr != NULL);

            clientPtr->numBytesRecvd += (clocktype) msg->packetSize;

            assert(msg->packet[msg->packetSize - 1] == 'd');

            if ((clientPtr->sessionIsClosed == FALSE) &&
                (clientPtr->itemSizeLeft == 0))
            {
                AppChanswitchSinrClientSendNextItem(node, clientPtr);
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
    int itemsToSend,
    clocktype waitTime)
{
    AppDataChanswitchSinrClient *clientPtr;

    /* Check to make sure the number of chanswitch_sinr items is a correct value. */

    if (itemsToSend < 0)
    {
        printf("CHANSWITCH_SINR Client: Node %d items to send needs to be >= 0\n",
               node->nodeId);

        exit(0);
    }

    clientPtr = AppChanswitchSinrClientNewChanswitchSinrClient(node,
                                         clientAddr,
                                         serverAddr,
                                         itemsToSend);

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

            serverPtr->numBytesRecvd +=
                    (clocktype) MESSAGE_ReturnPacketSize(msg);


            /*
             * Test if the received data contains the last byte
             * of an item.  If so, send a response packet back.
             * If the data contains a 'c', close the connection.
             */
            if (packet[msg->packetSize - 1] == 'd')
            {
                /* Do nothing since item is not completely received yet. */
            }
            else if (packet[msg->packetSize - 1] == 'e')
            {
                /* Item completely received, now send control info. */

                AppChanswitchSinrServerSendCtrlPkt(node, serverPtr);
            }
            else if (packet[msg->packetSize - 1] == 'c')
            {
                /*
                 * Client wants to close the session, so server also
                 * initiates a close.
                 */
                APP_TcpCloseConnection(
                    node,
                    serverPtr->connectionId);

                serverPtr->sessionFinish = getSimTime(node);
                serverPtr->sessionIsClosed = TRUE;
            }
            else
            {
               assert(FALSE);
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

