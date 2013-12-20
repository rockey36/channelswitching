#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "api.h"
#include "partition.h"
#include "antenna.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#include "phy_chanswitch.h"

#include "mac_csma.h"
#include "mac_dot11.h"
#include "mac_dot11-sta.h"

void PhyChanSwitchChangeState(
    Node* node,
    int phyIndex,
    PhyStatusType newStatus)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch *phychanswitch = (PhyDataChanSwitch *)thisPhy->phyVar;
    phychanswitch->previousMode = phychanswitch->mode;
    phychanswitch->mode = newStatus;


    Phy_ReportStatusToEnergyModel(
        node,
        phyIndex,
        phychanswitch->previousMode,
        newStatus);
}

double
PhyChanSwitchGetSignalStrength(Node *node,PhyDataChanSwitch* phychanswitch)
{
    double rxThreshold_mW;
    int dataRateToUse;
    PhyChanSwitchGetLowestTxDataRateType(phychanswitch->thisPhy, &dataRateToUse);
    rxThreshold_mW = phychanswitch->rxSensitivity_mW[dataRateToUse];
    return IN_DB(rxThreshold_mW);
}

static
void PhyChanSwitchReportExtendedStatusToMac(
    Node *node,
    int phyNum,
    PhyStatusType status,
    clocktype receiveDuration,
    Message* potentialIncomingPacket)
{
    PhyData* thisPhy = node->phyData[phyNum];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch *)thisPhy->phyVar;

    assert(status == phychanswitch->mode);

    if (potentialIncomingPacket != NULL) {
        MESSAGE_RemoveHeader(
            node,
            potentialIncomingPacket,
            sizeof(PhyChanSwitchPlcpHeader),
            TRACE_CHANSWITCH);
    }

    MAC_ReceivePhyStatusChangeNotification(
        node, thisPhy->macInterfaceIndex,
        phychanswitch->previousMode, status,
        receiveDuration, potentialIncomingPacket);

    if (potentialIncomingPacket != NULL) {
        MESSAGE_AddHeader(
            node,
            potentialIncomingPacket,
            sizeof(PhyChanSwitchPlcpHeader),
            TRACE_CHANSWITCH);
    }
}

static /*inline*/
void PhyChanSwitchReportStatusToMac(
    Node *node, int phyNum, PhyStatusType status)
{
    PhyChanSwitchReportExtendedStatusToMac(
        node, phyNum, status, 0, NULL);
}

static //inline//
void PhyChanSwitchLockSignal(
    Node* node,
    PhyDataChanSwitch* phychanswitch,
    Message *msg,
    double rxPower_mW,
    clocktype rxEndTime,
    Orientation rxDOA)
{

    char *plcp = MESSAGE_ReturnPacket(msg);
    memcpy(&phychanswitch->rxDataRateType,plcp,sizeof(int));

    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(getSimTime(node), currTime);
        printf("\ntime %s: LockSignal from %d at %d\n",
               currTime,
               msg->originatingNodeId,
               node->nodeId);
    }

    phychanswitch->rxMsg = msg;
    phychanswitch->rxMsgError = FALSE;
    phychanswitch->rxMsgPower_mW = rxPower_mW;
    phychanswitch->rxTimeEvaluated = getSimTime(node);
    phychanswitch->rxEndTime = rxEndTime;
    phychanswitch->rxDOA = rxDOA;
    phychanswitch->stats.totalSignalsLocked++;
}

static //inline//
void PhyChanSwitchUnlockSignal(PhyDataChanSwitch* phychanswitch) {
    phychanswitch->rxMsg = NULL;
    phychanswitch->rxMsgError = FALSE;
    phychanswitch->rxMsgPower_mW = 0.0;
    phychanswitch->rxTimeEvaluated = 0;
    phychanswitch->rxEndTime = 0;
    phychanswitch->rxDOA.azimuth = 0;
    phychanswitch->rxDOA.elevation = 0;
}

static /*inline*/
BOOL PhyChanSwitchCarrierSensing(Node* node, PhyDataChanSwitch* phychanswitch) {

    double rxSensitivity_mW = phychanswitch->rxSensitivity_mW[0];

    if (!ANTENNA_IsInOmnidirectionalMode(node, phychanswitch->
        thisPhy->phyIndex)) {
        rxSensitivity_mW =
            NON_DB(IN_DB(rxSensitivity_mW) +
                   phychanswitch->directionalAntennaGain_dB);
    }//if//

    if ((phychanswitch->interferencePower_mW + phychanswitch->noisePower_mW) >
        rxSensitivity_mW)
    {
        return TRUE;
    }

    return FALSE;
}



static //inline//
BOOL PhyChanSwitchCheckRxPacketError(
    Node* node,
    PhyDataChanSwitch* phychanswitch,
    double *sinrPtr)
{
    double sinr;
    double BER;
    double noise =
        phychanswitch->thisPhy->noise_mW_hz * phychanswitch->channelBandwidth;

    assert(phychanswitch->rxMsgError == FALSE);


    sinr = (phychanswitch->rxMsgPower_mW /
            (phychanswitch->interferencePower_mW + noise));
    
    // if(node->nodeId < 3) {
    //     printf("PhyChanSwitchCheckRxPacketError sinr = %f, rx msg power = %f, int+noise = %f at node %d \n", 
    //     IN_DB(sinr),IN_DB(phychanswitch->rxMsgPower_mW),IN_DB(phychanswitch->interferencePower_mW + noise),node->nodeId);

    // }

    if (sinrPtr != NULL)
    {
        *sinrPtr = sinr;
    }

    assert(phychanswitch->rxDataRateType >= 0 &&
           phychanswitch->rxDataRateType < phychanswitch->numDataRates);

    BER = PHY_BER(phychanswitch->thisPhy,
                  phychanswitch->rxDataRateType,
                  sinr);

    if (BER != 0.0) {
        double numBits =
            ((double)(getSimTime(node) - phychanswitch->rxTimeEvaluated) *
             (double)phychanswitch->dataRate[phychanswitch->rxDataRateType] /
             (double)SECOND);

        double errorProbability = 1.0 - pow((1.0 - BER), numBits);
        double rand = RANDOM_erand(phychanswitch->thisPhy->seed);

        assert((errorProbability >= 0.0) && (errorProbability <= 1.0));

        if (errorProbability > rand) {
            return TRUE;
        }
    }

    return FALSE;
}

