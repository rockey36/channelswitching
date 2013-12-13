#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include "app_Video.h"

#include "api.h"
#include "app_util.h"
#include "partition.h"
#include "chun/pGNUPlot.h"
#include "chun/common_message.h"

extern unsigned int WbestBandwidth;	/// Variable which contains previous calculated network bandwidth

#define REG_KEY_QUALNETCONFILTER				"Software\\OEFMON\\QulnetConnFilter"
#define REG_KEY_QUALNETCONFILTER_NUMBER_OF_PIN	"NumberOfPins"
#define REG_VALUE_DEFAULT						1
#define GNUPLOT_PATH							"C:\\utils\\Gnuplot\\binary\\wgnuplot.exe"

UINT RegReadInt( HKEY hKey, LPCTSTR lpKey,LPCTSTR lpValue,INT nDefault);

UINT getNumberOfFlows() {
	return RegReadInt(HKEY_CURRENT_USER,REG_KEY_QUALNETCONFILTER,
		REG_KEY_QUALNETCONFILTER_NUMBER_OF_PIN,REG_VALUE_DEFAULT);
}

/// @brief Getting app video client pointer
/// @return AppDataVideoClient 
AppDataVideoClient * GetAppDataVideoClient(Node *node,		///< Node
										   short sourcePort	///< source port
										   )
{
	if(node == NULL) {
			ERROR_ReportError("GetAppDataVideoClient error");
	}

	AppInfo *appList = node->appData.appPtr;
	AppDataVideoClient *videoClient;

	for (; appList != NULL; appList = appList->appNext)
	{
		if (appList->appType == APP_VIDEO_CLIENT)
		{

			videoClient = (AppDataVideoClient *) appList->appDetail;

			if (videoClient->sourcePort == sourcePort)
			{
				return videoClient;
			}
		}
	}

	return NULL;
}

/// @brief Getting app video server pointer
/// @return AppDataVideoServer 
AppDataVideoServer* GetAppDataVideoServer(Node *node,			///< Node
										  Address remoteAddr,	///< remote address
										  short sourcePort		///< source port
										  )
{
	AppInfo *appList = node->appData.appPtr;
	AppDataVideoServer *videoServer;

	for (; appList != NULL; appList = appList->appNext)
	{
		if (appList->appType == APP_VIDEO_SERVER)
		{
			videoServer = (AppDataVideoServer *) appList->appDetail;

			//if ((videoServer->sourcePort == sourcePort) && IO_CheckIsSameAddress(videoServer->remoteAddr, remoteAddr))
			{
				return videoServer;
			}
		}
	}
	return NULL;
}

/// @brief this function is called when new video traffic is created watch out also applications.cpp
/// @return AppDataVideoServer server pointer
AppDataVideoServer* GetNewAppVideoServer(Node *node,						///< Node pointer
										 Address serverAddr,				///< server address
										 NodeAddress serverNodeAddr,		///< server node address (1,2,3,4....)
										 Address RemoteAddr,				///< Remote address
										 NodeAddress RemoteNodeAddr			///< Remote node address (1,2,3,4....)
										 )
{
	AppDataVideoServer	*VideoServerPtr;
	//VideoServerPtr = (AppDataVideoServer*) MEM_malloc(sizeof(AppDataVideoServer));
	VideoServerPtr = new AppDataVideoServer;

	VideoServerPtr->sessionStart	 = getSimTime(node);
	VideoServerPtr->sessionLastReceived = getSimTime(node);
	VideoServerPtr->sessionIsClosed	 = FALSE;
	VideoServerPtr->localAddr		 = serverAddr;
	VideoServerPtr->localNodeAddr	 = serverNodeAddr;

	memcpy(&(VideoServerPtr->remoteAddr), &RemoteAddr, sizeof(Address));
	VideoServerPtr->remoteNodeAddr	= RemoteNodeAddr;

	VideoServerPtr->sourcePort		 = APP_VIDEO_SERVER;

	VideoServerPtr->nJitterBufferSize	= 150;

	sprintf(VideoServerPtr->strReceiverLogFile,"rd.log");

	return VideoServerPtr;
}

/// @brief 
/// @return AppDataVideoServer server pointer
AppDataVideoClient* GetNewAppVideoClient(Node *node,					///< Node pointer
										 Address clientAddr,			///< client address
										 NodeAddress clientNodeAddr,	///< client node address
										 Address serverAddr,			///< server address
										 NodeAddress ServerNodeAddr,	///< server node address
										 int nFPS,						///< Frame rate
										 int nJitter,
										 int nWidth,
										 int nHeight,
										 char* strDXManagerPath,		///< excution file path of DXManager
										 char* strGRFPath,				///< file path of GRF
										 char* strQoSPath				///< file path of QoS parameter
										 )
{
	//create new instance of video client
	AppDataVideoClient	*VideoClientPtr = new AppDataVideoClient;

	// we need to keep node address and remote address, copy it to pointer
	memcpy(&(VideoClientPtr->localAddr), &clientAddr, sizeof(Address));
	VideoClientPtr->localNodeAddr	= clientNodeAddr;
	memcpy(&(VideoClientPtr->remoteAddr), &serverAddr, sizeof(Address));
	VideoClientPtr->remoteNodeAddr	= ServerNodeAddr;

	//sprintf(VideoClientPtr->strNamedPIPE,"%d to %d",GetIPv4Address(clientAddr),GetIPv4Address(serverAddr));
	// we set unique ID, temporarily we use "test1" but it should be automatically changed

	sprintf(VideoClientPtr->strNamedPIPE,"test1");
	sprintf(VideoClientPtr->strNamedPIPEFull,"\\\\.\\pipe\\%s",VideoClientPtr->strNamedPIPE);
	sprintf(VideoClientPtr->strNamedPIPECmd	,"\\\\.\\pipe\\%s%s",VideoClientPtr->strNamedPIPE,"cmd");

	VideoClientPtr->sourcePort		= APP_VIDEO_CLIENT;
	VideoClientPtr->nFPS			= nFPS;
	VideoClientPtr->nJitterSize		= nJitter;
	VideoClientPtr->nVIdeoHeight	= nHeight;
	VideoClientPtr->nVideoWidth		= nWidth;

	VideoClientPtr->sve		= STATE_INACTIVE;
	VideoClientPtr->nMTU	= MTU_80211_SIZE;
	VideoClientPtr->pBuffer = (char*)MEM_malloc(sizeof(char) * VideoClientPtr->nMTU);
	memset(VideoClientPtr->pBuffer,0,sizeof(byte) * VideoClientPtr->nMTU);
	VideoClientPtr->TOS		= APP_DEFAULT_TOS;

	VideoClientPtr->hNamedPIPE		 =	INVALID_HANDLE_VALUE;
	VideoClientPtr->hNamedPIPECmd	 =	INVALID_HANDLE_VALUE;

	strcpy(VideoClientPtr->strSenderLogFile,"sl_01.log");
	strcpy(VideoClientPtr->strReceiverLogFile,"rl_01.log");
	strcpy(VideoClientPtr->strDXManagerPath,strDXManagerPath);
	strcpy(VideoClientPtr->strGRFPath,strGRFPath);
	strcpy(VideoClientPtr->strQoSPath,strQoSPath);

	VideoClientPtr->sessionStart	 = getSimTime(node);

	/* Remove sender and receiver log files if they are exist */
	remove(VideoClientPtr->strSenderLogFile);
	remove(VideoClientPtr->strReceiverLogFile);

	APP_RegisterNewApp(node, APP_VIDEO_CLIENT, VideoClientPtr);

	return VideoClientPtr;
}

