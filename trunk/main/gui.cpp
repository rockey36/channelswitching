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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#define HUMAN_IN_THE_LOOP_DEMO 1
#define HITL 1

#ifdef _WIN32
#include "qualnet_mutex.h"
#include "winsock.h"
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#ifndef s6_addr32
#define s6_addr32 s6_addr
#endif
#include <netdb.h>
#include "qualnet_mutex.h"
#endif


//#define SOCKET unsigned int

#include <sys/types.h>
#include <time.h>

#include "api.h"
#include "clock.h"
#include "qualnet_error.h"
#include "fileio.h"
#include "gui.h"
#include "partition.h"
#include "phy.h"
#include "external_util.h"
#include "scheduler.h"

#include "WallClock.h"

#ifdef HLA_INTERFACE
#include "hla.h"
#endif /* HLA_INTERFACE */

#ifdef CELLULAR_LIB
#include "phy_gsm.h"
#include "phy_cellular.h"
#endif
#ifdef UMTS_LIB
#include "phy_cellular.h"
#include "phy_umts.h"
#endif // CELLULAR_LIB

#ifdef WIRELESS_LIB
#include "phy_802_11.h"
#include "phy_chanswitch.h"
#include "phy_abstract.h"
#include "antenna_global.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#endif // WIRELESS_LIB

#ifdef ADVANCED_WIRELESS_LIB
#include "phy_dot16.h"
#endif

#ifdef SENSOR_NETWORKS_LIB
#include "phy_802_15_4.h"
#endif

using namespace std;

#define HASH_SIZE           128

#define DEBUG             0

//--------------------------------------------------------
// data structures for use in managing node data.
//--------------------------------------------------------
typedef struct nodeIdToNodeIndex * IdToNodeIndexMap;

struct nodeIdToNodeIndex
{
    NodeId           nodeID;
    int              index;
    IdToNodeIndexMap next;
};

//--------------------------------------------------------
// data structures for use in managing dynamic statistics
//--------------------------------------------------------
typedef struct {
    MetricData* metricDataPtr;
    BOOL* nodeEnabled;
} MetricFilter;

//--------------------------------------------------------
// Variables used by partition_private.c for controlling dynamic statistics.
//--------------------------------------------------------
clocktype GUI_statReportTime;
clocktype GUI_statInterval;

//--------------------------------------------------------
// These globals are safe-enough. If shared, only p0 will write to, in
// mpi these globals are in sep processes.
// Global variable set exclusively by command line arguments
unsigned int GUI_guiSocket       = 0;
BOOL         GUI_guiSocketOpened = FALSE;
BOOL         GUI_animationTrace = FALSE;
static int   g_statLayerEnabled[MAX_LAYERS];
//static int   metricsEnabledByLayer[MAX_LAYERS];




typedef struct GUI_InterfaceData {
    // How far the gui commands have stepped the gui simulation clock to
    clocktype               guiExternalTime;
    // How far out the gui will allow the simulation clock to advance to
    // until it must stop and wait for the next step command from the gui.
    clocktype               guiExternalTimeHorizon;
    // Increment of simulation time that the horizon advances for each step.
    clocktype               guiStepSize;


    int                     maxNodeID;
    bool                    firstIteration;
} GUI_InterfaceData;

//--------------------------------------------------------
// GUI uses several global variables.
// When running MPI simulations, these globals are fine.
// In shared-memory parallel, all of these variables need
// to be accessed synchronously with other partitions.
//--------------------------------------------------------
QNPartitionMutex       GUI_globalsMutex;
//--------------------------------------------------------
// These variables are used for animation and stats filtering
//--------------------------------------------------------
BOOL                    g_layerEnabled[MAX_LAYERS] = {TRUE, TRUE, TRUE, TRUE,
                                        TRUE, TRUE, TRUE, TRUE,
                                        TRUE, TRUE, TRUE, TRUE};
int                     g_numberOfNodes;
IdToNodeIndexMap        g_idToIndexMap[HASH_SIZE];
int                     g_nodesInitialized = 0;
// These arrays have as many slots as nodes in the entire simulation
// As a result, commands to change the value in a slot must
// be sent to all partitions (in mpi)
BOOL*                   g_nodeEnabledForAnimation;
int*                    g_nodeEnabledForStatistics;

//--------------------------------------------------------
// These are used internally to manage dynamic statistics.
//--------------------------------------------------------

#define METRIC_NOT_FOUND       -1
#define METRIC_ALLOCATION_UNIT 100

// These globals are used for dynamic states, and required synchronized
// access for shared-parallel simulations.
MetricLayerData        g_metricData[MAX_LAYERS];
static MetricFilter*   g_metricFilters;
static int             g_metricsAllocated = 0;
static int             g_numberOfMetrics  = 0;




static BOOL GUI_isConnected ();

/*------------------------------------------------------
 * We want to store node information in a simple array, but
 * node IDs are not necessarily contiguous, so we place the
 * node IDs into a hash along with the node's index into
 * the simple array.
 *------------------------------------------------------*/

/*------------------------------------------------------
 * HashNodeIndex takes a node ID and index and adds them to
 * the hash.
 *------------------------------------------------------*/
static void HashNodeIndex(NodeId nodeID,
                          int    index) {

    // We are called from GUI_InitNode (), which should only
    // be called during partition init, and after that no more.
    // So, other functions that access the g_idToIndexMap don't
    // have to lock.

    int              hash     = nodeID % HASH_SIZE;
    IdToNodeIndexMap mapEntry = g_idToIndexMap[hash];
    IdToNodeIndexMap newMapEntry;

    newMapEntry = (IdToNodeIndexMap)
        MEM_malloc(sizeof(struct nodeIdToNodeIndex));
    newMapEntry->nodeID = nodeID;
    newMapEntry->index  = index;
    newMapEntry->next   = mapEntry;
    g_idToIndexMap[hash]  = newMapEntry;
}

/*------------------------------------------------------
 * GetNodeIndexFromHash returns the array index for the given
 * node ID.
 *------------------------------------------------------*/
static int GetNodeIndexFromHash(NodeId nodeID) {

    int              hash     = nodeID % HASH_SIZE;
    IdToNodeIndexMap mapEntry = g_idToIndexMap[hash];

    while (mapEntry != NULL) {
        if (mapEntry->nodeID == nodeID) {
            return mapEntry->index;
        }
        mapEntry = mapEntry->next;
    }
    //printf("node ID is %d\n", nodeID); fflush(stdout);
    //assert(FALSE);
    return -1;
}

/*------------------------------------------------------------------------
 * We create two structures for keeping track of dynamic statistics.
 * First, there's an array, indexed by metric ID, of node filters.
 * Second, there's an array of metric names, organized by layer.
 * The first structure is used for filtering metrics, the latter for
 * assigning them IDs.  The following two functions allocate and
 * initialize these two data structures.
 *------------------------------------------------------------------------*/

/*------------------------------------------------------
 * This function allocates new entries in the filter
 * array.  It increases the array size by 100 at a time.
 * We should probably do something to avoid all the mallocs.
 *------------------------------------------------------*/
static void AllocateAMetricFilter(int metricID) {

    if (g_numberOfMetrics > g_metricsAllocated) {
        // reallocate metrics table
        int numberToAllocate = g_metricsAllocated + METRIC_ALLOCATION_UNIT;
        MetricFilter* oldFilters = g_metricFilters;
        g_metricFilters = (MetricFilter*) MEM_malloc(sizeof(MetricFilter) *
                                               numberToAllocate);
        assert(g_metricFilters != NULL);
        if (g_metricsAllocated > 0) {
            memcpy(g_metricFilters, oldFilters,
                   sizeof(MetricFilter) * g_metricsAllocated);
            MEM_free(oldFilters);
        }
        g_metricsAllocated = numberToAllocate;
    }

    // Create a filter for the metric and initialize it to false;
    g_metricFilters[metricID].nodeEnabled = (BOOL*) MEM_malloc(sizeof(BOOL) *
                                                         g_numberOfNodes);
    assert(g_metricFilters[metricID].nodeEnabled != NULL);
    memset(g_metricFilters[metricID].nodeEnabled, 0,
           sizeof(BOOL) * g_numberOfNodes);
}

/*------------------------------------------------------
 * This function takes the metric name and layer and returns
 * a unique identifier for the metric, allocating space for
 * it if necessary.
 *------------------------------------------------------*/
static int GetMetricID(const char*  metricName,
                       GuiLayers    layer,
                       GuiDataTypes metricDataType) {
    QNPartitionLock globalsLock(&GUI_globalsMutex);
    int metricID;

    if (g_metricData[layer].numberAllocated == 0) {
        // obviously a new metric
        metricID = g_numberOfMetrics;
        g_numberOfMetrics++;
        AllocateAMetricFilter(metricID);

        // allocate space for this
        g_metricData[layer].numberAllocated = METRIC_ALLOCATION_UNIT;
        g_metricData[layer].metricList =
            (MetricData*) MEM_malloc(sizeof(MetricData) * METRIC_ALLOCATION_UNIT);
        assert(g_metricData[layer].metricList != NULL);

        g_metricFilters[metricID].metricDataPtr
            = &g_metricData[layer].metricList[0];

        // assign the values
        g_metricData[layer].numberUsed = 1;
        g_metricData[layer].metricList[0].metricID = metricID;
        g_metricData[layer].metricList[0].metricLayerID = layer;
        g_metricData[layer].metricList[0].metricDataType = metricDataType;
        strcpy(g_metricData[layer].metricList[0].metricName, metricName);
    }
    else { // search for the metric
        int i;
        for (i = 0; i < g_metricData[layer].numberUsed; i++) {
            MetricData md = g_metricData[layer].metricList[i];
            if (!strcmp(metricName, md.metricName))
            {
                return md.metricID;
            }
        }
        // if we got here, it wasn't found, so we have to add it.
        metricID = g_numberOfMetrics;
        g_numberOfMetrics++;
        AllocateAMetricFilter(metricID);

        assert(i <= g_metricData[layer].numberAllocated);
        if (i == g_metricData[layer].numberAllocated) {
            int numberAllocated = g_metricData[layer].numberAllocated;
            int numberToAllocate = numberAllocated + METRIC_ALLOCATION_UNIT;
            MetricData* oldList  = g_metricData[layer].metricList;

            g_metricData[layer].numberAllocated += METRIC_ALLOCATION_UNIT;
            g_metricData[layer].metricList =
                (MetricData*) MEM_malloc(sizeof(MetricData) * numberToAllocate);
            assert(g_metricData[layer].metricList != NULL);
            memcpy(g_metricData[layer].metricList, oldList,
                   sizeof(MetricData) * numberAllocated);
        }
        // assign the values
        g_metricData[layer].numberUsed++;
        g_metricData[layer].metricList[i].metricID = metricID;
        g_metricData[layer].metricList[i].metricLayerID = layer;
        g_metricData[layer].metricList[i].metricDataType = metricDataType;
        strcpy(g_metricData[layer].metricList[i].metricName, metricName);

        g_metricFilters[metricID].metricDataPtr
            = &g_metricData[layer].metricList[i];
    }

    return metricID;
}

/*------------------------------------------------------
 * This function adds double-quotes at the beginning
 * and end of a string which is passed as an argument
 *------------------------------------------------------*/
static void EncloseStringWithinQuotes(char* str) {

    int i;
    int len = (int)strlen(str);

    for(i = len; i >= 0; i--)
    {
        str[i + 1] = str[i];
    }
    str[0] = '"';
    str[len + 1] = '"';
    str[len + 2] ='\0';
}


/*------------------------------------------------------
 * GUI_Initialize
 *
 * Initializes the GUI in order to start the animation.
 * This function is not really necessary during a live run, but
 * will be needed in a trace file.
 * Called for all partitions, and always called to setup global
 * variables for printing stats.
 *------------------------------------------------------*/