static
void PhyChanSwitchaInitializeDefaultParameters(Node* node, int phyIndex) {
    PhyDataChanSwitch* phychanswitch =
        (PhyDataChanSwitch*)(node->phyData[phyIndex]->phyVar);


    phychanswitch->numDataRates = PHY_CHANSWITCH_NUM_DATA_RATES;

    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH__6M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER__6M_dBm;
    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH__9M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER__9M_dBm;
    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_12M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER_12M_dBm;
    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_18M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER_18M_dBm;
    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_24M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER_24M_dBm;
    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_36M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER_36M_dBm;
    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_48M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER_48M_dBm;
    phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_54M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER_54M_dBm;


    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH__6M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__6M_dBm);
    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH__9M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__9M_dBm);
    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_12M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_12M_dBm);
    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_18M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_18M_dBm);
    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_24M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_24M_dBm);
    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_36M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_36M_dBm);
    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_48M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_48M_dBm);
    phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_54M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_54M_dBm);


    phychanswitch->dataRate[PHY_CHANSWITCH__6M] = PHY_CHANSWITCH_DATA_RATE__6M;
    phychanswitch->dataRate[PHY_CHANSWITCH__9M] = PHY_CHANSWITCH_DATA_RATE__9M;
    phychanswitch->dataRate[PHY_CHANSWITCH_12M] = PHY_CHANSWITCH_DATA_RATE_12M;
    phychanswitch->dataRate[PHY_CHANSWITCH_18M] = PHY_CHANSWITCH_DATA_RATE_18M;
    phychanswitch->dataRate[PHY_CHANSWITCH_24M] = PHY_CHANSWITCH_DATA_RATE_24M;
    phychanswitch->dataRate[PHY_CHANSWITCH_36M] = PHY_CHANSWITCH_DATA_RATE_36M;
    phychanswitch->dataRate[PHY_CHANSWITCH_48M] = PHY_CHANSWITCH_DATA_RATE_48M;
    phychanswitch->dataRate[PHY_CHANSWITCH_54M] = PHY_CHANSWITCH_DATA_RATE_54M;


    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH__6M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__6M;
    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH__9M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__9M;
    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH_12M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_12M;
    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH_18M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_18M;
    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH_24M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_24M;
    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH_36M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_36M;
    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH_48M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_48M;
    phychanswitch->numDataBitsPerSymbol[PHY_CHANSWITCH_54M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_54M;


    phychanswitch->lowestDataRateType = PHY_CHANSWITCH_LOWEST_DATA_RATE_TYPE;
    phychanswitch->highestDataRateType = PHY_CHANSWITCH_HIGHEST_DATA_RATE_TYPE;
    phychanswitch->txDataRateTypeForBC = PHY_CHANSWITCH_DATA_RATE_TYPE_FOR_BC;

    phychanswitch->channelBandwidth = PHY_CHANSWITCH_CHANNEL_BANDWIDTH;
    phychanswitch->rxTxTurnaroundTime = PHY_CHANSWITCH_RX_TX_TURNAROUND_TIME;



    return;
}

static
void PhyChanSwitchaSetUserConfigurableParameters(
    Node* node,
    int phyIndex,
    const NodeInput *nodeInput)
{
    double rxSensitivity_dBm;
    double txPower_dBm;
    BOOL   wasFound;

    PhyDataChanSwitch* phychanswitch =
        (PhyDataChanSwitch*)(node->phyData[phyIndex]->phyVar);


    //
    // Set PHY_CHANSWITCH-TX-POWER's
    //
    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER--6MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH__6M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER--9MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH__9M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER-12MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_12M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER-18MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_18M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER-24MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_24M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER-36MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_36M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER-48MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_48M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER-54MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitch->txDefaultPower_dBm[PHY_CHANSWITCH_54M] = (float)txPower_dBm;
    }


    //
    // Set PHY_CHANSWITCH-RX-SENSITIVITY's
    //
    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY--6MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH__6M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY--9MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH__9M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY-12MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_12M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY-18MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_18M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY-24MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_24M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY-36MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_36M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY-48MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_48M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY-54MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitch->rxSensitivity_mW[PHY_CHANSWITCH_54M] =
            NON_DB(rxSensitivity_dBm);
    }


    return;
}



static
void PhyChanSwitchbInitializeDefaultParameters(Node* node, int phyIndex) {
    PhyDataChanSwitch* phychanswitchb =
        (PhyDataChanSwitch*)(node->phyData[phyIndex]->phyVar);


    phychanswitchb->numDataRates = PHY_CHANSWITCH_NUM_DATA_RATES;

    phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCH__1M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER__1M_dBm;
    phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCH__2M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER__2M_dBm;
    phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCHb__6M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER__6M_dBm;
    phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCH_11M] =
        PHY_CHANSWITCH_DEFAULT_TX_POWER_11M_dBm;


    phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH__1M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__1M_dBm);
    phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH__2M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__2M_dBm);
    phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH__6M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__6M_dBm);
    phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH_11M] =
        NON_DB(PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_11M_dBm);


    phychanswitchb->dataRate[PHY_CHANSWITCH__1M] = PHY_CHANSWITCH_DATA_RATE__1M;
    phychanswitchb->dataRate[PHY_CHANSWITCH__2M] = PHY_CHANSWITCH_DATA_RATE__2M;
    phychanswitchb->dataRate[PHY_CHANSWITCH__6M] = PHY_CHANSWITCH_DATA_RATE__6M;
    phychanswitchb->dataRate[PHY_CHANSWITCH_11M] = PHY_CHANSWITCH_DATA_RATE_11M;


    phychanswitchb->numDataBitsPerSymbol[PHY_CHANSWITCH__1M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__1M;
    phychanswitchb->numDataBitsPerSymbol[PHY_CHANSWITCH__2M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__2M;
    phychanswitchb->numDataBitsPerSymbol[PHY_CHANSWITCH__6M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__6M;
    phychanswitchb->numDataBitsPerSymbol[PHY_CHANSWITCH_11M] =
        PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_11M;


    phychanswitchb->lowestDataRateType = PHY_CHANSWITCH_LOWEST_DATA_RATE_TYPE;
    phychanswitchb->highestDataRateType = PHY_CHANSWITCH_HIGHEST_DATA_RATE_TYPE;
    phychanswitchb->txDataRateTypeForBC = PHY_CHANSWITCH_DATA_RATE_TYPE_FOR_BC;

    phychanswitchb->rxDataRateType = PHY_CHANSWITCH_LOWEST_DATA_RATE_TYPE;

    phychanswitchb->channelBandwidth = PHY_CHANSWITCH_CHANNEL_BANDWIDTH;
    phychanswitchb->rxTxTurnaroundTime = PHY_CHANSWITCH_RX_TX_TURNAROUND_TIME;

    return;
}



static
void PhyChanSwitchbSetUserConfigurableParameters(
    Node* node,
    int phyIndex,
    const NodeInput *nodeInput)
{
    double rxSensitivity_dBm;
    double txPower_dBm;
    BOOL   wasFound;

    PhyDataChanSwitch* phychanswitchb =
        (PhyDataChanSwitch*)(node->phyData[phyIndex]->phyVar);


    //
    // Set PHY_CHANSWITCH-TX-POWER's
    //
    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER--1MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCH__1M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER--2MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCH__2M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER--6MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCH__6M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-TX-POWER-11MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phychanswitchb->txDefaultPower_dBm[PHY_CHANSWITCH_11M] = (float)txPower_dBm;
    }


    //
    // Set PHY_CHANSWITCH-RX-SENSITIVITY's
    //
    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY--1MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH__1M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY--2MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH__2M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY--6MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH__6M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-RX-SENSITIVITY-11MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phychanswitchb->rxSensitivity_mW[PHY_CHANSWITCH_11M] =
            NON_DB(rxSensitivity_dBm);
    }


    return;
}