/// @brief this function is called when new video traffic is created watch out also applications.cpp
/// @return AppDataVideoServer server pointer
AppDataVideoClient* AppVideoClientInit(Node *node,					///< Node pointer
									   Address clientAddr,			///< client address
									   NodeAddress clientNodeAddr,	///< client node address
									   Address serverAddr, 			///< server address
									   NodeAddress ServerNodeAddr,	///< server node address
									   int nFPS,					///< frame rate
									   int nJitter,					///< Jitter
									   int nWidth,					///< Width
									   int nHeight,					///< Height
									   clocktype startTime,			///< start time
									   char* strDXManagerPath,		///< path string of DXManager
									   char* strGRFPath,			///< path string of GRF file
									   char* strQoSPath				///< path string of QOS file
									   )
{
	AppTimer			*timer;
	AppDataVideoClient	*VideoClientPtr;
	char clientStr[MAX_STRING_LENGTH];
	char serverStr[MAX_STRING_LENGTH];
	IO_ConvertIpAddressToString(GetIPv4Address(clientAddr),
                                clientStr);
	IO_ConvertIpAddressToString(GetIPv4Address(serverAddr),
                                serverStr);

	printf("Video Service Initializing at Node %d \n",node->nodeId);
	printf("Video Init parameters client=[%s] server =[%s] DX=[%s], GRF=[%s], QOS=[%s] \n",
		clientStr,serverStr,strDXManagerPath,strGRFPath,strQoSPath);

	VideoClientPtr = GetNewAppVideoClient(node, clientAddr, clientNodeAddr,
		serverAddr, ServerNodeAddr,nFPS,nJitter,nWidth,nHeight, strDXManagerPath,strGRFPath, strQoSPath);

	if(VideoClientPtr == NULL) {
			ERROR_Assert(FALSE, "GetNewAppVideoClient error!!");
	}

	VideoClientPtr->nNumberOfFlows = RegReadInt(HKEY_CURRENT_USER,REG_KEY_QUALNETCONFILTER,
		REG_KEY_QUALNETCONFILTER_NUMBER_OF_PIN,REG_VALUE_DEFAULT);

	for(int i=0;i < VideoClientPtr->nNumberOfFlows;i++) {
		std::vector<PacketInfoAtom> flow;
		VideoClientPtr->SentPacketInfo.push_back(flow);
	}

	// If DXManager is not exist
	// Execute DXManager
	if(!AppVideoCreateDXManager(node, VideoClientPtr))
	{
		// If an error occur
		// terminate Exata
		ERROR_Assert(FALSE, "Executing DXManager is failed!!!");
	}


	// setting flow information
	Message* newMsg = MESSAGE_Alloc(node,APP_LAYER,APP_VIDEO_CLIENT,MSG_APP_TimerExpired);
	MESSAGE_InfoAlloc(node, newMsg, sizeof(AppTimer));
	timer = (AppTimer *)MESSAGE_ReturnInfo(newMsg);
	timer->sourcePort = APP_VIDEO_CLIENT;
	timer->type = APP_TIMER_SEND_PKT;

	MESSAGE_Send(node,newMsg,startTime);

	return VideoClientPtr;
}

// int CreateNamedVideoSendingPipe(AppDataVideoServer* pVideoServer)
// {
// 	if(pVideoServer->hNamedPIPE){
// 		CloseHandle(pVideoServer->hNamedPIPE);
// 		pVideoServer->hNamedPIPE = NULL;
// 	}
// 	pVideoServer->hNamedPIPE = CreateNamedPipe( 
// 		pVideoServer->strServerNamedPIPE,		// pipe name 
// 		PIPE_ACCESS_DUPLEX 
// 		| FILE_FLAG_WRITE_THROUGH,				// read/write access d
// 		PIPE_TYPE_BYTE 							// message type pipe 
// 		| PIPE_WAIT,							// blocking mode 
// 		1,										// max. instances  
// 		PIPE_BUFFSIZE,							// output buffer size 
// 		PIPE_BUFFSIZE,							// input buffer size 
// 		NMPWAIT_USE_DEFAULT_WAIT,									// client time-out 
// 		NULL);									// default security attribute FILE_FLAG_WRITE_THROUGH
// 
// 	if (pVideoServer->hNamedPIPE == INVALID_HANDLE_VALUE) {
// 		fprintf(stderr, "[error] Named PIPE is not created [%s]",pVideoServer->strServerNamedPIPE);
// 		return -1;
// 	} 
// 	return 0;
// }

/// @brief this function is called when new video traffic is created watch out also applications.cpp
/// @return void
void AppVideoServerInit(Node *node,Address serverAddr,NodeAddress serverNodeAddr,Address remoteAddr,NodeAddress remoteNodeAddr)
{
	printf("Video server init node : %d\n",node->nodeId);
	AppDataVideoServer	*VideoServerPtr;
	VideoServerPtr = GetNewAppVideoServer(node,serverAddr,serverNodeAddr,remoteAddr,remoteNodeAddr);

	// Add variables to hierarchy
	std::string path;
	D_Hierarchy *h = &node->partitionData->dynamicHierarchy;

	if (h->CreateApplicationPath(node,"VideoServer",VideoServerPtr->sourcePort,"numBytesRecvd",path)){
		h->AddObject(path,new D_Int64Obj(&VideoServerPtr->numBytesRecvd));
	}
	sprintf(VideoServerPtr->strServerNamedPIPE,"\\\\.\\pipe\\%s%s","test1","Recv");

#ifdef RECV_NAMEDPIPED_USED
	if ( ! WaitNamedPipe(VideoServerPtr->strServerNamedPIPE, 1000000)) { 
		ERROR_Assert(FALSE,"[SendVideoToDS] Could not open pipe: 10 second wait timed out\n");
	} 

	VideoServerPtr->hNamedPIPE = CreateFile( 
		VideoServerPtr->strServerNamedPIPE,   // pipe name 
		GENERIC_WRITE, 
		0,						// no sharing 
		NULL,					// default security attributes
		OPEN_EXISTING,			// opens existing pipe 
		0,						// default attributes 
		NULL);					// no template file 

	if(VideoServerPtr->hNamedPIPE == INVALID_HANDLE_VALUE) {
		ERROR_Assert(FALSE,"[SendVideoToDS] Could not open pipe [%s]\n",VideoServerPtr->strServerNamedPIPE);
	}
#endif

	int nNumberOfFlow = RegReadInt(HKEY_CURRENT_USER,REG_KEY_QUALNETCONFILTER,
		REG_KEY_QUALNETCONFILTER_NUMBER_OF_PIN,REG_VALUE_DEFAULT);

	for(int i=0;i<nNumberOfFlow;i++) {
		std::vector<PacketInfoAtom> flow;
		VideoServerPtr->RecvPacketInfo.push_back(flow);
	}

	//Delete existing log files
	DeleteFile(RECEIVER_LOG_FILE);
	DeleteFile(SENDER_LOG_FILE);

	APP_RegisterNewApp(node, APP_VIDEO_SERVER, VideoServerPtr);

#ifdef RTCP_USED
	SetRTCPTimer(node);
#endif
}

