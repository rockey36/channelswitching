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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sstream>

#include "api.h"
#include "partition.h"
#include "propagation.h"

#ifdef WIRELESS_LIB
#include "antenna.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#include "phy_802_11.h"
#include "phy_abstract.h"
#endif // WIRELESS_LIB

#include "phy_chanswitch.h"
//#include "energy_model.h"

#ifdef SENSOR_NETWORKS_LIB
#include "phy_802_15_4.h"
#endif //SENSOR_NETWORKS_LIB

#ifdef CELLULAR_LIB
#include "phy_gsm.h"
#include "phy_cellular.h"
#ifdef UMTS_LIB
#include "phy_umts.h"
#endif
#elif UMTS_LIB
#include "phy_cellular.h"
#include "phy_umts.h"
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
#include "phy_fcsc.h"
#include "phy_jtrs_wnw.h"
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
#include "link16_phy.h"
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
#include "phy_satellite_rsv.h"
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
#include "phy_dot16.h"
#endif // ADVANCED_WIRELESS_LIB

#ifdef ADDON_BOEINGFCS
#include "phy_stats.h"
//pcom
#include "mapping.h"
#endif
#ifdef ADDON_NGCNMS
#include "ngc_stats.h"
#endif
#include "prop_flat_binning.h"

#ifdef URBAN_LIB
#include "prop_cost_hata.h"
#endif // URBAN_LIB

#define DEBUG 0
#define DYNAMIC_STATS 0
#ifdef ADDON_BOEINGFCS
static BOOL
IsIpAddressInSubnet(NodeAddress address,
                    NodeAddress subnetAddress,
                    int numHostBits)
{
    NodeAddress subnetMask;
    if (numHostBits == 32)
    {
        subnetMask = 0;
    }
    else
    {
        subnetMask = 0xffffffff << numHostBits;
    }

    return (BOOL) ((address & subnetMask) == subnetAddress);
}
#endif

void PHY_Init(
    Node *node,
    const NodeInput *nodeInput)
{
    node->numberPhys = 0;
}


void PHY_SetRxSNRThreshold (
      Node *node,
      int phyIndex,
      double snr)
{

    PhyData* thisPhy = node->phyData[phyIndex];

    assert(thisPhy->phyRxModel == SNR_THRESHOLD_BASED);
    thisPhy->phyRxSnrThreshold = NON_DB(snr);

    return;

}

void PHY_SetDataRate(
      Node *node,
      int phyIndex,
      int dataRate)
{

    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel)
    {
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT:
        {
           PhyAbstractSetDataRate(node,phyIndex,dataRate);
           break;
        }
#endif // WIRELESS_LIB
        default:
        {
           break;
        }
    }

    return;

}


// Dynamic variables

class D_PhyTxPower : public D_Object
{
    // Makes it possible to obtain the value returned by GetPhyTxPower ()

    private:
        Node *  m_node;
        int     m_phyIndex;

    public:
        D_PhyTxPower (Node * node, int phyIndex) : D_Object(D_COMMAND)
        {
            m_phyIndex = phyIndex;
            m_node = node;
        }

        void ExecuteAsString(const std::string& in, std::string& out)
        {
            double txPower;
            PHY_GetTransmitPower (m_node, m_phyIndex, &txPower);
            std::stringstream ss;
            ss << txPower;
            out = ss.str ();
        }
};