void PhyChanSwitchInit(
    Node *node,
    const int phyIndex,
    const NodeInput *nodeInput)
{
    BOOL   wasFound;
    BOOL   yes;
    int dataRateForBroadcast;
    int i;
    int numChannels = PROP_NumberChannels(node);

    PhyDataChanSwitch *phychanswitch =
        (PhyDataChanSwitch *)MEM_malloc(sizeof(PhyDataChanSwitch));

    memset(phychanswitch, 0, sizeof(PhyDataChanSwitch));

    node->phyData[phyIndex]->phyVar = (void*)phychanswitch;

    phychanswitch->thisPhy = node->phyData[phyIndex];


    std::string path;
    D_Hierarchy *h = &node->partitionData->dynamicHierarchy;

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "totalTxSignals",
            path))
    {
        h->AddObject(
            path,
            new D_Int32Obj(&phychanswitch->stats.totalTxSignals));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "totalRxSignalsToMac",
            path))
    {
        h->AddObject(
            path,
            new D_Int32Obj(&phychanswitch->stats.totalRxSignalsToMac));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "totalSignalsLocked",
            path))
    {
        h->AddObject(
            path,
            new D_Int32Obj(&phychanswitch->stats.totalSignalsLocked));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "totalSignalsWithErrors",
            path))
    {
        h->AddObject(
            path,
            new D_Int32Obj(&phychanswitch->stats.totalSignalsWithErrors));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "energyConsumed",
            path))
    {
        h->AddObject(
            path,
            new D_Float64Obj(&phychanswitch->stats.energyConsumed));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "turnOnTime",
            path))
    {
        h->AddObject(
            path,
            new D_ClocktypeObj(&phychanswitch->stats.turnOnTime));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "txPower_dBm",
            path))
    {
        h->AddObject(
            path,
            new D_Float32Obj(&phychanswitch->txPower_dBm));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "ChanSwitch",
            "channelBandwidth",
            path))
    {
        h->AddObject(
            path,
            new D_Int32Obj(&phychanswitch->channelBandwidth));
    }

    //
    // Antenna model initialization
    //
    ANTENNA_Init(node, phyIndex, nodeInput);

    ERROR_Assert(((phychanswitch->thisPhy->antennaData->antennaModelType
                == ANTENNA_OMNIDIRECTIONAL) ||
           (phychanswitch->thisPhy->antennaData->antennaModelType
                == ANTENNA_SWITCHED_BEAM) ||
           (phychanswitch->thisPhy->antennaData->antennaModelType
                == ANTENNA_STEERABLE) ||
           (phychanswitch->thisPhy->antennaData->antennaModelType
                == ANTENNA_PATTERNED)) ,
            "Illegal antennaModelType.\n");


    if (node->phyData[phyIndex]->phyModel == PHY_CHANSWITCH){
        PhyChanSwitchaInitializeDefaultParameters(node, phyIndex);
        PhyChanSwitchaSetUserConfigurableParameters(node, phyIndex, nodeInput);
    }
    else {
        ERROR_ReportError(
            "PhyChanSwitchInit() needs to be called for an ChanSwitch model");
    }

    ERROR_Assert(phychanswitch->thisPhy->phyRxModel != SNR_THRESHOLD_BASED,
                 "ChanSwitch PHY model does not work with "
                 "SNR_THRESHOLD_BASED reception model.\n");

    //
    // Data rate options
    //
    IO_ReadBool(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-AUTO-RATE-FALLBACK",
        &wasFound,
        &yes);

    if (!wasFound || yes == FALSE) {
        BOOL wasFound1;
        int dataRate;

        IO_ReadInt(
            node->nodeId,
            node->phyData[phyIndex]->networkAddress,
            nodeInput,
            "PHY_CHANSWITCH-DATA-RATE",
            &wasFound1,
            &dataRate);

        if (wasFound1) {
            for (i = 0; i < phychanswitch->numDataRates; i++) {
                if (dataRate == phychanswitch->dataRate[i]) {
                    break;
                }
            }
            if (i >= phychanswitch->numDataRates) {
                ERROR_ReportError(
                    "Specified PHY_CHANSWITCH-DATA-RATE is not "
                    "in the supported data rate set");
            }

            phychanswitch->lowestDataRateType = i;
            phychanswitch->highestDataRateType = i;
        }
        else {
            ERROR_ReportError(
                "PHY_CHANSWITCH-DATA-RATE not set without "
                "PHY_CHANSWITCH-AUTO-RATE-FALLBACK turned on");
        }
    }

    PhyChanSwitchSetLowestTxDataRateType(phychanswitch->thisPhy);

    //
    // Set PHYCHANSWITCH-DATA-RATE-FOR-BROADCAST
    //
    IO_ReadInt(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-DATA-RATE-FOR-BROADCAST",
        &wasFound,
        &dataRateForBroadcast);

    if (wasFound) {
        for (i = 0; i < phychanswitch->numDataRates; i++) {
            if (dataRateForBroadcast == phychanswitch->dataRate[i]) {
                break;
            }
        }

        if (i < phychanswitch->lowestDataRateType ||
            i > phychanswitch->highestDataRateType)
        {
            ERROR_ReportError(
                "Specified PHY_CHANSWITCH-DATA-RATE-FOR-BROADCAST is not "
                "in the data rate set");
        }

        phychanswitch->txDataRateTypeForBC = i;
    }
    else {
        phychanswitch->txDataRateTypeForBC =
            MIN(phychanswitch->highestDataRateType,
                MAX(phychanswitch->lowestDataRateType,
                    phychanswitch->txDataRateTypeForBC));
    }


    //
    // Set PHYCHANSWITCH-ESTIMATED-DIRECTIONAL-ANTENNA-GAIN
    //
    IO_ReadDouble(
        node->nodeId,
        node->phyData[phyIndex]->networkAddress,
        nodeInput,
        "PHY_CHANSWITCH-ESTIMATED-DIRECTIONAL-ANTENNA-GAIN",
        &wasFound,
        &(phychanswitch->directionalAntennaGain_dB));

    if (!wasFound &&
        (phychanswitch->thisPhy->antennaData->antennaModelType
            != ANTENNA_OMNIDIRECTIONAL &&
          phychanswitch->thisPhy->antennaData->antennaModelType
            != ANTENNA_PATTERNED))
    {
        ERROR_ReportError(
            "PHY_CHANSWITCH-ESTIMATED-DIRECTIONAL-ANTENNA-GAIN is missing\n");
    }


    //
    // Initialize phy statistics variables
    //
    phychanswitch->stats.totalRxSignalsToMac = 0;
    phychanswitch->stats.totalSignalsLocked = 0;
    phychanswitch->stats.totalSignalsWithErrors = 0;
    phychanswitch->stats.totalTxSignals = 0;
    phychanswitch->stats.energyConsumed = 0.0;
    phychanswitch->stats.turnOnTime = getSimTime(node);

    // //add the channel checked array
    // phychanswitch->channelChecked =  new D_BOOL[numChannels];
    // for(i=0;i<numChannels;i++){
    //     phychanswitch->channelChecked[i] = FALSE;

    // }

    // //probe disabled by default
    // phychanswitch->isProbing = FALSE;


    //
    // Initialize status of phy
    //
    phychanswitch->rxMsg = NULL;
    phychanswitch->rxMsgError = FALSE;
    phychanswitch->rxMsgPower_mW = 0.0;
    phychanswitch->interferencePower_mW = 0.0;
    phychanswitch->noisePower_mW =
        phychanswitch->thisPhy->noise_mW_hz * phychanswitch->channelBandwidth;
    phychanswitch->rxTimeEvaluated = 0;
    phychanswitch->rxEndTime = 0;
    phychanswitch->rxDOA.azimuth = 0;
    phychanswitch->rxDOA.elevation = 0;
    phychanswitch->previousMode = PHY_IDLE;
    phychanswitch->mode = PHY_IDLE;
    PhyChanSwitchChangeState(node,phyIndex, PHY_IDLE);

    //
    // Setting up the channel to use for both TX and RX
    //
    for (i = 0; i < numChannels; i++) {
        if (phychanswitch->thisPhy->channelListening[i] == TRUE) {
            break;
        }
    }
    assert(i != numChannels);
    PHY_SetTransmissionChannel(node, phyIndex, i);

    return;
}


void PhyChanSwitchChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch *)thisPhy->phyVar;

   if (phychanswitch == NULL)
    {
        // not initialized yet, return;
        return;
    }

    if (startListening == TRUE)
    {
        if(phychanswitch->mode == PHY_TRANSMITTING)
        {
            PhyChanSwitchTerminateCurrentTransmission(node,phyIndex);
        }
        PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phychanswitch->interferencePower_mW));

        if (PhyChanSwitchCarrierSensing(node, phychanswitch) == TRUE) {
            PhyChanSwitchChangeState(node,phyIndex, PHY_SENSING);
        }
        else {
            PhyChanSwitchChangeState(node,phyIndex, PHY_IDLE);
        }

        if(phychanswitch->previousMode != phychanswitch->mode)
            PhyChanSwitchReportStatusToMac(node, phyIndex, phychanswitch->mode);
    }
    else if(phychanswitch->mode != PHY_TRANSMITTING)
    {
        PhyChanSwitchChangeState(node,phyIndex, PHY_TRX_OFF);
    }

}


void PhyChanSwitchTransmissionEnd(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch *)thisPhy->phyVar;
    int channelIndex;
    char clockStr[20];
    TIME_PrintClockInSecond(getSimTime(node), clockStr);

    if (DEBUG)
    {   
        printf("PHY_ChanSwitch: node %d transmission end at time %s, mode %d \n",
               node->nodeId, clockStr, phychanswitch->mode);
    }


    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    Int8 buf[MAX_STRING_LENGTH];
    sprintf(buf, "PhyChanSwitchTransmissionEnd: failed on node %d, time %s, mode %d \n",
            node->nodeId, clockStr, phychanswitch->mode);
    ERROR_Assert(phychanswitch->mode == PHY_TRANSMITTING, buf);

    // assert(phychanswitch->mode == PHY_TRANSMITTING);

    phychanswitch->txEndTimer = NULL;
    //GuiStart
    if (node->guiOption == TRUE) {
        GUI_EndBroadcast(node->nodeId,
                         GUI_PHY_LAYER,
                         GUI_DEFAULT_DATA_TYPE,
                         thisPhy->macInterfaceIndex,
                         getSimTime(node));
    }
    //GuiEnd

    if (!ANTENNA_IsLocked(node, phyIndex)) {
        ANTENNA_SetToDefaultMode(node, phyIndex);
    }//if//

    PHY_StartListeningToChannel(node, phyIndex, channelIndex);

}


BOOL PhyChanSwitchMediumIsIdle(Node* node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*)thisPhy->phyVar;
    BOOL IsIdle;
    int channelIndex;
    double oldInterferencePower = phychanswitch->interferencePower_mW;

    assert((phychanswitch->mode == PHY_IDLE) ||
           (phychanswitch->mode == PHY_SENSING));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        NULL,
        NULL,
        &(phychanswitch->interferencePower_mW));

    IsIdle = (!PhyChanSwitchCarrierSensing(node, phychanswitch));
    phychanswitch->interferencePower_mW = oldInterferencePower;

    return IsIdle;
}