static void GUI_Initialize(PartitionData * partitionData,
                    GUI_InterfaceData * data,
                    NodeInput*  nodeInput,
                    int         coordinateSystem,
                    Coordinates origin,
                    Coordinates dimensions,
                    clocktype   maxClock) {
    GuiReply reply;
    char     replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    int      layer;
    int      node;
    char     experimentName[MAX_STRING_LENGTH];
    char     backgroundImage[MAX_STRING_LENGTH];
    BOOL     wasFound;
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!GUI_isAnimateOrInteractive ())
    {
        return;
    }

    ctoa(maxClock, timeString);

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "EXPERIMENT-NAME",
                  &wasFound,
                  experimentName);

    if (!wasFound) {
        strcpy(experimentName, "\"qualnet\"");
    }
    else {
        EncloseStringWithinQuotes(experimentName);
    }

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "GUI-BACKGROUND-IMAGE-FILENAME",
                  &wasFound,
                  backgroundImage);

    if (!wasFound) {
        strcpy(backgroundImage, "\"\"");
    }
    else {
        EncloseStringWithinQuotes(backgroundImage);
    }

    reply.type = GUI_ANIMATION_COMMAND;
    /*
    // MS VS Express edition has a compilation bug.
    // This call to sprintf results in incorrect output -
    // should be:
    // 0 30 0 0.000000 0.000000 0.000000 1500.000000 1500.000000 0.000000 900000000000 default ""
    // but bad code produced by compiler results in
    // 0 0 30 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 (null) 900000030720 "default"
    // Arguments _are_ correct. Possibly a bug in microsoft's optimizer.
    // It appears that adding explicit casts solves the problem.
    sprintf(reply.args, "%d %d %d %f %f %f %f %f %f %s %s %s",
            GUI_INITIALIZE, partitionData->numNodes, coordinateSystem,
            origin.common.c1, origin.common.c2, origin.common.c3,
            dimensions.common.c1, dimensions.common.c2, dimensions.common.c3,
            timeString, experimentName, backgroundImage);
            */
    sprintf(replyBuff, "%d %d %d %f %f %f %f %f %f",
            (int) GUI_INITIALIZE, (int) partitionData->numNodes, (int) coordinateSystem,
            (double) origin.common.c1, (double) origin.common.c2, (double) origin.common.c3,
            (double) dimensions.common.c1, (double) dimensions.common.c2, (double) dimensions.common.c3);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(timeString);
    reply.args.append(" ");
    reply.args.append(experimentName);
    reply.args.append(" ");
    reply.args.append(backgroundImage);

    // Add list of terrain files
    char* fileList = TERRAIN_TerrainFileList(nodeInput);
    if (fileList != NULL) {
        reply.args.append(" ");
        reply.args.append(fileList);
        MEM_free(fileList);
    }

    // For mpi, all partitions will send GUI_INTIALIZE command.
    // For shared or 1 cpu, only one GUI_INITIALIZE
    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }

    // Initialize the GUI filtering and control variables.
    int numNodes = partitionData->numNodes;
    g_numberOfNodes = numNodes;
    data->maxNodeID     = numNodes - 1;
                                // fine if ordered sequentially, but
                                // will have to change if not.

    g_nodeEnabledForAnimation  = (BOOL*) MEM_malloc(numNodes * sizeof(BOOL));
    g_nodeEnabledForStatistics = (int*) MEM_malloc(numNodes * sizeof(BOOL));
    for (node = 0; node < g_numberOfNodes; node++) {
        g_nodeEnabledForAnimation[node] = TRUE;
        g_nodeEnabledForStatistics[node] = 0;
    }

    for (layer = 0; layer < MAX_LAYERS; layer++) {
        g_statLayerEnabled[layer]           = 0;
        g_metricData[layer].metricList      = NULL;
        g_metricData[layer].numberAllocated = 0;
        g_metricData[layer].numberUsed      = 0;
    }

    for (node = 0; node < HASH_SIZE; node++) {
        g_idToIndexMap[node] = NULL;
    }
}

/*------------------------------------------------------
 * GUI_SetEffect
 *
 * This function will allow the protocol designer to specify the
 * effect to use to display certain events.
 *------------------------------------------------------*/
void GUI_SetEffect(GuiEvents  event,
                   GuiLayers  layer,
                   int        type,
                   GuiEffects effect,
                   GuiColors  color) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];

    reply.type = GUI_SET_EFFECT;
    sprintf(replyBuff, "%d %d %d %d %d", event, layer, type, effect, color);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * InitNode
 *
 * Provides the initial location of the node, and also contains other
 * attributes such as transmission range, protocols, etc.
 *------------------------------------------------------*/
void GUI_InitNode(Node*      node,
                  NodeInput* nodeInput,
                  clocktype  time) {

    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    // This code calculates a reasonable default value for the radio
    // range, used by the GUI for displaying radio transmissions,
    // and otherwise initializes the GUI for this node.

    char name[MAX_STRING_LENGTH];
    char icon[MAX_STRING_LENGTH];
    BOOL wasFound;

   // Commented out Y Zhou 6-3-05
   /*
    IO_ReadString(node->nodeId, ANY_ADDRESS,
                  nodeInput, "USE-NODE-ICON", &wasFound, useIcon);
    if (wasFound && !strcmp(useIcon, "YES"))
    {
        IO_ReadString(node->nodeId, ANY_ADDRESS,
                      nodeInput, "NODE-ICON", &wasFound, icon);
        if (!wasFound) {
            strcpy(icon, GUI_DEFAULT_ICON);
        }
        else {
            EncloseStringWithinQuotes(icon);
        }
    }
    else
    {
        strcpy(icon, GUI_DEFAULT_ICON);
    }
   */

   /* Added Y Zhou 6-3-05 for USE-NODE-ICON removal */
   IO_ReadString(node->nodeId, ANY_ADDRESS,
                      nodeInput, "NODE-ICON", &wasFound, icon);
    if (!wasFound) {
            strcpy(icon, GUI_DEFAULT_ICON);
    }
    else {
            EncloseStringWithinQuotes(icon);
    }

    IO_ReadString(node->nodeId, ANY_ADDRESS,
                  nodeInput, "HOSTNAME", &wasFound, name);
    if (!wasFound) {
        sprintf(name, "\"host%u\"", node->nodeId);
    }
    else {
        EncloseStringWithinQuotes(name);
    }

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %.10f %.10f %.10f %d %d %d",
            GUI_INIT_NODE, node->nodeId,
            node->mobilityData->current->position.common.c1,
            node->mobilityData->current->position.common.c2,
            node->mobilityData->current->position.common.c3,
            0, // type
            node->mobilityData->current->orientation.azimuth,
            node->mobilityData->current->orientation.elevation);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(timeString);
    reply.args.append(" ");
    reply.args.append(icon); // icon file
    reply.args.append(" ");
    reply.args.append(name); // label

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }

    // enter the node into the hash
    HashNodeIndex(node->nodeId, g_nodesInitialized);
    g_nodesInitialized++;
}

/*------------------------------------------------------
 * InitWirelessInterface
 *
 * Sets up the wireless properties (power, sensitivity, range)
 * for a node's wireless interface.
 *------------------------------------------------------*/
