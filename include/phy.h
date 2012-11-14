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
// PACKAGE     :: PHYSICAL LAYER
// DESCRIPTION :: This file describes data structures and functions used by the Physical Layer.
//                Most of this functionality is enabled/used in the Wireless library.
// **/


#ifndef PHY_H
#define PHY_H

#include "propagation.h"
#include "dynamic.h"

#ifdef WIRELESS_LIB
#include "antenna_global.h"
#include "energy_model.h"
#endif // WIRELESS_LIB
#ifdef ADDON_NGCNMS
#include "power_control.h"
#endif


// /**
// CONSTANT    :: PHY_DEFAULT_NOISE_FACTORPHY_DEFAULT_NOISE_FACTOR : 10.0
// DESCRIPTION :: Default noise factor in physical medium
// **/
#define PHY_DEFAULT_NOISE_FACTOR     10.0

// /**
// CONSTANT    :: PHY_DEFAULT_TEMPERATUREPHY_DEFAULT_TEMPERATURE : 290.0
// DESCRIPTION :: Default temperature of physical medium.
// **/
#define PHY_DEFAULT_TEMPERATURE      290.0

// /**
// CONSTANT    :: PHY_DEFAULT_MIN_PCOM_VALUE : 0.0
// DESCRIPTION :: Default minimum pcom value threshold
// **/
#define PHY_DEFAULT_MIN_PCOM_VALUE      0.0

// /**
// CONSTANT    :: PHY_DEFAULT_SYNC_COLLISION_WINDOW : 1ms
// DESCRIPTION :: Default minimum pcom value threshold
// **/
#define PHY_DEFAULT_SYNC_COLLISION_WINDOW    1 * MILLI_SECOND

// /**
// ENUM        :: PhyModel
// DESCRIPTION :: Different phy types supported.
// **/
enum PhyModel {
    PHY802_11a,
    PHY802_11b,
    PHY_ABSTRACT,
    PHY_GSM,
    PHY_FCSC_PROTOTYPE,
    PHY_SPAWAR_LINK16,
    PHY_BOEINGFCS,
    PHY_SATELLITE_RSV,
    PHY802_16,
    PHY_CELLULAR,
    PHY_JTRS_WNW,
    PHY_JAMMING,
    PHY802_15_4,
    PHY_NONE,
		PHY_CHANSWITCH
};

// /**
// ENUM        :: PhyRxModel
// DESCRIPTION :: Different types of packet reception model
// **/
enum PhyRxModel {
    RX_802_11a,
    RX_802_11b,
		RX_CHANSWITCH,
    RX_802_16,
    RX_UMTS,
    RX_802_15_4,
    SNR_THRESHOLD_BASED,
    BER_BASED,
    PCOM_BASED
};

//**
// ENUM        :: PhyStatusType
// DESCRIPTION :: Status of physical layer
// **/
enum PhyStatusType {
  PHY_IDLE,
  PHY_SENSING,
  PHY_RECEIVING,
  PHY_TRANSMITTING,

  PHY_BUSY_TX,
  PHY_BUSY_RX,
  PHY_SUCCESS,
  PHY_TRX_OFF
};

// /**
// STRUCT      :: PhyBerEntry
// DESCRIPTION :: SNR/BER curve entry
// **/
struct PhyBerEntry {
    double snr;
    double ber;
};

// /**
// STRUCT      :: PhyBerTable
// DESCRIPTION :: Bit Error Rate table.
// **/
struct PhyBerTable {
    char         fileName[MAX_STRING_LENGTH];
    int          numEntries;

    // When the snr values all have the same interval
    // we can lookup entires by snr directly instead
    // of performing a binary search.
    bool         isFixedInterval;
    double       interval;
    double       snrStart;
    double       snrEnd;
    PhyBerEntry* entries;
};

void
PHY_BerTablesPrepare (PhyBerTable * berTables, const int tableCount);

PhyBerTable *
PHY_BerTablesAlloc (const int tableCount);