/// @brief Send video traffic with QoS
/// @return HRESULT
void SendToServerReceivedIPCData(Node* node,						///< The pointer of node
								 DWORD cbRead,						///< The number of byte read 
								 AppDataVideoClient *VideoClientPtr,///< The instance of server setting
								 int	Precedence					///< Precedence value
								 )
{
	RTPpacket_t			rp;
	char				szLog[MAX_STRING_LENGTH];
	char				szPrecedenceVal[10];
	unsigned			tos;

	if(!VideoClientPtr) {
		ERROR_Assert(FALSE, "AppDataVideoClient error!");
	}

	memset(&rp,0,sizeof(RTPpacket_t));

	/// for QoS support - first 1 byte is for QoS.

	sprintf(szPrecedenceVal,"%d",Precedence);
	APP_AssignTos("PRECEDENCE",szPrecedenceVal, &tos);

#if 0
	// test
	DWORD dwWritten;
	HANDLE m_File = ::CreateFile("C:\\working\\OEFMON_SVC\\CaseStudy_2\\sending.264", GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_ARCHIVE, NULL);

	if(!WriteFile(m_File,VideoClientPtr->pBuffer+12,cbRead-12,&dwWritten,NULL))
		return;
	CloseHandle(m_File);
#endif

	// sending UDP packets to receiver
	APP_UdpSendNewDataWithPriority(
		node,
		APP_VIDEO_SERVER,
		VideoClientPtr->localAddr,
		(short) VideoClientPtr->sourcePort,
		VideoClientPtr->remoteAddr,
		//NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
		ANY_INTERFACE,
		//0,
		VideoClientPtr->pBuffer,
		cbRead,
		tos,
		0,
		TRACE_VIDEO);

	RTPParsing(((byte*)VideoClientPtr->pBuffer),rp);

	// Write log
	if(VideoClientPtr->nNumberOfFlows < rp.ssrc || rp.ssrc < 0) {
		printf("Number of flows is %d \n",VideoClientPtr->nNumberOfFlows);
		printf("SSRC is %d \n",rp.ssrc);
		ERROR_Assert(FALSE, "[SendToServerReceivedIPCData] Flow id is exceed!!");
	}

	PacketInfoAtom Atom;		// = (PacketInfoAtom*) MEM_malloc(sizeof(PacketInfoAtom));
	Atom.flowId = rp.ssrc;
	Atom.Seq	= rp.seq;
	Atom.Size	= cbRead;
	Atom.time	= getSimTime(node);
	VideoClientPtr->SentPacketInfo[rp.ssrc].push_back(Atom);

#ifdef WRITE_SENDER_LOG
	sprintf(szLog,"flowId %d Timestamp %d Seq %d size %d",rp.ssrc,rp.timestamp,rp.seq,cbRead);

	chun_log(LOG_IPC_RECEIVE, "A packet is being sent flowId %d Timestamp %d Seq %d size %d \n",
		rp.ssrc,rp.timestamp,rp.seq,cbRead);

	SaveLog(SENDER_LOG_FILE,node,szLog);
#endif
}

/// @brief Execute DXmanger
/// @return bool
bool AppVideoCreateDXManager(Node *node,							///< The pointer of node
							 AppDataVideoClient *VideoClientPtr		///< The pointer of video client
							 )
{
	if(!VideoClientPtr) {
			ERROR_ReportWarning( "AppDataVideoClient error!");
	}
	//printf("Create Named PIPE [%s]\n",VideoClientPtr->strNamedPIPEFull);


	VideoClientPtr->hNamedPIPE = CreateNamedPipe( 
		VideoClientPtr->strNamedPIPEFull,		// pipe name 
		PIPE_ACCESS_DUPLEX 
		| FILE_FLAG_WRITE_THROUGH,				// read/write access 
		PIPE_TYPE_BYTE|							// message type pipe 
		PIPE_WAIT,								// blocking mode 
		1,										// max. instances  
		PIPE_BUFFSIZE,							// output buffer size 
		PIPE_BUFFSIZE,							// input buffer size 
		CLIENT_TIMEOUT,							// client time-out 
		NULL);									// default security attribute FILE_FLAG_WRITE_THROUGH

	VideoClientPtr->hNamedPIPECmd = CreateNamedPipe( 
		VideoClientPtr->strNamedPIPECmd,        // pipe name 
		PIPE_ACCESS_DUPLEX,						// read/write access 
		PIPE_TYPE_MESSAGE |						// message type pipe 
		PIPE_READMODE_MESSAGE |					// message-read mode 
		PIPE_WAIT,								// blocking mode 
		1,										// max. instances  
		PIPE_BUFFSIZE,							// output buffer size 
		PIPE_BUFFSIZE,						    // input buffer size 
		CLIENT_TIMEOUT,							// client time-out 
		NULL);									// default security attribute 

	if ( VideoClientPtr->hNamedPIPE == INVALID_HANDLE_VALUE || VideoClientPtr->hNamedPIPE == NULL ||
		 VideoClientPtr->hNamedPIPECmd == INVALID_HANDLE_VALUE || VideoClientPtr->hNamedPIPECmd == NULL) 
	{
		ERROR_Assert(FALSE, "[AppVideoCreateDXManager] Creating Named PIPE has been failed!");
		return false;
	}
	else {
		printf( "Named PIPE is created [%s,%s]",VideoClientPtr->strNamedPIPEFull, VideoClientPtr->strNamedPIPECmd);
	}

	//execute DX Manager
	
	VideoClientPtr->hDXConnector = ExcuteDXManager(
		VideoClientPtr->strDXManagerPath,
		VideoClientPtr->strGRFPath,
		VideoClientPtr->nJitterSize,
		VideoClientPtr->nVideoWidth,
		VideoClientPtr->nVIdeoHeight,
		VideoClientPtr->strNamedPIPE,
		VideoClientPtr->strQoSPath);

	if(VideoClientPtr->hDXConnector == NULL){
		ERROR_Assert(FALSE,"Directshow Connector is failed!!!");
		return false;
	}

	PostMessage((HWND) VideoClientPtr->hDXConnector, WM_REQUEST_IPCRUNNING, 0, 0);

	printf("Waiting for a client, Named pipe handle : 0x%x address :%s\n",VideoClientPtr->hNamedPIPE,VideoClientPtr->strNamedPIPEFull);

	if(!ConnectNamedPipe(VideoClientPtr->hNamedPIPE, NULL))	// Blocked I/O operation
	{
		printf("ConnectNamedPipe failed\n");

		FlushFileBuffers(VideoClientPtr->hNamedPIPE);
		DisconnectNamedPipe(VideoClientPtr->hNamedPIPE);
		printf("ConnectNamedPipe is failed!!! error : 0x%x \n",GetLastError());
		ERROR_Assert(FALSE, "failed!");
	}
	
	printf("Waiting for a client, Named pipe handle : 0x%x address :%s\n",VideoClientPtr->hNamedPIPECmd,VideoClientPtr->strNamedPIPECmd);

	if(!ConnectNamedPipe(VideoClientPtr->hNamedPIPECmd, NULL))	// Blocked I/O operation
	{
		FlushFileBuffers(VideoClientPtr->hNamedPIPECmd);
		DisconnectNamedPipe(VideoClientPtr->hNamedPIPECmd);
		printf("ConnectNamedPipe is failed!!! error : 0x%x \n",GetLastError());
		ERROR_Assert(FALSE, "failed!");

	}

	printf( "ConnectNamedPipe [%s]\n",VideoClientPtr->strNamedPIPEFull);
	printf( "ConnectNamedPipe [%s]\n",VideoClientPtr->strNamedPIPECmd);

	return true;
}