void GUI_InitWirelessInterface(Node* node,
                               int   interfaceIndex) {
#ifdef WIRELESS_LIB

    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    NodeInput*  nodeInput = node->partitionData->nodeInput;

    float conversionParameter = 0.0f;

    char antennaFile[MAX_STRING_LENGTH];
    char antennaAziFile[MAX_STRING_LENGTH];
    char antennaElvFile[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    BOOL wasFound = FALSE;
    BOOL wasAziFound = FALSE;
    BOOL wasElvFound = FALSE;
    BOOL wasParamFound = FALSE;
    BOOL wasModelFound = FALSE;

    // Input variables
    PathlossModel pathlossModel = TWO_RAY;

    // assign reasonable defaults, in case variables are undefined.
    double channel0Frequency  = 2.4e9;
    double directionalTxPower = 0.0;
    double txPower            = 15.0;
    double rxThreshold        = -81.0;
    double txAntennaGain      = 0.0;
    double rxAntennaGain      = 0.0;
    double antennaHeight      = 0.0;
    double distance           = 0.0;
    double systemLoss         = 0.0;
    double shadowingMean      = 4.0;

    PropProfile*     propProfile = node->partitionData->propChannel[0].profile;

    PhyData*         thisRadio;
    PhyModel         phyModel = PHY_NONE;
    PhyData802_11*   phy802_11 = NULL;
		PhyDataChanSwitch * phychanswitch = NULL;
    PhyDataAbstract* phyAbstract;
    AntennaOmnidirectional* omniDirectional;
    AntennaSwitchedBeam* switchedBeam;
    AntennaSteerable* steerable;
    AntennaPatterned* patterned;
#ifdef CELLULAR_LIB
    PhyDataGsm*      phyGsm;
#endif // CELLULAR_LIB
#ifdef ADVANCED_WIRELESS_LIB
    PhyDataDot16*    phyDot16 = NULL;
#endif
#ifdef SENSOR_NETWORKS_LIB
    PhyData802_15_4* phy802_15_4 = NULL;
#endif


    int phyIndex = node->macData[interfaceIndex]->phyNumber;

    thisRadio = node->phyData[phyIndex];
    phyModel  = node->phyData[phyIndex]->phyModel;

    switch (phyModel) {
        case PHY802_11a:
        case PHY802_11b:
            phy802_11          = (PhyData802_11*)thisRadio->phyVar;
            txPower            = (float)phy802_11->txDefaultPower_dBm[0];
            rxThreshold        = phy802_11->rxSensitivity_mW[0];
            directionalTxPower =
                txPower - phy802_11->directionalAntennaGain_dB;
            break;
        case PHY_CHANSWITCH:
            phychanswitch          = (PhyDataChanSwitch*)thisRadio->phyVar;
            txPower            = (float)phychanswitch->txDefaultPower_dBm[0];
            rxThreshold        = phychanswitch->rxSensitivity_mW[0];
            directionalTxPower =
                txPower - phychanswitch->directionalAntennaGain_dB;
            break;
        case PHY_ABSTRACT:
            phyAbstract = (PhyDataAbstract*)thisRadio->phyVar;
            txPower     = (float)phyAbstract->txPower_dBm;
            rxThreshold = phyAbstract->rxThreshold_mW;
            break;

#ifdef CELLULAR_LIB
        case PHY_GSM:
            phyGsm      = (PhyDataGsm*) thisRadio->phyVar;
            txPower     = (float)phyGsm->txPower_dBm;
            rxThreshold = phyGsm->rxThreshold_mW;
            break;
#ifdef UMTS_LIB
        case PHY_CELLULAR:
        {
            PhyUmtsData* phyUmts;
            PhyCellularData *phyCellular =
                (PhyCellularData*)thisRadio->phyVar;
            phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
            txPower     = (float)phyUmts->txPower_dBm;
            rxThreshold = phyUmts->rxSensitivity_mW[0];
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
            txPower     = (float)phyUmts->txPower_dBm;
            rxThreshold = phyUmts->rxSensitivity_mW[0];
            break;
        }
#endif // CELLULAR_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16:
            phyDot16 = (PhyDataDot16*) thisRadio->phyVar;
            txPower            = (float)phyDot16->txPower_dBm;
            rxThreshold        = phyDot16->rxSensitivity_mW[0];

            break;
#endif
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4:
            phy802_15_4 = (PhyData802_15_4*) thisRadio->phyVar;
            txPower            = (float)phy802_15_4->txPower_dBm;
            rxThreshold        = phy802_15_4->rxSensitivity_mW;
            break;
#endif

        // need case for FCSC
        // need case for Link-16
        default:
            break; // use defaults
    }

    switch (thisRadio->antennaData->antennaModelType) {
        case ANTENNA_OMNIDIRECTIONAL:
            omniDirectional = (AntennaOmnidirectional*)
                                thisRadio->antennaData->antennaVar;
            rxAntennaGain = omniDirectional->antennaGain_dB;
            antennaHeight = omniDirectional->antennaHeight;
            break;
        case ANTENNA_SWITCHED_BEAM:
            switchedBeam = (AntennaSwitchedBeam*)
                            thisRadio->antennaData->antennaVar;
            rxAntennaGain = switchedBeam->antennaGain_dB;
            antennaHeight = switchedBeam->antennaHeight;
            break;
        case ANTENNA_STEERABLE:
            steerable = (AntennaSteerable*)
                thisRadio->antennaData->antennaVar;
            rxAntennaGain = steerable->antennaGain_dB;
            antennaHeight = steerable->antennaHeight;
            break;
        case ANTENNA_PATTERNED:
            patterned = (AntennaPatterned*)
                thisRadio->antennaData->antennaVar;
            rxAntennaGain = patterned->antennaGain_dB;
            antennaHeight = patterned->antennaHeight;
            break;
    }

    rxThreshold       = IN_DB(rxThreshold);

    txAntennaGain     = rxAntennaGain;

    pathlossModel     = propProfile->pathlossModel;
    channel0Frequency = propProfile->frequency;
    shadowingMean     = propProfile->shadowingMean_dB;
    systemLoss        = thisRadio->systemLoss_dB;
    distance          = PHY_PropagationRange(node, phyIndex, FALSE);

    IO_ReadString(
            node->nodeId,
            MAPPING_GetInterfaceAddressForInterface(node,
            node->nodeId, interfaceIndex),
            nodeInput,
            "ANTENNA-MODEL",
            &wasModelFound,
            buf);

    if (!wasModelFound || (strcmp(buf, "OMNIDIRECTIONAL") == 0) ||
        (strcmp(buf, "SWITCHED-BEAM") == 0) ||
        (strcmp(buf, "STEERABLE") == 0))

    {

        IO_ReadString(node->nodeId,
                      MAPPING_GetInterfaceAddressForInterface(node,
                      node->nodeId, interfaceIndex),
                      nodeInput,
                      "ANTENNA-AZIMUTH-PATTERN-FILE",
                      &wasAziFound,
                      antennaAziFile);

        if(wasAziFound) {
                EncloseStringWithinQuotes(antennaAziFile);
        }

        IO_ReadString(node->nodeId,
                      MAPPING_GetInterfaceAddressForInterface(node,
                      node->nodeId, interfaceIndex),
                      nodeInput,
                      "ANTENNA-ELEVATION-PATTERN-FILE",
                      &wasElvFound,
                      antennaElvFile);

        if(wasElvFound) {
                EncloseStringWithinQuotes(antennaElvFile);
        }

    }

    else {
        NodeInput* antennaModelInput
            = ANTENNA_MakeAntennaModelInput(nodeInput,
                                            buf);

        switch (thisRadio->antennaData->antennaPatternType) {

            case ANTENNA_PATTERN_TRADITIONAL:
            case ANTENNA_PATTERN_ASCII2D:
                 IO_ReadString(node->nodeId,
                              MAPPING_GetInterfaceAddressForInterface(node,
                              node->nodeId, interfaceIndex),
                              antennaModelInput,
                              "ANTENNA-AZIMUTH-PATTERN-FILE",
                              &wasAziFound,
                              antennaAziFile);

                 if(wasAziFound) {
                    EncloseStringWithinQuotes(antennaAziFile);
                 }

                 IO_ReadString(node->nodeId,
                              MAPPING_GetInterfaceAddressForInterface(node,
                              node->nodeId, interfaceIndex),
                              antennaModelInput,
                              "ANTENNA-ELEVATION-PATTERN-FILE",
                              &wasElvFound,
                              antennaElvFile);

                 if(wasElvFound) {
                    EncloseStringWithinQuotes(antennaElvFile);
                 }

                 break;

            case ANTENNA_PATTERN_ASCII3D:
            case ANTENNA_PATTERN_NSMA:
                 IO_ReadString(node->nodeId,
                              MAPPING_GetInterfaceAddressForInterface(node,
                              node->nodeId, interfaceIndex),
                              antennaModelInput,
                              "ANTENNA-PATTERN-PATTERN-FILE",
                              &wasFound,
                              antennaFile);

                 if(wasFound) {
                    EncloseStringWithinQuotes(antennaFile);
                 }

                 break;

            default:
            {
                break;
            }
        }

        IO_ReadFloat(node->nodeId,
                     MAPPING_GetInterfaceAddressForInterface(node,
                     node->nodeId, interfaceIndex),
                     antennaModelInput,
                     "ANTENNA-PATTERN-CONVERSION-PARAMETER",
                     &wasParamFound,
                     &conversionParameter);

    }

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff,"%d %u %d %d %d %d %.5lf %.5lf %.5lf %.5lf %.5lf "
            "%.5lf %.5lf %d %.5lf %d",
            GUI_INIT_WIRELESS, node->nodeId, interfaceIndex,
            (int) distance,
            -1,   /* ANTENNA_OMNIDIRECTIONAL_PATTERN */
            thisRadio->antennaData->antennaModelType,
            txPower,
            rxThreshold,
            antennaHeight,
            channel0Frequency,
            directionalTxPower,
            systemLoss,
            shadowingMean,
            (int) pathlossModel,
            (wasParamFound) ? conversionParameter : 0.0,
            thisRadio->antennaData->antennaPatternType);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append((wasAziFound) ? antennaAziFile : "");
    reply.args.append(" ");
    reply.args.append((wasElvFound) ? antennaElvFile : "");
    reply.args.append(" ");
    reply.args.append((wasFound) ? antennaFile : "");

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
#else // WIRELESS_LIB
    ERROR_ReportWarning("Wireless library is required to initialize wireless interfaces");
#endif // WIRELESS_LIB
}

static
void GUI_ValidateNodeIdAndInterfaceIndex(
    char* variableNames,
    char* qualifier,
    NodeId* nodeId,
    int interfaceIndex,
    BOOL* isNodeId)
{
    if (!qualifier)
    {
        char err[MAX_STRING_LENGTH];

        sprintf(err, "%s must have a qualifier node id\n",
            variableNames);

        ERROR_ReportError(err);
    }

    IO_ParseNodeIdOrHostAddress(
        qualifier,
        nodeId,
        isNodeId);

    if (!(*isNodeId))
    {
        char err[MAX_STRING_LENGTH];

        sprintf(err, "Qualifier of %s must be a node id\n",
            variableNames);

        ERROR_ReportError(err);
    }

    if (interfaceIndex < 0)
    {
        char err[MAX_STRING_LENGTH];

        sprintf(err, "Variable %s requires a non-negative index, for example\n"
                "[%d] %s[1] value\n",
                variableNames, *nodeId, variableNames);

        ERROR_ReportError(err);
    }
}

/*------------------------------------------------------
 * GUI_InitializeInterfaces
 *
 * Sets the IP address for a node's interface.
 *------------------------------------------------------*/
void GUI_InitializeInterfaces(const NodeInput* nodeInput) {

    int i;

    // send over all the user-defined IP-ADDRESS,
    // IP-SUBNET-MASK and INTERFACE-NAME entries
    // presumably first two were validated in mapping.c
    for (i = 0; i < nodeInput->numLines; i++)
    {
        char* qualifier      = nodeInput->qualifiers[i];
        int   interfaceIndex = nodeInput->instanceIds[i];
        char* value          = nodeInput->values[i];
        BOOL  isNodeId       = FALSE;

        NodeId nodeId;

        // Change the IP address of a node's interface.
        if (!strcmp(nodeInput->variableNames[i], "IP-ADDRESS"))
        {
#ifndef ADDON_NGCNMS
            NodeAddress newInterfaceAddress;

            // Make sure that the qualifier is a node ID first.
            GUI_ValidateNodeIdAndInterfaceIndex(
                nodeInput->variableNames[i],
                qualifier,
                &nodeId,
                interfaceIndex,
                &isNodeId);

            // Get new interface address.
            IO_ParseNodeIdOrHostAddress(
                value,
                &newInterfaceAddress,
                &isNodeId);

            GUI_SetInterfaceAddress(
                nodeId,
                newInterfaceAddress,
                interfaceIndex,
                0);
#endif
        }
        // Change the Subnet mask of a node's interface.
        else if(!strcmp(nodeInput->variableNames[i], "IP-SUBNET-MASK"))
        {
            NodeAddress newSubnetMask;

            // Make sure that the qualifier is a node ID first.
            GUI_ValidateNodeIdAndInterfaceIndex(
                nodeInput->variableNames[i],
                qualifier,
                &nodeId,
                interfaceIndex,
                &isNodeId);

            // Get new subnet mask.
            IO_ParseNodeIdOrHostAddress(
                value,
                &newSubnetMask,
                &isNodeId);

            GUI_SetSubnetMask(
                nodeId,
                newSubnetMask,
                interfaceIndex,
                0);
        }
        // Change the Interface name of a node's interface.
        else if(!strcmp(nodeInput->variableNames[i], "INTERFACE-NAME"))
        {
            // Make sure that the qualifier is a node ID first.
            GUI_ValidateNodeIdAndInterfaceIndex(
                nodeInput->variableNames[i],
                qualifier,
                &nodeId,
                interfaceIndex,
                &isNodeId);

            GUI_SetInterfaceName(
                nodeId,
                value,
                interfaceIndex,
                0);
        }
    }
}


/*------------------------------------------------------
 * GUI_SetInterfaceAddress
 *
 * Sets the IP address for a node's interface.
 *------------------------------------------------------*/
void GUI_SetInterfaceAddress(NodeId      nodeID,
                             NodeAddress interfaceAddress,
                             int         interfaceIndex,
                             clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d %u %s",
            GUI_SET_INTERFACE_ADDRESS, nodeID, interfaceIndex,
            interfaceAddress, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}


/*------------------------------------------------------
 * GUI_SetSubnetMask
 *
 * Sets the Subnet mask for a node's interface.
 *------------------------------------------------------*/
void GUI_SetSubnetMask(NodeId      nodeID,
                       NodeAddress subnetMask,
                       int         interfaceIndex,
                       clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d %u %s",
            GUI_SET_SUBNET_MASK, nodeID, interfaceIndex,
            subnetMask, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}


/*------------------------------------------------------
 * GUI_SetInterfaceName
 *
 * Sets the Interface name for a node's interface.
 *------------------------------------------------------*/
void GUI_SetInterfaceName(NodeId      nodeID,
                          char*       interfaceName,
                          int         interfaceIndex,
                          clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d",
            GUI_SET_INTERFACE_NAME, nodeID, interfaceIndex);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(interfaceName);
    reply.args.append(" ");
    reply.args.append(timeString);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * MoveNode
 *
 * Moves the node to a new position.
 *------------------------------------------------------*/