// /**
// STRUCT      :: PhySignalMeasurement
// DESCRIPTION :: Measurement of the signal of received pkt
// **/
struct PhySignalMeasurement {
    clocktype rxBeginTime;  // signal arrival time
    double snr;  // signal to noise ratio
    double rss;  // receving signal strength in mW
    double cinr; // signal to interference noise ratio

    // only used by PHY802.16
    unsigned char fecCodeModuType; // coding modulation type for the burst
};

// /**
// STRUCT :: AntennaModel
// DESCRIPTION :: Structure for classifying different types of antennas.
// **/

struct AntennaModel {
#ifdef WIRELESS_LIB
    AntennaModelType   antennaModelType;
    int                numModels;
    AntennaPatternType antennaPatternType;
    void               *antennaVar;
#endif // WIRELESS_LIB
};


// /**
// STRUCT :: AntennaOmnidirectional
// DESCRIPTION :: Structure for an omnidirectional antenna.
// **/

struct AntennaOmnidirectional {
    float antennaHeight;
    float antennaGain_dB;
};


// /**
// STRUCT      :: struct phy_pcom_str
// DESCRIPTION :: Used by Phy layer to store PCOM values
// **/
typedef struct phy_pcom_str {
    clocktype startTime;
    clocktype endTime;
    double pcom;
    struct phy_pcom_str *next;
} PhyPcomItem;


// /**
// STRUCT      :: PhyData
// DESCRIPTION :: Structure for phy layer
// **/

struct PhyData {
    int         phyIndex;
    int         macInterfaceIndex;
    Address*    networkAddress;
    D_BOOL*     channelListenable;
    D_BOOL*     channelListening;
    BOOL        phyStats;
    int         channelIndexForTransmission;

    PhyModel       phyModel;
    PhyRxModel     phyRxModel;
    double         phyRxSnrThreshold;
    double         noise_mW_hz;
    int            numBerTables;
    PhyBerTable*   snrBerTables;
    RandomSeed     seed;
    void*          phyVar;

    double         systemLoss_dB;
    AntennaModel*  antennaData;

    // indicate whether the contention free propagation is enabled
    BOOL           contentionFreeProp;

#ifdef ADDON_NGCNMS
    PowerControlData*  powerControlData;
    int numNgcBytesTx;
#endif

    void            *nodeLinkLossList;
    void            *nodeLinkDelayList;
#ifdef ADDON_BOEINGFCS
    //boeingfcs pcom stuff
    PhyPcomItem** pcomTable;
    double minPcomValue;
    clocktype syncCollisionWindow;
    BOOL collisionWindowActive;
    NodeAddress networkId;
#endif
#ifdef WIRELESS_LIB
    //Morteza:added for battery model
    BOOL energyStats;
    EnergyModelType eType;
    PowerCosts*    powerConsmpTable;//added for energy modeling
    LoadProfile*  curLoad;
    EnergyModelGeneric genericEnergyModelParameters;
#endif //WIRELESS_LIB
    double         noiseFactor;  //added for 802.16
};


// /**
// STRUCT      :: PacketPhyStatus
// DESCRIPTION :: Used by Phy layer to report channel status to mac layer
// **/
struct PacketPhyStatus {
    PhyStatusType status;
    clocktype receiveDuration;
    const Message* thePacketIfItGetsThrough;
};

// /**
// FUNCTION   :: PHY_GlobalBerInit
// LAYER      :: Physical
// PURPOSE    :: Pre-load all the BER files.
// PARAMETERS ::
// + nodeInput : const NodeInput* : structure containing contents of input file
// RETURN     :: void :
// **/
void PHY_GlobalBerInit(NodeInput * nodeInput);

// /**
// FUNCTION   :: PHY_GetSnrBerTableByName
// LAYER      :: Physical
// PURPOSE    :: Get a pointer to a specific BER table.
// PARAMETERS ::
// + tableName : char* : name of the BER file
// RETURN     :: void :
// **/
PhyBerTable* PHY_GetSnrBerTableByName(char* tableName);

// /**
// FUNCTION   :: PHY_Init
// LAYER      :: Physical
// PURPOSE    :: Initialize physical layer
// PARAMETERS ::
// + node      : Node*            : node being initialized
// + nodeInput : const NodeInput* : structure containing contents of input file
// RETURN     :: void :
// **/
void PHY_Init(
    Node *node,
    const NodeInput *nodeInput);