/// @brief this function is called at the time next packet should be sent
/// @return bool
bool AppVideoClientScheduleNextPkt(Node *node,							///< The pointer of node
								   AppDataVideoClient *VideoClientPtr,	///< The pointer of video client
								   int	nNumberOfFlows					///< Number of flows
								   )
{
	DWORD				dwMode = 0,cbRead = 0,cbWrite = 0;
	const int			MsgSize	= 3;
	const int			timeoutLimit = 5;

	int					timeout = 0;
	int					FragCount = 0;

	BOOL				bCommandEnd = FALSE;
	BOOL				fSuccess	= FALSE;
	BYTE				Msg[MsgSize];
	IPC_MESSAGE			IPCMessage;
	
	chun_log(LOG_IPC_RECEIVE, "start AppVideoClientScheduleNextPkt \n");

	if(VideoClientPtr == NULL) {
		printf("VideoCleintPtr error!!!!!\n");
		return false;
	}

	// Read next IPC message
	while (bCommandEnd == FALSE)
	{
		chun_log(LOG_IPC_RECEIVE, "Waiting.... \n");

		fSuccess = ReadFile( 
			VideoClientPtr->hNamedPIPECmd,			// pipe handle 
			&IPCMessage,							// buffer to receive reply 
			IPC_MESSAGE_SIZE,						// size of buffer, 1 is for QoS Value
			&cbRead,								// number of bytes read 
			NULL);

		if(fSuccess == FALSE) {
			ERROR_Assert(FALSE,"[AppVideoClientScheduleNextPkt] Reading data failed!");
		}

		if(cbRead != IPC_MESSAGE_SIZE) {
			ERROR_Assert(FALSE,"[AppVideoClientScheduleNextPkt] Wrong IPC message format!");
		}

		//printf("read IPC message ipc_qos=%d,ipc_packet_size=%d \n",IPCMessage.ipc_qos,IPCMessage.ipc_packet_size);

		switch(IPCMessage.ipc_message_ID) {
			case IPC_MESSAGE_DATA_SEND:
				{
					chun_log(LOG_IPC_RECEIVE, "IPC_MESSAGE_DATA_SEND \n");

					memset(VideoClientPtr->pBuffer,0,sizeof(byte) * VideoClientPtr->nMTU);

					fSuccess = ReadFile(
						VideoClientPtr->hNamedPIPE,			// pipe handle 
						VideoClientPtr->pBuffer,			// buffer to receive reply 
						IPCMessage.ipc_packet_size,			// size of buffer
						&cbRead,							// number of bytes read 
						NULL);

					if(fSuccess == FALSE) {
						ERROR_Assert(FALSE,"[AppVideoClientScheduleNextPkt] Reading data failed!");
					}

					//printf("IPC_MESSAGE_DATA_SEND dump = 0x%x 0x%x 0x%x 0x%x \n", VideoClientPtr->pBuffer[0], VideoClientPtr->pBuffer[1], VideoClientPtr->pBuffer[2], VideoClientPtr->pBuffer[3]);


					FragCount++;
#if 0
					if(WbestBandwidth > 1000 || nNumberOfFlows == 0) {
						SendToServerReceivedIPCData(node,cbRead,VideoClientPtr,IPCMessage.ipc_qos);
					}
#else
					SendToServerReceivedIPCData(node,cbRead,VideoClientPtr,IPCMessage.ipc_qos);
#endif
				}
				break;
			case IPC_MESSAGE_END_FRAME:
				//printf("IPC_MESSAGE_END_FRAME \n");
				{
					chun_log(LOG_IPC_RECEIVE, "IPC_MESSAGE_END_FRAME \n");
					bCommandEnd = TRUE;
				}
				break;
			case IPC_MESSAGE_END_FLOW:
				{
					chun_log(LOG_IPC_RECEIVE, "IPC_MESSAGE_END_FLOW \n");

					if (--VideoClientPtr->nNumberOfFlows == 0) {
						printf("BYE Message!\n");
						bCommandEnd = TRUE;
					}
				}
				break;
			default:
				ERROR_Assert(FALSE,"[AppVideoClientScheduleNextPkt] Wrong IPC message type!");
		}
		/*

		// read next packet
		while (  )					// not overlapped 
		{

		if(fSuccess == TRUE && cbRead > 0) {

		printf("Message size is [%d]\n", cbRead);

		if(cbRead == CMD_LEN) // Command Mode
		{
		if(strncmp(VideoClientPtr->pBuffer+1,"END",3 ) == 0) // END
		{
		printf("END Message!\n");
		return true;
		}
		else if(strncmp(VideoClientPtr->pBuffer+1,"BYE",3 ) == 0) // END
		{
		return false;
		}
		}
		*/

		chun_log(LOG_IPC_RECEIVE, "end AppVideoClientScheduleNextPkt \n");

	}
	return true;
	//SendMessage(VideoClientPtr->hDXConnector,WM_USER_RATECONTROL,(WPARAM)30,(LPARAM)0);
}

void SendRTCPPacket(Node *node)
{
#ifdef	RTCP_USED
	RTCP_ReceiverReport rsr;
	AppDataVideoServer *serverPtr = GetAppDataVideoServer(node, , );

	if(!serverPtr){
		printf("[AppLayerVideoServer] serverPointer is incorrect!!!\n");
		return;
	}

	RTCP_InitializeReceiverReport(rsr);

	rsr.Length				= sizeof(RTCP_ReceiverReport);
	rsr.SSRC				= 0; // test
	rsr.SSRCOfFirstSource	= 0; // test
	rsr.FractLost			= ntohl(((unsigned int *) pData)[4]);
	rsr.CumNoOfPacketLost	= ntohl(((unsigned int *) pData)[5]);
	rsr.HighestSeqNum		= ntohl(((unsigned int *) pData)[6]);
	rsr.InterarrivalTime	= ntohl(((unsigned int *) pData)[7]);
	rsr.LastSenderReportTimestamp	= ntohl(((unsigned int *) pData)[8]);
	rsr.DelaySinceLastSenderReport	= ntohl(((unsigned int *) pData)[9]);


	APP_UdpSendNewDataWithPriority(
		node,
		APP_VIDEO_SERVER,
		VideoClientPtr->localAddr,
		(short) VideoClientPtr->sourcePort,
		VideoClientPtr->remoteAddr,
		//NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
		ANY_INTERFACE,
		//0,
		VideoClientPtr->pBuffer,
		cbRead,
		VideoClientPtr->TOS,
		0,
		TRACE_CBR);
#endif
}

void SetRTCPTimer(Node *node)
{
#ifdef RTCP_USED
	Message*			newRTCPMsg;
	AppTimer			*timer = NULL;

	newRTCPMsg = MESSAGE_Alloc(node,APP_LAYER,APP_VIDEO_SERVER,MSG_APP_TimerExpired);
	MESSAGE_InfoAlloc(node, newRTCPMsg, sizeof(AppTimer));
	timer = (AppTimer *)MESSAGE_ReturnInfo(newRTCPMsg);
	timer->sourcePort = APP_VIDEO_CLIENT;
	timer->type = APP_TIMER_RTCP_SEND;
	MESSAGE_Send(node,newRTCPMsg,SECOND * RTCP_TIMER_INTERVAL);
#endif
}