void GUI_MoveNode(NodeId      nodeID,
                  Coordinates position,
                  clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %.15f %.15f %.15f %s",
            GUI_MOVE_NODE, nodeID,
            position.common.c1, position.common.c2,
            position.common.c3, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * SetNodeOrientation
 *
 * Changes the orientation of a node.
 *------------------------------------------------------*/
void GUI_SetNodeOrientation(NodeId      nodeID,
                            Orientation orientation,
                            clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d %d %s",
            GUI_SET_ORIENTATION,
            nodeID,
            orientation.azimuth,
            orientation.elevation,
            timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * SetNodeIcon
 *
 * Changes the icon associated with a node.
 *------------------------------------------------------*/
void GUI_SetNodeIcon(NodeId      nodeID,
                     const char* iconFile,
                     clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    char iconFileName[MAX_STRING_LENGTH];
    memset(iconFileName, 0, MAX_STRING_LENGTH);
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;


    if (!strstr(iconFile, GUI_DEFAULT_ICON))
    {
        memcpy(iconFileName, iconFile, strlen(iconFile));
        EncloseStringWithinQuotes(iconFileName);
    }
    else
    {
        memcpy(iconFileName, GUI_DEFAULT_ICON, strlen(iconFile));
    }

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u",
            GUI_SET_NODE_ICON,
            nodeID);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(iconFileName); //iconFile
    reply.args.append(" ");
    reply.args.append(timeString);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * SetNodeLabel
 *
 * Changes the label (the node name) of a node.
 *------------------------------------------------------*/
void GUI_SetNodeLabel(NodeId      nodeID,
                      char*       label,
                      clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u",
            GUI_SET_NODE_LABEL,
            nodeID);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(label);
    reply.args.append(" ");
    reply.args.append(timeString);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * SetNodeRange
 *
 * Changes the range of a node
 *------------------------------------------------------*/
void GUI_SetNodeRange(NodeId      nodeID,
                      int         interfaceIndex,
                      double      range,
                      clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d %d %s",
            GUI_SET_NODE_RANGE,
            nodeID,
            interfaceIndex,
            (int) range,
            timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}


/*------------------------------------------------------
 * SetNodeType
 *
 * Changes the type of a node
 *------------------------------------------------------*/
void GUI_SetNodeType(NodeId      nodeID,
                     int         type,
                     clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d %s",
            GUI_SET_NODE_TYPE,
            nodeID,
            type,
            timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * SetPatternIndex
 *
 * Changes the antenna pattern index.
 *------------------------------------------------------*/
void GUI_SetPatternIndex(Node*      node,
                         int         interfaceIndex,
                         int         index,
                         clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    int nodeAzimuthAngle = 0;
    int nodeElevationAngle = 0;
    int mobility = 0;
    ctoa(time, timeString);

    if (node->mobilityData->mobilityType != NO_MOBILITY) {
        nodeAzimuthAngle = node->mobilityData->current->orientation.azimuth;
        nodeElevationAngle = node->mobilityData->current->orientation.elevation;
        mobility = 1;
    }

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d %d %s %d %d %d",
            GUI_SET_PATTERN,
            node->nodeId,
            interfaceIndex,
            index,
            timeString,
            nodeAzimuthAngle,
            nodeElevationAngle,
            mobility);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * SetPatternAndAngle
 *
 * Changes the antenna pattern index and the angle of
 * orientation at which to display the pattern.
 *------------------------------------------------------*/
void GUI_SetPatternAndAngle(Node*      node,
                            int         interfaceIndex,
                            int         index,
                            int         angleInDegrees,
                            int         elevationAngle,
                            clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    int nodeAzimuthAngle = 0;
    int nodeElevationAngle = 0;
    int mobility = 0;
    ctoa(time, timeString);

    if (node->mobilityData->mobilityType != NO_MOBILITY) {
        nodeAzimuthAngle = node->mobilityData->current->orientation.azimuth;
        nodeElevationAngle = node->mobilityData->current->orientation.elevation;
        mobility = 1;
    }

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %d %d %d %d %s %d %d %d",
            GUI_SET_PATTERNANGLE,
            node->nodeId,
            interfaceIndex,
            index,
            angleInDegrees,
            elevationAngle,
            timeString,
            nodeAzimuthAngle,
            nodeElevationAngle,
            mobility);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * AddLink
 *
 * Adds a link between two nodes.  In a wired topology, this could be
 * a static link; in wireless, a dynamic one.
 *
 * For simplicity, only one "link" between two nodes will be shown (per
 * layer).  If addLink is called when a link already exists, the "type"
 * of the link will be updated.
 *------------------------------------------------------*/
void GUI_AddLink(NodeId      sourceID,
                 NodeId      destID,
                 GuiLayers   layer,
                 int         type,
                 NodeAddress subnetAddress,
                 int         numHostBits,
                 clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)] &&
         !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)]))
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %u %d %u %d %s",
            GUI_ADD_LINK, layer, sourceID, destID, type, subnetAddress,
            numHostBits, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * AddLink
 *
 * Adds a link between two nodes.  In a wired topology, this could be
 * a static link; in wireless, a dynamic one.
 *
 * For simplicity, only one "link" between two nodes will be shown (per
 * layer).  If addLink is called when a link already exists, the "type"
 * of the link will be updated.
 *------------------------------------------------------*/
void GUI_AddLink(NodeId      sourceID,
                 NodeId      destID,
                 GuiLayers   layer,
                 int         type,
                 int         tla,
                 int         nla,
                 int         sla,
                 clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)] &&
         !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)]))
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %u %d %d %d %d %s",
            GUI_ADD_LINK, layer, sourceID, destID, type, tla, nla,
            sla, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
     }
}

/*------------------------------------------------------
 * AddLink
 *
 * Adds a link between two nodes.  In a wired topology, this could be
 * a static link; in wireless, a dynamic one.
 *
 * For simplicity, only one "link" between two nodes will be shown (per
 * layer).  If addLink is called when a link already exists, the "type"
 * of the link will be updated.
 *------------------------------------------------------*/
void GUI_AddLink(NodeId       sourceID,
                 NodeId       destID,
                 GuiLayers    layer,
                 int          type,
                 char*        ip6Addr,
                 unsigned int IPv6subnetPrefixLen,
                 clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    char     addrString[MAX_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)] &&
         !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)]))
        return;

    ctoa(time, timeString);
    IO_ConvertIpAddressToString(ip6Addr, addrString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %u %d %s %d %s",
            GUI_ADD_LINK, layer, sourceID, destID, type,
            addrString, IPv6subnetPrefixLen,
            timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
     }
}

/*------------------------------------------------------
 * DeleteLink
 *
 * Removes the aforementioned link.
 *------------------------------------------------------*/
void GUI_DeleteLink(NodeId      sourceID,
                    NodeId      destID,
                    GuiLayers   layer,
                    clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)] &&
         !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)]))
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;

    sprintf(replyBuff, "%d %d %u %u %s",
            GUI_DELETE_LINK, layer, sourceID, destID, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Multicast
 *------------------------------------------------------*/
void GUI_Multicast(NodeId      nodeID,
                   GuiLayers   layer,
                   int         type,
                   int         interfaceIndex,
                   clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d %d %s",
            GUI_MULTICAST, layer, nodeID, type, interfaceIndex, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Broadcast
 *
 * At lower layers, indicates a radio broadcast.  At network layer, can
 * be used to represent a multicast.
 *------------------------------------------------------*/
void GUI_Broadcast(NodeId      nodeID,
                   GuiLayers   layer,
                   int         type,
                   int         interfaceIndex,
                   clocktype   time) {

    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d %d %s",
            GUI_BROADCAST, layer, nodeID, type, interfaceIndex, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * EndBroadcast
 *
 * Signals the end of a wireless broadcast.
 *------------------------------------------------------*/
void GUI_EndBroadcast(NodeId      nodeID,
                      GuiLayers   layer,
                      int         type,
                      int         interfaceIndex,
                      clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d %d %s",
            GUI_ENDBROADCAST, layer, nodeID, type, interfaceIndex,
            timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Unicast
 *
 * Sends a packet/frame/signal to a destination.
 *------------------------------------------------------*/
void GUI_Unicast(NodeId      sourceID,
                 NodeId      destID,
                 GuiLayers   layer,
                 int         type,
                 int         sendingInterfaceIndex,
                 int         receivingInterfaceIndex,
                 clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)] &&
         !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)]))
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %u %d %d %d %s",
            GUI_UNICAST, layer, sourceID, destID, type, sendingInterfaceIndex,
            receivingInterfaceIndex, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Receive
 *
 * Shows a packet/frame/signal being successfully received at
 * a destination.
 *------------------------------------------------------*/
void GUI_Receive(NodeId      sourceID,
                 NodeId      destID,
                 GuiLayers   layer,
                 int         type,
                 int         sendingInterfaceIndex,
                 int         receivingInterfaceIndex,
                 clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)])
        return;

    // Possible for this call to be source less.
    if ((sourceID != 0) &&
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %u %d %d %d %s",
            GUI_RECEIVE, layer, sourceID, destID, type, sendingInterfaceIndex,
            receivingInterfaceIndex, timeString);
    reply.args.append(replyBuff);
    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Drop
 *
 * Shows a packet/frame/signal being dropped.
 *------------------------------------------------------*/
void GUI_Drop(NodeId      sourceID,
              NodeId      destID,
              GuiLayers   layer,
              int         type,
              int         sendingInterfaceIndex,
              int         receivingInterfaceIndex,
              clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)])
        return;

    // Possible for this call to be source less.
    if ((sourceID != 0) &&
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %u %d %d %d %s",
            GUI_DROP, layer, sourceID, destID, type, sendingInterfaceIndex,
            receivingInterfaceIndex, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Collision
 *
 * Shows a collision.
 *------------------------------------------------------*/
void GUI_Collision(NodeId      nodeID,
                   GuiLayers   layer,
                   clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %s",
            GUI_COLLISION, GUI_CHANNEL_LAYER, nodeID, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Create Subnet
 *
 * Creates a subnet.  Normally done at startup.
 *------------------------------------------------------*/
void GUI_CreateSubnet(GuiSubnetTypes type,
                      NodeAddress    subnetAddress,
                      int            numHostBits,
                      const char*    nodeList,
                      clocktype      time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d",
            GUI_CREATE_SUBNET, type, subnetAddress,
            numHostBits);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(nodeList);
    reply.args.append(" ");
    reply.args.append(timeString);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Create Subnet
 *
 * Creates a subnet.  Normally done at startup.
 *------------------------------------------------------*/
// This function is overloaded function for ipv6.

void GUI_CreateSubnet(GuiSubnetTypes type,
                      char*          ip6Addr,
                      unsigned int   IPv6subnetPrefixLen,
                      const char*    nodeList,
                      clocktype      time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    char addrString[MAX_STRING_LENGTH];

    ctoa(time, timeString);
    IO_ConvertIpAddressToString(ip6Addr, addrString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %s %d",
            GUI_CREATE_SUBNET, type,
            addrString, IPv6subnetPrefixLen);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(nodeList);
    reply.args.append(" ");
    reply.args.append(timeString);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}


/*------------------------------------------------------
 * Create Hierarchy
 *
 * Since the GUI supports hierarchical design, this function informs
 * the GUI of the contents of a hierarchical component.
 * This method is only called by partition 0.
 *------------------------------------------------------*/
void GUI_CreateHierarchy(int   componentID,
                         char* nodeList) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d",
            GUI_CREATE_HIERARCHY, componentID);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(nodeList);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * MoveHierarchy
 *
 * Moves the hierarchy to a new position.
 *------------------------------------------------------*/
void GUI_MoveHierarchy(int         hierarchyID,
                       Coordinates centerCoordinates,
                       Orientation orientation,
                       clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %.15f %.15f %.15f %d %d %s",
            GUI_MOVE_HIERARCHY, hierarchyID,
            centerCoordinates.common.c1, centerCoordinates.common.c2,
            centerCoordinates.common.c3, orientation.azimuth,
            orientation.elevation, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}


/*------------------------------------------------------
 * Create Weather
 *------------------------------------------------------*/
void GUI_CreateWeatherPattern(int   patternID,
                              char* inputLine) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d",
            GUI_CREATE_WEATHER_PATTERN, patternID);
    reply.args.append(replyBuff);
    reply.args.append(" ");
    reply.args.append(inputLine);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * MoveWeather
 *
 * Moves the hierarchy to a new position.
 *------------------------------------------------------*/
void GUI_MoveWeatherPattern(int         patternID,
                            Coordinates coordinates,
                            clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %s (%.15f,%.15f)",
            GUI_MOVE_WEATHER_PATTERN, patternID, timeString,
            coordinates.common.c1, coordinates.common.c2);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}


/*------------------------------------------------------
 * Add Application
 *
 * Shows label beside the client and the server as app link is setup.
 *------------------------------------------------------*/
void GUI_AddApplication(NodeId      sourceID,
                        NodeId      destID,
                        char*       appName,
                        int         uniqueId,
                        clocktype   time){
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[GUI_APP_LAYER] ||
        (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)] &&
         !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)]))
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %u %s %d %s",
            GUI_ADD_APP, sourceID, destID, appName, uniqueId, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * Delete Application
 *
 * deletes the labels shown by AddApplication.
 *------------------------------------------------------*/
void GUI_DeleteApplication(NodeId      sourceID,
                           NodeId      destID,
                           char*       appName,
                           int         uniqueId,
                           clocktype   time){
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[GUI_APP_LAYER] ||
        (!g_nodeEnabledForAnimation[GetNodeIndexFromHash(sourceID)] &&
         !g_nodeEnabledForAnimation[GetNodeIndexFromHash(destID)]))
        return;

    ctoa(time, timeString);

    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %u %u %s %d %s",
            GUI_DELETE_APP, sourceID, destID, appName,
            uniqueId, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * AddInterfaceQueue
 *
 * Creates a queue for a node, interface and priority
 *------------------------------------------------------*/