/*
 * FUNCTION    PHY_CreateAPhyForMac
 * PURPOSE     Initialization function for the phy layer.
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void PHY_CreateAPhyForMac(
    Node *node,
    const NodeInput *nodeInput,
    int interfaceIndex,
    Address *networkAddress,
    PhyModel phyModel,
    int* phyNumber)
{
    char buf[10*MAX_STRING_LENGTH];
    BOOL wasFound;
    double temperature;
    double noiseFactor;
    double noise_mW_hz;
    double snr;

    int phyIndex = node->numberPhys;

    PhyData *thisPhy;
    NodeInput berTableInput;
    int i;

    int numberChannels = PROP_NumberChannels(node);
    D_Hierarchy *hierarchy = &node->partitionData->dynamicHierarchy;

    node->numberPhys++;
    *phyNumber = phyIndex;
    assert(phyIndex < MAX_NUM_PHYS);

    thisPhy = (PhyData *)MEM_malloc(sizeof(PhyData));
    memset(thisPhy, 0, sizeof(PhyData));
    node->phyData[phyIndex] = thisPhy;

    thisPhy->phyIndex = phyIndex;
    thisPhy->macInterfaceIndex = interfaceIndex;

    thisPhy->networkAddress = (Address*) MEM_malloc(sizeof(Address));

    memcpy(thisPhy->networkAddress, networkAddress, (sizeof(Address)));

    thisPhy->phyModel = phyModel;

    assert(phyModel == PHY802_11a ||
           phyModel == PHY802_11b ||
           phyModel == PHY_GSM ||
           phyModel == PHY_CELLULAR ||
           phyModel == PHY_ABSTRACT ||
           phyModel == PHY_FCSC_PROTOTYPE ||
           phyModel == PHY_SPAWAR_LINK16 ||
           phyModel == PHY_BOEINGFCS ||
           phyModel == PHY_SATELLITE_RSV ||
           phyModel == PHY802_16 ||
           phyModel == PHY_JTRS_WNW ||
           phyModel == PHY_JAMMING ||
           phyModel == PHY802_15_4 ||
					 phyModel == PHY_CHANSWITCH);

#ifndef ADDON_BOEINGFCS
    // enable the contention free propagation abstraction
    // NOTE: Removed ABSTRACT-CONTENTION-FREE-PROPAGATION parameter
    // and forced on all the time.
    thisPhy->contentionFreeProp = TRUE;
#else
    // temporary fix until ces code conforms to
    // ABSTRACT-CONTENTION-FREE-PROPAGATION
    thisPhy->contentionFreeProp = FALSE;
#endif

    thisPhy->channelListenable = new D_BOOL[numberChannels];
    thisPhy->channelListening = new D_BOOL[numberChannels];
	thisPhy->channelSwitch = new D_BOOL[numberChannels];



#ifdef ADDON_NGCNMS
    // Dynamic Hierarchy
    BOOL createListenableChannelMaskPath = FALSE;
    BOOL createListeningChannelMaskPath = FALSE;
    char listenableChannelMaskPath[MAX_STRING_LENGTH] = "";
    char listeningChannelMaskPath[MAX_STRING_LENGTH] = "";

    for (i = 0; i < numberChannels; i++) {

        if (hierarchy->IsEnabled()) {
          // Create a node path
          createListenableChannelMaskPath =
            hierarchy->CreateNodeInterfaceChannelPath(
                  node,
                  interfaceIndex,
                  i,
                  "PHY-LISTENABLE-CHANNEL-MASK",
                  listenableChannelMaskPath);

            // add object to path
            if (createListenableChannelMaskPath) {
                hierarchy->AddObject(
                    listenableChannelMaskPath,
                    new D_BOOLObj(&thisPhy->channelListenable[i]));
            }
#ifdef ADDON_NGCNMS
#ifdef D_LISTENING_ENABLED

            if (createListenableChannelMaskPath) {
                hierarchy->AddListener(
                    listenableChannelMaskPath,
                    "listener",
                    "",
                    "NMS",
                    new listenableChannelCallBack());
            }
#endif
#endif
            // Create a node path
            createListeningChannelMaskPath =
                hierarchy->CreateNodeInterfaceChannelPath(
                    node,
                    interfaceIndex,
                    i,
                    "PHY-LISTENING-CHANNEL-MASK",
                    listeningChannelMaskPath);
            // add object to path
            if (createListeningChannelMaskPath) {
                hierarchy->AddObject(
                    listeningChannelMaskPath,
                    new D_BOOLObj(&thisPhy->channelListening[i]));
            }
#ifdef ADDON_NGCNMS
#ifdef D_LISTENING_ENABLED
            if (createListeningChannelMaskPath) {
              hierarchy->AddListener(
                  listeningChannelMaskPath,
                  "listener",
                  "",
                  "NMS",
                  new listeningChannelCallBack());
            }
#endif
#endif
        }
        thisPhy->channelListenable[i] = FALSE;
        thisPhy->channelListening[i] = FALSE;
    }
#endif // ADDON_NGCNMS

    if (hierarchy->IsEnabled()) {
        BOOL createdTxPowerPath;
        std::string path;
        createdTxPowerPath = hierarchy->CreateNodeInterfacePath(
            node,
            interfaceIndex,
            "TxPower",
            path);
        if (createdTxPowerPath)
        {
            hierarchy->AddObject (path, new D_PhyTxPower (node, phyIndex));
            hierarchy->SetExecutable (path, TRUE);

            // Create a symlink, so that the txPower can be accessed
            // just via the phyindex instead of the interface's IP address.
            std::string phyPowerByIndexPath;
            hierarchy->BuildNodeInterfaceIndexPath (
                node, phyIndex,
                "TxPower",
                phyPowerByIndexPath);
            hierarchy->AddLink (phyPowerByIndexPath, path);
        }
    }

#ifdef ADDON_NGCNMS
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-LISTENABLE-CHANNEL-MASK",
        &wasFound,
        buf,
        TRUE);
#else
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-LISTENABLE-CHANNEL-MASK",
        &wasFound,
        buf);
#endif

    if (wasFound) {
        if (strlen(buf) > (unsigned)numberChannels) {
            char errorMessage[10*MAX_STRING_LENGTH];
            char addr[MAX_STRING_LENGTH];
            NodeAddress subnetAddress;

            subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                node->nodeId, interfaceIndex);

            IO_ConvertIpAddressToString(subnetAddress, addr);
            sprintf(errorMessage,
                "[%s] PHY-LISTENABLE-CHANNEL-MASK %s \n"
                "contains channels more than the total number of channels\n"
                "total number of channels: %d",addr, buf, numberChannels);

            ERROR_ReportError(errorMessage);
        }
        if (strlen(buf) < (unsigned)numberChannels) {
            char errorMessage[10*MAX_STRING_LENGTH];
            char addr[MAX_STRING_LENGTH];
            NodeAddress subnetAddress;

            subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                node->nodeId, interfaceIndex);

            IO_ConvertIpAddressToString(subnetAddress, addr);
            sprintf(errorMessage,
                "[%s] PHY-LISTENABLE-CHANNEL-MASK %s \n"
                "contains channels less than the total number of channels\n"
                "total number of channels %d",addr, buf, numberChannels);

            ERROR_ReportError(errorMessage);
        }
        assert(strlen(buf) == (unsigned) numberChannels);

        for (i = 0; i < numberChannels; i++) {
            if (buf[i] == '1') {
                thisPhy->channelListenable[i] = TRUE;
            }
            else if (buf[i] == '0') {
                // Do nothing as it is initialized to FALSE
            }
            else {
                ERROR_ReportError(
                    "Error: PHY-LISTENABLE-CHANNEL-MASK is "
                    "incorrectly formatted.\n");
            }
        }
    }


    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-LISTENING-CHANNEL-MASK",
        &wasFound,
        buf);

    if (wasFound) {
        int numberChannels = PROP_NumberChannels(node);

        if (strlen(buf) > (unsigned)numberChannels) {
            char errorMessage[10*MAX_STRING_LENGTH];
            char addr[MAX_STRING_LENGTH];
            NodeAddress subnetAddress;

            subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                node->nodeId, interfaceIndex);

            IO_ConvertIpAddressToString(subnetAddress, addr);
            sprintf(errorMessage,
                "[%s] PHY-LISTENABLE-CHANNEL-MASK %s \n"
                "contains channels more than the total number of channels\n"
                "total number of channels %d",addr, buf, numberChannels);

            ERROR_ReportError(errorMessage);
        }
        if (strlen(buf) < (unsigned)numberChannels) {
            char errorMessage[10*MAX_STRING_LENGTH];
            char addr[MAX_STRING_LENGTH];
            NodeAddress subnetAddress;

            subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                node->nodeId, interfaceIndex);

            IO_ConvertIpAddressToString(subnetAddress, addr);
            sprintf(errorMessage,
                "[%s] PHY-LISTENABLE-CHANNEL-MASK %s \n"
                "contains channels less than the total number of channels\n"
                "total number of channels %d",addr, buf, numberChannels);

            ERROR_ReportError(errorMessage);
        }
        assert(strlen(buf) == (unsigned)numberChannels);

        for (i = 0; i < numberChannels; i++) {
            if (buf[i] == '1') {
                PHY_StartListeningToChannel(node, phyIndex, i);
            }
            else if (buf[i] == '0') {
                // Do nothing as it is initialized to FALSE
            }
            else {
                char errorMessage[10*MAX_STRING_LENGTH];
                char addr[MAX_STRING_LENGTH];
            NodeAddress subnetAddress;

            subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                node->nodeId, interfaceIndex);

            IO_ConvertIpAddressToString(subnetAddress, addr);
                sprintf(errorMessage,
                    "[%s] PHY-LISTENABLE-CHANNEL-MASK %s \n"
                    "is incorrectly formatted\n",addr, buf);

                ERROR_ReportError(errorMessage);
            }
        }
    }

	//PHY-CHANSWITCH-CHANNEL-MASK
	//Only used by Channel Switching 
	IO_ReadString(
		node,
		node->nodeId,
		interfaceIndex,
		nodeInput,
		"PHY-CHANSWITCH-CHANNEL-MASK",
		&wasFound,
		buf);

	if (wasFound) {
		int numberChannels = PROP_NumberChannels(node);

		if (strlen(buf) > (unsigned)numberChannels) {
			char errorMessage[10*MAX_STRING_LENGTH];
			char addr[MAX_STRING_LENGTH];
			NodeAddress subnetAddress;

			subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
				node->nodeId, interfaceIndex);

			IO_ConvertIpAddressToString(subnetAddress, addr);
			sprintf(errorMessage,
				"[%s] PHY-CHANSWITCH-CHANNEL-MASK %s \n"
				"contains channels more than the total number of channels\n"
				"total number of channels %d",addr, buf, numberChannels);

			ERROR_ReportError(errorMessage);
		}
		if (strlen(buf) < (unsigned)numberChannels) {
			char errorMessage[10*MAX_STRING_LENGTH];
			char addr[MAX_STRING_LENGTH];
			NodeAddress subnetAddress;

			subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
				node->nodeId, interfaceIndex);

			IO_ConvertIpAddressToString(subnetAddress, addr);
			sprintf(errorMessage,
				"[%s] PHY-CHANSWITCH-CHANNEL-MASK %s \n"
				"contains channels less than the total number of channels\n"
				"total number of channels %d",addr, buf, numberChannels);

			ERROR_ReportError(errorMessage);
		}
		assert(strlen(buf) == (unsigned)numberChannels);

		for (i = 0; i < numberChannels; i++) {
			
			if (buf[i] == '1') {
				thisPhy->channelSwitch[i] = TRUE;
			}
			else if (buf[i] == '0') {
				// Do nothing as it is initialized to FALSE
			}
			
			else {
				char errorMessage[10*MAX_STRING_LENGTH];
				char addr[MAX_STRING_LENGTH];
			NodeAddress subnetAddress;

			subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
				node->nodeId, interfaceIndex);

			IO_ConvertIpAddressToString(subnetAddress, addr);
				sprintf(errorMessage,
					"[%s] PHY-CHANSWITCH-CHANNEL-MASK %s \n"
					"is incorrectly formatted\n",addr, buf);

				ERROR_ReportError(errorMessage);
			}
		}
	}


    //
    // Get the temperature
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-TEMPERATURE",
        &wasFound,
        &temperature);

    if (wasFound == FALSE) {
        temperature = PHY_DEFAULT_TEMPERATURE;
    }
    else {
        assert(wasFound == TRUE);
    }


    //
    // Get the noise factor
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-NOISE-FACTOR",
        &wasFound,
        &noiseFactor);

    if (wasFound == FALSE) {
        noiseFactor = PHY_DEFAULT_NOISE_FACTOR;
    }
    else {
        assert(wasFound == TRUE);
    }


    //
    // Calculate thermal noise
    //
    noise_mW_hz
        = BOLTZMANN_CONSTANT * temperature * noiseFactor * 1000.0;

    thisPhy->noise_mW_hz = noise_mW_hz;
    thisPhy->noiseFactor = noiseFactor;
#ifdef ADDON_BOEINGFCS
    PhyStatsInitLossStats(node, phyIndex);
    PhyStatsInitDelayStats(node, phyIndex);
#endif

    //
    // Set PHY-RX-MODEL
    //

#ifdef ADDON_NGCNMS
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-RX-MODEL",
        &wasFound,
        buf,
        TRUE);
#else
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-RX-MODEL",
        &wasFound,
        buf);
#endif

    if (wasFound) {
#ifdef WIRELESS_LIB
        if (strcmp(buf, "PHY802.11a") == 0) {
            thisPhy->phyRxModel = RX_802_11a;
            Phy802_11aSetBerTable(thisPhy);
        }
        else if (strcmp(buf, "PHY802.11b") == 0) {
            thisPhy->phyRxModel = RX_802_11b;
            Phy802_11bSetBerTable(thisPhy);
        }
        else if (strcmp(buf, "PHY_CHANSWITCH") == 0) {
            thisPhy->phyRxModel = RX_CHANSWITCH;
            Phy802_11aSetBerTable(thisPhy);
        }
        else
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
        if (strcmp(buf, "PHY802.15.4") == 0) {
            thisPhy->phyRxModel = RX_802_15_4;
            //PhyDot16SetBerTable(thisPhy);
        }
        else
#endif // SENSOR_NETWORKS_LIB
#ifdef ADVANCED_WIRELESS_LIB
        if (strcmp(buf, "PHY802.16") == 0) {
            thisPhy->phyRxModel = RX_802_16;
            PhyDot16SetBerTable(thisPhy);
        }
        else
#endif //ADVANCED_WIRELESS_LIB
#ifdef UMTS_LIB
        if (strcmp(buf, "PHY-UMTS") == 0) {
            thisPhy->phyRxModel = RX_UMTS;
            PhyUmtsSetBerTable(thisPhy);
        }
        else
#endif //UMTS_LIB
#ifdef SENSOR_NETWORKS_LIB
        if (strcmp(buf, "PHY802.15.4") == 0) {
            thisPhy->phyRxModel = RX_802_15_4;
            //PhyDot16SetBerTable(thisPhy);
        }
        else
#endif //SENSOR_NETWORKS_LIB

        if (strcmp(buf, "SNR-THRESHOLD-BASED") == 0) {
            thisPhy->phyRxModel = SNR_THRESHOLD_BASED;

#ifdef ADDON_NGCNMS
            IO_ReadDouble(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-SNR-THRESHOLD",
                &wasFound,
                &snr,
                TRUE,
                FALSE,
                0,
                FALSE,
                0);
#else
            IO_ReadDouble(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-SNR-THRESHOLD",
                &wasFound,
                &snr);
#endif
            if (wasFound) {
                thisPhy->phyRxSnrThreshold = NON_DB(snr);
            }
            else {
                ERROR_ReportError("PHY-RX-SNR-THRESHOLD is missing.\n");
            }
        }
        else if (strcmp(buf, "BER-BASED") == 0) {
            int i;
            int numBerTables = 0;

            thisPhy->phyRxModel = BER_BASED;

            //
            // Read PHY-RX-BER-TABLE-FILE
            //
            IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-BER-TABLE-FILE",
                0,
                TRUE,
                &wasFound,
                &berTableInput);

            if (wasFound != TRUE) {
                ERROR_ReportError(
                    "PHY-RX-BER-TABLE-FILE is not specified\n");
            }

            assert(wasFound == TRUE);

            numBerTables++;

            // Scan all PHY-RX-BER-TABLE-FILE variables
            // and see how many of them are defined
            while (TRUE) {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-BER-TABLE-FILE",
                    numBerTables,
                    FALSE,
                    &wasFound,
                    &berTableInput);

                if (wasFound != TRUE) {
                    break;
                }
                numBerTables++;
            }

            thisPhy->numBerTables = numBerTables;
            thisPhy->snrBerTables = PHY_BerTablesAlloc (numBerTables);

            for (i = 0; i < numBerTables; i++) {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-BER-TABLE-FILE",
                    i,
                    (i == 0),
                    &wasFound,
                    &berTableInput);

                assert(wasFound == TRUE);

                memcpy(&(thisPhy->snrBerTables[i]),
                       PHY_GetSnrBerTableByName(berTableInput.ourName),
                       sizeof(PhyBerTable));
            }
        }
#ifdef ADDON_BOEINGFCS
        else if (strcmp(buf, "PCOM-BASED") == 0) {
            int i;
            BOOL retVal;
            thisPhy->phyRxModel = PCOM_BASED;

                //read in min-pcom-value
            double minPcom = PHY_DEFAULT_MIN_PCOM_VALUE;
            IO_ReadDouble(node->nodeId,
                          networkAddress,
                          nodeInput,
                          "MIN-PCOM-VALUE",
                          &retVal,
                          &minPcom);
            thisPhy->minPcomValue = minPcom;

            clocktype collisionWindow = PHY_DEFAULT_SYNC_COLLISION_WINDOW;
            IO_ReadTime(node->nodeId,
                        networkAddress,
                        nodeInput,
                        "SYNC-COLLISION-WINDOW",
                        &retVal,
                        &collisionWindow);
            thisPhy->syncCollisionWindow = collisionWindow;
            thisPhy->collisionWindowActive = FALSE;

                //read in file, generate data structure
            thisPhy->pcomTable =
                    (PhyPcomItem**)MEM_malloc((node->numNodes + 1) *
                    sizeof(PhyPcomItem*));
            for (i = 0; i < node->numNodes; i++)
            {
                thisPhy->pcomTable[i] = NULL;
            }

            //char clockStr[MAX_STRING_LENGTH];
            //TIME_PrintClockInSecond(collisionWindow, clockStr);
            //printf("Node %d ~ min-pcom: %f, sync-collision-window: %s\n",
            //       node->nodeId, minPcom, clockStr);
        }
#endif
        else {
            char errorbuf[MAX_STRING_LENGTH];
            sprintf(errorbuf, "Unknown PHY-RX-MODEL %s.\n", buf);
            ERROR_ReportError(errorbuf);
        }
    }
    else {
        ERROR_ReportError("PHY-RX-MODEL is missing.");
    }


    //
    // Stats option
    //
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-LAYER-STATISTICS",
        &wasFound,
        buf);

    if (wasFound) {
        if (strcmp(buf, "YES") == 0) {
            thisPhy->phyStats = TRUE;
        }
        else if (strcmp(buf, "NO") == 0) {
            thisPhy->phyStats = FALSE;
        }
        else {
            fprintf(stderr, "%s is not a valid choice.\n", buf);
            assert(FALSE);
        }
    }
    else {
        thisPhy->phyStats = FALSE;
    }

    RANDOM_SetSeed(thisPhy->seed,
                   node->globalSeed,
                   node->nodeId,
                   interfaceIndex,
                   *phyNumber);

    //
    // Energy Stats option
    //
    IO_ReadString(
        node->nodeId,
        networkAddress,
        nodeInput,
        "ENERGY-MODEL-STATISTICS", &wasFound,
        buf);

    if (wasFound) {
        if (strcmp(buf, "YES") == 0) {
            thisPhy->energyStats = TRUE;
        }
        else if (strcmp(buf, "NO") == 0) {
            thisPhy->energyStats = FALSE;
        }
        else {
            fprintf(stderr, "%s is not a valid choice.\n", buf);
            assert(FALSE);
        }
    }
    else {
        thisPhy->energyStats = FALSE;
    }


//

    // PHY model initialization
    //
    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11Init(node, phyIndex, nodeInput);
            break;
        }
				case PHY_CHANSWITCH: {
						PhyChanSwitchInit(node, phyIndex, nodeInput);
						break;
				}
        case PHY_ABSTRACT: {
            PhyAbstractInit(node, phyIndex, nodeInput);

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4: {
            Phy802_15_4Init(node, phyIndex, nodeInput);

            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmInit(node, phyIndex, nodeInput);

            break;
        }
        case PHY_CELLULAR: {
             PhyCellularInit(node, phyIndex, nodeInput);

             break;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
             PhyCellularInit(node, phyIndex, nodeInput);

             break;
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            PhyFcscInit(node, phyIndex, nodeInput);

            break;
        }
        case PHY_JTRS_WNW: {
            PhyJtrsInit(node, phyIndex, nodeInput);
            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16Init(node, phyIndex, nodeInput);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvInit(node, phyIndex, nodeInput);
            break;
        }
#endif

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            if (thisPhy->phyRxModel != RX_802_16) {
                char errorStr[MAX_STRING_LENGTH];
                 sprintf(errorStr,
                    "\nPHY802.16: Specified PHY-RX-MODEL is not supported!\n"
                    "Please specify PHY802.16 for it.");
                ERROR_ReportError(errorStr);
           }

            PhyDot16Init(node, phyIndex, nodeInput);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingInit(node, phyIndex, nodeInput);
            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model");
        }
    }/*switch*/

