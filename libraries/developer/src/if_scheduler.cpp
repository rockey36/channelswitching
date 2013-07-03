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
// PROTOCOL     :: Queue-Scheduler
// LAYER        ::
// REFERENCES   :: None
// COMMENTS     :: None
// **/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "api.h"
#include "sch_strictprio.h"
#include "sch_roundrobin.h"
#include "sch_wrr.h"
#include "sch_fq.h"
#include "sch_wfq.h"
#include "sch_scfq.h"

#include "sch_graph.h"
#include "sch_atm.h"

#define DEBUG_WEIGHT  0


// Resource Management API

//**
// FUNCTION   :: WfqScheduler::setQueueBehavior
// LAYER      ::
// PURPOSE    :: Set the queue behavior
// PARAMETERS ::
// + priority : const int : Priority of a queue
// + suspend : BOOL : The queue status
// RETURN :: void
// **/
void Scheduler::setQueueBehavior(const int priority, QueueBehavior suspend)
{
    int i = 0;

    if (priority == ALL_PRIORITIES)
    {
        for (i = 0; i < numQueues; i++)
        {
            queueData[i].queue->setQueueBehavior((BOOL)suspend);
        }
    }
    else
    {
        for (i = 0; i < numQueues; i++)
        {
            if (queueData[i].priority == priority)
            {
                queueData[i].queue->setQueueBehavior((BOOL)suspend);
                break;
            }
        }
    }
    return;
}// end of setQueueBehavior


//**
// FUNCTION   :: WfqScheduler::getQueueBehavior
// LAYER      ::
// PURPOSE    :: Returns a Boolean value of TRUE if the queue is suspended
// PARAMETERS ::
// + priority : const int : Priority of a queue
// RETURN :: BOOL : TRUE or FALSE
//       If return TRUE mean queueBehavior is suspended
// **/
QueueBehavior Scheduler::getQueueBehavior(const int priority)
{
    int i = 0;

    if (priority == ALL_PRIORITIES)
    {
        for (i = 0; i < numQueues; i++)
        {
            if (!queueData[i].queue->getQueueBehavior())
            {
                return RESUME;
            }
        }
    }
    else
    {
        for (i = 0; i < numQueues; i++)
        {
            if (queueData[i].priority == priority)
            {
                return (QueueBehavior)queueData[i].queue->getQueueBehavior();
            }
        }
    }
    return SUSPEND;
}//end of getQueueBehavior

//**
// FUNCTION   :: Scheduler::setRawWeight
// LAYER      ::
// PURPOSE    :: Set the raw weight of a queue
// PARAMETERS ::
// + priority : const int : Priority of a queue
// + rawWeight: double    : Raw weight of the queue
// RETURN :: BOOL : TRUE or FALSE
// **/

void Scheduler::setRawWeight(const int priority, double rawWeight)
{
    int i = 0;

    for (i = 0; i < numQueues; i++)
    {
        if (queueData[i].priority == priority)
        {
            break;
        }
    }

    if (i < numQueues)
    {
        queueData[i].rawWeight = (float)rawWeight;

        if (DEBUG_WEIGHT)
        {
            printf("Queue (priority = %d) has raw weight %f\n",
                   priority, rawWeight);
        }
    }
}


//**
// FUNCTION   :: Scheduler::normalizeWeight
// LAYER      ::
// PURPOSE    :: Compute the weight of queues by normalizing the raw weight
// PARAMETERS :: None
// RETURN     :: void : Null
// **/

void Scheduler::normalizeWeight()
{
    int i;
    double sumRawWeight = 0.0;

    // Calculate Sum of all raw weight
    for (i = 0; i < numQueues; i++)
    {
        sumRawWeight += queueData[i].rawWeight;
    }

    // normalize the raw weight
    for (i = 0; i < numQueues; i++)
    {
        queueData[i].weight = queueData[i].rawWeight /(float)sumRawWeight;

        if (DEBUG_WEIGHT)
        {
            printf("Queue (priority = %d, rawWeight = %f) has normalized "
                   "weight as %f\n",
                   queueData[i].priority, queueData[i].rawWeight,
                   queueData[i].weight);
        }
    }
}