// /**
// FUNCTION           :: PHY_CreateAPhyForMac
// LAYER              :: Physical
// PURPOSE            :: Initialization function for the phy layer
// PARAMETERS         ::
// + node              : Node*            : node being initialized
// + nodeInput         : const NodeInput* : structure containing contents of input file
// + interfaceIndex    : int              : interface being initialized.
// + networkAddress    : int              : address of the interface.
// + phyModel          : PhyModel         : Which phisical model is used.
// + phyNumber         : int*             : returned value to be used as phyIndex
// RETURN             :: void :
// **/
void PHY_CreateAPhyForMac(
    Node *node,
    const NodeInput *nodeInput,
    int interfaceIndex,
    Address *networkAddress,
    PhyModel phyModel,
    int* phyNumber);

// /**
// FUNCTION   :: PHY_Finalize
// LAYER      :: Physical
// PURPOSE    :: Called at the end of simulation to collect the results of
//               the simulation of the Phy Layer.
// PARAMETERS ::
// + node      : Node *   : node for which results are to be collected
// RETURN     :: void :
// **/
void PHY_Finalize(Node *node);

// /**
// FUNCTION   :: PHY_ProcessEvent
// LAYER      :: Physical
// PURPOSE    :: Models the behaviour of the Phy Layer on receiving the
//               message encapsulated in msgHdr
// PARAMETERS ::
// + node      : Node*     : node which received the message
// + msg       : Message*  : message received by the layer
// RETURN     :: void :
// **/
void PHY_ProcessEvent(Node *node, Message *msg);

// /**
// API        :: PHY_GetStatus
// LAYER      :: Physical
// PURPOSE    :: Retrieves the Phy's current status
// PARAMETERS ::
// + node      : Node *   : node for which stats are to be collected
// + phyNum    : int      : interface for which stats are to be collected
// RETURN     :: PhyStatusType : status of interface.
// **/
PhyStatusType PHY_GetStatus(Node* node, int phyNum);

// /**
// API            :: PHY_SetTransmitPower
// LAYER          :: Physical
// PURPOSE        :: Sets the Radio's transmit power in mW
// PARAMETERS     ::
// + node          : Node * : node for which transmit power is to be set
// + phyIndex      : int    : interface for which transmit power is to be set
// + newTxPower_mW : double : transmit power(mW)
// RETURN         :: void :
// **/
void PHY_SetTransmitPower(
    Node *node,
    int phyIndex,
    double newTxPower_mW);



// /**
// API            :: PHY_SetRxSNRThreshold
// LAYER          :: Physical
// PURPOSE        :: Sets the Radio's Rx SNR Threshold
// PARAMETERS     ::
// + node          : Node * : node for which transmit power is to be set
// + phyIndex      : int    : interface for which transmit power is to be set
// + snr : double : threshold value to be set
// RETURN         :: void :
// **/
void PHY_SetRxSNRThreshold(
    Node *node,
    int phyIndex,
    double snr);

// /**
// API            :: PHY_SetDataRate
// LAYER          :: Physical
// PURPOSE        :: Sets the Radio's Data Rate
// PARAMETERS     ::
// + node          : Node * : node for which transmit power is to be set
// + phyIndex      : int    : interface for which transmit power is to be set
// + dataRate : int : dataRate value to be set
// RETURN         :: void :
// **/
void PHY_SetDataRate(
    Node *node,
    int phyIndex,
    int dataRate);


// /**
// API            :: PHY_GetTransmitPower
// LAYER          :: Physical
// PURPOSE        :: Gets the Radio's transmit power in mW.
// PARAMETERS     ::
// + node          : Node *  : Node that is
//                             being instantiated in.
// + phyIndex      : int     : interface index.
// + txPower_mW    : double* : transmit power(mW)
// RETURN         :: void :
// **/
void PHY_GetTransmitPower(
    Node *node,
    int phyIndex,
    double *txPower_mW);