BOOL PhyChanSwitchMediumIsIdleInDirection(Node* node, int phyIndex,
                                      double azimuth) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*)thisPhy->phyVar;
    BOOL IsIdle;
    int channelIndex;
    double oldInterferencePower = phychanswitch->interferencePower_mW;

    assert((phychanswitch->mode == PHY_IDLE) ||
           (phychanswitch->mode == PHY_SENSING));

    if (ANTENNA_IsLocked(node, phyIndex)) {
       return (phychanswitch->mode == PHY_IDLE);
    }//if//

    // ZEB - Removed to support patterned antenna types
//    assert(ANTENNA_IsInOmnidirectionalMode(node, phyIndex));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);


    ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex, azimuth);

    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        NULL,
        NULL,
        &(phychanswitch->interferencePower_mW));

    IsIdle = (!PhyChanSwitchCarrierSensing(node, phychanswitch));
    phychanswitch->interferencePower_mW = oldInterferencePower;
    ANTENNA_SetToDefaultMode(node, phyIndex);

    return IsIdle;
}


void PhyChanSwitchSetSensingDirection(Node* node, int phyIndex,
                                  double azimuth) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*)thisPhy->phyVar;
    int channelIndex;

    assert((phychanswitch->mode == PHY_IDLE) ||
           (phychanswitch->mode == PHY_SENSING));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);
    ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex, azimuth);

    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        NULL,
        NULL,
        &(phychanswitch->interferencePower_mW));
}



void PhyChanSwitchFinalize(Node *node, const int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;
    char buf[MAX_STRING_LENGTH];
	
    if (thisPhy->phyStats == FALSE) {
        return;
    }

    assert(thisPhy->phyStats == TRUE);

    sprintf(buf, "Signals transmitted = %d",
            (int) phychanswitch->stats.totalTxSignals);
    IO_PrintStat(node, "Physical", "ChanSwitch", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Signals received and forwarded to MAC = %d",
            (int) phychanswitch->stats.totalRxSignalsToMac);
    IO_PrintStat(node, "Physical", "ChanSwitch", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Signals locked on by PHY = %d",
            (int) phychanswitch->stats.totalSignalsLocked);
    IO_PrintStat(node, "Physical", "ChanSwitch", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Signals received but with errors = %d",
            (int) phychanswitch->stats.totalSignalsWithErrors);
    IO_PrintStat(node, "Physical", "ChanSwitch", ANY_DEST, phyIndex, buf);

    PhyChanSwitchChangeState(node,phyIndex, PHY_IDLE);

}



void PhyChanSwitchSignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;
	// PhyChanSwitchChangeState(node,phyIndex,PHY_IDLE);
    char clockStr[20];
    TIME_PrintClockInSecond(getSimTime(node), clockStr);
    Int8 buf[MAX_STRING_LENGTH];

    if(phychanswitch->mode == PHY_TRANSMITTING){
        printf("PhyChanSwitchSignalArrivalFromChannel: terminating transmission at node %d, time %s \n",
            node->nodeId,clockStr);
        PhyChanSwitchTerminateCurrentTransmission(node,phyIndex);
        PhyChanSwitchChangeState(node,phyIndex, PHY_IDLE);

        if(phychanswitch->previousMode != phychanswitch->mode)
            PhyChanSwitchReportStatusToMac(node, phyIndex, phychanswitch->mode);
    }

    sprintf(buf, "PhyChanSwitchSignalArrivalFromChannel: failed on node %d, time %s, mode %d \n",
            node->nodeId, clockStr, phychanswitch->mode);
    ERROR_Assert(phychanswitch->mode != PHY_TRANSMITTING, buf);

    // assert(phychanswitch->mode != PHY_TRANSMITTING);


    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(getSimTime(node), currTime);
        printf("\ntime %s: SignalArrival from %d at %d\n",
               currTime,
               propRxInfo->txMsg->originatingNodeId,
               node->nodeId);
    }
    switch (phychanswitch->mode) {
        case PHY_RECEIVING: {
            double rxPower_mW =
                NON_DB(ANTENNA_GainForThisSignal(node, phyIndex,
                        propRxInfo) + propRxInfo->rxPower_dBm);


            if (!phychanswitch->rxMsgError) {
               phychanswitch->rxMsgError = PhyChanSwitchCheckRxPacketError(
                   node,
                   phychanswitch,
                   NULL);
            }//if//

            phychanswitch->rxTimeEvaluated = getSimTime(node);
            phychanswitch->interferencePower_mW += rxPower_mW;

            break;
        }

        //
        // If the phy is idle or sensing,
        // check if it can receive this signal.
        //
        case PHY_IDLE:
        case PHY_SENSING:
        {
            double rxInterferencePower_mW = NON_DB(
                ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                propRxInfo->rxPower_dBm);

            double rxPowerInOmni_mW = NON_DB(
                ANTENNA_DefaultGainForThisSignal(node, phyIndex, propRxInfo)
                + propRxInfo->rxPower_dBm);

            if (rxPowerInOmni_mW >= phychanswitch->rxSensitivity_mW[0]) {
                PropTxInfo *propTxInfo
                    = (PropTxInfo *)MESSAGE_ReturnInfo(propRxInfo->txMsg);
                clocktype txDuration = propTxInfo->duration;
                double rxPower_mW;

                if (!ANTENNA_IsLocked(node, phyIndex)) {
                   ANTENNA_SetToBestGainConfigurationForThisSignal(
                       node, phyIndex, propRxInfo);

                   PHY_SignalInterference(
                       node,
                       phyIndex,
                       channelIndex,
                       propRxInfo->txMsg,
                       &rxPower_mW,
                       &(phychanswitch->interferencePower_mW));
                }
                else {
                    // ChanSwitch will listen to this signal, taken care of in SignalEnd
                    rxPower_mW = rxInterferencePower_mW;
                }

                PhyChanSwitchLockSignal(
                    node,
                    phychanswitch,
                    propRxInfo->txMsg,
                    rxPower_mW,
                    (propRxInfo->rxStartTime + propRxInfo->duration),
                    propRxInfo->rxDOA);
                PhyChanSwitchChangeState(node,phyIndex, PHY_RECEIVING);
                PhyChanSwitchReportExtendedStatusToMac(
                    node,
                    phyIndex,
                    PHY_RECEIVING,
                    txDuration,
                    propRxInfo->txMsg);
            }
            else {
                //
                // Otherwise, check if the signal changes the phy status
                //

                PhyStatusType newMode;

                phychanswitch->interferencePower_mW += rxInterferencePower_mW;

                if (PhyChanSwitchCarrierSensing(node, phychanswitch)) {
                   newMode = PHY_SENSING;
                } else {
                   newMode = PHY_IDLE;
                }//if//

                if (newMode != phychanswitch->mode) {
                    PhyChanSwitchChangeState(node,phyIndex, newMode);
                    PhyChanSwitchReportStatusToMac(
                        node,
                        phyIndex,
                        newMode);

                }//if//
            }//if//

            break;
        }

        default:
            abort();

    }//switch (phychanswitch->mode)//
}