void GUI_AddInterfaceQueue(NodeId       nodeID,
                           GuiLayers    layer,
                           int          interfaceIndex,
                           unsigned int priority,
                           int          queueSize,
                           clocktype    currentTime)
{
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];

    // The queue creation cannot be filtered, or it can never
    // be enabled.

    ctoa(currentTime, timeString);
    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d %u %d %s",
            GUI_QUEUE_CREATE, layer, nodeID, interfaceIndex,
            priority, queueSize, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * QueueInsertPacket
 *
 * Inserting one packet to a queue for a node, interface and priority
 *------------------------------------------------------*/
void GUI_QueueInsertPacket(NodeId       nodeID,
                           GuiLayers    layer,
                           int          interfaceIndex,
                           unsigned int priority,
                           int          packetSize,
                           clocktype    currentTime)
{
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(currentTime, timeString);
    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d %u %d %s",
            GUI_QUEUE_ADD, layer, nodeID, interfaceIndex,
            priority, packetSize, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * QueueDropPacket
 *
 * Dropping one packet from a queue for a node, interface and priority
 *------------------------------------------------------*/
void GUI_QueueDropPacket(NodeId       nodeID,
                         GuiLayers    layer,
                         int          interfaceIndex,
                         unsigned int priority,
                         clocktype    currentTime)
{
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(currentTime, timeString);
    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d %u %s",
            GUI_QUEUE_DROP, layer, nodeID, interfaceIndex,
            priority, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * QueueDequeuePacket
 *
 * Dequeuing one packet from a queue for a node, interface and priority
 *------------------------------------------------------*/
void GUI_QueueDequeuePacket(NodeId       nodeID,
                            GuiLayers    layer,
                            int          interfaceIndex,
                            unsigned int priority,
                            int          packetSize,
                            clocktype    currentTime)
{
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (!g_layerEnabled[layer] ||
        !g_nodeEnabledForAnimation[GetNodeIndexFromHash(nodeID)])
        return;

    ctoa(currentTime, timeString);
    reply.type = GUI_ANIMATION_COMMAND;
    sprintf(replyBuff, "%d %d %u %d %u %d %s",
            GUI_QUEUE_REMOVE, layer, nodeID, interfaceIndex,
            priority, packetSize, timeString);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

/*------------------------------------------------------
 * DefineMetric
 *
 * This function defines a metric by giving it a name and a
 * description.  The system will assign a number to this data
 * item.  Future references to the data should use the number
 * rather than the name.  The link ID will be used to associate
 * a metric with a particular application link, or MAC interface, etc.
 *------------------------------------------------------*/
int GUI_DefineMetric(const char*  name,
                     NodeId       nodeID,
                     GuiLayers    layer,
                     int          linkID,
                     GuiDataTypes datatype,
                     GuiMetrics   metrictype) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    int      metricID;

    metricID = GetMetricID(name, layer, datatype);

    reply.type = GUI_STATISTICS_COMMAND;
    sprintf(replyBuff, "%d %d \"%s\" %u %d %d %d %d",
            GUI_DEFINE_METRIC, metricID, name, nodeID, layer, linkID,
            (int) datatype, (int) metrictype);
    reply.args.append(replyBuff);

    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }

    //printf("Metric (\"%s\") assigned ID %d\n", name, metricID);
    return metricID;
}

/*------------------------------------------------------
 * SendIntegerData
 *
 * Sends data for an integer metric.
 *------------------------------------------------------*/
void GUI_SendIntegerData(NodeId      nodeID,
                         int         metricID,
                         int         value,
                         clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (g_metricFilters[metricID].nodeEnabled[GetNodeIndexFromHash(nodeID)])
    {
        ctoa(time, timeString);

        reply.type = GUI_STATISTICS_COMMAND;
        sprintf(replyBuff, "%d %u %d %d %s",
                GUI_SEND_INTEGER, nodeID, metricID, value, timeString);
        reply.args.append(replyBuff);

        if (GUI_isConnected ()) {
            GUI_SendReply(GUI_guiSocket, reply);
        }

#ifdef HLA_INTERFACE
        if (HlaIsActive())
        {
            assert(metricID < g_numberOfMetrics);
            assert(g_metricFilters[metricID].metricDataPtr != NULL);

            HlaUpdateMetric(
                nodeID,
                *g_metricFilters[metricID].metricDataPtr,
                &value,
                time);
        }
#endif /* HLA_INTERFACE */
    }
}

/*------------------------------------------------------
 * SendUnsignedData
 *
 * Sends data for an unsigned metric.
 *------------------------------------------------------*/
void GUI_SendUnsignedData(NodeId      nodeID,
                          int         metricID,
                          unsigned    value,
                          clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (g_metricFilters[metricID].nodeEnabled[GetNodeIndexFromHash(nodeID)])
    {
        ctoa(time, timeString);

        reply.type = GUI_STATISTICS_COMMAND;
        sprintf(replyBuff, "%d %u %d %d %s",
                GUI_SEND_UNSIGNED, nodeID, metricID, value, timeString);
        reply.args.append(replyBuff);

        if (GUI_isConnected ()) {
            GUI_SendReply(GUI_guiSocket, reply);
        }

#ifdef HLA_INTERFACE
        if (HlaIsActive())
        {
            assert(metricID < g_numberOfMetrics);
            assert(g_metricFilters[metricID].metricDataPtr != NULL);

            HlaUpdateMetric(
                nodeID,
                *g_metricFilters[metricID].metricDataPtr,
                &value,
                time);
        }
#endif /* HLA_INTERFACE */
    }
}

/*------------------------------------------------------
 * SendRealData
 *
 * Sends data for a double metric.
 *------------------------------------------------------*/
void GUI_SendRealData(NodeId      nodeID,
                      int         metricID,
                      double      value,
                      clocktype   time) {
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    char     timeString[MAX_CLOCK_STRING_LENGTH];
    QNPartitionLock globalsLock(&GUI_globalsMutex);

    if (g_metricFilters[metricID].nodeEnabled[GetNodeIndexFromHash(nodeID)])
    {
        ctoa(time, timeString);

        reply.type = GUI_STATISTICS_COMMAND;
        sprintf(replyBuff, "%d %u %d %.15f %s",
                GUI_SEND_REAL, nodeID, metricID, value, timeString);
        reply.args.append(replyBuff);

        if (GUI_isConnected ()) {
            GUI_SendReply(GUI_guiSocket, reply);
        }

#ifdef HLA_INTERFACE
        if (HlaIsActive())
        {
            assert(metricID < g_numberOfMetrics);
            assert(g_metricFilters[metricID].metricDataPtr != NULL);

            HlaUpdateMetric(
                nodeID,
                *g_metricFilters[metricID].metricDataPtr,
                &value,
                time);
        }
#endif /* HLA_INTERFACE */
    }
}

/*------------------------------------------------------
 * FUNCTION     GUI_CreateReply
 * PURPOSE      Function used to replace newline characters in a string being
 *              sent to the GUI.
 *
 * Parameters:
 *    replyType:  the type of reply
 *    msg:        the reply message
 *------------------------------------------------------*/
GuiReply GUI_CreateReply(GuiReplies replyType,
                         std::string* msgString) {

    GuiReply reply;
    int      m = 0;
    const char* msg = msgString->c_str();

    reply.type = replyType;

    // replace '\n' with "<p>" since the GUI uses '\n' to signify
    // the end of a message.

    do {
        if (msg[m] == '\n') {
            reply.args.append("<br>");
        }
        else {
            reply.args += msg[m];
        }
    } while (msg[m++] != '\0');

    return reply;
}

#ifdef _WIN32

/*------------------------------------------------------
 * GUI_ConnectToGUI
 *------------------------------------------------------*/
void GUI_ConnectToGUI(char*          hostname,
                      unsigned short portnum) {
    LPHOSTENT   lpHostEntry;
    SOCKADDR_IN saServer;
    SOCKET_HANDLE      theSocket;
    WORD        wVersionRequested = MAKEWORD(1,1);
    WSADATA     wsaData;
    int         returnValue;

    //
    // Initialize WinSock and check the version
    //
    returnValue = WSAStartup(wVersionRequested, &wsaData);
    if (wsaData.wVersion != wVersionRequested) {
        fprintf(stderr,"\n Wrong version\n");
        GUI_guiSocketOpened = FALSE;
        return;
    }

    //
    // Find the server
    //
    lpHostEntry = gethostbyname(hostname);
    if (lpHostEntry == NULL) {
        fprintf(stderr,"gethostbyname()");
        GUI_guiSocketOpened = FALSE;
        return;
    }

    //
    // Create a TCP/IP datagram socket
    //
    theSocket = socket(AF_INET,          // Address family
                       SOCK_STREAM,      // Socket type
                       IPPROTO_TCP);     // Protocol
    if (theSocket == INVALID_SOCKET) {
        ERROR_ReportWarning("failed to create the socket\n");
        GUI_guiSocketOpened = FALSE;
        return;
    }

    //
    // Fill in the address structure for the server
    //
    saServer.sin_family = AF_INET;
    saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);

    // ^ Server's address
    saServer.sin_port = htons(portnum);

    returnValue = connect(theSocket,
                          (LPSOCKADDR)&saServer,
                          sizeof(struct sockaddr));
    if (returnValue != SOCKET_ERROR) {
        GUI_guiSocket       = theSocket;
        GUI_guiSocketOpened = TRUE;
        return;
    }
    else {
        ERROR_ReportWarning("failed to connect to the socket\n");
        GUI_guiSocketOpened = FALSE;
        return;
    }
}

/*------------------------------------------------------
 * GUI_DisconnectFromGUI
 *------------------------------------------------------*/
void GUI_DisconnectFromGUI(SOCKET_HANDLE socket) {
    if (socket != 0) {
        std::string replyText = " ";
        GUI_SendReply(socket, GUI_CreateReply(GUI_FINISHED, &replyText));

        if (DEBUG) {
            printf ("windows gui disconnect sent GUI_FINISHED\n");
            fflush(stdout);
        }

        GUI_guiSocketOpened = FALSE;
        closesocket(socket);

        //
        // Release WinSock
        //
        WSACleanup();
    }
}

#else // unix

/*------------------------------------------------------
 * GUI_ConnectToGUI
 *------------------------------------------------------*/
void GUI_ConnectToGUI(char*          hostname,
                      unsigned short portnum) {
    struct sockaddr_in sa;
    struct hostent*    hp;

    int localSocket;

    if ((hp = gethostbyname(hostname)) == NULL) { /* do we know the host's */
        errno = ECONNREFUSED;                       /* address? */
        printf("couldn't get hostinfo\n");
        return;
    }

    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr,hp->h_addr,hp->h_length);     /* set address */
    sa.sin_family= hp->h_addrtype;
    sa.sin_port= htons((unsigned short)portnum);

    if ((localSocket = socket(hp->h_addrtype,SOCK_STREAM,0)) < 0) {
        printf("couldn't create socket\n");
        GUI_guiSocketOpened = FALSE;
        return;
    }

    if (connect(localSocket,(struct sockaddr *)&sa, sizeof(sa)) < 0) {
        printf("couldn't connect to server\n");
        close(localSocket);
        GUI_guiSocketOpened = FALSE;
        return;
    }

    GUI_guiSocket       = localSocket;
    GUI_guiSocketOpened = TRUE;
}

/*------------------------------------------------------
 * GUI_DisconnectFromGUI
 *------------------------------------------------------*/
void GUI_DisconnectFromGUI(SOCKET_HANDLE socket) {
    if (socket != 0) {
        std::string replyText = " ";
        GUI_SendReply(socket, GUI_CreateReply(GUI_FINISHED, &replyText));

        if (DEBUG) {
            printf ("unix gui disconnect sent GUI_FINISHED\n");
            fflush(stdout);
        }

        GUI_guiSocketOpened = FALSE;
        close(socket);
    }
}
#endif