// /**
// API        :: PHY_GetTransmissionDelay
// LAYER      :: Physical
// PURPOSE    :: Get transmission delay
//               based on the first (usually lowest) data rate
//               WARNING: This function call is to be replaced with
//               PHY_GetTransmissionDuration() with an appropriate data rate
// PARAMETERS ::
// + node      : Node *    : node pointer to node
// + phyIndex  : int       : interface index
// + size      : int       : size of the frame in bytes
// RETURN     :: clocktype : transmission delay.
// **/
clocktype PHY_GetTransmissionDelay(
    Node *node,
    int phyIndex,
    int size);

// /**
// API            :: PHY_GetTransmissionDuration
// LAYER          :: Physical
// PURPOSE        :: Get transmission duration of a structured signal fragment.
// PARAMETERS     ::
// + node          : Node *    : node pointer to node
// + phyIndex      : int       : interface index.
// + dataRateIndex : int       : data rate.
// + size          : int       : size of frame in bytes.
// RETURN         :: clocktype : transmission duration
// **/
clocktype PHY_GetTransmissionDuration(
    Node *node,
    int phyIndex,
    int dataRateIndex,
    int size);

// /**
// API            :: PHY_GetFrameModel
// LAYER          :: Physical
// PURPOSE        :: Get Physical Model
// PARAMETERS     ::
// + node          : Node *    : node pointer to node
// + phyNum        : int       : interface index
// RETURN         :: PhyModel : Physical Model
// **/
PhyModel PHY_GetModel(
    Node* node,
    int phyNum);

// /**
// API                        :: PHY_StartTransmittingSignal
// LAYER                      :: Physical
// PURPOSE                    :: Starts transmitting a packet.
// PARAMETERS                 ::
// + node                      : Node *    : node pointer to node
// + phyNum                    : int       : interface index
// + msg                       : Message*  : packet to be sent
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + delayUntilAirborne        : clocktype : delay until airborne
// RETURN                     :: void :
// **/
void PHY_StartTransmittingSignal(
   Node* node,
   int phyNum,
   Message* msg,
   BOOL useMacLayerSpecifiedDelay,
   clocktype delayUntilAirborne,
   NodeAddress destAddr = ANY_DEST);

// /**
// API                        :: PHY_StartTransmittingSignal
// LAYER                      :: Physical
// PURPOSE                    :: Starts transmitting a packet.
//                               Function is being overloaded
// PARAMETERS                 ::
// + node                      : Node *    : node pointer to node
// + phyNum                    : int       : interface index
// + msg                       : Message*  : packet to be sent
// + duration                  : clocktype : specified transmission delay
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + delayUntilAirborne        : clocktype : delay until airborne
// RETURN                     :: void :
// **/
void PHY_StartTransmittingSignal(
   Node* node,
   int phyNum,
   Message* msg,
   clocktype duration,
   BOOL useMacLayerSpecifiedDelay,
   clocktype delayUntilAirborne,
   NodeAddress destAddr = ANY_DEST);

// /**
// API                        :: PHY_StartTransmittingSignal
// LAYER                      :: Physical
// PURPOSE                    :: Starts transmitting a packet.
// PARAMETERS                 ::
// + node                      : Node *    : node pointer to node
// + phyNum                    : int       : interface index
// + msg                       : Message*  : packet to be sent
// + bitSize                   : int       : specified size of the packet in bits
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + delayUntilAirborne        : clocktype : delay until airborne
// RETURN                     :: void :
// **/
void PHY_StartTransmittingSignal(
   Node* node,
   int phyNum,
   Message* msg,
   int bitSize,
   BOOL useMacLayerSpecifiedDelay,
   clocktype delayUntilAirborne,
   NodeAddress destAddr = ANY_DEST);

// /**
// API           :: PHY_SignalArrivalFromChannel
// LAYER         :: Physical
// PURPOSE       :: Called when a new signal arrives
// PARAMETERS    ::
// + node         : Node *      : node pointer to node
// + phyIndex     : int         : interface index
// + channelIndex : int         : channel index
// + propRxInfo   : PropRxInfo* : information on the arrived signal
// RETURN        :: void :
// **/
void PHY_SignalArrivalFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo
   );