void PhyChanSwitchTerminateCurrentReceive(
    Node* node, int phyIndex, const BOOL terminateOnlyOnReceiveError,
    BOOL* frameError,
    clocktype* endSignalTime)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*)thisPhy->phyVar;


    *endSignalTime = phychanswitch->rxEndTime;

    if (!phychanswitch->rxMsgError) {
       phychanswitch->rxMsgError = PhyChanSwitchCheckRxPacketError(
           node,
           phychanswitch,
           NULL);
    }//if//

    *frameError = phychanswitch->rxMsgError;

    if ((terminateOnlyOnReceiveError) && (!phychanswitch->rxMsgError)) {
        return;
    }//if//

    if (thisPhy->antennaData->antennaModelType == ANTENNA_OMNIDIRECTIONAL) {

        phychanswitch->interferencePower_mW += phychanswitch->rxMsgPower_mW;
    }
    else {
        int channelIndex;
        PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

        ERROR_Assert(((thisPhy->antennaData->antennaModelType
                    == ANTENNA_SWITCHED_BEAM) ||
               (thisPhy->antennaData->antennaModelType
                    == ANTENNA_STEERABLE) ||
               (thisPhy->antennaData->antennaModelType
                    == ANTENNA_PATTERNED)) ,
                "Illegal antennaModelType");

        if (!ANTENNA_IsLocked(node, phyIndex)) {
            ANTENNA_SetToDefaultMode(node, phyIndex);
        }//if//

        PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phychanswitch->interferencePower_mW));
    }//if//


    PhyChanSwitchUnlockSignal(phychanswitch);
    if (PhyChanSwitchCarrierSensing(node, phychanswitch)) {
        PhyChanSwitchChangeState(node,phyIndex, PHY_SENSING);
    } else {
            PhyChanSwitchChangeState(node,phyIndex, PHY_IDLE);

    }//if//
}

void PhyChanSwitchTerminateCurrentTransmission(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*)thisPhy->phyVar;
     int channelIndex;
    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    //GuiStart
    if (node->guiOption == TRUE) {
        GUI_EndBroadcast(node->nodeId,
                         GUI_PHY_LAYER,
                         GUI_DEFAULT_DATA_TYPE,
                         thisPhy->macInterfaceIndex,
                         getSimTime(node));
    }
    //GuiEnd
    assert(phychanswitch->mode == PHY_TRANSMITTING);

    //Cancel the timer end message so that 'PhyChanSwitchTransmissionEnd' is
    // not called.
    if( phychanswitch->txEndTimer)
    {
        MESSAGE_CancelSelfMsg(node, phychanswitch->txEndTimer);
        phychanswitch->txEndTimer = NULL;
    }
}




void PhyChanSwitchSignalEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;
    double sinr = -1.0;

    BOOL receiveErrorOccurred = FALSE;

    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(getSimTime(node), currTime);
        printf("\ntime %s: SignalEnd from %d at %d\n",
               currTime,
               propRxInfo->txMsg->originatingNodeId,
               node->nodeId);
    }
    assert(phychanswitch->mode != PHY_TRANSMITTING);

    if (phychanswitch->mode == PHY_RECEIVING) {
        if (phychanswitch->rxMsgError == FALSE) {
            phychanswitch->rxMsgError = PhyChanSwitchCheckRxPacketError(
                node,
                phychanswitch,
                &sinr);
            phychanswitch->rxTimeEvaluated = getSimTime(node);
        }//if
    }//if//

    receiveErrorOccurred = phychanswitch->rxMsgError;

    //
    // If the phy is still receiving this signal, forward the frame
    // to the MAC layer.
    //

    if ((phychanswitch->mode == PHY_RECEIVING) &&
        (phychanswitch->rxMsg == propRxInfo->txMsg))
    {
        Message *newMsg;

        if (!ANTENNA_IsLocked(node, phyIndex)) {
            ANTENNA_SetToDefaultMode(node, phyIndex);
            //GuiStart
            if (node->guiOption) {
                GUI_SetPatternIndex(node,
                                    thisPhy->macInterfaceIndex,
                                    ANTENNA_OMNIDIRECTIONAL_PATTERN,
                                    getSimTime(node));
            }
            //GuiEnd
            PHY_SignalInterference(
                node,
                phyIndex,
                channelIndex,
                NULL,
                NULL,
                &(phychanswitch->interferencePower_mW));
        }//if//

        //Perform Signal measurement
        PhySignalMeasurement sigMeasure;
        sigMeasure.rxBeginTime = propRxInfo->rxStartTime;

        double noise = phychanswitch->thisPhy->noise_mW_hz *
                           phychanswitch->channelBandwidth;
        if (phychanswitch->thisPhy->phyModel == PHY_CHANSWITCH &&
            (phychanswitch->rxDataRateType == PHY_CHANSWITCH__6M ||
             phychanswitch->rxDataRateType == PHY_CHANSWITCH_11M))
        {
            noise = noise * 11.0;
        }

        sigMeasure.rss = IN_DB(phychanswitch->rxMsgPower_mW);
        sigMeasure.snr = IN_DB(phychanswitch->rxMsgPower_mW /noise);
        sigMeasure.cinr = IN_DB(phychanswitch->rxMsgPower_mW /
                             (phychanswitch->interferencePower_mW + noise));

		//printf("message power from PhyChanSwitchSignalEndFromChannel = %f \n", phychanswitch->rxMsgPower_mW);

        PhyChanSwitchUnlockSignal(phychanswitch);

        if (PhyChanSwitchCarrierSensing(node, phychanswitch) == TRUE) {

            PhyChanSwitchChangeState(node,phyIndex, PHY_SENSING);
        }
        else {
            PhyChanSwitchChangeState(node,phyIndex, PHY_IDLE);
        }

        if (!receiveErrorOccurred) {
            newMsg = MESSAGE_Duplicate(node, propRxInfo->txMsg);

            MESSAGE_RemoveHeader(
                node, newMsg, sizeof(PhyChanSwitchPlcpHeader), TRACE_CHANSWITCH);

            PhySignalMeasurement* signalMeaInfo;		
            MESSAGE_InfoAlloc(node,
                              newMsg,
                              sizeof(PhySignalMeasurement));
            signalMeaInfo = (PhySignalMeasurement*)
                            MESSAGE_ReturnInfo(newMsg);
            memcpy(signalMeaInfo,&sigMeasure,sizeof(PhySignalMeasurement));


            MESSAGE_SetInstanceId(newMsg, (short) phyIndex);
            MAC_ReceivePacketFromPhy(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                newMsg);

            phychanswitch->stats.totalRxSignalsToMac++;
        }
        else {
            PhyChanSwitchReportStatusToMac(
                node,
                phyIndex,
                phychanswitch->mode);

            phychanswitch->stats.totalSignalsWithErrors++;
        }//if//
    }
    else {
        PhyStatusType newMode;

        double rxPower_mW =
            NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                   propRxInfo->rxPower_dBm);

        phychanswitch->interferencePower_mW -= rxPower_mW;

        if (phychanswitch->interferencePower_mW < 0.0) {
            phychanswitch->interferencePower_mW = 0.0;
        }


        if (phychanswitch->mode != PHY_RECEIVING) {
           if (PhyChanSwitchCarrierSensing(node, phychanswitch) == TRUE) {
               newMode = PHY_SENSING;
           } else {
               newMode = PHY_IDLE;
           }//if//

           if (newMode != phychanswitch->mode) {
                PhyChanSwitchChangeState(node,phyIndex, newMode);
               PhyChanSwitchReportStatusToMac(
                   node,
                   phyIndex,
                   newMode);
           }//if//
        }//if//
    }//if//
}



