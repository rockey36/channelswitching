#ifndef APP_VIDEO_H
#define APP_VIDEO_H
// Source code goes here

#include "chun/chun_Global.h"
#include "chun/chun_RTP.h"
#include "chun/chun_IPC.h"

#include <map>
#include <vector>
using namespace std;
/* Initialization */
// Client Init
AppDataVideoClient* AppVideoClientInit(Node *node, Address clientAddr, NodeAddress clientNodeAddr,Address serverAddr, NodeAddress ServerNodeAddr,
									   int nFPS,
									   int nJitter, int nWidth, int nHeight, clocktype startTime,char* strDXManagerPath,char* strGRFPath,char* strQoSPath);

// Prototype For Server Init
void AppVideoServerInit(Node *node,Address serverAddr,NodeAddress serverNodeAddr,Address remoteAddr,NodeAddress remoteNodeAddr);

// Prototype For Server Process Event
void AppLayerVideoServer(Node *node, Message *msg);
void AppLayerVideoClient(Node *node, Message *msg);

// Get init server/client data types
AppDataVideoServer* GetAppDataVideoServer(Node *node, Address remoteAddr, short sourcePort);
AppDataVideoClient* GetAppDataVideoClient(Node *node, short sourcePort);

// Get Init variable of client and server
AppDataVideoClient* GetNewAppVideoClient(Node *node, Address clientAddr, NodeAddress clientNodeAddr,Address serverAddr, NodeAddress ServerNodeAddr,
								 int nFPS,char* strDXManagerPath,char* strGRFPath,char* strQoSPath);
AppDataVideoServer* GetNewAppVideoServer(Node *node, Address serverAddr, NodeAddress serverNodeAddr,
										 Address RemoteAddr,NodeAddress RemoteNodeAddr);


/* Finalize */
// Prototype For client Finalize
void AppVideoClientFinalize(Node *node, AppInfo* appInfo);
// Prototype For Server Finalize
void AppVideoServerFinalize(Node *node, AppInfo* appInfo);

/* client operation */
bool AppVideoClientScheduleNextPkt(Node *node, AppDataVideoClient *VideoClientPtr,int	nNumberOfFlows);

void SendToServerReceivedIPCData(Node* node,						///< The pointer of node
								 DWORD cbRead,						///< The number of byte read 
								 AppDataVideoClient *VideoClientPtr,///< The instance of server setting
								 int	Precedence					///< Precedence value
								 );

bool AppVideoCreateDXManager(Node *node, AppDataVideoClient *VideoClientPtr);

/* Trace operation */
void AppVideoServerPrintStats(Node *node, AppDataVideoServer *serverPtr);
void AppVideoClientPrintStats(Node *node, AppDataVideoClient *clientPtr);

/* etc operation */
int SaveLog(char* FileName,Node* node,char* strLlog);
int SaveBandwidthLog(char* FileName,Node* node,int nBandwidth);

FILE* MAP_OpenFileP(AppDataVideoServer *serverPtr,int payload_type);
void MAP_CleanUp(AppDataVideoServer* serverPtr);

void SaveE2EnPLRInFile(Node *node, AppDataVideoServer *serverPtr);
void SaveThroughputInFile(Node *node, AppDataVideoServer *serverPtr);
bool requestNextFrameProcessing(Node *node,AppDataVideoClient *clientPtr);

/* received file */
PacketInfoAtom* FindPacketInVector(vector<PacketInfoAtom> VPIA,int SeqNum);

#endif /* APP_VIDEO_H */