// /**
// API           :: PHY_SignalEndFromChannel
// LAYER         :: Physical
// PURPOSE       :: Called when the current signal ends
// PARAMETERS    ::
// + node         : Node *      : node pointer to node
// + phyIndex     : int         : interface index
// + channelIndex : int         : channel index
// + propRxInfo   : PropRxInfo* : information on the arrived signal
// RETURN        :: void :
// **/
void PHY_SignalEndFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo);

void PHY_ChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening);

// /**
// API        :: PHY_GetTxDataRate
// LAYER      :: Physical
// PURPOSE    :: Get transmission data rate
// PARAMETERS ::
// + node      : Node *      : node pointer to node
// + phyIndex  : int         : interface index
// RETURN     :: int :
// **/
int PHY_GetTxDataRate(Node *node, int phyIndex);

// /**
// API        :: PHY_GetRxDataRate
// LAYER      :: Physical
// PURPOSE    :: Get reception data rate
// PARAMETERS ::
// + node      : Node *      : node pointer to node
// + phyIndex  : int         : interface index
// RETURN     :: int :
// **/
int PHY_GetRxDataRate(Node *node, int phyIndex);

// /**
// API        :: PHY_GetTxDataRateType
// LAYER      :: Physical
// PURPOSE    :: Get transmission data rate type
// PARAMETERS ::
// + node      : Node *      : node pointer to node
// + phyIndex  : int         : interface index
// RETURN     :: int :
// **/
int PHY_GetTxDataRateType(Node *node, int phyIndex);

// /**
// API        :: PHY_GetRxDataRateType
// LAYER      :: Physical
// PURPOSE    :: Get reception data rate type
// PARAMETERS ::
// + node      : Node *      : node pointer to node
// + phyIndex  : int         : interface index
// RETURN     :: int :
// **/
int PHY_GetRxDataRateType(Node *node, int phyIndex);

// /**
// API            :: PHY_SetTxDataRateType
// LAYER          :: Physical
// PURPOSE        :: Set transmission data rate type
// PARAMETERS     ::
// + node          : Node *      : node pointer to node
// + phyIndex      : int         : interface index
// +  dataRateType : int         : rate of data
// RETURN         :: void :
// **/
void PHY_SetTxDataRateType(Node *node, int phyIndex, int dataRateType);

// /**
// API            :: PHY_GetLowestTxDataRateType
// LAYER          :: Physical
// PURPOSE        :: Get the lowest transmission data rate type
// PARAMETERS     ::
// + node          : Node*       : node pointer to node
// + phyIndex      : int         : interface index
// + dataRateType  : int*        : rate of data
// RETURN         :: void :
// **/
void PHY_GetLowestTxDataRateType(Node *node, int phyIndex,
                                 int* dataRateType);

// /**
// API            :: PHY_SetLowestTxDataRateType
// LAYER          :: Physical
// PURPOSE        :: Set the lowest transmission data rate type
// PARAMETERS     ::
// + node          : Node*       : node pointer to node
// + phyIndex      : int         : interface index
// RETURN         :: void :
// **/
void PHY_SetLowestTxDataRateType(Node *node, int phyIndex);

// /**
// API            :: PHY_GetHighestTxDataRateType
// LAYER          :: Physical
// PURPOSE        :: Get the highest transmission data rate type
// PARAMETERS     ::
// + node          : Node*       : node pointer to node
// + phyIndex      : int         : interface index
// + dataRateType  : int*        : rate of data
// RETURN         :: void :
void PHY_GetHighestTxDataRateType(Node *node, int phyIndex,
                                  int* dataRateType);

// /**
// API            :: PHY_SetHighestTxDataRateType
// LAYER          :: Physical
// PURPOSE        :: Set the highest transmission data rate type
// PARAMETERS     ::
// + node          : Node*       : node pointer to node
// + phyIndex      : int         : interface index
// RETURN         :: void :
void PHY_SetHighestTxDataRateType(Node *node, int phyIndex);