#ifdef WIRELESS_LIB
    // ENERGY model initialization
    //
    ENERGY_Init(node, phyIndex, nodeInput);
    //
#endif // WIRELESS_LIB

// Enabled all the time.
// Remove code if no issues are found
#if 0
    // check if the contention free propagation abstraction is enabled
    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "ABSTRACT-CONTENTION-FREE-PROPAGATION",
                  &wasFound,
                  buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            thisPhy->contentionFreeProp = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            thisPhy->contentionFreeProp = FALSE;
        }
        else
        {
            ERROR_ReportError("Value of the parameter "
                "ABSTRACT-CONTENTION-FREE-PROPAGATION must be YES or NO");
        }
    }
    else
    {
        // default value is NO
        thisPhy->contentionFreeProp = FALSE;
    }
#endif

#ifdef ADDON_NGCNMS
    PowerControlInit(node, phyIndex, nodeInput);
#endif
} //PHY_CreateAPhyForMacLayer//


#ifdef ADDON_BOEINGFCS
//initialize pcom data structure
void PHY_PcomInit(PartitionData *partitionData)
{
    int i, j;
    BOOL wasFound;
    NodeInput pcomTableInput;
    Node *node = partitionData->firstNode;

    while (node)
    {
        for (i = 0; i < node->numberInterfaces; i++)
        {
            PhyData *thisPhy = node->phyData[i];
            if (thisPhy && thisPhy->phyRxModel == PCOM_BASED)
            {
                IO_ReadCachedFile(
                        node->nodeId,
                        thisPhy->networkAddress,
                        partitionData->nodeInput,
                        "PHY-PCOM-FILE",
                        &wasFound,
                        &pcomTableInput);

                ERROR_Assert(wasFound == TRUE, "Need to specify PHY-PCOM-FILE!");

                for (j = 0; j < pcomTableInput.numLines; j++) {
                    NodeId srcNodeId;
                    NodeId destNodeId;
                    NodeAddress sourceAddr;
                    NodeAddress destAddr;
                    NodeAddress networkId;
                    int numHostBits;
                    char iotoken[MAX_STRING_LENGTH];
                    char currentLine[MAX_STRING_LENGTH];
                    char networkIdStr[MAX_STRING_LENGTH];
                    char *delims = " \t\n";
                    char *token = NULL;
                    char *next;
                    char *strPtr;

                    clocktype startTime;
                    double pcomValue;

                    BOOL isValid;
                    BOOL validEntry = TRUE;
                    BOOL inSubnet = FALSE;

                    strcpy(currentLine, pcomTableInput.inputStrings[j]);
                    //get the start time for this pcom value
                    token = IO_GetDelimitedToken(iotoken, currentLine, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    startTime = TIME_ConvertToClock(token);

                    //get the network id
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    strcpy(networkIdStr, token);

                    IO_ParseNodeIdHostOrNetworkAddress(
                            networkIdStr,
                            &networkId,
                            &numHostBits,
                            &isValid);

                    //save network Id into networkId

                    //get the source node
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    srcNodeId = atoi(token);

                    //get the destination node
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    destNodeId = atoi(token);

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    pcomValue = (double)atof(token);

                    PhyPcomItem *index;
                    PhyPcomItem *iterator;
                    PhyPcomItem *temp;
                    NodeId otherNodeId;
                    int ifaceIndex;

                    //remember, we are creating a table at the receiver for
                    //all potential transmitting nodes
                    //create and store data structure if this line
                    //has pcom data for this node
                    //assume links are symmetric
                    if ((srcNodeId == node->nodeId || destNodeId == node->nodeId) &&
                         (srcNodeId != destNodeId))
                    {
                        //printf("Node %d init: reading in nodes %d to %d\n",
                        //       node->nodeId, srcNodeId, destNodeId);
                        NodeAddress myAddress;
                        if (srcNodeId == node->nodeId)
                        {
                            otherNodeId = destNodeId;
                            myAddress =
                                    MAPPING_GetInterfaceAddressForInterface(node,
                                                                            srcNodeId,
                                                                            i);
                            numHostBits = MAPPING_GetNumHostBitsForInterface(node,
                                                                             srcNodeId,
                                                                             i);
                        }
                        else if (destNodeId == node->nodeId)
                        {
                            otherNodeId = srcNodeId;
                            myAddress =
                                    MAPPING_GetInterfaceAddressForInterface(node,
                                                                            destNodeId,
                                                                            i);
                            numHostBits = MAPPING_GetNumHostBitsForInterface(node,
                                                                             destNodeId,
                                                                             i);
                        }

                        //check to see if we are in the subnet
                        if (!IsIpAddressInSubnet(myAddress, networkId, numHostBits))
                        {
                            //printf("not valid address for subnet!\n");
                            validEntry = FALSE;
                        }

                        //this doesnt work in parallel
                        Node *otherNode = MAPPING_GetNodePtrFromHash(
                                                partitionData->nodeIdHash,
                                                otherNodeId);

#ifdef PARALLEL
                        if (otherNode == NULL)
                        {
                            //printf("pointer to node %d is null\n", otherNodeId);
                            //it might be in another partition
                            Node *oNode = MAPPING_GetNodePtrFromHash(
                                            partitionData->remoteNodeIdHash,
                                            otherNodeId);
                            if (oNode)
                            {
                                otherNode = oNode;
                            }
                        }
#endif

                        //check that other node is not null
                        if (otherNode)
                        {

                            //now that we have a pointer to the other node, we should loop through
                            //the interfaces and get the interfaceAddress for each one
                            //then we can see if the node has an interface in the subnet
                            //printf("number phys %d, interfaces %d\n", node->numberPhys, node->numberInterfaces);
                            //printf("finding interface address for %d(%d-%d)\n", otherNode->nodeId, otherNode->numberPhys, otherNode->numberInterfaces);
                            for (ifaceIndex = 0; ifaceIndex < otherNode->numberInterfaces; ifaceIndex++)
                            {
                                NodeAddress otherAddress =
                                        MAPPING_GetInterfaceAddressForInterface(otherNode,
                                        otherNodeId,
                                        ifaceIndex);
                                char othernodeaddress[MAX_STRING_LENGTH];
                                IO_ConvertIpAddressToString(otherAddress, othernodeaddress);
                                //printf("  interface %d ~ %s\n", ifaceIndex, othernodeaddress);
                                if (IsIpAddressInSubnet(otherAddress, networkId, numHostBits))
                                {
                                    //printf("valid!\n");
                                    //they are in the subnet
                                    inSubnet = TRUE;
                                    break;
                                }
                            }

                            if (validEntry && inSubnet)
                            {

                                temp = (PhyPcomItem*) MEM_malloc(sizeof(PhyPcomItem));
                                temp->startTime = startTime;
                                temp->endTime = CLOCKTYPE_MAX;
                                temp->pcom = pcomValue;
                                temp->next = NULL;

                                char clockStr[MAX_STRING_LENGTH];
                                TIME_PrintClockInSecond(startTime, clockStr);
                                //printf("  pcom table entry ~ time: %s, nodes %d to %d, pcom: %f\n",
                                //       clockStr, srcNodeId, destNodeId, pcomValue);

                                if (thisPhy->pcomTable[otherNodeId] == NULL) //first entry
                                {
                                    thisPhy->pcomTable[otherNodeId] = temp;
                                }
                                else //loop through linked list until we get to last entry
                                {
                                    iterator = thisPhy->pcomTable[otherNodeId];
                                    while (iterator)
                                    {
                                        index = iterator;
                                        iterator = iterator->next;
                                    }

                                    if (index->startTime >= temp->startTime)
                                    {
                                    //time is not in order, discard this entry
                                        ERROR_ReportWarning("Out of order time value");
                                        MEM_free(temp);
                                    }
                                    else
                                    {
                                        index->endTime = temp->startTime;
                                        index->next = temp;
                                        char clockStr[MAX_STRING_LENGTH];
                                        TIME_PrintClockInSecond(startTime, clockStr);
                                        //printf("Node %d added entry for node %d: time %s, pcom %f\n",
                                        //       node->nodeId, otherNodeId, clockStr, pcomValue);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        node = node->nextNodeData;
    }
}
#endif
/*
 * FUNCTION    PHY_Finalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of the Phy Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */
void PHY_Finalize(Node *node) {
    int phyNum;
#ifdef ADDON_BOEINGFCS
    int i;
#endif
    for (phyNum = 0; (phyNum < node->numberPhys); phyNum++) {
#ifdef ADDON_BOEINGFCS
        //free pcom table
        if (node->phyData[phyNum]->phyRxModel == PCOM_BASED)
        {
            PhyPcomItem *iterator;
            PhyPcomItem *temp;
            for (i = 0; i < node->numNodes; i++)
            {
                iterator = node->phyData[phyNum]->pcomTable[i];
                while (iterator)
                {
                    temp = iterator;
                    iterator = iterator->next;
                    MEM_free(temp);
                }
                node->phyData[phyNum]->pcomTable[i] = NULL;
            }
            MEM_free(node->phyData[phyNum]->pcomTable);
        }
#endif
        switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
            case PHY802_11b:
            case PHY802_11a: {
                Phy802_11Finalize(node, phyNum);

                break;
            }
						case PHY_CHANSWITCH: {
                PhyChanSwitchFinalize(node, phyNum);

                break;
            }
            case PHY_ABSTRACT: {
                PhyAbstractFinalize(node, phyNum);

                break;
            }
#endif // WIRELESS_LIB

#ifdef CELLULAR_LIB
            case PHY_GSM: {
                PhyGsmFinalize(node, phyNum);

                break;
            }
            case PHY_CELLULAR: {
                PhyCellularFinalize(node, phyNum);

                break;
            }
#elif UMTS_LIB
            case PHY_CELLULAR: {
                PhyCellularFinalize(node, phyNum);

                break;
            }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
            case PHY_FCSC_PROTOTYPE: {
                PhyFcscFinalize(node, phyNum);

                break;
            }
            case PHY_JTRS_WNW: {
                PhyJtrsFinalize(node, phyNum);
                break;
            }
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_LINK16
            case PHY_SPAWAR_LINK16: {
                PhySpawar_Link16Finalize(node, phyNum);

                break;
            }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
            case PHY_SATELLITE_RSV: {
                PhySatelliteRsvFinalize(node, phyNum);
                break;
            }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
            case PHY802_16: {
                PhyDot16Finalize(node, phyNum);
                break;
            }
#endif // ADVANCED_WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
            case PHY802_15_4: {
                Phy802_15_4Finalize(node, phyNum);
                break;
            }
#endif // SENSOR_NETWORKS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
            case PHY_JAMMING: {
                PhyJammingFinalize(node, phyNum);
                break;
            }
#endif // PHY_SYNC
#endif // NETSEC_LIB
            default: {
                ERROR_ReportError("Unknown or disabled PHY model");
            }
        }
    ENERGY_PrintStats(node,phyNum);
#ifdef ADDON_NGCNMS
        PowerControlFinalize(node, phyNum);
#endif
    }
}



/*
 * FUNCTION    PHY_ProcessEvent
 * PURPOSE     Models the behaviour of the Phy Layer on receiving the
 *             message encapsulated in msgHdr
 *
 * Parameters:
 *     node:     node which received the message
 *     msgHdr:   message received by the layer
 */
void PHY_ProcessEvent(Node *node, Message *msg) {
    int phyIndex = MESSAGE_GetInstanceId(msg);

#ifdef ADDON_NGCNMS
    PropFlatBinningUpdateNodePosition(node, phyIndex);
#endif

    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    Phy802_11TransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
        case PHY_CHANSWITCH: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyChanSwitchTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
        case PHY_ABSTRACT: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyAbstractTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }
                case MSG_PHY_CollisionWindowEnd: {
                    PhyAbstractCollisionWindowEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
       case PHY802_15_4: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    Phy802_15_4TransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyGsmTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                 default: abort();
            }

            break;
        }
        case PHY_CELLULAR: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyCellularTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                 default: abort();
           }

            break;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyCellularTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                 default: abort();
           }

            break;
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyFcscTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }

        case PHY_JTRS_WNW: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyJtrsTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);
                    break;
                }
                default: abort();
            }
            break;
        }
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhySpawar_Link16TransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhySatelliteRsvTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyDot16TransmissionEnd(node, phyIndex, msg);
                    MESSAGE_Free(node, msg);
                    break;
                }
                default: abort();
            }
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyJammingTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);
                    break;
                }
                default: abort();
            }
            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default:
            ERROR_ReportError("Unknown or disabled PHY model\n");
    }
}