/*------------------------------------------------------
 * GUI_ReceiveCommand
 *------------------------------------------------------*/
GuiCommand GUI_ReceiveCommand(SOCKET_HANDLE socket) {
    char buffer[256];
    char *args;
    int  returnValue;
    GuiCommand command;
    int i = 0;
    //int* bufptr = (int*) &buffer[0];

    command.args[0] = '\0';

    while (TRUE) {
        returnValue = recv(socket,         // Connected socket
                           &buffer[i],     // Receive buffer
                           1,              // Size of receive buffer
                           0);             // Flags
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
        // skip carriage return on windows
        if (((int) buffer[i]) != 13) {
            i++;
        }
    }
    //printf("got command '%s'\n", buffer);
    //fflush(stdout);
    if (strlen(buffer) < 1) {
        // this is an error
        command.type = GUI_UNRECOGNIZED;
    }
    else {
        //sscanf(buffer, "%d", &command.type);
        command.type = (GuiCommands) atoi(buffer);

        args = strstr(buffer, " ");
        if (args != NULL) {
            strcpy(command.args, args);
        }
    }
    return command;
}

/*------------------------------------------------------
 * GUI_SendReply
 *------------------------------------------------------*/
void GUI_SendReply(SOCKET_HANDLE   socket,
                   GuiReply reply) {
    char buffer[GUI_MAX_COMMAND_LENGTH + 10];
    std::string outString;

    sprintf(buffer, "%d ", reply.type);
    outString.append(buffer);
    outString.append(reply.args);
    outString.append("\n");

    send(socket, outString.c_str(), outString.size(), 0);
    //printf("sent over socket: %s", buffer); fflush(stdout);
}

/*------------------------------------------------------
 * GUI_SetLayerFilter
 *------------------------------------------------------*/
void GUI_SetLayerFilter(char* args,
                        BOOL  offOrOn) {
    int layer;
    sscanf(args, "%d", &layer);
    QNPartitionLock globalsLock(&GUI_globalsMutex);
    g_layerEnabled[layer] = offOrOn;
}

/*------------------------------------------------------
 * GUI_SetNodeFilter
 *------------------------------------------------------*/
void GUI_SetNodeFilter(PartitionData * partitionData,
                       char* args,
                       BOOL  offOrOn) {
    QNPartitionLock globalsLock(&GUI_globalsMutex);
    //args should contain the nodeID
    NodeId      nodeID = atoi(args);
    if (nodeID == 0) {
        // set all nodes
        int index;
        for (index = 0; index < g_numberOfNodes; index++) {
            g_nodeEnabledForAnimation[index] = offOrOn;
        }
    }
    else {
        int index = GetNodeIndexFromHash(nodeID);

        g_nodeEnabledForAnimation[index] = offOrOn;
    }
}

/*------------------------------------------------------
 * GUI_SetMetricFilter
 *------------------------------------------------------*/
void GUI_SetMetricFilter(char* args,
                         BOOL  offOrOn) {
    QNPartitionLock globalsLock(&GUI_globalsMutex);
    //args should contain both metric ID and node or link ID and layer
    //we don't currently deal with link ID, so assume it's a node ID
    int metricID;
    int nodeID;
    int layer;
    sscanf(args, "%d %d %d", &metricID, &nodeID, &layer);
    if (nodeID == 0) {
        // all nodes
        int i;
        for (i = 0; i < g_numberOfNodes; i++) {
            g_metricFilters[metricID].nodeEnabled[i] = offOrOn;
            if (offOrOn) {
                g_nodeEnabledForStatistics[i]++;
                g_statLayerEnabled[layer]++;
            }
            else {
                g_nodeEnabledForStatistics[i]--;
                g_statLayerEnabled[layer]--;
            }
        }
    }
    else {
        int index = GetNodeIndexFromHash(nodeID);
        g_metricFilters[metricID].nodeEnabled[index] = offOrOn;
        if (offOrOn) {
            g_nodeEnabledForStatistics[index]++;
            g_statLayerEnabled[layer]++;
        }
        else {
            g_nodeEnabledForStatistics[index]--;
            g_statLayerEnabled[layer]--;
        }
    }
}

/*------------------------------------------------------
 * GUI_NodeIsEnabledForAnimation
 *------------------------------------------------------*/
BOOL GUI_NodeIsEnabledForAnimation(NodeId nodeID) {
    int index = GetNodeIndexFromHash(nodeID);
    return g_nodeEnabledForAnimation[index];
}

/*------------------------------------------------------
 * GUI_NodeIsEnabledForStatistics
 *------------------------------------------------------*/
BOOL GUI_NodeIsEnabledForStatistics(NodeId nodeID) {
    int index = GetNodeIndexFromHash(nodeID);
    if (g_nodeEnabledForStatistics[index] > 0)
        return TRUE;
    else
        return FALSE;
}

/*------------------------------------------------------
 * GUI_LayerIsEnabledForStatistics
 *------------------------------------------------------*/
BOOL GUI_LayerIsEnabledForStatistics(GuiLayers layer) {
    return g_statLayerEnabled[layer];
}

// /*
//  *   This function is installed as an ErrorHandler so that when
//  *   ERORR_WriteMessage () reports an error this handler will
//  *   be triggered so that we can notify the GUI of the warning/error.
//  */
// MT Safe
void EXTERNAL_GUI_ErrorHandler (int type, const char * errorMessage)
{
    std::string error;

    // local copy so we can modify it.
    error = errorMessage;

    GuiReplies replyType;
    switch (type) {
        case ERROR_ASSERTION:
            replyType = GUI_ASSERTION;
            break;
        case ERROR_ERROR:
            replyType = GUI_ERROR;
            break;
        case ERROR_WARNING:
            replyType = GUI_WARNING;
            break;
        default:
            abort();
    }

    if (GUI_isConnected ()) {
        GuiCommand command;
        GUI_SendReply(GUI_guiSocket,
                      GUI_CreateReply(replyType, &error));
        if (type != ERROR_WARNING) {
            // The simulation is exiting via a call to abort ().
            // However, before terminating we wait for the IDE to
            // send us a final handshake sent so that we know it
            // got the message that the simulation is aborting.
            do {
                command = GUI_ReceiveCommand(GUI_guiSocket);
            } while (command.type != GUI_STOP);
            GUI_DisconnectFromGUI(GUI_guiSocket);
        }
    }
}

#ifdef D_LISTENING_ENABLED
GuiDynamicObjectCallback::GuiDynamicObjectCallback(
    EXTERNAL_Interface* iface,
    GuiEvents eventType)
    : m_Iface(iface), m_EventType(eventType)
{
    // empty
}

void GuiDynamicObjectCallback::operator () (const std::string& newValue)
{
    // Send gui command to add new object if connected
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    reply.type = GUI_DYNAMIC_COMMAND;
    if (m_EventType == GUI_DYNAMIC_AddObject
        || m_EventType == GUI_DYNAMIC_ObjectPermissions)
    {
        sprintf(replyBuff, "%d ", m_EventType);
        reply.args.append(replyBuff);
        reply.args.append(newValue);
        reply.args.append(" ");

        // Add rwx permissions
        if (m_Iface->partition->dynamicHierarchy.IsReadable(newValue))
        {
            reply.args.append("r");
        }
        if (m_Iface->partition->dynamicHierarchy.IsWriteable(newValue))
        {
            reply.args.append("w");
        }
        if (m_Iface->partition->dynamicHierarchy.IsExecutable(newValue))
        {
            reply.args.append("x");
        }
    }
    else
    {
        sprintf(replyBuff, "%d ", m_EventType);
        reply.args.append(replyBuff);
        reply.args.append(newValue);
        reply.args.append(" ");
    }
    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}

GuiDynamicObjectValueCallback::GuiDynamicObjectValueCallback(
    EXTERNAL_Interface* iface,
    const std::string &path)
    : m_Iface(iface), m_Path(path)
{
    // empty
}

void GuiDynamicObjectValueCallback::operator () (const std::string& newValue)
{
    // Send gui command to add new object if connected
    GuiReply reply;
    char replyBuff[GUI_MAX_COMMAND_LENGTH];
    reply.type = GUI_DYNAMIC_COMMAND;
    sprintf(replyBuff, "%d ",
        GUI_DYNAMIC_ObjectValue);
    reply.args.append(replyBuff);
    reply.args.append(m_Path);
    reply.args.append(" ");
    reply.args.append(newValue);
    if (GUI_isConnected ()) {
        GUI_SendReply(GUI_guiSocket, reply);
    }
    else {
        printf("%s\n", reply.args.c_str());
    }
}
#endif

static BOOL GUI_isConnected ()
{
    return GUI_guiSocketOpened;
}
BOOL GUI_isAnimate ()
{
    return GUI_animationTrace;
}
bool GUI_isAnimateOrInteractive ()
{
    // Animation (.trc) or interactive behavior is true.
    if (GUI_isConnected () || GUI_isAnimate ())
        return true;
    return false;
}

// Main, before creating threads for each partition will
// call EXTERNAL_Bootstrap (), which will call this. In MPI
// this will be called on each process.
bool GUI_EXTERNAL_Bootstrap (int argc, char * argv [], NodeInput * nodeInput,
    int numberOfProcessors, int thisPartitionId)
{
    bool    active = false;
    bool    reportErrors = true;
    BOOL     wasFound;
    char     buf[MAX_STRING_LENGTH];

    // main does this for us.
    // PrintSimTimeInterval = maxClock / NUM_SIM_TIME_STATUS_PRINTS;

    //
    /* Set the statistics and status print intervals. */
    // These global variable have to be set here because
    // these global variables are used by main, however
    // GUI_Initialize () won't be called unless external interfaces
    // are active. We always know bootstrap is called, so set
    // these variable here.
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STAT_RUN_TIME_INTERVAL",
        &wasFound,
        buf);

    if (wasFound == FALSE) {
        GUI_statInterval = CLOCKTYPE_MAX;
    }
    else {
        GUI_statInterval = TIME_ConvertToClock(buf);
        if (GUI_statInterval <= 0) {
            ERROR_ReportError("Invalid entry for STAT_RUN_TIME_INTERVAL\n");
        }
    }
    GUI_statReportTime = GUI_statInterval;
    if (thisPartitionId != 0)
    {
        // We are MPI, and each process will examine the argc/argv.
        // Only p0 needs to complain about probelms with the cmd line
        // args.
        reportErrors = false;
    }

    int thisArg = 2;
    while (thisArg < argc) {
        /*
         * Set up socket connection to GUI
         */
        if (!strcmp(argv[thisArg], "-interactive")) {
            char hostname[64];
            int  portNumber;
            //
            // if thisarg is -interactive, then next two must be host port
            //
            if (argc < thisArg + 3) {
                if (reportErrors) {
                    ERROR_ReportError(
                        "Not enough arguments to -interactive.\n"
                        "Correct Usage:\t -interactive host port.\n");
                }
            }

            strcpy(hostname, argv[thisArg+1]);
            portNumber = atoi(argv[thisArg+2]);

            GUI_ConnectToGUI(hostname, (unsigned short) portNumber);
#ifdef PARALLEL //Parallel
            if (GUI_guiSocketOpened) {
                PARALLEL_SetGreedy(false);
            }
#endif //endParallel
            // Add our handler to send error messages back to the gui.
            ERROR_InstallHandler (&EXTERNAL_GUI_ErrorHandler);
            // guiEnabled = true;
            active = true;
            thisArg += 3;
        }

        /*
         * Check whether animation is enabled.  If the GUI is interactive,
         * then animation is implied.
         */
        else if (!strcmp(argv[thisArg], "-animate")) {
            GUI_animationTrace = TRUE;
            active = true;
            thisArg++;
        }
        else {
          thisArg++;
        }
    }
    return active;
}

void GUI_EXTERNAL_Finalize (EXTERNAL_Interface *iface) {
    // Close socket connection to GUI.
    if (GUI_isConnected ()) {
        if (DEBUG) {
            printf ("Finalizing - performing disconnect\n");
            fflush(stdout);
        }
        GUI_DisconnectFromGUI(GUI_guiSocket);
    }
}