// /**
// API            :: PHY_GetHighestTxDataRateTypeForBC
// LAYER          :: Physical
// PURPOSE        :: Get the highest transmission data rate type for broadcast
// PARAMETERS     ::
// + node          : Node*       : node pointer to node
// + phyIndex      : int         : interface index
// + dataRateType  : int*        : rate of data
// RETURN         :: void :
void PHY_GetHighestTxDataRateTypeForBC(
    Node *node, int phyIndex, int* dataRateType);

// /**
// API            :: PHY_SetHighestTxDataRateTypeForBC
// LAYER          :: Physical
// PURPOSE        :: Set the highest transmission data rate type for broadcast
// PARAMETERS     ::
// + node          : Node*       : node pointer to node
// + phyIndex      : int         : interface index
// RETURN         :: void :
void PHY_SetHighestTxDataRateTypeForBC(
    Node *node, int phyIndex);

// /**
// API        :: PHY_ComputeSINR
// LAYER      :: Physical
// PURPOSE    :: Compute SINR
// PARAMETERS ::
// + phyData              : PhyData * : PHY layer data
// + signalPower_mW       : double *  : Signal power
// + interferencePower_mW : double*   : Interference power
// + bandwidth            : int       : Bandwidth
// RETURN                :: double    : Signal to Interference and Noise Ratio
// **/
double PHY_ComputeSINR(
    PhyData *phyData,
    double signalPower_mW,
    double interferencePower_mW,
    int bandwidth);

// /**
// API                   :: PHY_SignalInterference
// LAYER                 :: Physical
// PURPOSE               :: Compute Power from the desired signal
//                          and interference
// PARAMETERS            ::
// + node                 : Node*     : Node that is being
//                                      instantiated in
// + phyIndex             : int       : interface number
// + channelIndex         : int       : channel index
// + msg                  : Message * : message including desired signal
// + signalPower_mW       : double *  : power from the desired signal
// + interferencePower_mW : double*   : power from interfering signals
// RETURN                :: void :
// **/
void PHY_SignalInterference(
    Node *node,
    int phyIndex,
    int channelIndex,
    Message *msg,
    double *signalPower_mW,
    double *interferencePower_mW,
    int     subChannelIndex = -1);

// /**
// API            :: PHY_BER
// LAYER          :: Physical
// PURPOSE        :: Get BER
// PARAMETERS     ::
// + phyData       : PhyData * : PHY layer data
// + berTableIndex : int       : index for BER tables
// + sinr          : double    : Signal to Interference and Noise Ratio
// RETURN         :: double    : Bit Error Rate
// **/
double PHY_BER(
    PhyData *phyData,
    int berTableIndex,
    double sinr);

// /**
// API           :: PHY_StartListeningToChannel
// LAYER         :: Physical
// PURPOSE       :: Start listening to the specified channel
// PARAMETERS    ::
// + node         : Node*  : Node that is being
//                           instantiated in
// + phyIndex     : int    : interface number
// + channelIndex : int    : channel index
// RETURN        :: void :
// **/
void PHY_StartListeningToChannel(
    Node *node,
    int phyIndex,
    int channelIndex);

// /**
// API           :: PHY_StopListeningToChannel
// LAYER         :: Physical
// PURPOSE       ::
// PARAMETERS    ::
// + node         : Node*  : Node that is being
//                           instantiated in
// + phyIndex     : int    : interface number
// + channelIndex : int    : channel index
// RETURN        :: void :
// **/
void PHY_StopListeningToChannel(
    Node *node,
    int phyIndex,
    int channelIndex);

// /**
// API           :: PHY_CanListenToChannel
// LAYER         :: Physical
// PURPOSE       :: Check if it can listen to the channel
// PARAMETERS    ::
// + node         : Node*  : Node that is being
//                           instantiated in
// + phyIndex     : int    : interface number
// + channelIndex : int    : channel index
// RETURN        :: BOOL :
// **/
BOOL PHY_CanListenToChannel(Node *node, int phyIndex, int channelIndex);