//
// FUNCTION    PHY_GetStatus
// PURPOSE     Retrieves the Phy's current status.
//

PhyStatusType
PHY_GetStatus(Node *node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetStatus(node, phyNum);

            break;
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchGetStatus(node, phyNum);

            break;
        }
        case PHY_ABSTRACT: {
            return PhyAbstractGetStatus(node, phyNum);

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4: {
            return Phy802_15_4GetStatus(node, phyNum);

            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            return PhyGsmGetStatus(node, phyNum);

            break;
        }
        case PHY_CELLULAR: {
            return PhyCellularGetStatus(node, phyNum);

            break;
         }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            return PhyCellularGetStatus(node, phyNum);

            break;
         }
#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            return PhySpawar_Link16GetStatus(node, phyNum);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetStatus(node, phyNum);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            return PhyDot16GetStatus(node, phyNum);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            return PhyJtrsGetStatus(node, phyNum);
            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            return PhyJammingGetStatus(node, phyNum);
            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }

    // Never reachable, but required to suppress a warning by some compilers.
    return (PhyStatusType) 0;
}

// /**
// FUNCTION   :: AbstractAddCFPropInfo
// LAYER      :: Physical
// Purpose    :: Add the corresponding info field to indicate that a packet
//               wants to be treated using the abstract contention free
//               propagation.
// PARAMETERS ::
// + node      : Node *      : pointer to node
// + msg       : Message *   : Packet to be sent
// + destAddr  : NodeAddress : destination (next hop) of this packet
// RETURN     :: void        : NULL
// **/
static
void AbstractAddCFPropInfo(Node* node, Message *msg, NodeAddress destAddr)
{
    NodeId destId;
    char* infoPtr;

    //destId = MAPPING_GetNodeIdFromInterfaceAddress(node, destAddr);
    destId = destAddr;

    infoPtr = MESSAGE_AddInfo(node,
                              msg,
                              sizeof(NodeId),
                              INFO_TYPE_AbstractCFPropagation);

    ERROR_Assert(infoPtr != NULL, "Unable to add an info field!");

    memcpy(infoPtr, (char*) &destId, sizeof(NodeId));
}