// Prototype For Client Process Event
void AppLayerVideoClient(Node *node, Message *msg)
{
	char				szLog[MAX_STRING_LENGTH];
	char				buf[MAX_STRING_LENGTH];
	char				error[MAX_STRING_LENGTH];

	AppTimer			*timer = NULL;
	AppDataVideoClient	*VideoClientPtr = NULL;
	HWND				hWnd;
	bool				bNoError = true;
	bool				bMsgSend = true;

	switch (msg->eventType)
	{
	// for the purpose of network feedback 
	case MSG_APP_FromTransport:
		{
			printf("Not yet implemented");
			break;
		}
	// In this event, get a RTP packets from Qualnet connector filter and send it to receiver node
	case MSG_APP_TimerExpired:	
		{
			timer = (AppTimer *) MESSAGE_ReturnInfo(msg);
			//printf("MSG_APP_TimerExpired!\n");

			if(timer == NULL)
				assert(FALSE);

			switch (timer->type)
			{
			case APP_TIMER_SEND_PKT:
				{
					VideoClientPtr = GetAppDataVideoClient(node,APP_VIDEO_CLIENT);

					if (VideoClientPtr == NULL) {
						ERROR_Assert(FALSE,"APP_TIMER_SEND_PKT error!");
					}
	
					if (VideoClientPtr->sve == STATE_INACTIVE) // play DirectShow
					{
						VideoClientPtr->sve				 = STATE_ACTIVE;
						SendMessage((HWND) VideoClientPtr->hDXConnector, WM_USER_PLAY, 0, 0);
						VideoClientPtr->sessionStart	 = getSimTime(node);
					}

					// Let QualnetConnector send next frame to next connected filter
					// However, if jitter size is not over it ignore


					//printf("Wait [%d] ", VideoClientPtr->nNumberOfFlows);
					for( int i=0;i < VideoClientPtr->nNumberOfFlows; i++) {
						//printf("----AppVideoClientScheduleNextPkt [%d]",VideoClientPtr->nNumberOfFlows);
						if(false == (bNoError = AppVideoClientScheduleNextPkt(node, VideoClientPtr, i))) {
							printf("error AppVideoClientScheduleNextPkt! \n");
							bMsgSend = false;
							break;
						}
					}

					if(getSimTime(node) >
						(VideoClientPtr->sessionStart + ( VideoClientPtr->nJitterSize * MILLI_SECOND ))) {
						//printf("active\n");
						//printf("Ask processing %llu %llu \n",getSimTime(node), VideoClientPtr->sessionStart + (VideoClientPtr->nJitterSize * MILLI_SECOND));

						if(false == requestNextFrameProcessing(node, VideoClientPtr)) {
							bMsgSend = false;
						}
						//printf("requestNextFrameProcessing \n");
					}
					else {
						printf("discard\n");
					}

					if(bMsgSend == true) {
						Message* newMsg;
						newMsg = MESSAGE_Alloc(node,APP_LAYER,APP_VIDEO_CLIENT,MSG_APP_TimerExpired);
						MESSAGE_InfoAlloc(node, newMsg, sizeof(AppTimer));
						timer = (AppTimer *)MESSAGE_ReturnInfo(newMsg);
						timer->sourcePort = VideoClientPtr->sourcePort;
						timer->type = APP_TIMER_SEND_PKT;

						MESSAGE_Send( node, newMsg, SECOND/VideoClientPtr->nFPS );
					}
				}

#if 0
					if(VideoClientPtr->sessionFinish > 0)
					{
						Message* newMsg;
						newMsg = MESSAGE_Alloc(node,APP_LAYER,APP_VIDEO_CLIENT,MSG_APP_TimerExpired);
						MESSAGE_InfoAlloc(node, newMsg, sizeof(AppTimer));
						timer = (AppTimer *)MESSAGE_ReturnInfo(newMsg);
						timer->sourcePort = VideoClientPtr->sourcePort;
						timer->type = APP_TIMER_SEND_PKT;
						MESSAGE_Send( node, newMsg, SECOND/VideoClientPtr->nFPS );
					}
					else
					{
						if(VideoClientPtr->nNumberOfFlows > 0) {
							for( int i=0;i < VideoClientPtr->nNumberOfFlows; i++) {

								if(false == (bNoError = AppVideoClientScheduleNextPkt(node, VideoClientPtr))) {
									break;
								}
							}

							if(bNoError == true)
							{
								Message* newMsg;
								newMsg = MESSAGE_Alloc(node,APP_LAYER,APP_VIDEO_CLIENT,MSG_APP_TimerExpired);
								MESSAGE_InfoAlloc(node, newMsg, sizeof(AppTimer));
								timer = (AppTimer *)MESSAGE_ReturnInfo(newMsg);
								timer->sourcePort = VideoClientPtr->sourcePort;
								timer->type = APP_TIMER_SEND_PKT;

								MESSAGE_Send( node, newMsg, SECOND/VideoClientPtr->nFPS );
							}
							else
							{
								VideoClientPtr->sessionFinish = getSimTime(node);
							}
						}
					}
				}
#endif
				break;
			default:
				assert(FALSE);
			}
		}
		break;
		default:
			TIME_PrintClockInSecond(getSimTime(node), buf, node);
			sprintf(error, "Video Client: at time %sS, node %d "
				"received message of unknown type"
				" %d\n", buf, node->nodeId, msg->eventType);
			ERROR_ReportError(error);
		}
	MESSAGE_Free(node, msg);
}

int SendDataToDirectShowConnector(Node *node, AppDataVideoServer* appVideoServer, char* dataBuffer, int nByte)
{
	DWORD		dwMode,cbWritten;
	BOOL		fSuccess;

	char		*p_buffer = NULL;

	if(appVideoServer == NULL) {
		ERROR_Assert(FALSE,"[SendVideoToDS] appVideoServer error!");
	}
	//printf("SendVideoToDS - NodeID[%d], NamedPIPE = [%s]\n",node->nodeId,appVideoServer->strServerNamedPIPE);

	if(appVideoServer->VideoClientAppData == NULL) {

		Node* ClientNode;
		
		ClientNode = MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash,
			appVideoServer->remoteNodeAddr);

		if(NULL == ClientNode) {
			ClientNode = MAPPING_GetNodePtrFromHash(node->partitionData->remoteNodeIdHash,
				appVideoServer->remoteNodeAddr);
		}
		if(NULL == ClientNode) {
			ERROR_ReportError("[SendDataToDirectShowConnector] Cannot find client information");
		}
		
		appVideoServer->VideoClientAppData = GetAppDataVideoClient(ClientNode, APP_VIDEO_CLIENT);

		if(appVideoServer->VideoClientAppData == NULL ) {
			ERROR_Assert(FALSE,"[SendDataToDirectShowConnector] Client application data area is null");
		}
	}

	if (appVideoServer->VideoClientAppData->hDXConnector == NULL) {
			ERROR_Assert(FALSE,"[SendDataToDirectShowConnector] DirectShow Connector is not running");
	}

	if(nByte < 1) {
		ERROR_Assert(FALSE,"[SendDataToDirectShowConnector] Data to send is not correct");
	}

	p_buffer = (char *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, nByte);

	if(p_buffer == NULL) {
		ERROR_Assert(FALSE,"[SendDataToDirectShowConnector] Buffer is null");
		return -1;
	}

	memcpy(p_buffer, dataBuffer, nByte);

	COPYDATASTRUCT tip;
	tip.dwData = IPC_MESSAGE_RECV_DATA;
	tip.cbData = nByte;
	tip.lpData = p_buffer;


	if(IsWindow( (HWND) appVideoServer->VideoClientAppData->hDXConnector )) {

		SendMessage( (HWND) appVideoServer->VideoClientAppData->hDXConnector, WM_COPYDATA, 
		(WPARAM)NULL, (LPARAM)&tip );
	}
	else {
		char errStr[MAX_STRING_LENGTH];
		sprintf(errStr, "DirectShow connector not found. Attempted window handle = %d \n", 
			(int) appVideoServer->VideoClientAppData->hDXConnector);
		ERROR_Assert(FALSE,errStr);
		return	-1;
	}

	::HeapFree(::GetProcessHeap(), 0, p_buffer);
	
	return 0;

#if 0
	fSuccess = WriteFile( 
		appVideoServer->hNamedPIPE, // pipe handle 
		dataBuffer,					// message 
		nByte,						// message length 
		&cbWritten,				    // bytes written 
		NULL);					    // not overlapped 

	if (!fSuccess) {
		ERROR_Assert(FALSE,"[SendVideoToDS] WriteFile to pipe failed\n");
		return -1;
	}
#endif

	return 0;
}