// /**
// API           :: PHY_IsListeningToChannel
// LAYER         :: Physical
// PURPOSE       :: Check if it is listening to the channel
// PARAMETERS    ::
// + node         : Node*  : Node that is being
//                           instantiated in
// + phyIndex     : int    : interface number
// + channelIndex : int    : channel index
// RETURN        :: BOOL :
// **/
BOOL PHY_IsListeningToChannel(Node *node, int phyIndex, int channelIndex);

// /**
// API           :: PHY_SetTransmissionChannel
// LAYER         :: Physical
// PURPOSE       :: Set the channel index used for transmission
// PARAMETERS    ::
// + node         : Node*  : Node that is being
//                           instantiated in
// + phyIndex     : int    : interface number
// + channelIndex : int    : channel index
// RETURN        :: void :
// **/
void PHY_SetTransmissionChannel(Node *node, int phyIndex, int channelIndex);

// /**
// API           :: PHY_GetTransmissionChannel
// LAYER         :: Physical
// PURPOSE       :: Get the channel index used for transmission
// PARAMETERS    ::
// + node         : Node*  : Node that is being
//                           instantiated in
// + phyIndex     : int    : interface number
// + channelIndex : int    : channel index
// RETURN        :: void :
// **/
void PHY_GetTransmissionChannel(Node *node, int phyIndex, int *channelIndex);

// /**
// API        :: PHY_MediumIsIdle
// LAYER      :: Physical
// PURPOSE    :: Check if the medium is idle
// PARAMETERS ::
// + node      : Node*  : Node that is being
//                        instantiated in
// + phyNum    : int    : interface number
// RETURN     :: BOOL :
// **/
BOOL PHY_MediumIsIdle(Node* node, int phyNum);

// /**
// API        :: PHY_MediumIsIdleInDirection
// LAYER      :: Physical
// PURPOSE    :: Check if the medium is idle if sensed directionally
// PARAMETERS ::
// + node      : Node*  : Node that is being
//                        instantiated in
// + phyNum    : int    : interface number
// + azimuth   : double : azimuth (in degrees)
// RETURN     :: BOOL :
// **/
BOOL PHY_MediumIsIdleInDirection(Node* node, int phyNum, double azimuth);

// /**
// API        :: PHY_SetSensingDirection
// LAYER      :: Physical
// PURPOSE    :: Set the sensing direction
// PARAMETERS ::
// + node      : Node*  : Node that is being
//                        instantiated in
// + phyNum    : int    : interface number
// + azimuth   : double : azimuth (in degrees)
// RETURN     :: void :
// **/
void PHY_SetSensingDirection(Node* node, int phyNum, double azimuth);

// /**
// API                        :: PHY_StartTransmittingSignalDirectionally
// LAYER                      :: Physical
// PURPOSE                    :: Start transmitting a signal directionally
// PARAMETERS                 ::
// + node                      : Node* : Node that is
//                                       being instantiated in
// + phyNum                    : int   : interface number
// + msg                       : Message*  : signal to transmit
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + delayUntilAirborne        : clocktype : delay until airborne
// + directionAzimuth          : double    : azimuth to transmit the signal
// RETURN                     :: void :
// **/
void PHY_StartTransmittingSignalDirectionally(
   Node* node,
   int phyNum,
   Message* msg,
   BOOL useMacLayerSpecifiedDelay,
   clocktype delayUntilAirborne,
   double directionAzimuth);

// /**
// API        :: PHY_LockAntennaDirection
// LAYER      :: Physical
// PURPOSE    :: Lock the direction of antenna
// PARAMETERS ::
// + node      : Node* : Node that is being
//                       instantiated in
// + phyNum    : int   : interface number
// RETURN     :: void :
// **/
void PHY_LockAntennaDirection(
   Node* node,
   int phyNum);

// /**
// API        :: PHY_UnlockAntennaDirection
// LAYER      :: Physical
// PURPOSE    :: Unlock the direction of antenna
// PARAMETERS ::
// + node      : Node* : Node that is being
//                       instantiated in
// + phyNum    : int   : interface number
// RETURN     :: void :
// **/
void PHY_UnlockAntennaDirection(
   Node* node,
   int phyNum);

