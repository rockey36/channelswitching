
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
 */

#ifndef PHY_CHANSWITCH_H
#define PHY_CHANSWITCH_H

#include "dynamic.h"

/*
 * 802.11a parameters OFDM PHY
 */

// PLCP_SIZE's for 802.11a microseconds
#define PHY_CHANSWITCH_SHORT_TRAINING_SIZE  8 // 10 short symbols
#define PHY_CHANSWITCH_LONG_TRAINING_SIZE   8 // 2 OFDM symbols
#define PHY_CHANSWITCH_SIGNAL_SIZE          4 // 1 OFDM symbol
#define PHY_CHANSWITCH_OFDM_SYMBOL_DURATION 4 // 4 usec

#define PHY_CHANSWITCH_CHANNEL_BANDWIDTH    20000000 //    20 MHz

#define PHY_CHANSWITCH_SERVICE_BITS_SIZE    16 // 2bytes * 8 = 16bits.
#define PHY_CHANSWITCH_TAIL_BITS_SIZE       6  // 6bits  / 8 = 3/4bytes.

#define PHY_CHANSWITCH_PREAMBLE_AND_SIGNAL \
    (PHY_CHANSWITCH_SHORT_TRAINING_SIZE + \
     PHY_CHANSWITCH_LONG_TRAINING_SIZE + \
     PHY_CHANSWITCH_SIGNAL_SIZE)

#define PHY_CHANSWITCH_SYNCHRONIZATION_TIME \
    (PHY_CHANSWITCH_PREAMBLE_AND_SIGNAL * MICRO_SECOND)


#define PHY_CHANSWITCH_RX_TX_TURNAROUND_TIME  (2 * MICRO_SECOND)
#define PHY_CHANSWITCH_PHY_DELAY PHY_CHANSWITCH_RX_TX_TURNAROUND_TIME

#define PHY_CHANSWITCH_NUM_DATA_RATES 8

/*
 * Table 78  Rate-dependent parameters
 * Data rate|Mod Coding rate|Coded bits per sym|Data bits per OFDM sym (Ndbps)
 *      6   | BPSK     1/2  |   1              |  24
 *      9   | BPSK     3/4  |   1              |  36
 *      12  | QPSK     1/2  |   2              |  48
 *      18  | QPSK     3/4  |   2              |  72
 *      24  | 16-QAM   1/2  |   4              |  96
 *      36  | 16-QAM   3/4  |   4              |  144
 *      48  | 64-QAM   2/3  |   6              |  192
 *      54  | 64-QAM   3/4  |   6              |  216
 */
#define PHY_CHANSWITCH__6M  0
#define PHY_CHANSWITCH__9M  1
#define PHY_CHANSWITCH_12M  2
#define PHY_CHANSWITCH_18M  3
#define PHY_CHANSWITCH_24M  4
#define PHY_CHANSWITCH_36M  5
#define PHY_CHANSWITCH_48M  6
#define PHY_CHANSWITCH_54M  7

#define PHY_CHANSWITCH_LOWEST_DATA_RATE_TYPE  PHY_CHANSWITCH__6M
#define PHY_CHANSWITCH_HIGHEST_DATA_RATE_TYPE PHY_CHANSWITCH_54M
#define PHY_CHANSWITCH_DATA_RATE_TYPE_FOR_BC  PHY_CHANSWITCH__6M

#define PHY_CHANSWITCH_NUM_DATA_RATES 8
#define PHY_CHANSWITCH_NUM_BER_TABLES  8

#define PHY_CHANSWITCH_DATA_RATE__6M   6000000
#define PHY_CHANSWITCH_DATA_RATE__9M   9000000
#define PHY_CHANSWITCH_DATA_RATE_12M  12000000
#define PHY_CHANSWITCH_DATA_RATE_18M  18000000
#define PHY_CHANSWITCH_DATA_RATE_24M  24000000
#define PHY_CHANSWITCH_DATA_RATE_36M  36000000
#define PHY_CHANSWITCH_DATA_RATE_48M  48000000
#define PHY_CHANSWITCH_DATA_RATE_54M  54000000

#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__6M   24
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__9M   36
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_12M   48
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_18M   72
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_24M   96
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_36M  144
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_48M  192
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_54M  216

#define PHY_CHANSWITCH_DEFAULT_TX_POWER__6M_dBm  20.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER__9M_dBm  20.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER_12M_dBm  19.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER_18M_dBm  19.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER_24M_dBm  18.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER_36M_dBm  18.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER_48M_dBm  16.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER_54M_dBm  16.0