// Prototype For Server Process Event
void AppLayerVideoServer(Node *node, Message *msg)
{
	char					*dataBuffer;

	char					cType;
	int						index;
	int						size;
	int						nByte;
	RTPpacket_t				rp;
	char					szLog[MAX_STRING_LENGTH];
	char					error[MAX_STRING_LENGTH];

	switch(msg->eventType)
	{
	case MSG_APP_TimerExpired:
		{
			AppTimer			*timer = NULL;
			timer = (AppTimer *) MESSAGE_ReturnInfo(msg);
			printf("MSG_APP_TimerExpired!\n");

			if(timer == NULL)
				assert(FALSE);

			switch (timer->type)
			{
			case APP_TIMER_RTCP_SEND:
#ifdef RTCP_USED
				SetRTCPTimer(node);
				SendRTCPPacket(node):
#endif
				break;
			}
		}
		break;
	case MSG_APP_FromTransport:	// When receiving packet, check timestamp and throw away if it's too old
		{
			UdpToAppRecv *udp = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);
			dataBuffer = (char*)MESSAGE_ReturnPacket(msg);
			nByte = MESSAGE_ReturnPacketSize(msg);

			AppDataVideoServer *serverPtr = GetAppDataVideoServer(node,udp->sourceAddr,APP_VIDEO_SERVER);

			if(NULL == serverPtr) {
				ERROR_Assert(FALSE,"[AppLayerVideoServer] serverPointer is incorrect!!");
			}

			if(SendDataToDirectShowConnector(node,serverPtr,dataBuffer,nByte) != 0)
			{
				ERROR_Assert(FALSE,"[AppLayerVideoServer] SendVideoToDS error!!");
			}

			/* save packet information into packet information vector */
			PacketInfoAtom Atom;
			// trace recd pkt
			ActionData acnData;
			acnData.actionType = RECV;
			acnData.actionComment = NO_COMMENT;
			TRACE_PrintTrace(node,msg,TRACE_APPLICATION_LAYER,PACKET_IN,&acnData);

			RTPParsing((byte*)dataBuffer,rp);

			// FILE* fp = MAP_OpenFileP(serverPtr,rp.pt);
			// fwrite((byte*)dataBuffer+RTP_HDR_SIZE,nByte-RTP_HDR_SIZE,1,fp);
			// REQ: Check  timestamp and/ throw it away

			Atom.Seq	= rp.seq;
			Atom.Size	= nByte;
			Atom.flowId = rp.ssrc;
			Atom.time	= getSimTime(node);

			if( rp.ssrc < 0 || rp.ssrc > serverPtr->RecvPacketInfo.size()) {
				ERROR_Assert(FALSE,"[AppLayerVideoServer] SSRC error");
			}

			serverPtr->RecvPacketInfo[rp.ssrc].push_back(Atom);
			sprintf(szLog,"FlowId %d Timestamp %d Seq %d size %d",rp.ssrc, rp.timestamp, rp.seq, nByte);
			SaveLog(RECEIVER_LOG_FILE,node,szLog);
			break;
		}
	}
}

int SaveLog(char* FileName,Node* node,char* strLlog)
{
	char	CurTime[MAX_STRING_LENGTH];
	FILE*	LogFp;

	LogFp = fopen(FileName,"a+");

	if(LogFp == NULL){
		printf("[error] savelog()/ file is not opened \n");
		return -1;
	}

	TIME_PrintClockInSecond(getSimTime(node),CurTime);
	fprintf(LogFp,"[%s] %s\n",CurTime,strLlog);

	fclose(LogFp);

	return 1;
}

void
AppVideoServerFinalize(Node *node, AppInfo* appInfo)
{
	AppDataVideoServer *serverPtr = (AppDataVideoServer*)appInfo->appDetail;

	PacketInfoAtom*	DataAtom;

	if (node->appData.appStats == TRUE)
	{
		AppVideoServerPrintStats(node, serverPtr);
	}

#ifdef RECV_NAMEDPIPED_USED
	if(serverPtr->hNamedPIPE)
	{
		FlushFileBuffers(serverPtr->hNamedPIPE);
		DisconnectNamedPipe(serverPtr->hNamedPIPE);
		CloseHandle(serverPtr->hNamedPIPE);
	}
#endif

	serverPtr->RecvPacketInfo.clear();
	MAP_CleanUp(serverPtr);
}

void AppVideoClientFinalize(Node *node, AppInfo* appInfo)
{
	AppDataVideoClient *clientPtr = (AppDataVideoClient*)appInfo->appDetail;

	if (node->appData.appStats == TRUE)
	{
		AppVideoClientPrintStats(node, clientPtr);
	}

#ifdef RECV_NAMEDPIPED_USED
	/* close Named PIPE */
	if(clientPtr->hNamedPIPE)
	{
		CloseHandle(clientPtr->hNamedPIPE);
	}
#endif

	/* Close DXConnector */
	if(clientPtr->hDXConnector)
	{
		SendMessage((HWND) clientPtr->hDXConnector, WM_CLOSE, 0, 0);
 		if(TerminateProcess(clientPtr->hDXConnector, 0)) {
 			unsigned long nCode; //ExitDX Connector process
 			GetExitCodeProcess(clientPtr->hDXConnector, &nCode);
 		}
	}
}

bool	requestNextFrameProcessing(Node *node,AppDataVideoClient *clientPtr)
{
	/* Close DXConnector */
	if(clientPtr->hDXConnector) {

		if( SendMessage((HWND) clientPtr->hDXConnector, WM_REQUEST_PROCESSING, 0, 0) == TRUE ) {

			return true;
		}
	}

	printf("Request processing failed! \n");

	return false;
}	

void AppVideoClientPrintStats(Node *node, AppDataVideoClient *clientPtr)
{
	clocktype throughput;

	char addrStr[MAX_STRING_LENGTH];
	char startStr[MAX_STRING_LENGTH];
	char closeStr[MAX_STRING_LENGTH];
	char sessionStatusStr[MAX_STRING_LENGTH];
	char throughputStr[MAX_STRING_LENGTH];

	char buf[MAX_STRING_LENGTH];
	char buf1[MAX_STRING_LENGTH];

	unsigned long numBytesRecvd = 0;
	unsigned int  vectorSize = 0;
	int i =0;
	int NumberOfFlows = getNumberOfFlows();

	TIME_PrintClockInSecond(clientPtr->sessionStart, startStr, node);
	TIME_PrintClockInSecond(clientPtr->sessionLastSent, closeStr, node);

	if (clientPtr->sessionIsClosed == FALSE) {
		clientPtr->sessionFinish = getSimTime(node);
		sprintf(sessionStatusStr, "Not closed");
	}
	else {
		sprintf(sessionStatusStr, "Closed");
	}

	printf("clientPtr->nNumberOfFlows [%d] ",clientPtr->nNumberOfFlows);
	for(int nFlow=0;nFlow < NumberOfFlows; nFlow++) {

		/* get number of bytes */
		vectorSize = clientPtr->SentPacketInfo[nFlow].size();
		printf("VectorSize = [%d]\n",vectorSize);

		for(i=0; i < vectorSize ;i++ ){
			PacketInfoAtom *pia = &(clientPtr->SentPacketInfo[nFlow][i]);
			//printf("numBytesSent = [%d]\n",numBytesRecvd);
			numBytesRecvd += pia->Size;
		}

		sprintf(buf, "The number of packets in flow %d sent = %d", nFlow, vectorSize);
		IO_PrintStat(node,"Application","Video Client",ANY_DEST,clientPtr->sourcePort,buf);

		/* calculate throughput */
		throughput = (clocktype)((numBytesRecvd * 8.0 * SECOND)	/ (clientPtr->sessionFinish	- clientPtr->sessionStart));

		ctoa(throughput, throughputStr);

		sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
		IO_PrintStat(node,"Application","Video Client",ANY_DEST,clientPtr->sourcePort,buf);
	}
}

PacketInfoAtom* FindPacketInVector(vector<PacketInfoAtom> VPIA,int SeqNum)
{
	//should be fix..
	int vectorSize = VPIA.size();

	for(int i=0;i<vectorSize;i++)
	{
		if(VPIA[i].Seq == SeqNum)
		{
			return &VPIA[i];
		}
	}
	return NULL;
}

int SaveVectorInFile(char* FileName,vector <float> vData)
{
	char	SzLogString[MAX_STRING_LENGTH];
	FILE*	FileP;

	FileP = fopen(FileName,"w+");

	if(FileP == NULL){
		printf("[error] savelog()/ file is not opened \n");
		return -1;
	}

	for(int i=0;i<vData.size();i++)
	{
		sprintf(SzLogString,"%d, %f\n",i, vData[i]);
		fprintf(FileP,SzLogString);
	}
	fclose(FileP);

	return 0;
}

int SaveVectorInFile(char* FileName,vector <unsigned int> vData)
{
	char	SzLogString[MAX_STRING_LENGTH];
	FILE*	FileP;

	FileP = fopen(FileName,"w+");

	if(FileP == NULL){
		printf("[error] savelog()/ file is not opened \n");
		return -1;
	}

	for(int i=0;i<vData.size();i++)
	{
		sprintf(SzLogString,"%d, %d\n",i, vData[i]);
		fprintf(FileP,SzLogString);
	}
	fclose(FileP);

	return 0;
}