void PhyChanSwitchSetTransmitPower(PhyData *thisPhy, double newTxPower_mW) {
    PhyDataChanSwitch *phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    phychanswitch->txPower_dBm = (float)IN_DB(newTxPower_mW);
}


void PhyChanSwitchGetTransmitPower(PhyData *thisPhy, double *txPower_mW) {
    PhyDataChanSwitch *phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    *txPower_mW = NON_DB(phychanswitch->txPower_dBm);
}



int PhyChanSwitchGetTxDataRate(PhyData *thisPhy) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    return phychanswitch->dataRate[phychanswitch->txDataRateType];
}


int PhyChanSwitchGetRxDataRate(PhyData *thisPhy) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    return phychanswitch->dataRate[phychanswitch->rxDataRateType];
}


int PhyChanSwitchGetTxDataRateType(PhyData *thisPhy) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    return phychanswitch->txDataRateType;
}


int PhyChanSwitchGetRxDataRateType(PhyData *thisPhy) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    return phychanswitch->rxDataRateType;
}


void PhyChanSwitchSetTxDataRateType(PhyData* thisPhy, int dataRateType) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    assert(dataRateType >= 0 && dataRateType < phychanswitch->numDataRates);
    phychanswitch->txDataRateType = dataRateType;
    phychanswitch->txPower_dBm =
        phychanswitch->txDefaultPower_dBm[phychanswitch->txDataRateType];

    return;
}


void PhyChanSwitchGetLowestTxDataRateType(PhyData* thisPhy, int* dataRateType) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    *dataRateType = phychanswitch->lowestDataRateType;

    return;
}


void PhyChanSwitchSetLowestTxDataRateType(PhyData* thisPhy) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    phychanswitch->txDataRateType = phychanswitch->lowestDataRateType;
    phychanswitch->txPower_dBm =
        phychanswitch->txDefaultPower_dBm[phychanswitch->txDataRateType];

    return;
}


void PhyChanSwitchGetHighestTxDataRateType(PhyData* thisPhy,
                                       int* dataRateType) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    *dataRateType = phychanswitch->highestDataRateType;

    return;
}


void PhyChanSwitchSetHighestTxDataRateType(PhyData* thisPhy) {
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    phychanswitch->txDataRateType = phychanswitch->highestDataRateType;
    phychanswitch->txPower_dBm =
        phychanswitch->txDefaultPower_dBm[phychanswitch->txDataRateType];

    return;
}

void PhyChanSwitchGetHighestTxDataRateTypeForBC(
    PhyData* thisPhy,
    int* dataRateType)
{
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    *dataRateType = phychanswitch->txDataRateTypeForBC;

    return;
}


void PhyChanSwitchSetHighestTxDataRateTypeForBC(
    PhyData* thisPhy)
{
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*) thisPhy->phyVar;

    phychanswitch->txDataRateType = phychanswitch->txDataRateTypeForBC;
    phychanswitch->txPower_dBm =
        phychanswitch->txDefaultPower_dBm[phychanswitch->txDataRateType];

    return;
}


clocktype PhyChanSwitchGetFrameDuration(
    PhyData *thisPhy,
    int dataRateType,
    int size)
{
    switch (thisPhy->phyModel) {
        case PHY_CHANSWITCH: {
            const PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch*)
                                                thisPhy->phyVar;
            const int numOfdmSymbols =
                (int)ceil((size * 8 +
                           PHY_CHANSWITCH_SERVICE_BITS_SIZE +
                           PHY_CHANSWITCH_TAIL_BITS_SIZE) /
                          phychanswitch->numDataBitsPerSymbol[dataRateType]);
            return
                PHY_CHANSWITCH_SYNCHRONIZATION_TIME +
                (numOfdmSymbols * PHY_CHANSWITCH_OFDM_SYMBOL_DURATION) *
                MICRO_SECOND;
        }

        default:
        {
            ERROR_ReportError("Unknown PHY model!\n");
            break;
        }
    }

    // never reachable
    return 0;
}



double PhyChanSwitchGetLastAngleOfArrival(Node* node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch (thisPhy->antennaData->antennaModelType) {

    case ANTENNA_SWITCHED_BEAM:
        {
            return AntennaSwitchedBeamGetLastBoresightAzimuth(node,
                phyIndex);
            break;
        }

    case ANTENNA_STEERABLE:
        {
            return AntennaSteerableGetLastBoresightAzimuth(node, phyIndex);
            break;
        }

    case ANTENNA_PATTERNED:
        {
            return AntennaPatternedGetLastBoresightAzimuth(node, phyIndex);
            break;
        }

    default:
        {
            ERROR_ReportError("AOA not supported for this Antenna Model\n");
            break;
        }
    }//switch//

    abort();
    return 0.0; // just for eliminating compilation warning.
}