#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__6M_dBm  -85.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__9M_dBm  -85.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_12M_dBm  -83.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_18M_dBm  -83.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_24M_dBm  -78.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_36M_dBm  -78.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_48M_dBm  -69.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_54M_dBm  -69.0


#define PHY_CHANSWITCH__1M  0
#define PHY_CHANSWITCH__2M  1
#define PHY_CHANSWITCHb__6M  2
#define PHY_CHANSWITCH_11M  3

//#define PHY_CHANSWITCH_LOWEST_DATA_RATE_TYPE  PHY_CHANSWITCH__1M
//#define PHY_CHANSWITCH_HIGHEST_DATA_RATE_TYPE PHY_CHANSWITCH_11M
//#define PHY_CHANSWITCH_DATA_RATE_TYPE_FOR_BC  PHY_CHANSWITCH__2M

//#define PHY_CHANSWITCH_NUM_DATA_RATES  4
//#define PHY_CHANSWITCH_NUM_BER_TABLES  4

#define PHY_CHANSWITCH_DATA_RATE__1M   1000000
#define PHY_CHANSWITCH_DATA_RATE__2M   2000000
//#define PHY_CHANSWITCH_DATA_RATE__6M   5500000
#define PHY_CHANSWITCH_DATA_RATE_11M  11000000

#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__1M   1
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__2M   2
//#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL__6M   5.5
#define PHY_CHANSWITCH_NUM_DATA_BITS_PER_SYMBOL_11M  11

#define PHY_CHANSWITCH_DEFAULT_TX_POWER__1M_dBm  15.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER__2M_dBm  15.0
//#define PHY_CHANSWITCH_DEFAULT_TX_POWER__6M_dBm  15.0
#define PHY_CHANSWITCH_DEFAULT_TX_POWER_11M_dBm  15.0

#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__1M_dBm  -94.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__2M_dBm  -91.0
//#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY__6M_dBm  -87.0
#define PHY_CHANSWITCH_DEFAULT_RX_SENSITIVITY_11M_dBm  -83.0

typedef struct phy_chanswitch_plcp_header {
    int rate;
} PhyChanSwitchPlcpHeader;

//
// PHY power consumption rate in mW.
// Note:
// BATTERY_SLEEP_POWER is not used at this point for the following reasons:
// * Monitoring the channel is assumed to consume as much power as receiving
//   signals, thus the phy mode is either TX or RX in ad hoc networks.
// * Power management between APs and stations needs to be modeled in order to
//   simulate the effect of the sleep (or doze) mode for WLAN environment.
// Also, the power consumption for transmitting signals is calculated as
// (BATTERY_TX_POWER_COEFFICIENT * txPower_mW + BATTERY_TX_POWER_OFFSET).
//
// The values below are set based on these assumptions and the WaveLAN
// specifications.
//
// *** There is no guarantee that the assumptions above are correct! ***
//
#define BATTERY_SLEEP_POWER          (50.0 / SECOND)
#define BATTERY_RX_POWER             (900.0 / SECOND)
#define BATTERY_TX_POWER_OFFSET      BATTERY_RX_POWER
#define BATTERY_TX_POWER_COEFFICIENT (16.0 / SECOND)


/*
 * Structure for phy statistics variables
 */
typedef struct phy_chanswitch_stats_str {
    D_Int32 totalTxSignals;
    D_Int32 totalRxSignalsToMac;
    D_Int32 totalSignalsLocked;
    D_Int32 totalSignalsWithErrors;
    D_Float64 energyConsumed;
    D_Clocktype turnOnTime;
} PhyChanSwitchStats;

/*
 * Structure for Phy.
 */
typedef struct struct_phy_chanswitch_str {
    PhyData*  thisPhy;

    int       txDataRateTypeForBC;
    int       txDataRateType;
    D_Float32 txPower_dBm;
    float     txDefaultPower_dBm[PHY_CHANSWITCH_NUM_DATA_RATES];

    int       rxDataRateType;
    double    rxSensitivity_mW[PHY_CHANSWITCH_NUM_DATA_RATES];

    int       numDataRates;
    int       dataRate[PHY_CHANSWITCH_NUM_DATA_RATES];
    double    numDataBitsPerSymbol[PHY_CHANSWITCH_NUM_DATA_RATES];
    int       lowestDataRateType;
    int       highestDataRateType;

    double    directionalAntennaGain_dB;

    Message*  rxMsg;
    double    rxMsgPower_mW;
    clocktype rxTimeEvaluated;
    BOOL      rxMsgError;
    clocktype rxEndTime;
    Orientation rxDOA;

    Message *txEndTimer;
    D_Int32   channelBandwidth;
    clocktype rxTxTurnaroundTime;
    double    noisePower_mW;
    double    interferencePower_mW;

    PhyStatusType mode;
    PhyStatusType previousMode;

    PhyChanSwitchStats  stats;
} PhyDataChanSwitch;