/*--
 *  Return the simulation time that the IDE has advanced to
 */
clocktype GUI_EXTERNAL_QueryExternalTime (EXTERNAL_Interface *iface) {
    GUI_InterfaceData *     data;
    data = (GUI_InterfaceData*) iface->data;
    return data->guiExternalTime;
}

/*--
 *  Return the simulation time that the IDE will allow the simulation
 *  to execute out to.
 */
void GUI_EXTERNAL_SimulationHorizon (EXTERNAL_Interface *iface) {
    // Don't need to do anything, as the iface's horizon is
    // advanced as step commands arrive from the IDE.
    GUI_InterfaceData *     data;

    data = (GUI_InterfaceData*) iface->data;
    iface->horizon = data->guiExternalTimeHorizon;
    return;
}

void DeactivateNode(EXTERNAL_Interface* iface, char* nodeId)
{
    NodeAddress id = atoi(nodeId);
    Node* node;

    node = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        id);

    if (node != NULL)
    {
        EXTERNAL_DeactivateNode(
            iface,
            node);
    }


}

void ActivateNode(EXTERNAL_Interface* iface, char* nodeId)
{
    NodeAddress id = atoi(nodeId);
    Node* node;

    node = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        id);


    if (node != NULL)
    {
        EXTERNAL_ActivateNode(
            iface,
            node);
    }
}

#ifdef HITL
// These are ultra simple and primitive functions used to modify
// all nodes of the scenario during the simulation.
void SetCbrPrecedence(PartitionData* partitionData, char* p)
{
    int precedence = *p - '0';
    int i;
    int j;
    int childrenA;
    int childrenB;
    //char* childNameA;
    //char* childNameB;
    std::string childNameA;
    std::string childNameB;
    //char path[MAX_STRING_LENGTH];
    std::string path;
    char value[MAX_STRING_LENGTH];

    if (precedence < 0 || precedence > 7)
    {
        return;
    }

    // Put in TOS form
    sprintf(value, "%d", precedence << 5);

    // NOTE: Here we should add * capability to paths
    try
    {
        // Loop over all nodes
        if (partitionData->dynamicHierarchy.IsEnabled () == false)
        {
            ERROR_ReportWarning("CBR precedence cannot be changed because DYNAMIC-ENABLED isn't set for this scenario");
            return;
        }
        childrenA = partitionData->dynamicHierarchy.GetNumChildren("/node");
        for (i = 0; i < childrenA; i++)
        {
            childNameA = (partitionData->dynamicHierarchy.GetChildName("/node", i)).c_str();

            //sprintf(path, "/node/%s/cbrClient", childNameA);
            path = "/node/"+childNameA+"/cbrClient";
            try
            {
                // Loop over all cbr apps
                childrenB = partitionData->dynamicHierarchy.GetNumChildren(path);
                for (j = 0; j < childrenB; j++)
                {
                    childNameB = (partitionData->dynamicHierarchy.GetChildName(path, j)).c_str();
                    //sprintf(
                    //    path,
                     //   "/node/%s/cbrClient/%s/tos",
                      //  childNameA,
                       // childNameB);
                    path = "/node/"+childNameA+"/cbrClient/"+childNameB+"/tos";

                    // If this is a valid path (no exception was thrown)
                    // then write the new value
                    partitionData->dynamicHierarchy.WriteAsString(path, value);
                }
            }
            catch (D_Exception &e)
            {
                // Ignore -- path doesn't exist, not an error.
            }
        }
    }
    catch (D_Exception &e)
    {
        e.GetFullErrorString(path);
        printf("exception: %s\n", path.c_str());
    }

    partitionData->dynamicHierarchy.Print();
}

// for n command: change the tos for AppForward (IPNE)
void SetCbrPrecedenceAppForward(PartitionData* partitionData, char* p)
{
    int precedence = *p - '0';
    int i;
    int j;
    int childrenA;
    int childrenB;
    //char* childNameA;
    //char* childNameB;
    //char path[MAX_STRING_LENGTH];
    std::string childNameA;
    std::string childNameB;
    std::string path;
    char value[MAX_STRING_LENGTH];

    if (precedence < 0 || precedence > 7)
    {
        return;
    }

    // Put in TOS form
    sprintf(value, "%d", precedence << 5);

    // NOTE: Here we should add * capability to paths
    try
    {
        // Loop over all nodes
        if (partitionData->dynamicHierarchy.IsEnabled () == false)
        {
            ERROR_ReportWarning("CBR precedence cannot be changed because DYNAMIC-ENABLED isn't set for this scenario");
            return;
        }
        childrenA = partitionData->dynamicHierarchy.GetNumChildren("/node");
        for (i = 0; i < childrenA; i++)
        {
            childNameA = partitionData->dynamicHierarchy.GetChildName("/node", i);
            //sprintf(path, "/node/%s/appForward", childNameA);
            path = "/node/"+childNameA+"/appForward";
            try
            {
                // Loop over all cbr apps
                childrenB = partitionData->dynamicHierarchy.GetNumChildren(path);
                for (j = 0; j < childrenB; j++)
                {
                    childNameB = partitionData->dynamicHierarchy.GetChildName(path, j);
                    /*
                    sprintf(
                        path,
                        "/node/%s/appForward/%s/tos",
                        childNameA,
                        childNameB);
                    */
                    path = "/node/"+childNameA+"/appForward/"+childNameB+"/tos";

                    // If this is a valid path (no exception was thrown)
                    // then write the new value
                    partitionData->dynamicHierarchy.WriteAsString(path, value);
                }
            }
            catch (D_Exception &e)
            {
                // Ignore -- path doesn't exist, not an error.
            }
        }
    }

    catch (D_Exception &e)
    {
        e.GetFullErrorString(path);
        printf("exception: %s\n", path.c_str());
    }

    partitionData->dynamicHierarchy.Print();
}

// These are ultra simple and primitive functions used to modify
// all nodes of the scenario during the simulation.
// T command
void SetCbrTrafficRate(PartitionData* partitionData, char* r)
{
    int i;
    int j;
    int childrenA;
    int childrenB;
    std::string childNameA;
    std::string childNameB;
    std::string path;
    //char value[MAX_STRING_LENGTH];
    std::string value;

    //parse input rate -> input is MS
    while (*r != 0 && isspace(*r)) {
        r++;
    }

    //sanity check
    char temp[10];
    strncpy(temp, r, strlen(r));
    for(i = 0; i < (int) strlen(r);i++)
    {
        if (!isdigit(temp[i]) && (temp[i]!= '.')) {
            printf("should be digit\n");
            return;
        }
    }
    if(!strncmp(r, "10", 2) && isspace(*(r+2)))
    {
        value = "10.0MS";
    }
    else
    {
        value.assign(r);
        value.append("MS");
    }

    // NOTE: Here we should add * capability to paths
    try
    {
        if (partitionData->dynamicHierarchy.IsEnabled () == false)
        {
            ERROR_ReportWarning("CBR traffic rate cannot be changed because DYNAMIC-ENABLED isn't set for this scenario");
            return;
        }
        // Loop over all nodes
        childrenA = partitionData->dynamicHierarchy.GetNumChildren("/node");
        for (i = 0; i < childrenA; i++)
        {
            childNameA = partitionData->dynamicHierarchy.GetChildName("/node", i);

            //sprintf(path, "/node/%s/cbrClient", childNameA);
            path = "/node/"+childNameA+"/cbrClient";
            try
            {
                // Loop over all cbr apps
                childrenB = partitionData->dynamicHierarchy.GetNumChildren(path);
                for (j = 0; j < childrenB; j++)
                {
                    childNameB = partitionData->dynamicHierarchy.GetChildName(path, j);
                    //sprintf(
                    //    path,
                    //    "/node/%s/cbrClient/%s/interval",
                    //    childNameA,
                    //    childNameB);
                    path = "/node/"+childNameA+"/cbrClient/"+childNameB+"/interval";

                    // If this is a valid path (no exception was thrown)
                    // then write the new value
                    partitionData->dynamicHierarchy.WriteAsString(path, value);
                }
            }
            catch (D_Exception &e)
            {
                // Ignore -- path doesn't exist, not an error.
            }
        }
    }
    catch (D_Exception &e)
    {
        e.GetFullErrorString(path);
        printf("exception: %s\n", path.c_str());
    }
    partitionData->dynamicHierarchy.Print();
}

// modify the CBR interval for all nodes of the scenario
// L command
void SetCbrTrafficRateTimes(PartitionData* partitionData, char* r)
{
    int i;
    int j;
    int childrenA;
    int childrenB;
    std::string childNameA;
    std::string childNameB;
    std::string path;
    std::string value;

    //parse input rate -> input is MS
    while (*r != 0 && isspace(*r)) {
        r++;
    }

    //sanity check
    char temp[10];
    strncpy(temp, r, strlen(r));
    for(i = 0; i < (int) strlen(r);i++)
    {
        if (!isdigit(temp[i]) && (temp[i]!= '.')) {
            printf("should be digit\n");
            return;
        }
    }

    // NOTE: Here we should add * capability to paths
    try
    {
        if (partitionData->dynamicHierarchy.IsEnabled () == false)
        {
            ERROR_ReportWarning("CBR traffic rate cannot be changed because DYNAMIC-ENABLED isn't set for this scenario");
            return;
        }
        // Loop over all nodes
        childrenA = partitionData->dynamicHierarchy.GetNumChildren("/node");
        for (i = 0; i < childrenA; i++)
        {
            childNameA = partitionData->dynamicHierarchy.GetChildName("/node", i);

            //sprintf(path, "/node/%s/cbrClient", childNameA);
            path = "/node/"+childNameA+"/cbrClient";
            try
            {
                // Loop over all cbr apps
                childrenB = partitionData->dynamicHierarchy.GetNumChildren(path);
                for (j = 0; j < childrenB; j++)
                {
                    childNameB = partitionData->dynamicHierarchy.GetChildName(path, j);
                    path = "/node/"+childNameA+"/cbrClient/"+childNameB+"/interval";

                    // If this is a valid path (no exception was thrown)
                    // read the value and multiply
                    partitionData->dynamicHierarchy.ReadAsString(path, value);
                    std::string newValue;
                    float tempValue = (float) strtod(value.c_str(),NULL);
                    float tempInput = (float) strtod(r,NULL);

                    std::stringstream ss;
                    ss << ((tempValue * tempInput)/1000000);
                    ss >> newValue;

                    newValue = newValue + "MS";
                    partitionData->dynamicHierarchy.WriteAsString(path, newValue);
                }
            }
            catch (D_Exception &e)
            {
                // Ignore -- path doesn't exist, not an error.
            }
        }
    }
    catch (D_Exception &e)
    {
        e.GetFullErrorString(path);
        printf("exception: %s\n", path.c_str());
    }
    partitionData->dynamicHierarchy.Print();
}

#endif

/*--
 *
 *  Block waiting for more commands from the gui.
 */
