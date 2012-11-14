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
 * Calculates prop range
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "phy_802_11.h"
#include "phy_chanswitch.h"
#ifdef CELLULAR_LIB
#include "phy_gsm.h"
#include "phy_cellular.h"
#elif UMTS_LIB
#include "phy_cellular.h"
#endif // CELLULAR_LIB
#include "phy_abstract.h"
#ifdef MILITARY_RADIOS_LIB
#include "phy_jtrs_wnw.h"
#endif // MILITARY_RADIOS_LIB
#include "propagation.h"
//#include "mac_dot16_bs.h"

#define LONGEST_DISTANCE 1000000.0  // 1000km
#define DELTA            0.001      // 1mm
#define PACKET_SIZE      (1024 * 8) // 1kbytes
#define RADIO_NUMBER   0

int main(int argc, char **argv) {
    NodeInput       nodeInput;
    int             numNodes = 0;
    unsigned*       nodeIdArray;
    PropProfile*    propProfile;
    Node*           node;
    int             radioNumber = RADIO_NUMBER;
    PhyData*        thisRadio;

    PhyModel         phyModel = PHY_NONE;
    PartitionData*  partitionData;

    double distanceNotReachable = LONGEST_DISTANCE;
    double distance = distanceNotReachable;
    AddressMapType *addressMapPtr = MAPPING_MallocAddressMap();
    Address networkAddress;

#ifdef ADDON_BOEINGFCS
    ChangeParamTypeQueue changeParamTypeQueue;
#endif

    IO_InitializeNodeInput(&nodeInput, true);
    IO_ReadNodeInput(&nodeInput, argv[1]);

    MAPPING_BuildAddressMap(
       &nodeInput,
       &numNodes,
       &nodeIdArray,
       addressMapPtr);

    /* Create Partition */
    /* This code should closely resemble the code in partition.c */
    {
        int i;

        partitionData = (PartitionData *) MEM_malloc(sizeof(PartitionData));
        memset(partitionData, 0, sizeof(PartitionData));

        partitionData->partitionId          = 0;
        partitionData->numPartitions        = 1;
        partitionData->numNodes             = numNodes;
        partitionData->seedVal              = 1; //arbitrary
        partitionData->numberOfEvents       = 0;
        partitionData->numberOfMobilityEvents = 0;
        //partitionData->terrainData          = terrainData;
        //partitionData->maxSimClock          = maxSimClock;
        //partitionData->startRealTime        = startRealTime;
        partitionData->nodeInput            = &nodeInput;
        partitionData->addressMapPtr        = addressMapPtr;

        ANTENNA_GlobalAntennaModelPreInitialize(partitionData);
        ANTENNA_GlobalAntennaPatternPreInitialize(partitionData);

        partitionData->nodePositions =
            (NodePositions*)MEM_malloc(sizeof(NodePositions) * numNodes);

        for (i = 0; i < numNodes; i++) {
            partitionData->nodePositions[i].nodeId = nodeIdArray[i];
        }

        partitionData->guiOption            = FALSE; //arbitrary
        partitionData->traceEnabled         = FALSE; //arbitrary

        partitionData->nodeData = (Node **)MEM_malloc(sizeof(Node *) * numNodes);
        memset(partitionData->nodeData, 0, sizeof(Node *) * numNodes);

        partitionData->propChannel = NULL;
        partitionData->numChannels = 0;
        partitionData->numProfiles = 0;

        partitionData->mobilityHeap.minTime = CLOCKTYPE_MAX;
        partitionData->mobilityHeap.heapNodePtr = NULL;
        partitionData->mobilityHeap.heapSize = 0;
        partitionData->mobilityHeap.length = 0;
    }

    PROP_GlobalInit(partitionData, &nodeInput);
    node = NODE_CreateNode(partitionData, nodeIdArray[0], 0, 0);

    NETWORK_PreInit(node, &nodeInput);
    PHY_Init(node, &nodeInput);
    PHY_GlobalBerInit(&nodeInput);

    { // Since MacInit was rewritten, do the necessary functions
        int interfaceIndex;
        char phyModelName[MAX_STRING_LENGTH];
        BOOL found;

        // assume first node on network N8-1.0
        NetworkIpAddNewInterface(
            node,
            257,
            8,
            &interfaceIndex,
            &nodeInput,
            TRUE);

        node->macData[interfaceIndex] =
            (MacData *)MEM_malloc(sizeof(MacData));

        node->macData[interfaceIndex]->interfaceIndex = interfaceIndex;
        node->macData[interfaceIndex]->propDelay = MAC_PROPAGATION_DELAY;
        node->macData[interfaceIndex]->phyNumber = -1;

        networkAddress.networkType = NETWORK_IPV4;
        networkAddress.interfaceAddr.ipv4 = 257;

        IO_ReadString(
            node->nodeId,
            &networkAddress,
            &nodeInput,
            "PHY-MODEL",
            &found,
            phyModelName);

        assert(found == TRUE);

        if (strcmp(phyModelName, "PHY802.11a") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY802_11a,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY802_11a;
        }
        else if (strcmp(phyModelName, "PHY802.11b") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY802_11b,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY802_11b;
        }
				else if (strcmp(phyModelName, "PHY_CHANSWITCH") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY_CHANSWITCH,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY_CHANSWITCH;
        }
#ifdef CELLULAR_LIB
        else if (strcmp(phyModelName, "PHY-GSM") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY_GSM,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY_GSM;
        }
        else if (strcmp(phyModelName, "PHY-CELLULAR") == 0) {
			PHY_CreateAPhyForMac(
				 node,
				 &nodeInput,
				 interfaceIndex,
				 &networkAddress,
				 PHY_CELLULAR,
				 &node->macData[interfaceIndex]->phyNumber);

			phyModel = PHY_CELLULAR;
		}
#elif UMTS_LIB
        else if (strcmp(phyModelName, "PHY-CELLULAR") == 0) {
			PHY_CreateAPhyForMac(
				 node,
				 &nodeInput,
				 interfaceIndex,
				 &networkAddress,
				 PHY_CELLULAR,
				 &node->macData[interfaceIndex]->phyNumber);

			phyModel = PHY_CELLULAR;
		}
#endif // CELLULAR_LIB
        else if (strcmp(phyModelName, "PHY-ABSTRACT") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY_ABSTRACT,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY_ABSTRACT;
        }
#ifdef ADVANCED_WIRELESS_LIB
        else if (strcmp(phyModelName, "PHY802.16") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY802_16,
                    &node->macData[interfaceIndex]->phyNumber);

            //MacDot16BsInit(node, interfaceIndex, &nodeInput);
        }
#endif // ADVANCED_WIRELESS_LIB
#ifdef MILITARY_RADIOS_LIB
        else if (strcmp(phyModelName, "PHY-JTRS-WNW") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY_JTRS_WNW,
                    &node->macData[interfaceIndex]->phyNumber);
            phyModel = PHY_JTRS_WNW;
        }
#endif // MILITARY_RADIOS_LIB
#ifdef SENSOR_NETWORKS_LIB
        else if (strcmp(phyModelName, "PHY802.15.4") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    &nodeInput,
                    interfaceIndex,
                    &networkAddress,
                    PHY802_15_4,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY802_15_4;
        }
#endif // SENSOR_NETWORKS_LIB
        else {
            ERROR_ReportError("Unknown PHY-MODEL");
        }
    }

    PROP_Init(node, 0, &nodeInput);

    propProfile = node->partitionData->propChannel[0].profile;

    thisRadio = node->phyData[radioNumber];

    distance = PHY_PropagationRange(node, radioNumber, TRUE);

    return 0;
}