// /**
// API                        :: PHY_StartTransmittingSignal
// LAYER                      :: Physical
// PURPOSE                    :: starts transmitting a packet,
//                               accepts a parameter called duration which
//                               specifies transmission delay.Used for
//                               non-byte aligned messages.
//                               Function is being overloaded
// PARAMETERS                 ::
// + node                      : Node *    : node pointer to node
// + phyNum                    : int       : interface index
// + msg                       : Message*  : packet to be sent
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + delayUntilAirborne        : clocktype : delay until airborne
// RETURN                     :: void
// **/
void PHY_StartTransmittingSignal(
    Node *node,
    int phyNum,
    Message *msg,
    clocktype duration,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    NodeAddress destAddr)
{

    // check if need abstract contention free propagation
    if (node->phyData[phyNum]->contentionFreeProp && destAddr != ANY_DEST)
    {
        AbstractAddCFPropInfo(node, msg, destAddr);
    }

    switch(node->phyData[phyNum]->phyModel) {

#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
        //As of now only PHY abstract has been modified to carry
        //transmission delay

        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB

      case PHY802_15_4: {
            Phy802_15_4StartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif //
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
         }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
         }
#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignal(node, phyNum, msg,
              useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16StartTransmittingSignal(node, phyNum, msg,
              useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            PhyJtrsStartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingStartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
            break;
        }
    }//switch//
}


//
// FUNCTION    PHY_StartTransmittingSignal
// PURPOSE     Starts transmitting a packet.
//

void PHY_StartTransmittingSignal(
    Node *node,
    int phyNum,
    Message *msg,
    int bitSize,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    NodeAddress destAddr)
{

    // check if need abstract contention free propagation
    if (node->phyData[phyNum]->contentionFreeProp && destAddr != ANY_DEST)
    {
        AbstractAddCFPropInfo(node, msg, destAddr);
    }

    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignal(
                node, phyNum, msg, bitSize,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB

       case PHY802_15_4: {
            Phy802_15_4StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, 0,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, 0,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignal(
                                                node, phyNum, msg,
                                                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16StartTransmittingSignal(
                  node, phyNum, msg,
                  useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            PhyJtrsStartTransmittingSignal(
                node, phyNum, msg, bitSize,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingStartTransmittingSignal(
                node, phyNum, msg, bitSize,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}

//
// FUNCTION    PHY_StartTransmittingSignal
// PURPOSE     Starts transmitting a packet.
//

void PHY_StartTransmittingSignal(
    Node *node,
    int phyNum,
    Message *msg,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    NodeAddress destAddr)
{

    // check if need abstract contention free propagation
    if (node->phyData[phyNum]->contentionFreeProp && destAddr != ANY_DEST)
    {
        AbstractAddCFPropInfo(node, msg, destAddr);
    }
#ifdef ADDON_NGCNMS
    NgcStatsHist(node, node->phyData[phyNum]->phyIndex, msg);
#endif

    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }

        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB

      case PHY802_15_4: {
            Phy802_15_4StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, 0,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignal(
                                                node, phyNum, msg,
                                                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16StartTransmittingSignal(
                  node, phyNum, msg,
                  useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            PhyJtrsStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}


void PHY_SignalArrivalFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo
    )
{
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_ABSTRACT: {
#ifdef ADDON_NGCNMS
            BOOL msgSendIgnore;
#endif
            PhyAbstractSignalArrivalFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo
#ifdef ADDON_NGCNMS
                , &msgSendIgnore
#endif
                );

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
       case PHY802_15_4: {
            Phy802_15_4SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }

#endif
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_CELLULAR: {
            PhyCellularSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            PhyFcscSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif /* FCSC */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            PhyJtrsSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}


void PHY_SignalEndFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo)
{
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
         case PHY802_15_4: {
            Phy802_15_4SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_CELLULAR: {
            PhyCellularSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            PhyFcscSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSignalEndFromChannel(node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // SATELLITE_LIB_RSV

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            PhyJtrsSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}

void PHY_ChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening)
{
    switch (node->phyData[phyIndex]->phyModel) {
#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {

            PhyDot16ChannelListeningSwitchNotification(
                node,
                phyIndex,
                channelIndex,
                startListening);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB


#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11ChannelListeningSwitchNotification(
                node,
                phyIndex,
                channelIndex,
                startListening);
            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchChannelListeningSwitchNotification(
                node,
                phyIndex,
                channelIndex,
                startListening);
            break;
        }
        case PHY_ABSTRACT:{
            PhyAbstractChannelListeningSwitchNotification(
                node,
                phyIndex,
                channelIndex,
                startListening);
            break;
        }
#endif // WIRELESS_LIB

        default: {
            // the PHY model doesn't want to response to this notification
            break;
        }
    }
}

int PHY_GetTxDataRate(Node *node, int phyIndex) {
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetTxDataRate(node->phyData[phyIndex]);
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchGetTxDataRate(node->phyData[phyIndex]);
        }
        case PHY_ABSTRACT: {
            return PhyAbstractGetDataRate(node->phyData[phyIndex]);
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
         case PHY802_15_4: {
            return Phy802_15_4GetDataRate(node->phyData[phyIndex]);
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            return PhyGsmGetDataRate(node->phyData[phyIndex]);
        }
        case PHY_CELLULAR: {
            return PhyCellularGetDataRate(node, node->phyData[phyIndex]);
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            return PhyCellularGetDataRate(node, node->phyData[phyIndex]);
        }
#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            return PhySpawar_Link16GetDataRate(node->phyData[phyIndex]);
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetTxDataRate(node->phyData[phyIndex]);
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            return PhyDot16GetTxDataRate(node->phyData[phyIndex]);
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            return PhyJtrsGetDataRate(node->phyData[phyIndex]);
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            return PhyJammingGetDataRate(node->phyData[phyIndex]);
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    // Never reachable, but required to suppress a warning by some compilers.
    return 0;
}



int PHY_GetRxDataRate(Node *node, int phyIndex) {
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetRxDataRate(node->phyData[phyIndex]);
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchGetRxDataRate(node->phyData[phyIndex]);
        }
        case PHY_ABSTRACT: {
            return PhyAbstractGetDataRate(node->phyData[phyIndex]);
        }
#endif // WIRELESS_LIB

#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4: {
            return Phy802_15_4GetDataRate(node->phyData[phyIndex]);
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            return PhyGsmGetDataRate(node->phyData[phyIndex]);
        }
        case PHY_CELLULAR: {
            return PhyCellularGetDataRate(node, node->phyData[phyIndex]);
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            return PhyCellularGetDataRate(node, node->phyData[phyIndex]);
        }
#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            return PhySpawar_Link16GetDataRate(node->phyData[phyIndex]);
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetRxDataRate(node->phyData[phyIndex]);
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            return PhyDot16GetRxDataRate(node->phyData[phyIndex]);
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            return PhyJtrsGetDataRate(node->phyData[phyIndex]);
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            return PhyJammingGetDataRate(node->phyData[phyIndex]);
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    // Never reachable, but required to suppress a warning by some compilers.
    return 0;
}


void PHY_SetTxDataRateType(Node *node, int phyIndex, int dataRateType) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSetTxDataRateType(thisPhy, dataRateType);
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetTxDataRateType(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default:{
        }
    }
}


int PHY_GetRxDataRateType(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetRxDataRateType(thisPhy);
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchGetRxDataRateType(thisPhy);
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetRxDataRateType(thisPhy);
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
    return 0;
}


int PHY_GetTxDataRateType(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetTxDataRateType(thisPhy);
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchGetTxDataRateType(thisPhy);
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetTxDataRateType(thisPhy);
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
    return 0;
}


void PHY_SetLowestTxDataRateType(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetLowestTxDataRateType(thisPhy);
            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSetLowestTxDataRateType(thisPhy);
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetLowestTxDataRateType(thisPhy);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_GetLowestTxDataRateType(Node* node, int phyIndex, int* dataRateType) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetLowestTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchGetLowestTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY_ABSTRACT: {
            *dataRateType = 0;
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetLowestTxDataRateType(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_SetHighestTxDataRateType(Node* node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetHighestTxDataRateType(thisPhy);
            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSetHighestTxDataRateType(thisPhy);
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetHighestTxDataRateType(thisPhy);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_GetHighestTxDataRateType(Node* node, int phyIndex, int* dataRateType) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetHighestTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchGetHighestTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY_ABSTRACT: {
            *dataRateType = 0;
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetHighestTxDataRateType(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_SetHighestTxDataRateTypeForBC(
    Node* node,
    int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetHighestTxDataRateTypeForBC(thisPhy);
            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSetHighestTxDataRateTypeForBC(thisPhy);
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetHighestTxDataRateTypeForBC(thisPhy);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_GetHighestTxDataRateTypeForBC(
    Node* node,
    int phyIndex,
    int* dataRateType)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetHighestTxDataRateTypeForBC(thisPhy, dataRateType);
            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchGetHighestTxDataRateTypeForBC(thisPhy, dataRateType);
            return;
        }
        case PHY_ABSTRACT: {
            *dataRateType = 0;
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetHighestTxDataRateTypeForBC(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}

PhyModel PHY_GetModel(
    Node* node,
    int phyNum)
{
    return node->phyData[phyNum]->phyModel;
}


//
// FUNCTION    PHY_GetTransmissionDuration
// PURPOSE     Calculates the transmission duration
//
clocktype PHY_GetTransmissionDuration(
    Node *node,
    int phyIndex,
    int dataRateIndex,
    int size)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetFrameDuration(thisPhy, dataRateIndex, size);
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchGetFrameDuration(thisPhy, dataRateIndex, size);
        }
        case PHY_ABSTRACT: {
            int dataRate = PHY_GetTxDataRate(node, phyIndex);
            return PhyAbstractGetFrameDuration(thisPhy, size, dataRate);
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
      case PHY802_15_4: {
            int dataRate = PHY_GetTxDataRate(node, phyIndex);
            PhyData802_15_4* phy802_15_4 = (PhyData802_15_4*)thisPhy->phyVar;
            clocktype turnaroundTime = phy802_15_4->RxTxTurnAroundTime;
            return Phy802_15_4GetFrameDuration(thisPhy, size, dataRate) +
                    turnaroundTime;
        }
#endif
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            int dataRate;
            clocktype delay;
            dataRate = PHY_GetTxDataRate(node, phyIndex);
            delay = PhyGsmGetTransmissionDuration(size, dataRate);
            return delay;
        }
        case PHY_CELLULAR: {
            // not implemented yet.
            clocktype delay;
            delay = 10000;
            return delay;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            // not implemented yet.
            clocktype delay;
            delay = 10000;
            return delay;
        }
#endif // CELLULAR_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            int dataRate;
            clocktype delay;

            dataRate = PHY_GetTxDataRate(node, phyIndex);
            delay= PhySatelliteRsvGetTransmissionDuration(size, dataRate);

            return delay;
        }
#endif // SATELLITE_LIB
#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            // not implemented yet.
            return 10000;
        }
#endif // ADVANCED_WIRELESS_LIB
#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            int dataRate = PHY_GetTxDataRate(node, phyIndex);
            return PhyJtrsGetFrameDuration(thisPhy, size, dataRate);
        }
#endif // MILITARY_RADIOS_LIB
#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            int dataRate = PHY_GetTxDataRate(node, phyIndex);
            return PhyJammingGetFrameDuration(thisPhy, size, dataRate);
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    // Never reachable, but required to suppress a warning by some compilers.
    return 0;
}


//
// FUNCTION    PHY_GetTransmissionDelay
// PURPOSE     Calculates the transmission duration
//             based on the first (usually lowest) data rate
//             WARNING: This function call is to be replaced with
//             PHY_GetTransmissionDuration() with an appropriate data rate
//
clocktype PHY_GetTransmissionDelay(Node *node, int phyIndex, int size) {

    PhyData* thisPhy = node->phyData[phyIndex];
    int txDataRateType = 0;

#ifdef WIRELESS_LIB
    if (thisPhy->phyModel == PHY802_11b || thisPhy->phyModel == PHY802_11a)
    {
        txDataRateType = Phy802_11GetTxDataRateType(thisPhy);
    }
		else if (thisPhy->phyModel == PHY_CHANSWITCH)
    {
        txDataRateType = PhyChanSwitchGetTxDataRateType(thisPhy);
    }
#endif // WIRELESS_LIB

    return PHY_GetTransmissionDuration(node, phyIndex, txDataRateType, size);
}



//
// FUNCTION    PHY_SetTransmitPower
// PURPOSE     Sets the Radio's transmit power in mW.
//
void PHY_SetTransmitPower(
    Node *node,
    int phyIndex,
    double newTxPower_mW)
{
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }

        case PHY_ABSTRACT: {
            PhyAbstractSetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
   case PHY802_15_4: {
            Phy802_15_4SetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }
#endif
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmSetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }
        case PHY_CELLULAR: {
            PhyCellularSetTransmitPower(node, node->phyData[phyIndex], newTxPower_mW);

            return;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularSetTransmitPower(node, node->phyData[phyIndex], newTxPower_mW);

            return;
        }

#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16SetTransmitPower(node->phyData[phyIndex],
                                      newTxPower_mW);


            return;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetTransmitPower(node->phyData[phyIndex],
                                         newTxPower_mW);

            return;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16SetTransmitPower(node, phyIndex,
                                         newTxPower_mW);

            return;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            PhyJtrsSetTransmitPower(node->phyData[phyIndex], newTxPower_mW);
            return;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingSetTransmitPower(node->phyData[phyIndex],
                                       newTxPower_mW);
            return;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    return;
}



//
// FUNCTION    PHY_GetTransmitPower
// PURPOSE     Gets the Radio's transmit power in mW.
//
void PHY_GetTransmitPower(
    Node *node,
    int phyIndex,
    double *txPower_mW)
{
    if (node->numberPhys < 1)
        {
        *txPower_mW = -1.0;
        return;
        }
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchGetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }

        case PHY_ABSTRACT: {
            PhyAbstractGetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
     case PHY802_15_4: {
            Phy802_15_4GetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmGetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
        case PHY_CELLULAR: {
            PhyCellularGetTransmitPower(node, node->phyData[phyIndex], txPower_mW);

            return;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularGetTransmitPower(node, node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif // CELLULAR_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16GetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16GetTransmitPower(node, phyIndex, txPower_mW);

            return;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            PhyJtrsGetTransmitPower(node->phyData[phyIndex], txPower_mW);
            return;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            PhyJammingGetTransmitPower(node->phyData[phyIndex], txPower_mW);
            return;
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    return;
}



double PHY_GetLastSignalsAngleOfArrival(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetLastAngleOfArrival(node, phyNum);

            break;
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchGetLastAngleOfArrival(node, phyNum);

            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetLastAngleOfArrival(node, phyNum);

            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            return PhyAbstractGetLastAngleOfArrival(node, phyNum);
            break;
        }
#endif
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }
    // Never reachable, but required to suppress a warning by some compilers.
    return 0.0;
}


void PHY_TerminateCurrentReceive(
    Node* node, int phyIndex, const BOOL terminateOnlyOnReceiveError,
    BOOL* receiveError, clocktype* endSignalTime)
{
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11TerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);

            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchTerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);

            break;
        }

       case PHY_ABSTRACT:
        {
            PhyAbstractTerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);
        }
        break;
#endif // WIRELESS_LIB
#ifdef ADVANCED_WIRELESS_LIB
       case PHY802_16:
        {
                PhyDot16TerminateCurrentReceive(node,
                                                phyIndex,
                                                terminateOnlyOnReceiveError,
                                                receiveError,
                                                endSignalTime);
                break;
        }
#endif // ADVANCED_WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
     case PHY802_15_4:
        {
            Phy802_15_4TerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);

            break;
        }
#endif
#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvTerminateCurrentReceive(
                 node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                 endSignalTime);

            break;
        }