// /**
// API        :: PHY_GetLastSignalsAngleOfArrival
// LAYER      :: Physical
// PURPOSE    :: Get the AOA of the last signal
// PARAMETERS ::
// + node      : Node*  : Node that is being
//                        instantiated in
// + phyNum    : int    : interface number
// RETURN     :: double : AOA
// **/
double PHY_GetLastSignalsAngleOfArrival(Node* node, int phyNum);


// /**
// API                           :: PHY_TerminateCurrentReceive
// LAYER                         :: Physical
// PURPOSE                       :: Terminate the current signal reception
// PARAMETERS                    ::
// + node                         : Node*      : Node pointer that the
//                                               protocol is being
//                                               instantiated in
// + phyNum                       : int        : interface number
// + terminateOnlyOnReceiveError  : const BOOL : terminate only when
//                                               the error happened
// + receiveError                 : BOOL*      : if error happened
// + endSignalTime                : clocktype* : end of signal
// RETURN                        :: void :
// **/
void PHY_TerminateCurrentReceive(
   Node* node, int phyNum, const BOOL terminateOnlyOnReceiveError,
   BOOL*   receiveError, clocktype* endSignalTime);

// /**
// FUNCTION         :: PHY_PropagationRange
// PURPOSE          :: Calculates an estimated radio range for the PHY.
//                     Supports only TWO-RAY and FREE-SPACE.
// PARAMETERS       ::
// + node            : Node*    : the node of interest
// + interfaceIndex  : int      : the interface to use
// + printAll        : BOOL     : if TRUE, prints the range for all data
//                                rates, otherwise returns the longest.
// RETURN           :: double   : the range in meters
// **/
double PHY_PropagationRange(Node *node,
                            int   interfaceIndex,
                            BOOL  printAll);
#ifdef ADDON_BOEINGFCS
void PHY_PcomInit(PartitionData *partitionData);
#endif

// /**
// FUNCTION :: ENERGY_Init
// PURPOSE  ::  This function declares energy model variables and initializes them.
//      Moreover, the function read energy model specifications and configures
//      the parameters which are configurable.
// PARAMETERS ::
// + node : Node* : the node of interest.
// + phyIndex : const int : the PHY index.
// + nodeInput : NodeInput* : the node input.
// RETURN   :: void :
// **/
void
ENERGY_Init(Node *node,
            const int phyIndex,
            const NodeInput *nodeInput);

// /**
// FUNCTION :: ENERGY_PrintStats
// PURPOSE :: To print the statistic of Energy Model.
// PARAMETERS ::
// + node : Node*    : the node of interest.
// + phyIndex  : const int : the PHY index.
// RETURN           :: void :
// **/
void
ENERGY_PrintStats(Node *node,
                  const int phyIndex);

// /**
// FUNCTION :: Phy_ReportStatusToEnergyModel
// PURPOSE  :: This function should be called whenever a state transition occurs
//  in any place in PHY layer. As input parameters, the function reads the current
//  state and the new state of PHY layer and based on the new sates calculates the cost
//  of the load that should be taken off the battery.
//  The function then interacts with battery model and updates the charge of battery.
// PARAMETERS ::
// + node : Node* : the node of interest.
// + phyIndex : const int : the PHY index.
// + prevStatus : PhyStatusType : the the previous status.
// + newStatus : PhyStatusType : the the new status.
// RETURN           :: void :
// **/
void
Phy_ReportStatusToEnergyModel(Node* node,
                            const int phyIndex,
                            PhyStatusType prevStatus,
                            PhyStatusType newStatus);

// /**
// FUNCTION :: Generic_UpdateCurrentLoad
// PURPOSE :: To update the current load of generic energy model.
// PARAMETERS ::
// + node : Node*    : the node of interest.
// + phyIndex  : const int : the PHY index.
// RETURN           :: void :
// **/
void
Generic_UpdateCurrentLoad(Node* node, const int phyIndex);


#ifdef ADDON_NGCNMS

void PHY_Reset(Node* node, int interfaceIndex);

#endif

#endif /* PHY_H */