//**
// FUNCTION   :: Scheduler::SelectSpecificPriorityQueue
// LAYER      ::
// PURPOSE    :: Returns pointer to QueueData associated with the queue.
//               this is a Private
// PARAMETERS ::
// + priority : int : Queue priority
// RETURN :: QueueData* : Pointer of queue
// **/

QueueData* Scheduler::SelectSpecificPriorityQueue(int priority)
{
    int i = 0;

    ERROR_Assert(numQueues > 0,
        "Number of queue at the interface should be > 0");

    // The "n" number of separate priority queues is specified, QualNet
    // currently generates priorities starting from "0" to "n-1".
    // The highest priority is numbered "n-1", and the lowest priority is "0".
    // If the incoming packet priority value is equal to the same priority
    // Return the queue for enqueue operation.

    if (priority < numQueues)
    {
        if (queueData[priority].priority == priority)
        {
            return &queueData[priority];
        }
    }

    // If the above procedure fails to return a queue,
    // following block is executed

    for (i = numQueues - 1; i >= 0; i--)
    {
        if (queueData[i].priority <= priority)
        {
            // The queue whose priority same as packet priority
            // does not exit. So return the nearest higher priority queue

            return &queueData[i];
        }
    }

    // If the incoming packet priority is lower than the lowest priority
    // queue at the interface, insert the packet in the lowest priority queue.

    return &queueData[0];
}


//**
// FUNCTION   :: Scheduler::numQueue
// LAYER      ::
// PURPOSE    :: Returns number of queues under this Scheduler
// PARAMETERS :: None
// RETURN :: int : Number of queue.
// **/

int Scheduler::numQueue()
{
    return numQueues;
}


//**
// FUNCTION   :: Scheduler::GetQueuePriority
// LAYER      ::
// PURPOSE    :: Returns Priority for the queues under this Scheduler
// PARAMETERS ::
// + queueIndex : int : Queue index
// RETURN :: int : Return priority of a queue
// **/

int Scheduler::GetQueuePriority(int queueIndex)
{
    ERROR_Assert(queueIndex < numQueues, "Queue Index value corrupted!");
    return queueData[queueIndex].priority;
}


//**
// FUNCTION   :: Scheduler::isEmpty
// LAYER      ::
// PURPOSE    :: Returns a Boolean value of TRUE if the array of stored
//               messages in each queue that the scheduler controls are
//               empty, and FALSE otherwise
// PARAMETERS ::
// + priority : const int : Priority of a queue
// RETURN :: BOOL : TRUE or FALSE
// **/

BOOL Scheduler::isEmpty(const int priority)
{
    int i = 0;

    if (priority == ALL_PRIORITIES)
    {
        for (i = 0; i < numQueues; i++)
        {
            if (!queueData[i].queue->isEmpty())
            {
                return FALSE;
            }
        }
    }
    else
    {
        for (i = 0; i < numQueues; i++)
        {
            if ((queueData[i].priority == priority) &&
                (queueData[i].queue->isEmpty()))
            {
                return TRUE;
            }
        }
        return FALSE;
    }
    return TRUE;
}

// **** ADDED ON MERGE ***

#ifdef ADDON_BOEINGFCS
BOOL Scheduler::isEmpty(Node* node, const int priority)
{
    int i = 0;

    if (priority == ALL_PRIORITIES)
    {
        for (i = 0; i < numQueues; i++)
        {
            if (!queueData[i].queue->isEmpty())
            {
                return FALSE;
            }
        }
    }
    else
    {
        for (i = 0; i < numQueues; i++)
        {
            if ((queueData[i].priority == priority) &&
                (queueData[i].queue->isEmpty()))
            {
                return TRUE;
            }
        }
        return FALSE;
    }
    return TRUE;
}