#endif // SATELLITE_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW:
        {
            PhyJtrsTerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);
        }
        break;
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING:
        {
            PhyJammingTerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);
        }
        break;
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


//
// FUNCTION    PHY_StartTransmittingDirectionally
// PURPOSE     Starts transmitting a packet using directional antenna.
//

void PHY_StartTransmittingSignalDirectionally(
    Node *node,
    int phyNum,
    Message *msg,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    double azimuthAngle)
{
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignalDirectionally(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay,
                delayUntilAirborne,
                azimuthAngle);

            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchStartTransmittingSignalDirectionally(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay,
                delayUntilAirborne,
                azimuthAngle);

            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignalDirectionally(
                                                             node, phyNum, msg,
                                                             useMacLayerSpecifiedDelay,
                                                             delayUntilAirborne,
                                                             azimuthAngle);

            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignalDirectionally(
                    node, phyNum, msg,
                    useMacLayerSpecifiedDelay,
                    delayUntilAirborne,
                    azimuthAngle);
            break;
        }
#endif
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


//
// FUNCTION    PHY_LockAntennaDirection
// PURPOSE     Tells smart radios to lock the antenna direction
//             to the last received packet.
//
void PHY_LockAntennaDirection(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11LockAntennaDirection(node, phyNum);
            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchLockAntennaDirection(node, phyNum);
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvLockAntennaDirection(node, phyNum);
            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractLockAntennaDirection(node, phyNum);
            break;
        }
#endif
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


//
// FUNCTION    PHY_UnlockAntennaDirection
// PURPOSE     Tells smart radios to unlock the antenna direction
//             (go back to omnidirectional).
//
void PHY_UnlockAntennaDirection(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11UnlockAntennaDirection(node, phyNum);
            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchUnlockAntennaDirection(node, phyNum);
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvUnlockAntennaDirection(node, phyNum);
            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractUnlockAntennaDirection(node, phyNum);
            break;
        }
#endif

        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}

BOOL PHY_MediumIsIdle(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11MediumIsIdle(node, phyNum);
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchMediumIsIdle(node, phyNum);
        }
        case PHY_ABSTRACT: {
            return PhyAbstractMediumIsIdle(node, phyNum);
        }
#endif // WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW: {
            return PhyJtrsMediumIsIdle(node, phyNum);
        }
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING: {
            return PhyJammingMediumIsIdle(node, phyNum);
        }
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//

    // Should not get here.
    return FALSE;
}

BOOL PHY_MediumIsIdleInDirection(Node* node, int phyNum, double azimuth) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11MediumIsIdleInDirection(node, phyNum, azimuth);
        }
        case PHY_CHANSWITCH: {
            return PhyChanSwitchMediumIsIdleInDirection(node, phyNum, azimuth);
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvMediumIsIdleInDirection(node, phyNum, azimuth);
        }
#endif // SATELLITE_LIB