static
void StartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne,
    BOOL sendDirectionally,
    double azimuthAngle)
{
    // if (DEBUG)
    // {
        char clockStr[20];
        TIME_PrintClockInSecond(getSimTime(node), clockStr);
        printf("ChanSwitch.cpp: node %d start transmitting at time %s "
               "originated from node %d at time %15" TYPES_64BITFMT "d\n",
               node->nodeId, clockStr,
               packet->originatingNodeId, packet->packetCreationTime);
    // }

    clocktype delayUntilAirborne = initDelayUntilAirborne;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch *)thisPhy->phyVar;
    int channelIndex;
    Message *endMsg;
    int packetsize = MESSAGE_ReturnPacketSize(packet);
    clocktype duration;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (!useMacLayerSpecifiedDelay) {
        delayUntilAirborne = phychanswitch->rxTxTurnaroundTime;
#ifdef NETSEC_LIB
        // Wormhole nodes do not delay
        if (node->macData[thisPhy->macInterfaceIndex]->isWormhole)
        {
            delayUntilAirborne = 0;
        }
#endif // NETSEC_LIB
    }//if//

    assert(phychanswitch->mode != PHY_TRANSMITTING);

    if (sendDirectionally) {
        ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex,
                                               azimuthAngle);
    }//if//


    if (phychanswitch->mode == PHY_RECEIVING) {
        if (thisPhy->antennaData->antennaModelType ==
        ANTENNA_OMNIDIRECTIONAL) {
            phychanswitch->interferencePower_mW += phychanswitch->rxMsgPower_mW;
        }
        else {
            if (!sendDirectionally) {
               ANTENNA_SetToDefaultMode(node, phyIndex);
            }//if//

            //GuiStart
            if (node->guiOption) {
                GUI_SetPatternIndex(node,
                                    thisPhy->macInterfaceIndex,
                                    ANTENNA_OMNIDIRECTIONAL_PATTERN,
                                    getSimTime(node));
            }
            //GuiEnd
            PHY_SignalInterference(
                node,
                phyIndex,
                channelIndex,
                NULL,
                NULL,
                &(phychanswitch->interferencePower_mW));
        }
        PhyChanSwitchUnlockSignal(phychanswitch);
    }
    PhyChanSwitchChangeState(node,phyIndex, PHY_TRANSMITTING);

    duration =
        PhyChanSwitchGetFrameDuration(
            thisPhy, phychanswitch->txDataRateType, packetsize);

    MESSAGE_AddHeader(node, packet, sizeof(PhyChanSwitchPlcpHeader),
                      TRACE_CHANSWITCH);

    char *plcp1 = MESSAGE_ReturnPacket(packet);
    memcpy(plcp1,&phychanswitch->txDataRateType,sizeof(int));

    if (PHY_IsListeningToChannel(node, phyIndex, channelIndex))
    {
        PHY_StopListeningToChannel(node, phyIndex, channelIndex);
    }

    if (thisPhy->antennaData->antennaModelType == ANTENNA_PATTERNED)
    {
        if (!sendDirectionally) {
            PROP_ReleaseSignal(
                node,
                packet,
                phyIndex,
                channelIndex,
                phychanswitch->txPower_dBm,
                duration,
                delayUntilAirborne);
        }
        else {
            PROP_ReleaseSignal(
                node,
                packet,
                phyIndex,
                channelIndex,
                (float)(phychanswitch->txPower_dBm -
                phychanswitch->directionalAntennaGain_dB),
                duration,
                delayUntilAirborne);
        }
    }
    else {
        if (ANTENNA_IsInOmnidirectionalMode(node, phyIndex)) {

            PROP_ReleaseSignal(
                node,
                packet,
                phyIndex,
                channelIndex,
                phychanswitch->txPower_dBm,
                duration,
                delayUntilAirborne);
        } else {

            PROP_ReleaseSignal(
                node,
                packet,
                phyIndex,
                channelIndex,
                (float)(phychanswitch->txPower_dBm -
                 phychanswitch->directionalAntennaGain_dB),
                duration,
                delayUntilAirborne);

        }//if//
    }

    //GuiStart
     if (node->guiOption == TRUE) {
        GUI_Broadcast(node->nodeId,
                      GUI_PHY_LAYER,
                      GUI_DEFAULT_DATA_TYPE,
                      thisPhy->macInterfaceIndex,
                      getSimTime(node));
    }
    //GuiEnd

    endMsg = MESSAGE_Alloc(node,
                            PHY_LAYER,
                            0,
                            MSG_PHY_TransmissionEnd);

    MESSAGE_SetInstanceId(endMsg, (short) phyIndex);
    MESSAGE_Send(node, endMsg, delayUntilAirborne + duration + 1);

    phychanswitch->txEndTimer = endMsg;
    /* Keep track of phy statistics and battery computations */
    phychanswitch->stats.totalTxSignals++;
    phychanswitch->stats.energyConsumed
        += duration * (BATTERY_TX_POWER_COEFFICIENT
                       * NON_DB(phychanswitch->txPower_dBm)
                       + BATTERY_TX_POWER_OFFSET
                       - BATTERY_RX_POWER);
}



//
// Used by the MAC layer to start transmitting a packet.
//
void PhyChanSwitchStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne)
{
    StartTransmittingSignal(
        node, phyIndex,
        packet,
        useMacLayerSpecifiedDelay,
        initDelayUntilAirborne,
        FALSE, 0.0);
}


void PhyChanSwitchStartTransmittingSignalDirectionally(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne,
    double azimuthAngle)
{
    StartTransmittingSignal(
        node, phyIndex,
        packet,
        useMacLayerSpecifiedDelay,
        initDelayUntilAirborne,
        TRUE, azimuthAngle);
}



void PhyChanSwitchLockAntennaDirection(Node* node, int phyNum) {
    ANTENNA_LockAntennaDirection(node, phyNum);
}



void PhyChanSwitchUnlockAntennaDirection(Node* node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataChanSwitch* phychanswitch = (PhyDataChanSwitch *)thisPhy->phyVar;

    ANTENNA_UnlockAntennaDirection(node, phyIndex);

    if ((phychanswitch->mode != PHY_RECEIVING) &&
        (phychanswitch->mode != PHY_TRANSMITTING))
    {
        int channelIndex;

        PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

        ANTENNA_SetToDefaultMode(node, phyIndex);

        PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phychanswitch->interferencePower_mW));
    }//if//

}