double
PhyChanSwitchGetSignalStrength(Node *node,PhyDataChanSwitch* phychanswitch);


static /*inline*/
PhyStatusType PhyChanSwitchGetStatus(Node *node, int phyNum) {
    PhyDataChanSwitch* phychanswitch =
       (PhyDataChanSwitch*)node->phyData[phyNum]->phyVar;
    return (phychanswitch->mode);
}

void PhyChanSwitchInit(
    Node* node,
    const int phyIndex,
    const NodeInput *nodeInput);

void PhyChanSwitchChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening);


void PhyChanSwitchTransmissionEnd(Node *node, int phyIndex);


void PhyChanSwitchFinalize(Node *node, const int phyIndex);


void PhyChanSwitchSignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo);


void PhyChanSwitchSignalEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo);


void PhyChanSwitchSetTransmitPower(PhyData *thisPhy, double newTxPower_mW);


void PhyChanSwitchGetTransmitPower(PhyData *thisPhy, double *txPower_mW);

double PhyChanSwitchGetAngleOfArrival(Node* node, int phyIndex);

clocktype PhyChanSwitchGetTransmissionDelay(PhyData *thisPhy, int size);
clocktype PhyChanSwitchGetReceptionDuration(PhyData *thisPhy, int size);

void PhyChanSwitchStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne);

void PhyChanSwitchStartTransmittingSignalDirectionally(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne,
    double azimuthAngle);



int PhyChanSwitchGetTxDataRate(PhyData *thisPhy);
int PhyChanSwitchGetRxDataRate(PhyData *thisPhy);
int PhyChanSwitchGetTxDataRateType(PhyData *thisPhy);
int PhyChanSwitchGetRxDataRateType(PhyData *thisPhy);

void PhyChanSwitchSetTxDataRateType(PhyData* thisPhy, int dataRateType);
void PhyChanSwitchGetLowestTxDataRateType(PhyData* thisPhy, int* dataRateType);
void PhyChanSwitchSetLowestTxDataRateType(PhyData* thisPhy);
void PhyChanSwitchGetHighestTxDataRateType(PhyData* thisPhy, int* dataRateType);
void PhyChanSwitchSetHighestTxDataRateType(PhyData* thisPhy);
void PhyChanSwitchGetHighestTxDataRateTypeForBC(
    PhyData* thisPhy, int* dataRateType);
void PhyChanSwitchSetHighestTxDataRateTypeForBC(
    PhyData* thisPhy);

void PhyChanSwitchTerminateCurrentTransmission(Node* node, int phyIndex);
double PhyChanSwitchGetLastAngleOfArrival(Node* node, int phyIndex);

void PhyChanSwitchLockAntennaDirection(Node* node, int phyNum);

void PhyChanSwitchUnlockAntennaDirection(Node* node, int phyNum);

void PhyChanSwitchTerminateCurrentReceive(
    Node* node, int phyIndex, const BOOL terminateOnlyOnReceiveError,
    BOOL* frameError, clocktype* endSignalTime);

//
// The following functions are defined in phy_chanswitch_ber_*.c
//
//void PhyChanSwitchSetBerTable(PhyData* thisPhy);
//void PhyChanSwitchbSetBerTable(PhyData* thisPhy);


//void PhyChanSwitch_InitBerTable(void);
//void PhyChanSwitchb_InitBerTable(void);


clocktype PhyChanSwitchGetFrameDuration(
    PhyData *thisPhy,
    int dataRateType,
    int size);

BOOL PhyChanSwitchMediumIsIdle(
    Node* node,
    int phyIndex);

BOOL PhyChanSwitchMediumIsIdleInDirection(
    Node* node,
    int phyIndex,
    double azimuth);

void PhyChanSwitchSetSensingDirection(Node* node, int phyIndex, double azimuth);

void PhyChanSwitchChangeState(Node* node, int phyIndex, PhyStatusType newStatus);

#endif /* PHY_CHANSWITCH_H */