void SaveThroughputInFile(Node *node, AppDataVideoServer *serverPtr)
{
	char			szTemp[MAX_STRING_LENGTH];
	char			szFileToSave[MAX_STRING_LENGTH];
	unsigned int	nMAXSimSecond = 0;
	unsigned int	nThroughput  = 0;
	unsigned int	RecvPKVectorSize = 0,SentPKVectorSize = 0;
	unsigned int	nLastVectorIndex = 0;
	unsigned int	nTempTime = 0;
	unsigned int	nCurProcessTime = 0;
	vector <unsigned int>	arrThroughput;

	int NumberOfFlows = getNumberOfFlows();

	if(serverPtr == NULL) {
		ERROR_Assert(FALSE,"[SaveThroughputInFile] server pointer error!\n");
		return;
	}

	if(serverPtr->VideoClientAppData == NULL ) {
		ERROR_Assert(FALSE,"[SaveThroughputInFile] server pointer error!\n");
		return;
	}

	for(int nFlow=0;nFlow < NumberOfFlows;nFlow++)
	{
		std::vector<PacketInfoAtom>	RecvPacketInfoLocal = serverPtr->RecvPacketInfo[nFlow];
		std::vector<PacketInfoAtom>	SentPacketInfoLocal = serverPtr->VideoClientAppData->SentPacketInfo[nFlow];

		RecvPKVectorSize = RecvPacketInfoLocal.size();
		SentPKVectorSize = SentPacketInfoLocal.size();

		TIME_PrintClockInSecond(serverPtr->VideoClientAppData->sessionFinish,szTemp);
		nMAXSimSecond = (unsigned int)atof(szTemp);

		for(int i=0;i<nMAXSimSecond+1;i++)
		{
			// Increase by 1 second
			nCurProcessTime		= i;
			nThroughput			= 0;

			for(int j=nLastVectorIndex;j<RecvPKVectorSize;j++)
			{
				TIME_PrintClockInSecond(RecvPacketInfoLocal[j].time,szTemp);
				nTempTime = (unsigned int)atof(szTemp);

				if(nTempTime > nCurProcessTime)
				{
					nLastVectorIndex = j;
					break;
				}
				else if(nTempTime == nCurProcessTime)
				{
					nThroughput += RecvPacketInfoLocal[j].Size;
				}
				else
				{

				}
			}

			nThroughput = (nThroughput*8) / 1000;
			arrThroughput.push_back(nThroughput);

			sprintf(szFileToSave,THROUGHPUTFILE,nFlow);
			SaveVectorInFile(szFileToSave,arrThroughput);
		}

		arrThroughput.clear();
		nLastVectorIndex = 0;
	}
}

void SaveE2EnPLRInFile(Node *node, AppDataVideoServer *serverPtr)
{
	unsigned int			i,j;
	unsigned int			RecvPKVectorSize = 0,SentPKVectorSize = 0;
	unsigned int			nLastProcessTime = 0;
	unsigned int			nCurProcessTime = 0,nTempTime = 0;
	unsigned int			nMAXSimSecond = 0;
	unsigned int			nNumOfPacketInTime = 0,nNumOfLostPacketInTime  =0;
	clocktype				nEndToEndDelay;
	float					nCumE2EDelay =0;
	unsigned int			nLastVectorIndex = 0;
	bool					bPacketFound = FALSE;

	char					szTemp[MAX_STRING_LENGTH];

	char					szFileToSaveEndToEndDelay[MAX_STRING_LENGTH];
	char					szFileToSaveLostFile[MAX_STRING_LENGTH];
	char					szFileToSavePLRFile[MAX_STRING_LENGTH];

	char					szPlotToSaveEndToEndDelay[MAX_STRING_LENGTH];
	char					szPlotToSaveLostFile[MAX_STRING_LENGTH];
	char					szPlotToSavePLRFile[MAX_STRING_LENGTH];

	vector <unsigned int>	arrEndToEndDelay;
	vector <unsigned int>	arrLostpacketNum;
	vector <float>			arrPLR;

	int NumberOfFlows = getNumberOfFlows();

	if(serverPtr == NULL) {
		printf("[SaveE2EnPLRInFile] server pointer error!\n");
		return;
	}

	if(serverPtr->VideoClientAppData == NULL) {
		printf("[SaveE2EnPLRInFile] client pointer error!\n");
		return;
	}

	for(int nFlow=0 ;nFlow < NumberOfFlows; nFlow++) {

		std::vector<PacketInfoAtom>	RecvPacketInfoLocal = serverPtr->RecvPacketInfo[nFlow];
		std::vector<PacketInfoAtom>	SentPacketInfoLocal = serverPtr->VideoClientAppData->SentPacketInfo[nFlow];

		int RecvPKVectorSize = RecvPacketInfoLocal.size();
		int SentPKVectorSize = SentPacketInfoLocal.size();

		TIME_PrintClockInSecond(serverPtr->VideoClientAppData->sessionFinish,szTemp);
		nMAXSimSecond = (unsigned int)atof(szTemp);

		printf("start SaveNetworkMetricInFile [%d]\n",nMAXSimSecond);

		for(int i=0;i< nMAXSimSecond+1;i++)
		{
			nCurProcessTime			= i;
			nNumOfPacketInTime		= 0;
			nNumOfLostPacketInTime	= 0;
			nCumE2EDelay			= 0.0;

			for(int j=0;j<SentPKVectorSize;j++)
				//for(int j=nLastVectorIndex;j<SentPKVectorSize;j++)
			{
				PacketInfoAtom *SentPk = &(SentPacketInfoLocal[j]);
				TIME_PrintClockInSecond(SentPk->time,szTemp);
				nTempTime = (unsigned int)atof(szTemp);

				//printf("index [%d], nCurProcessTime[%d] nTempTime[%d]\n",j,nCurProcessTime,nTempTime);
#if 0
				if(nTempTime > nCurProcessTime)
				{
					nLastVectorIndex = j;
					break;
				}
				else if(nTempTime == nCurProcessTime)
#endif
					if(nTempTime == nCurProcessTime)
					{
						for (int k=0;k<RecvPKVectorSize;k++)
						{
							if(RecvPacketInfoLocal[k].Seq == SentPk->Seq)
							{
								nNumOfPacketInTime++;
								nEndToEndDelay = RecvPacketInfoLocal[k].time - SentPk->time;
								TIME_PrintClockInSecond(nEndToEndDelay,szTemp);
								//printf("%s\n",szTemp);
								TIME_PrintClockInSecond(nEndToEndDelay * 1000,szTemp);
								//printf("%s\n",szTemp);
								nCumE2EDelay = nCumE2EDelay + atof(szTemp);
								bPacketFound = TRUE;
								break;
							}
						}
						if(!bPacketFound)
						{
							nNumOfLostPacketInTime++;
						}
						bPacketFound = FALSE;
					}
					else
					{
						// unexpected...
					}
			}
			if(nNumOfPacketInTime > 0)
			{
				arrPLR.push_back((float)(nNumOfLostPacketInTime)/(float)(nNumOfLostPacketInTime + nNumOfPacketInTime));
				arrEndToEndDelay.push_back(nCumE2EDelay/nNumOfPacketInTime);
				arrLostpacketNum.push_back(nNumOfLostPacketInTime);
			}
			else
			{
				arrPLR.push_back(0.0);
				arrEndToEndDelay.push_back(0);
				arrLostpacketNum.push_back(0);
			}
		}

		// store values in files.
		sprintf(szFileToSaveEndToEndDelay,ENDTOENDLOGFILE,nFlow);
		SaveVectorInFile(szFileToSaveEndToEndDelay,arrEndToEndDelay);

		sprintf(szFileToSaveLostFile,NUMLOSTFILE,nFlow);
		SaveVectorInFile(szFileToSaveLostFile,arrLostpacketNum);

		sprintf(szFileToSavePLRFile,PLRLOGFILE,nFlow);
		SaveVectorInFile(szFileToSavePLRFile,arrPLR);

		arrEndToEndDelay.clear();
		arrLostpacketNum.clear();
		arrPLR.clear();

#ifdef GNUPLOT_USED
		CpGnuplot plotEndToEnd(GNUPLOT_PATH);
		CpGnuplot plotLostPacket(GNUPLOT_PATH);
		CpGnuplot plotPLR(GNUPLOT_PATH);

		sprintf(szPlotToSaveEndToEndDelay, "plot '%s' title \"Flow %d \" with lines",szFileToSaveEndToEndDelay,nFlow);
		sprintf(szPlotToSaveLostFile, "plot '%s' title \"Flow %d \" with lines",szFileToSaveLostFile,nFlow);
		sprintf(szPlotToSavePLRFile, "plot '%s' title \"Flow %d \" with lines",szFileToSavePLRFile,nFlow);

		printf(szPlotToSaveEndToEndDelay);
		printf(szPlotToSaveLostFile);
		printf(szPlotToSavePLRFile);

		plotEndToEnd.cmd("set xlabel \"Time (s)\"");
		plotEndToEnd.cmd("set ylabel \"End-to-end delay (ms)\"");
		plotEndToEnd.cmd(szPlotToSaveEndToEndDelay);

		plotLostPacket.cmd("set xlabel \"Time (s)\"");
		plotLostPacket.cmd("set ylabel \"The number of lost packet\"");
		plotLostPacket.cmd(szPlotToSaveLostFile);

		plotPLR.cmd("set xlabel \"Time (s)\"");
		plotPLR.cmd("set ylabel \"Packet Loss Ratio\"");
		plotPLR.cmd(szPlotToSavePLRFile);

		Sleep(10000);
#endif
	}
}