int Scheduler::sizeOfQueue(const int priority)
{
    int i = 0;

    if (priority != ALL_PRIORITIES) {
        for (i = 0; i < numQueues; i++)
        {
            if (priority == queueData[i].priority)
            {
                return queueData[i].queue->sizeOfQueue();
            }
        }
    }
}

//**
// FUNCTION   :: Scheduler::weightOfQueue
// LAYER      ::
// PURPOSE    :: This function prototype returns the total number of bytes
//               stored in the array of either a specific queue, or all
//               queues that the scheduler controls.
// PARAMETERS ::
// + priority : const int : Priority of a queue
// RETURN :: BOOL : TRUE or FALSE
// **/

double Scheduler::weightOfQueue(const int priority)
{
    int i = 0;

    if (priority != ALL_PRIORITIES) {
        for (i = 0; i < numQueues; i++)
        {
            if (priority == queueData[i].priority)
            {
                return queueData[i].weight;
            }
        }
    }
}

char* Scheduler::getQueueType(const int priority)
{
    return queueData[priority].queue->getQueueType();
}

BOOL Scheduler::getQueueStats(const int priority)
{
    return queueData[priority].queue->getQueueStats();
}

void* Scheduler::getConfigParams(const int priority)
{
    return queueData[priority].queue->getConfigParams();
}

#endif


//**
// FUNCTION   :: Scheduler::bytesInQueue
// LAYER      ::
// PURPOSE    :: This function prototype returns the total number of bytes
//               stored in the array of either a specific queue, or all
//               queues that the scheduler controls.
// PARAMETERS ::
// + priority : const int : Priority of a queue
// RETURN :: BOOL : TRUE or FALSE
// **/

int Scheduler::bytesInQueue(const int priority)
{
    int i = 0;

    if (priority == ALL_PRIORITIES)
    {
        int totalBytesInQueues = 0;

        // Return total number of packets at the interface
        for (i = 0; i < numQueues; i++)
        {
            totalBytesInQueues += queueData[i].queue->bytesInQueue();
        }
        return totalBytesInQueues;
    }
    else
    {
        for (i = 0; i < numQueues; i++)
        {
            if (priority == queueData[i].priority)
            {
                return queueData[i].queue->bytesInQueue();
            }
        }
    }

    // Error: No Queue exits with such priority value
    char errStr[MAX_STRING_LENGTH] = {0};
    sprintf(errStr, "Scheduler Error:"
        " No Queue exits with priority value %d", priority);
    ERROR_Assert(FALSE, errStr);
    return 0; // Unreachable
}

//added: Scheduler:sizeOfQueue
int Scheduler::sizeOfQueue(const int priority)
{
    int i = 0;

    if (priority == ALL_PRIORITIES)
    {
        int totalBytesInQueues = 0;

        // Return total number of packets at the interface
        for (i = 0; i < numQueues; i++)
        {
            totalBytesInQueues += queueData[i].queue->sizeOfQueue();
        }
        return totalBytesInQueues;
    }
    else
    {
        for (i = 0; i < numQueues; i++)
        {
            if (priority == queueData[i].priority)
            {
                return queueData[i].queue->sizeOfQueue();
            }
        }
    }

    // Error: No Queue exits with such priority value
    char errStr[MAX_STRING_LENGTH] = {0};
    sprintf(errStr, "Scheduler Error:"
        " No Queue exits with priority value %d", priority);
    ERROR_Assert(FALSE, errStr);
    return 0; // Unreachable
}


//**
// FUNCTION   :: Scheduler::numberInQueue
// LAYER      ::
// PURPOSE    :: This function prototype returns the number of messages
//               stored in the array of either a specific queue, or all
//               queues that the scheduler controls.
// PARAMETERS ::
// + priority : const int : Priority of a queue
// RETURN :: int : Bytes in queue is used.
// **/