void GUI_EXTERNAL_ReceiveCommands (EXTERNAL_Interface *iface) {
    char        timeString[MAX_CLOCK_STRING_LENGTH];
    GuiCommand  command;
    BOOL        waitingForStepCommand;
    GuiReply    reply;
    PartitionData * partitionData = iface->partition;
    GUI_InterfaceData *     data;
    data = (GUI_InterfaceData*) iface->data;

    if (data->guiExternalTimeHorizon > partitionData->maxSimClock)
        return;

    // Time of the next event that the simulation will next execute.
    clocktype advanceTime = GetNextInternalEventTime (partitionData);

    if (data->firstIteration)
    {
        // For the first iteration:
        // Intialization is now complete. We now make a blocking
        // call to wait for the gui to send us the first
        // GUI_STEP command. Because GUI_ReceiveCommand
        // is going to block waiting for the user to hit play we
        // are in effect in a paused state.
        data->firstIteration = false;
        partitionData->wallClock->pause ();
    }
    if (advanceTime == CLOCKTYPE_MAX) {
        // In the case of IPNE, events will eventually come
        // from the external interface, but there might
        // not be any yet. So, we are going to
        // proceed as though everything is ready right now.
        // This will "heartbeat" between the simulation
        // and the gui.
        // advanceTime = EXTERNAL_QuerySimulationTime(iface);
        advanceTime = data->guiExternalTimeHorizon;
    }

    while (data->guiExternalTimeHorizon <= advanceTime) {
        // The simulation has reached the gui-horizion. So we will
        // now notify the gui we've reached the horizon
        // (GUI_STEPPED==STEP_REPLY) and block waiting for the next
        // STEP-forward message from the gui.

        // Tell the Animator we've reached the simulation time
        // allowed by the last step.
        ctoa (EXTERNAL_QuerySimulationTime(iface), timeString);
        reply.type = GUI_STEPPED;
        reply.args.append(timeString);
        GUI_SendReply(GUI_guiSocket, reply);

        if (DEBUG) {
            printf ("Sent a GUI_STEPPED reply to gui '%s'\n", reply.args.c_str());
            fflush(stdout);
        }

        waitingForStepCommand = TRUE;
        while (waitingForStepCommand) {
            command = GUI_ReceiveCommand(GUI_guiSocket);
            switch (command.type) {
                case GUI_STEP:
                {
                    data->guiExternalTime = data->guiExternalTimeHorizon;
                    //guiExternalTimeHorizon = advanceTime + guiStepSize;
                    // Instead of just opening the horizion a fixed
                    // amount we will open up out to the next internal
                    // event time.
                    // data->guiExternalTimeHorizon += data->guiStepSize;
                    clocktype nextEventTime =
                        GetNextInternalEventTime (partitionData);
                    if (advanceTime > nextEventTime)
                    {
                        // A new earlier event has been scheduled
                        // while we have been looping here in our
                        // handshake while loop, so use the updated
                        // upper limit on how long we handshake for.
                        advanceTime = nextEventTime;
                    }
                    if (nextEventTime == CLOCKTYPE_MAX)
                    {
                       data->guiExternalTimeHorizon = CLOCKTYPE_MAX;
                    }
                    else
                    {
                        data->guiExternalTimeHorizon =
                            GetNextInternalEventTime (partitionData) +
                            data->guiStepSize;
                    }
                    if (DEBUG) {
                        printf("set our gui horizon to %" 
                                TYPES_64BITFMT "d \n",
                               data->guiExternalTimeHorizon);
                        fflush(stdout);
                    }

                    waitingForStepCommand = FALSE;
                    partitionData->wallClock->resume ();
                    break;
                }
                case GUI_DISABLE_LAYER:
                    GUI_SetLayerFilter(command.args, FALSE);
                    break;
                case GUI_ENABLE_LAYER:
                    GUI_SetLayerFilter(command.args, TRUE);
                    break;
                case GUI_SET_COMM_INTERVAL:
                    data->guiStepSize = TIME_ConvertToClock(command.args);
                    break;
                case GUI_ENABLE_NODE:
                    GUI_SetNodeFilter(iface->partition, command.args, TRUE);
                    break;
                case GUI_DISABLE_NODE:
                    GUI_SetNodeFilter(iface->partition, command.args, FALSE);
                    break;
                case GUI_SET_STAT_INTERVAL:
                    GUI_statInterval = TIME_ConvertToClock(command.args);
                    // guiExternalTime hasn't been advanced yet.
                    GUI_statReportTime = data->guiExternalTime;
                    break;
                case GUI_ENABLE_METRIC:
                    GUI_SetMetricFilter(command.args, TRUE);
                    break;
                case GUI_DISABLE_METRIC:
                    GUI_SetMetricFilter(command.args, FALSE);
                    break;
                case GUI_STOP:
                    // Shut the simulation down asap.
                    waitingForStepCommand = FALSE;
                    data->guiExternalTimeHorizon = CLOCKTYPE_MAX;
                    EXTERNAL_SetSimulationEndTime (iface);
                    break;
                case GUI_PAUSE:
                    partitionData->wallClock->pause ();
                    break;
                case GUI_DYNAMIC_ReadAsString:
                {
                    std::string value;
                    try
                    {
                        // Skip past first whitespace
                        char* firstChar = command.args;
                        while (*firstChar != 0 && *firstChar == ' ')
                        {
                            firstChar++;
                        }

                        // Get value from hierarchy
                        partitionData->dynamicHierarchy.ReadAsString(
                            firstChar,
                            value);

                        // Send gui command for object value
                        GuiReply reply;
                        char resultStr[GUI_MAX_COMMAND_LENGTH];

                        reply.type = GUI_DYNAMIC_COMMAND;
                        sprintf(resultStr, "%d ", GUI_DYNAMIC_ObjectValue);
                        reply.args = resultStr;
                        reply.args += firstChar;
                        reply.args += " ";
                        reply.args += value;

                        GUI_SendReply(GUI_guiSocket, reply);
                    }
                    catch (D_Exception &e)
                    {
                        e.GetFullErrorString(value);
                        ERROR_ReportError((char*) value.c_str());
                    }
                    break;
                }
                case GUI_DYNAMIC_WriteAsString:
                {
                    try
                    {
                        char path[MAX_STRING_LENGTH];
                        char value[MAX_STRING_LENGTH];

                        sscanf(command.args, "%s %s", path, value);
                        //printf("writing %s %s\n", path, value);

                        // Get value from hierarchy
                        partitionData->dynamicHierarchy.WriteAsString(
                            path,
                            value);
                    }
                    catch (D_Exception &e)
                    {
                        std::string err;
                        e.GetFullErrorString(err);
                        ERROR_ReportError((char*) err.c_str());
                    }
                    break;
                }
                case GUI_DYNAMIC_Listen:
                {
#ifdef D_LISTENING_ENABLED
                    try
                    {
                        // Skip past first whitespace
                        char* firstChar = command.args;
                        while (*firstChar != 0 && *firstChar == ' ')
                        {
                            firstChar++;
                        }

                        // Get value from hierarchy
                        partitionData->dynamicHierarchy.AddListener(
                            firstChar,
                            "",
                            "",
                            "GUI",
                            new GuiDynamicObjectValueCallback(iface, firstChar));
                    }
                    catch (D_Exception &e)
                    {
                        std::string value;
                        e.GetFullErrorString(value);
                        ERROR_ReportError((char*) value.c_str());
                    }
                    break;
#endif // D_LISTENING_ENABLED
                    break;
                }
                case GUI_USER_DEFINED: {
#ifdef HITL
// BEGIN HUMAN IN THE LOOP CODE
                    char *ch;
                    switch (command.args[1]) {
                        case 'p': case 'P':
                            // This command will change the precedence of
                            // all Voip applications.
                            // Advance to the first non-whitespace character
                            ch = &command.args[2];
                            ch ++;
                            while (*ch != 0 && isspace(*ch)) {
                                ch++;
                            }
                            // If the first non-whitespace character is a
                            // valid number then set the precedence
                            if ((*ch >= '0') && (*ch <= '7')) {
                                //printf("Setting precedence to %c\n", *ch);
                                SetCbrPrecedence(partitionData, ch);
                            }
                            break;
                        case 'n': case 'N':
                            // This command will change the precedence of appForward
                            // Advance to the first non-whitespace character
                            ch = &command.args[2];
                            ch ++;
                            while (*ch != 0 && isspace(*ch)) {
                                ch++;
                            }
                            // If the first non-whitespace character is a
                            // valid number then set the precedence
                            if ((*ch >= '0') && (*ch <= '7')) {
                                //printf("Setting precedence to %c\n", *ch);
                                SetCbrPrecedenceAppForward(partitionData, ch);
                            }
                            break;

                        case 't': case 'T':
                            // This command will change the traffic of all
                            // Cbr applications
                            // Advance to the first non-whitespace character
                            SetCbrTrafficRate(partitionData,  &command.args[2]);
                            break;

                        case 'l': case 'L':
                            // This command will change the traffic of all
                            // Cbr applications
                            SetCbrTrafficRateTimes(partitionData,  &command.args[2]);
                            break;

                        case 'd': case 'D':

                            DeactivateNode(iface, &command.args[2]);
                            break;
                        case 'a': case 'A':
                            ActivateNode(iface, &command.args[2]);
                            break;

/*
                        case 'm': case 'M':
                            CustomSetNodePosition(partitionData, &command.args[2], nextEvent);
                            break;
*/
                        default:
                            //printf("Unknown user command\n");
                            break;
                    }//SWITCH
//END HUMAN IN THE LOOP CODE
#endif
                    break;
                }
                case GUI_UNRECOGNIZED:
                    break; // this should be an error, but what to do?
                default:
                    break;
            }//switch
        }//waiting for step
    }
}

// Called for all partitions
void GUI_EXTERNAL_Registration(EXTERNAL_Interface* iface,
    NodeInput *nodeInput)
{
    PartitionData* partitionData = iface->partition;
    GUI_InterfaceData *     data;

    data = (GUI_InterfaceData*) MEM_malloc(sizeof(GUI_InterfaceData));
    memset(data, 0, sizeof(GUI_InterfaceData));
    iface->data = data;
    data->guiExternalTime = 0;
    data->guiExternalTimeHorizon = 0;
    data->guiStepSize = GUI_DEFAULT_STEP;
    data->firstIteration = true;

    //
    // Initialize GUI, all partitons do this for setup of
    // bookeeping data structs, and a few GUI specific globals that are
    // accessed by partition_private. Note that only p0 will actually
    // send a GUI_INIT message to the gui.
    //
    GUI_Initialize (partitionData,
                    data,
                    nodeInput,
                    partitionData->terrainData.coordinateSystemType,
                    partitionData->terrainData.origin,
                    partitionData->terrainData.dimensions,
                    partitionData->maxSimClock);
#if 0
    if (!GUI_isConnected ())
    {
        //return;
    }
#endif
    if (GUI_isAnimateOrInteractive ())
    {
#ifdef D_LISTENING_ENABLED
        // Add listener for new levels if dynamic hierarchy is being used
        if (iface->partition->dynamicHierarchy.IsEnabled())
        {
            iface->partition->dynamicHierarchy.AddObjectListener(
                new GuiDynamicObjectCallback(iface, GUI_DYNAMIC_AddObject));
            iface->partition->dynamicHierarchy.AddObjectPermissionsListener(
                new GuiDynamicObjectCallback(iface, GUI_DYNAMIC_ObjectPermissions));
            iface->partition->dynamicHierarchy.AddLinkListener(
                new GuiDynamicObjectCallback(iface, GUI_DYNAMIC_AddLink));
            iface->partition->dynamicHierarchy.AddRemoveListener(
                new GuiDynamicObjectCallback(iface, GUI_DYNAMIC_RemoveObject));
    }
#endif

        //
        // Read and create hierarchy information (only partition 0 does this)
        //
        if (partitionData->partitionId == 0)
        {
            for (int i = 0; i < nodeInput->numLines; i++)
            {
                char* currentLine = nodeInput->values[i];

                if (strcmp(nodeInput->variableNames[i], "COMPONENT") == 0)
                {
                    int componentId = atoi(currentLine);

                    GUI_CreateHierarchy(componentId,
                                        strstr(currentLine, "{"));
                }
            }
        }

#ifndef USE_MPI
        if (partitionData->partitionId != 0) {
            // We are parallel and we are shared memory, so we are
            // done performing init steps.
            // In shared memory, only partition 0 creates an external
            // interface for waiting for the gui commands, while
            // the gui socket will be shared by all partitions to send
            // commands back to the UI, and the exteranl itnerface/step
            // controls will be all handled by partition 0.
            return;
        }
#endif
        if (GUI_isConnected ())
        {
            // Our partition has a connection for the control socket
            // Register Receive function - this reads the commands sent
            // from the gui into the running simulation.
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_RECEIVE,
                (EXTERNAL_Function) GUI_EXTERNAL_ReceiveCommands);
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_TIME,
                (EXTERNAL_Function) GUI_EXTERNAL_QueryExternalTime);
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_SIMULATION_HORIZON,
                (EXTERNAL_Function) GUI_EXTERNAL_SimulationHorizon);
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_FINALIZE,
                (EXTERNAL_Function) GUI_EXTERNAL_Finalize);
        }
    }
}