void AppVideoServerPrintStats(Node *node, AppDataVideoServer *serverPtr)
{
	clocktype throughput;
	clocktype avgJitter;

	char addrStr[MAX_STRING_LENGTH];
	char jitterStr[MAX_STRING_LENGTH];
	char clockStr[MAX_STRING_LENGTH];
	char startStr[MAX_STRING_LENGTH];
	char closeStr[MAX_STRING_LENGTH];
	char sessionStatusStr[MAX_STRING_LENGTH];
	char throughputStr[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];

	unsigned long numBytesRecvd = 0;

	Int64 nEndToEndDelay=0;
	Int64 AvrEndToEndDelay=0;

	char timeStr[MAX_STRING_LENGTH];

	unsigned int  vectorSize		= 0;
	unsigned int  SenderVectorSize	= 0;
	unsigned int  SameCount			= 0;

	int NumberOfFlows = getNumberOfFlows();

	int i =0, j=0;

	if(serverPtr == NULL) {
		printf("[SaveE2EnPLRInFile] server pointer error!\n");
		return;
	}

	if(serverPtr->VideoClientAppData == NULL) { 
		printf("[SaveE2EnPLRInFile] client pointer error!\n");
		return;
	}

	serverPtr->sessionLastReceived = getSimTime(node);
	TIME_PrintClockInSecond(serverPtr->sessionStart, startStr, node);
	TIME_PrintClockInSecond(serverPtr->sessionLastReceived, closeStr, node);

	for(int nFlow=0;nFlow<NumberOfFlows;nFlow++)
	{
		std::vector<PacketInfoAtom>	RecvPacketInfoLocal = serverPtr->RecvPacketInfo[nFlow];
		std::vector<PacketInfoAtom>	SentPacketInfoLocal = serverPtr->VideoClientAppData->SentPacketInfo[nFlow];

		int RecvPKVectorSize = RecvPacketInfoLocal.size();
		int SentPKVectorSize = SentPacketInfoLocal.size();

		numBytesRecvd = 0;
		for(i = 0 ; i < RecvPKVectorSize ;i++ ){
			PacketInfoAtom *pia = &(RecvPacketInfoLocal[i]);
			numBytesRecvd += pia->Size;
		}

		sprintf(buf, "The number of packets in flow %d received = %d", nFlow, RecvPKVectorSize);
		IO_PrintStat(node,"Application","Video Server",ANY_DEST,serverPtr->sourcePort,buf);
		printf("%s\n",buf);

		sprintf(buf, "The number of bytes in flow %d received = %d", nFlow, numBytesRecvd);
		IO_PrintStat(node,"Application","Video Server",ANY_DEST,serverPtr->sourcePort,buf);
		printf("%s\n",buf);

		throughput = (clocktype)((numBytesRecvd * 8.0 * SECOND)	/ (serverPtr->VideoClientAppData->sessionFinish	- serverPtr->sessionStart));

		ctoa(throughput, throughputStr);

		sprintf(buf, "Throughput (bits/s) in flow %d = %s", nFlow, throughputStr);
		IO_PrintStat(node,"Application","Video Server",ANY_DEST,serverPtr->sourcePort,buf);
		printf("%s\n",buf);
	}

	/* calculate loss ratio */
	/* calculate End-to-End delay(ms) and PLR(%) */
	SaveE2EnPLRInFile(node,serverPtr);
	/* calculate throughput */
	SaveThroughputInFile(node,serverPtr);

	/*
	if(SameCount > 0)
	{
		TIME_PrintClockInSecond(AvrEndToEndDelay/SameCount,timeStr);
		sprintf(buf, "Average End-to-End Delay (ms) = %d", (unsigned int)(atof(timeStr)*1000));
	}
	else
		sprintf(buf, "Average End-to-End Delay (ms) = %d", 0);

	
	IO_PrintStat(node,"Application","Video Server",ANY_DEST,serverPtr->sourcePort,buf);
	printf("%s\n",buf);

	sprintf(buf, "The number of sent packet = %d", SenderVectorSize);
	printf("%s\n",buf);


	sprintf(buf, "The number of lost packet = %d", SenderVectorSize - vectorSize);
	IO_PrintStat(node,"Application","Video Server",ANY_DEST,serverPtr->sourcePort,buf);
	printf("%s\n",buf);
	*/
}

void MAP_CleanUp(AppDataVideoServer* serverPtr)
{
	map<int, FILE*>::iterator Iter_Pos;
	for( Iter_Pos = serverPtr->PayloadTypeFilePMap.begin(); Iter_Pos != serverPtr->PayloadTypeFilePMap.end(); ++Iter_Pos)
	{
		fclose((FILE*)Iter_Pos->second);
	}
}

/* find file pointer in the MAP, if it is not exist, make a file pointer and insert it into MAP */
FILE* MAP_OpenFileP(AppDataVideoServer *serverPtr,int payload_type)
{
	if (serverPtr->PayloadTypeFilePMap.count(payload_type) == 0)
	{
		char FileName[MAX_STRING_LENGTH];
		sprintf(FileName, "%d", payload_type);

		FILE* fileP = fopen(FileName,"wb");
		pair<int, FILE*> NewItem(payload_type, fileP);

		serverPtr->PayloadTypeFilePMap.insert(NewItem);

		return fileP;
	}
	else
	{
		map<int, FILE*>::iterator k = serverPtr->PayloadTypeFilePMap.find(payload_type);
		
		if (k == serverPtr->PayloadTypeFilePMap.end()){
			assert(false);
		}

		return k->second;
	}
}

UINT RegReadInt( HKEY hKey, LPCTSTR lpKey,LPCTSTR lpValue,INT nDefault)
{
	HKEY key;
	DWORD dwDisp;
	UINT Result;
	DWORD Size;
	if( RegCreateKeyEx( hKey, lpKey, 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_READ, NULL,
		&key, &dwDisp ) != ERROR_SUCCESS )
		return 0;
	Size = sizeof( LONG );
	if( RegQueryValueEx( key, lpValue, 0, NULL, (LPBYTE)&Result, &Size ) != ERROR_SUCCESS )
		Result = nDefault;
	RegCloseKey( key );
	return Result;
}