#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            return PhyAbstractMediumIsIdleInDirection(node, phyNum, azimuth);
        }
#endif
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//

    // Should not get here.
    return FALSE;
}

void PHY_SetSensingDirection(Node* node, int phyNum, double azimuth) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetSensingDirection(node, phyNum, azimuth);
            break;
        }
        case PHY_CHANSWITCH: {
            PhyChanSwitchSetSensingDirection(node, phyNum, azimuth);
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetSensingDirection(node, phyNum, azimuth);
            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractSetSensingDirection(node, phyNum, azimuth);
            break;
        }
#endif

        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


#define LONGEST_DISTANCE 1000000.0  // 1000km
#define DELTA            0.001      // 1mm
#define PACKET_SIZE      (1024 * 8) // 1kbytes
#define RANGE_TO_RETURN_IN_CASE_OF_ERROR 350.0

double PHY_PropagationRange(Node* node,
                            int   interfaceIndex,
                            BOOL  printAllDataRates) {
#ifdef WIRELESS_LIB
// if printAll is true, print out the range for all data rates because
// it's being called by radio_range.  Otherwise, figure out the lowest
// data rate and return the range for that.

    PropProfile*    propProfile;
    int             radioNumber = interfaceIndex;
    PhyData*        thisRadio;

    int             numIterations;
    int             index;

    PhyModel         phyModel = PHY_NONE;
    PhyRxModel       phyRxModel;

#ifdef CELLULAR_LIB
    PhyDataGsm*      phyGsm;
    PhyCellularData* phyCellular;
#elif UMTS_LIB
    PhyCellularData* phyCellular;
#endif // CELLULAR_LIB

#ifdef ADVANCED_WIRELESS_LIB
    PhyDataDot16*    phy802_16 = NULL;
#endif

    PhyData802_11*   phy802_11 = NULL;
    PhyDataChanSwitch*   phyChanSwitch = NULL;
    PhyDataAbstract* phyAbstract;
#ifdef SENSOR_NETWORKS_LIB
    PhyData802_15_4* phy802_15_4;
#endif //SENSOR_NETWORKS_LIB
#ifdef MILITARY_RADIOS_LIB
    PhyDataJtrs*     phyJtrs;
#endif // MILITARY_RADIOS_LIB
#ifdef NETSEC_LIB
#ifdef PHY_SYNC
    PhyJammingData* phyJamming;
#endif // PHY_SYNC
#endif // NETSEC_LIB
    AntennaOmnidirectional* omniDirectional;
    AntennaSwitchedBeam* switchedBeam;
    AntennaSteerable* steerable;
    AntennaPatterned* patterned;

    double rxAntennaGain_dB;
    double txAntennaGain_dB = 0.0;
    double rxSystemLoss_dB;
    double txSystemLoss_dB;
    float  rxAntennaHeight;
    float  txAntennaHeight = 0.0;
    double distanceReachable = 0.0;
    double distanceNotReachable = LONGEST_DISTANCE;
    double distance = distanceNotReachable;
    double pathloss_dB = 0.0;
    double txPower_dBm = 0.0;
    double rxPower_dBm;
    double rxPower_mW;
    double noise_mW = 0.0;
    double rxThreshold_mW = 0.0;
    int     dataRateToUse = 0;

    propProfile = node->partitionData->propChannel[0].profile;

    thisRadio = node->phyData[radioNumber];
    phyModel = node->phyData[radioNumber]->phyModel;

    switch (phyModel) {
        case PHY802_11a:
        case PHY802_11b:
            phy802_11 = (PhyData802_11*)thisRadio->phyVar;
            noise_mW =
                phy802_11->thisPhy->noise_mW_hz *
                phy802_11->channelBandwidth;
            if (printAllDataRates) {
                txPower_dBm = (float)phy802_11->txDefaultPower_dBm[0];
                rxThreshold_mW = phy802_11->rxSensitivity_mW[0];
            }
            else {
                dataRateToUse = phy802_11->lowestDataRateType;
                if (phyModel == PHY802_11b && dataRateToUse > 1) {
                    noise_mW *= 11.0;
                }
                txPower_dBm = (float)phy802_11->txDefaultPower_dBm[
                dataRateToUse];
                rxThreshold_mW = phy802_11->rxSensitivity_mW[dataRateToUse];
            }
            phyRxModel = BER_BASED;
            break;

        case PHY_CHANSWITCH: 
           phyChanSwitch = (PhyDataChanSwitch*)thisRadio->phyVar;
            noise_mW =
                phyChanSwitch->thisPhy->noise_mW_hz *
                phyChanSwitch->channelBandwidth;
            if (printAllDataRates) {
                txPower_dBm = (float)phyChanSwitch->txDefaultPower_dBm[0];
                rxThreshold_mW = phyChanSwitch->rxSensitivity_mW[0];
            }
            else {
                dataRateToUse = phyChanSwitch->lowestDataRateType;
                txPower_dBm = (float)phyChanSwitch->txDefaultPower_dBm[
                dataRateToUse];
                rxThreshold_mW = phyChanSwitch->rxSensitivity_mW[dataRateToUse];
            }
            phyRxModel = BER_BASED;
            break;

#ifdef CELLULAR_LIB
        case PHY_GSM:
            phyGsm = (PhyDataGsm*) thisRadio->phyVar;
            txPower_dBm = (float)phyGsm->txPower_dBm;
            noise_mW =
                phyGsm->thisPhy->noise_mW_hz * PHY_GSM_CHANNEL_BANDWIDTH;
                //200kHz
            rxThreshold_mW = phyGsm->rxThreshold_mW;
            phyRxModel = BER_BASED;

            break;
#ifdef UMTS_LIB
            case PHY_CELLULAR:
            {
                PhyUmtsData* phyUmts;
                PhyCellularData *phyCellular =
                    (PhyCellularData*)thisRadio->phyVar;
                phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
                txPower_dBm = (float)phyUmts->txPower_dBm;
                rxThreshold_mW = phyUmts->rxSensitivity_mW[0];
                noise_mW = phyUmts->noisePower_mW;
                phyRxModel = BER_BASED;

                break;
            }
#endif
#elif UMTS_LIB
            case PHY_CELLULAR:
            {
                PhyUmtsData* phyUmts;
                PhyCellularData *phyCellular =
                    (PhyCellularData*)thisRadio->phyVar;
                phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
                txPower_dBm = (float)phyUmts->txPower_dBm;
                rxThreshold_mW = phyUmts->rxSensitivity_mW[0];
                noise_mW = phyUmts->noisePower_mW;
                phyRxModel = BER_BASED;

                break;
            }

#endif // CELLULAR_LIB

        case PHY_ABSTRACT:
            phyAbstract = (PhyDataAbstract*)thisRadio->phyVar;
            txPower_dBm = (float)phyAbstract->txPower_dBm;
#ifdef ADDON_BOEINGFCS
            noise_mW =
                phyAbstract->thisPhy->noise_mW_hz *
                phyAbstract->bandwidth;
#else
            noise_mW =
                phyAbstract->thisPhy->noise_mW_hz *
                phyAbstract->dataRate;
#endif
            rxThreshold_mW = phyAbstract->rxThreshold_mW;
            phyRxModel = SNR_THRESHOLD_BASED;

            break;
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4:
            phy802_15_4 = (PhyData802_15_4*)thisRadio->phyVar;
            txPower_dBm = (float)phy802_15_4->txPower_dBm;
            noise_mW =
                phy802_15_4->noisePower_mW;
            rxThreshold_mW = phy802_15_4->rxSensitivity_mW;
            phyRxModel = SNR_THRESHOLD_BASED;

            break;
#endif

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16:
            phy802_16 = (PhyDataDot16*)thisRadio->phyVar;
            noise_mW =
                phy802_16->thisPhy->noise_mW_hz *
                phy802_16->channelBandwidth;

            dataRateToUse = PHY802_16_QPSK_R_1_2;
            txPower_dBm = (float)phy802_16->txPower_dBm;
            rxThreshold_mW = phy802_16->rxSensitivity_mW[dataRateToUse];
            phyRxModel = BER_BASED;

            break;
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_JTRS_WNW:
            phyJtrs = (PhyDataJtrs*)thisRadio->phyVar;
            txPower_dBm = (float)phyJtrs->txPower_dBm;
            noise_mW =
                phyJtrs->thisPhy->noise_mW_hz *
                phyJtrs->dataRate;
            rxThreshold_mW = phyJtrs->rxThreshold_mW;
            phyRxModel = SNR_THRESHOLD_BASED;
#endif // MILITARY_RADIOS_LIB

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING:
            phyJamming = (PhyJammingData*)thisRadio->phyVar;
            txPower_dBm = (float)phyJamming->txPower_dBm;
            noise_mW =
                phyJamming->thisPhy->noise_mW_hz *
                phyJamming->dataRate;
            rxThreshold_mW = phyJamming->rxThreshold_mW;
            phyRxModel = SNR_THRESHOLD_BASED;
#endif // PHY_SYNC
#endif // NETSEC_LIB
        default:
            if (printAllDataRates) {
                ERROR_ReportError("Unsupported Radio type.");
            }
            else {
                ERROR_ReportWarning("Cannot calculate the radio range");
                return RANGE_TO_RETURN_IN_CASE_OF_ERROR;
            }
    }


    switch (thisRadio->antennaData->antennaModelType)
    {
    case ANTENNA_OMNIDIRECTIONAL:
        {
            omniDirectional =
                (AntennaOmnidirectional*)thisRadio->antennaData->antennaVar;
            txAntennaGain_dB = omniDirectional->antennaGain_dB;
            txAntennaHeight = omniDirectional->antennaHeight;
            break;
        }
    case ANTENNA_SWITCHED_BEAM:
        {
            switchedBeam = (AntennaSwitchedBeam*)thisRadio->antennaData->
                antennaVar;
            txAntennaGain_dB = switchedBeam->antennaGain_dB;
            txAntennaHeight = switchedBeam->antennaHeight;
            break;
        }
    case ANTENNA_STEERABLE:
        {
            steerable =
                (AntennaSteerable*)thisRadio->antennaData->antennaVar;
            txAntennaGain_dB = steerable->antennaGain_dB;
            txAntennaHeight = steerable->antennaHeight;
            break;
        }
    case ANTENNA_PATTERNED:
        {
            patterned =
                (AntennaPatterned*)thisRadio->antennaData->antennaVar;
            txAntennaGain_dB = patterned->antennaGain_dB;
            txAntennaHeight = patterned->antennaHeight;
            break;
        }
    default:
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL-TYPE %d.\n",
                thisRadio->antennaData->antennaModelType);
            ERROR_ReportError(err);
            break;
        }

    }

    rxAntennaGain_dB = txAntennaGain_dB;
    txSystemLoss_dB = thisRadio->systemLoss_dB;
    rxSystemLoss_dB = thisRadio->systemLoss_dB;

    rxAntennaHeight = txAntennaHeight;

    // Calculate radio range for all data rates
    numIterations = 1;
    if (printAllDataRates) {
        if (phyModel == PHY802_11a)
        {
            numIterations = PHY802_11a_NUM_DATA_RATES;
        } 
				else if (phyModel == PHY_CHANSWITCH)
        {
            numIterations = PHY_CHANSWITCH_NUM_DATA_RATES;
        } 
				else if (phyModel == PHY802_11b) {
            numIterations = PHY802_11b_NUM_DATA_RATES;
        }
    }