int Scheduler::numberInQueue(const int priority)
{
    int i = 0;

    if (priority == ALL_PRIORITIES)
    {
        int numInQueue = 0;

        // Return total number of packets at the interface
        for (i = 0; i < numQueues; i++)
        {
            numInQueue += queueData[i].queue->packetsInQueue();
        }
        return numInQueue;
    }
    else
    {
        for (i = 0; i < numQueues; i++)
        {
            if (priority == queueData[i].priority)
            {
                return queueData[i].queue->packetsInQueue();
            }
        }
    }

    // Error: No Queue exists with such a priority value
    char errStr[MAX_STRING_LENGTH] = {0};
    sprintf(errStr, "Scheduler Error:"
        " No Queue exists with priority value %d", priority);
    ERROR_Assert(FALSE, errStr);
    return 0; // Unreachable
}


//**
// FUNCTION   :: Scheduler::qosInformationUpdate
// LAYER      ::
// PURPOSE    :: This function enable Qos monitoring for all
//               queues that the scheduler controls.
// PARAMETERS ::
// + queueIndex : int : Queue index
// + qDelayVal : int* : Queue delay
// + totalTransmissionVal : int* : Transmission value
// + currentTime : const clocktype : Current simulation time
// + isResetTotalTransmissionVal : BOOL : Total Transmission is set or not
// RETURN :: void : Null
// **/

void Scheduler::qosInformationUpdate(
    int queueIndex,
    int* qDelayVal,
    int* totalTransmissionVal,
    const clocktype currentTime,
    BOOL isResetTotalTransmissionVal)
{
    Queue* queuePtr = NULL;

    queuePtr = queueData[queueIndex].queue;

    (*queuePtr).qosQueueInformationUpdate(qDelayVal,
                                          totalTransmissionVal,
                                          currentTime,
                                          isResetTotalTransmissionVal);

}


//**
// FUNCTION   :: Scheduler::collectGraphData
// LAYER      ::
// PURPOSE    :: This function enable data collection for performance
//               study of schedulers.
// PARAMETERS ::
// + priority : int : Priority of the queue
// + packetSize : int : Size of packet
// + currentTime : const clocktype : Current simulation time
// RETURN :: void : Null
// **/

void Scheduler::collectGraphData(
    int priority,
    int packetSize,
    const clocktype currentTime)
{
    if (schedGraphStatPtr == NULL)
    {
        return;
    }

    SchedGraphStat* graphPtr = NULL;
    graphPtr = (SchedGraphStat*) schedGraphStatPtr;

    (*graphPtr).collectStatisticsForGraph(this,
                                    numQueues,
                                    priority,
                                    packetSize,
                                    currentTime);
}


//**
// FUNCTION   :: Scheduler::invokeQueueFinalize
// LAYER      ::
// PURPOSE    :: This function invokes queue finalization.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + layer : const char* : The layer string
// + interfaceIndex : const int : Interface Index
// + instanceId : const int : Instance Ids
// + invokingProtocol : const char* : The protocol string
// + splStatStr : const char* Special string for stat print
// RETURN :: void : Null
// **/

void Scheduler::invokeQueueFinalize(
    Node* node,
    const char* layer,
    const int interfaceIndex,
    const int instanceId,
    const char* invokingProtocol,
    const char* splStatStr)
{
    Queue* queuePtr = queueData[instanceId].queue;

    (*queuePtr).finalize(node,
                layer,
                interfaceIndex,
                instanceId,
                invokingProtocol,
                splStatStr);
}

void Scheduler::invokeQueueFinalize(
    Node* node,
    const char* layer,
    const int interfaceIndex,
    const int instanceId,
    const int fcsQosQueue,
    const char* invokingProtocol,
    const char* splStatStr)
{
    Queue* queuePtr = queueData[instanceId].queue;

    (*queuePtr).finalize(node,
                layer,
                interfaceIndex,
                instanceId,
                fcsQosQueue,
                invokingProtocol,
                splStatStr);
}

// -------------------------------------------------------------------------
// Super Constructor: Scheduler
// -------------------------------------------------------------------------

//**
// FUNCTION   :: SCHEDULER_Setup
// LAYER      ::
// PURPOSE    :: This function runs the generic and then algorithm-specific
//               scheduler initialization routine.
// PARAMETERS ::
// + scheduler : Scheduler** : Pointer of pointer to Scheduler class
// + schedulerTypeString[] : const char : Scheduler Type string
// + enableSchedulerStat : BOOL : Scheduler Statistics is set YES or NO
// + graphDataStr : const char* : Scheduler's graph statistics is set or not
// RETURN :: void : Null
// **/

void SCHEDULER_Setup(
    Scheduler** scheduler,
    const char schedulerTypeString[],
    BOOL enableSchedulerStat,
    const char* graphDataStr)
{
    if (!strcmp(schedulerTypeString, "STRICT-PRIORITY"))
    {
        *scheduler = new StrictPriorityScheduler(enableSchedulerStat, graphDataStr);
    }
    else if (!strcmp(schedulerTypeString, "ROUND-ROBIN"))
    {
        *scheduler = new RoundRobinScheduler(enableSchedulerStat, graphDataStr);
    }
    else if (!strcmp(schedulerTypeString, "WEIGHTED-ROUND-ROBIN"))
    {
        *scheduler = new WrrScheduler(enableSchedulerStat, graphDataStr);
    }
    else if (!strcmp(schedulerTypeString, "WEIGHTED-FAIR"))
    {
        *scheduler = new WfqScheduler(enableSchedulerStat, graphDataStr);
    }
    else if (!strcmp(schedulerTypeString, "SELF-CLOCKED-FAIR"))
    {
        *scheduler = new ScfqScheduler(enableSchedulerStat, graphDataStr);
    }
    else if (!strcmp(schedulerTypeString, "ATM"))
    {
        *scheduler = new AtmScheduler(enableSchedulerStat);
    }
    else
    {
        // Error:
        char errStr[MAX_STRING_LENGTH] = {0};
        sprintf(errStr, "Scheduler Error:"
            " Unknown Scheduler Type Specified: %s", schedulerTypeString);
        ERROR_ReportError(errStr);
    }

    if (*scheduler == NULL)
    {
        // Error:
        char errStr[MAX_STRING_LENGTH] = {0};
        sprintf(errStr, "Scheduler Error: Failed to assign memory for"
            " scheduler %s", schedulerTypeString);
        ERROR_ReportError(errStr);
    }
}


//**
// FUNCTION   :: GenericPacketClassifier
// LAYER      ::
// PURPOSE    :: Classify a packet for a specific queue
// PARAMETERS ::
// + scheduler : Scheduler* : Pointer to a Scheduler class.
// + pktPriority : int : Incoming packet's priority
// RETURN :: int : Integer.
// **/

int GenericPacketClassifier(
        Scheduler *scheduler,
        int pktPriority)
{
    int i = 0;

    ERROR_Assert((*scheduler).numQueue() > 0,
        "Number of queue at the interface should be > 0");

    // The "n" number of separate priority queues is specified, QualNet
    // currently generates priorities starting from "0" to "n-1".
    // The highest priority is numbered "n-1", and the lowest priority is "0".
    // If the incoming packet priority value is equal to the same priority
    // Return the queue for enqueue operation.

    if (pktPriority < (*scheduler).numQueue())
    {
        if ((*scheduler).GetQueuePriority(pktPriority) == pktPriority)
        {
            return pktPriority;
        }
    }

    // If the above procedure fails to return a queue,
    // following block is executed

    for (i = (*scheduler).numQueue() - 1; i >= 0; i--)
    {
        if ((*scheduler).GetQueuePriority(i) <= pktPriority)
        {
            // The queue whose priority same as packet priority
            // does not exit. So return the nearest higher priority queue

            return (*scheduler).GetQueuePriority(i);
        }
    }

    // If the incoming packet priority is lower than the lowest priority
    // queue at the interface, insert the packet in the lowest priority queue.

    return (*scheduler).GetQueuePriority(0);
}