#ifdef ADVANCED_WIRELESS_LIB
    if (printAllDataRates) {
        if ( phyModel == PHY802_16 ) {
            numIterations = PHY802_16_NUM_DATA_RATES - 1;
        }
    }
#endif

    for (index = dataRateToUse; index < (dataRateToUse + numIterations); index++)
    {
        while (distanceNotReachable - distanceReachable > DELTA) {
            BOOL reachable;

            switch (propProfile->pathlossModel) {
                case FREE_SPACE: {
                    pathloss_dB = PROP_PathlossFreeSpace(distance,
                                                         propProfile->wavelength);
                    break;
                }
                case TWO_RAY: {
                    pathloss_dB = PROP_PathlossTwoRay(distance,
                                                      propProfile->wavelength,
                                                      txAntennaHeight,
                                                      rxAntennaHeight);

                    break;
                }
#ifdef URBAN_LIB
                case COST231_HATA: {

                    pathloss_dB = PathlossCOST231Hata(distance,
                                                      propProfile->wavelength,
                                                      txAntennaHeight,
                                                      rxAntennaHeight,
                                                      propProfile);
                    break;
                }
#endif // URBAN_LIB
                default: {
                    if (printAllDataRates) {
                        ERROR_ReportError("Cannot estimate the range with this pathloss model.");
                    }
                    else { // use TWO_RAY as a bad estimate.
                        pathloss_dB =
                            PROP_PathlossTwoRay(distance,
                                                propProfile->wavelength,
                                                txAntennaHeight,
                                                rxAntennaHeight);
                    }
                }
            }

            rxPower_dBm = txPower_dBm + txAntennaGain_dB - txSystemLoss_dB -
                          pathloss_dB - propProfile->shadowingMean_dB +
                          rxAntennaGain_dB - rxSystemLoss_dB;
            rxPower_mW = NON_DB(rxPower_dBm);

#ifdef ADVANCED_WIRELESS_LIB

            if (phyModel == PHY802_16) {
                rxPower_mW *= phy802_16->PowerLossDueToCPTime;
            }
#endif
            if (phyRxModel == BER_BASED) {
                double BER, PER;
                double snr = rxPower_mW / noise_mW;

                if (rxPower_mW < rxThreshold_mW) {
                    reachable = FALSE;
                }
                else {
                    BER = PHY_BER(thisRadio, index, snr);

                    PER = 1.0 - pow((1.0 - BER), (double)PACKET_SIZE);

                    if (PER <= 0.1) {
                        reachable = TRUE;
                    }
                    else {
                        reachable = FALSE;
                    }
                }
            }
            else {
                // SNR_THRESHOLD_BASED

                if (rxPower_mW >= rxThreshold_mW) {
                    reachable = TRUE;
                }
                else {
                    reachable = FALSE;
                }
            }

            if (reachable == TRUE) {
                distanceReachable = distance;
                distance += (distanceNotReachable - distanceReachable) / 2.0;
            }
            else {
                assert(reachable == FALSE);

                distanceNotReachable = distance;
                distance -= (distanceNotReachable - distanceReachable) / 2.0;
            }
        } // end of while //

        if (phyRxModel == SNR_THRESHOLD_BASED) {
            if (rxThreshold_mW <
                noise_mW * thisRadio->phyRxSnrThreshold)
            {
                printf("Error:   the radio cannot receive packets with power above\n"
                   "         reception threshold (%f [dBm])\n"
                   "         due to too high PHY-RX-SNR-THRESHOLD (%f [dB])\n"
                   "         or too high thermal noise (%f [dBm]).\n",
                   IN_DB(rxThreshold_mW),
                   IN_DB(thisRadio->phyRxSnrThreshold),
                   IN_DB(noise_mW));
                exit(1);
            }
        }

        if (phyModel == PHY802_11a || phyModel == PHY802_11b || phyModel == PHY_CHANSWITCH){
            int modelType = 'a';

            if (phyModel == PHY802_11b) {
                modelType = 'b';
            }

            if (phyModel == PHY_CHANSWITCH) {
                modelType = 'c';
            }
				

            // printf("%.15f %.15f %d %.15f %.3f\n",
            //        txPower_dBm, rxThreshold_mW, index,
            //        noise_mW, distanceReachable);
            if (printAllDataRates) {
                printf("radio range: %8.3fm, for 802.11%c data rate %4.1f Mbps\n",
                       distanceReachable, modelType,
                       ((float)phy802_11->dataRate[index] / 1000000.0));
            }
            else {
                return distanceReachable;
            }
            // Initialize for the next data rate
            rxThreshold_mW = phy802_11->rxSensitivity_mW[index + 1];
            txPower_dBm = phy802_11->txDefaultPower_dBm[index + 1];
            distanceReachable = 0.0;
            distanceNotReachable = LONGEST_DISTANCE;
            distance = distanceNotReachable;
            if (phyModel == PHY802_11b && index == 1) {
                noise_mW *= 11.0;
            }
#ifdef ADVANCED_WIRELESS_LIB
        }
        else if (phyModel == PHY802_16)
        {
            if (printAllDataRates) {
                printf("radio range: %8.3fm, for 802.16 data rate %5.2f Mbps\n",
                distanceReachable,
                ((float)PhyDot16GetDataRate(thisRadio, index)
                 / 1000000.0));
            }
            else {
                return distanceReachable;
            }
            // Initialize for the next data rate
            rxThreshold_mW = phy802_16->rxSensitivity_mW[index + 1];
            distanceReachable = 0.0;
            distanceNotReachable = LONGEST_DISTANCE;
            distance = distanceNotReachable;
#endif
        } else {
            if (printAllDataRates) {
                printf("radio range: %.3fm\n", distanceReachable);
            }
            else {
                return distanceReachable;
            }
        }
    } // end of for //

    return distanceReachable;
#else // WIRELESS_LIB
    return 0.0;
#endif // WIRELESS_LIB
}

void
PHY_BerTablesPrepare (PhyBerTable * berTables, const int tableCount)
{
    int             tableIndex;
    PhyBerTable *   berTable;

    bool            isFixedInterval;
    double          interval;
    int             entryCount;
    int             entryIndex;
    double          prevSnr;

    double          epsilon;


    for (tableIndex = 0; tableIndex < tableCount; tableIndex++)
    {
        berTable = &(berTables [tableIndex]);
        berTable->isFixedInterval = false;
        entryCount = berTable->numEntries;
        if (entryCount < 3)
            continue;

        // This epislon (how accurate for floating point math errors)
        // is fairly coarse. This happens because our snr
        // tables are being filled from auto-init source code (not actualy math),
        // and the numeric strings in our source code
        // only have about 9 digits worth of precision.
        epsilon = 1.0e-13;
        interval = berTable->entries [1].snr - berTable->entries [0].snr;
        prevSnr = berTable->entries [1].snr;
        isFixedInterval = true;
        for (entryIndex = 2; entryIndex < entryCount; entryIndex++)
        {
            if (fabs ((berTable->entries [entryIndex].snr - prevSnr) - interval)
                > epsilon)
            {
                isFixedInterval = false;
                break;
            }
            prevSnr = berTable->entries [entryIndex].snr;
        }
        if (isFixedInterval)
        {
            berTable->isFixedInterval = true;
            berTable->interval = interval;
            berTable->snrStart = berTable->entries [0].snr;
            berTable->snrEnd = berTable->entries [entryCount - 1].snr;
        }
    }
}

PhyBerTable *
PHY_BerTablesAlloc (const int tableCount)
{
    PhyBerTable *   newBerTables;

    newBerTables = (PhyBerTable*) MEM_malloc(tableCount * sizeof(PhyBerTable));
    memset(newBerTables, 0, tableCount * sizeof(PhyBerTable));

    return newBerTables;
}

#ifdef ADDON_NGCNMS

void PHY_Reset(Node* node, int interfaceIndex)
{
    // currently PhyAbstract is the only
    // supported protocol.
    int i;

    for (i = 0; i < node->numberPhys; i++)
    {
        if(node->disabled)
        {
            PhyAbstractReset(node, i);
        }
        else if (i == interfaceIndex &&
                 node->phyData[i]->macInterfaceIndex == i)
        {
            PhyAbstractReset(node, i);
        }

    }
}

#endif
