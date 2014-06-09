#ifndef CHUN_DEF_H
#define CHUN_DEF_H

//#include <windows.h>
//#include "chun_Global.h"
#include "api.h"
//#include "partition.h"
//#include "app_util.h"
//#include "network_ip.h"
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include <api.h>
// #include <WinSock2.h>

//
// Handle to an Object
//

// #ifdef STRICT
// typedef void *HANDLE;
// #if 0 && (_MSC_VER > 1000)
// #define DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name
// #else
// #define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name
// #endif
// #else
// typedef PVOID HANDLE;
// #define DECLARE_HANDLE(name) typedef HANDLE name
// #endif
// typedef HANDLE *PHANDLE;


//////////////////////////////////////////////////////////////////////////
// Defines
#define		PIPE_BUFFSIZE				4096
#define		RTP_HDR_SIZE				12
#define		APP_TIMER_BASE				100
#define		APP_TIMER_RTCP_SEND			(APP_TIMER_BASE+1)

#define		RTP_HDR_SIZE				12
#define		H264PAYLOADTYPE				105						 //!< RTP payload type fixed here for simplicity
#define		H264SSRC					0x12345678               //!< SSRC, chosen to simplify debugging
#define		MTU_80211_SIZE				1450


#define CLIENT_TIMEOUT		1000
#define CMD_LEN				4

#define RATE_ADAPTIVE_VIDEO_TRANSMISSION			1
#define RATE_ADAPTIVE_VIDEO_TRANSMISSION_INIT		4
#define RATE_ADAPTIVE_VIDEO_TRANSMISSION_INTERVAL	1
#define APP_TIMER_RATE_ADAPTIVE_VIDEO				(APP_TIMER_BASE+2)
//#define DATA_PARTITION_USED	1

#define RECEIVER_LOG_FILE	"ReceiverLog.log"
#define SENDER_LOG_FILE		"SenderLog.log"

#define WRITE_SENDER_LOG

// #ifndef RTCP_USED
// #define RTCP_USED			1
// #ifndef RTCP_TIMER_INTERVAL
// #define	RTCP_TIMER_INTERVAL 1
// #endif
// #endif
//////////////////////////////////////////////////////////////////////////
// Macros

#ifndef MIN
#define MIN(a,b) (((a)>(b)) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif

#define return_if_fail(expr) if (!(expr)) {printf("%s:%i- assertion"#expr "failed\n",__FILE__,__LINE__); return;}
#define return_val_if_fail(expr,ret) if (!(expr)) {printf("%s:%i- assertion" #expr "failed\n",__FILE__,__LINE__); return (ret);}

//////////////////////////////////////////////////////////////////////////
// Debugs

#ifdef CHUN_DEBUG
static inline void debug_log(const char *fmt,...)
{
	va_list args;
	va_start (args, fmt);
	printf(ORTP_DEBUG, fmt, args);
	va_end (args);
}
#else
#define debug_log(...)
#endif


//////////////////////////////////////////////////////////////////////////
// log files
#define THROUGHPUTFILE	"throughput_flow_%d.log"
#define ENDTOENDLOGFILE	"endtoend_flow_%d.log"
#define PLRLOGFILE		"plr_flow_%d.log"
#define NUMLOSTFILE		"numlost_flow_%d.log"


//////////////////////////////////////////////////////////////////////////
// Enum

///@brief - the packet type of RTCP
enum RTCP_Packet_Type {
	RTCP_SENDER_REPORT = 0,
	RTCP_RECEIVER_REPORT,
	RTCP_GOODBYE,
	RTCP_APP_DEFINED
};

///@brief - the status of video encoder
enum STATE_VIDEO_ENCODER{
	STATE_INACTIVE = 0,
	STATE_ACTIVE,
	STATE_WAIT,
};

//////////////////////////////////////////////////////////////////////////
// TYPEDEF
typedef unsigned char	byte;

//////////////////////////////////////////////////////////////////////////
// STRUCTURE

typedef struct struct_app_packet_info
{
	int					flowId;
	Int64				time;
	unsigned int		Seq;
	unsigned int		Size;
}PacketInfoAtom;

typedef struct struct_app_video_client_str
{
	void*					hDXConnector;
	Address					localAddr;
	NodeAddress				localNodeAddr;

	Address					remoteAddr;
	NodeAddress				remoteNodeAddr;

	short					sourcePort;
	int						nFPS;
	STATE_VIDEO_ENCODER		sve;
	void*					hNamedPIPE;
	void*					hNamedPIPECmd;
	unsigned int			nMTU;

	char strDXManagerPath[MAX_STRING_LENGTH];
	char strGRFPath[MAX_STRING_LENGTH];
	char strQoSPath[MAX_STRING_LENGTH];
	char strNamedPIPE[MAX_STRING_LENGTH];
	char strNamedPIPEFull[MAX_STRING_LENGTH];
	char strNamedPIPECmd[MAX_STRING_LENGTH];


	/* log files */
	char strSenderLogFile[MAX_STRING_LENGTH];
	char strReceiverLogFile[MAX_STRING_LENGTH];

	char*					pBuffer;
	unsigned int			TOS;
	clocktype				sessionStart;
	clocktype				sessionFinish;
	clocktype				sessionLastSent;
	clocktype				endTime;
	BOOL					sessionIsClosed;

	int						nJitterSize;
	int						nVideoWidth;
	int						nVIdeoHeight;
	int						nNumberOfFlows;

#if RATE_ADAPTIVE_VIDEO_TRANSMISSION
	unsigned int			m_CurBW;
#endif

	std::vector<std::vector<PacketInfoAtom>>	SentPacketInfo;
	BOOL	paused; //chanswitch magic
} AppDataVideoClient;

typedef struct struct_app_video_server_str
{
	Address					localAddr;
	NodeAddress				localNodeAddr;

	Address					remoteAddr;
	NodeAddress				remoteNodeAddr;

	short					sourcePort;

	AppDataVideoClient*		VideoClientAppData;
	char					strServerNamedPIPE[MAX_STRING_LENGTH];


	/* log files */
	/* for trace data */
	clocktype			sessionStart;
	clocktype			sessionLastReceived;
	BOOL				sessionIsClosed;
	D_Int64				numBytesRecvd;
	UInt32				numPktsRecvd;
	int					nJitterBufferSize;		// MS

	char strReceiverLogFile[MAX_STRING_LENGTH];

	std::vector<std::vector<PacketInfoAtom>>		RecvPacketInfo;
	std::map<int, FILE*>			PayloadTypeFilePMap;

} AppDataVideoServer;

///@brief - the structure of RTCP Sender Report
typedef struct RTCP_SenderReport
{
	unsigned int v;          
	unsigned int p;          
	unsigned int RC;          
	unsigned int PayloadType;         
	unsigned int Length;          

	unsigned int SSRC;         
	unsigned int NTPTimestampMSB;        						 
	unsigned int NTPTimestampLSB;  
	unsigned int RTPTimestamp;       
	unsigned int SendersPacketCount;       
	unsigned int SendersOctetCount;       
} _RTCP_SenderReport;

typedef struct RTCP_ReceiverReport
{
	unsigned int v;          
	unsigned int p;          
	unsigned int RC;          
	unsigned int PT;         
	unsigned int Length;          

	unsigned int SSRC;         
	unsigned int SSRCOfFirstSource;        						 
	unsigned int FractLost;  
	unsigned int CumNoOfPacketLost;       
	unsigned int HighestSeqNum;       
	unsigned int InterarrivalTime;  
	unsigned int LastSenderReportTimestamp;
	unsigned int DelaySinceLastSenderReport;
} _RTCP_ReceiverReport;


typedef struct RTPpacket_t
{
	unsigned int v;          //!< Version, 2 bits, MUST be 0x2
	unsigned int p;          //!< Padding bit, Padding MUST NOT be used
	unsigned int x;          //!< Extension, MUST be zero */
	unsigned int cc;         /*!< CSRC count, normally 0 in the absence
							 of RTP mixers */
	unsigned int m;          //!< Marker bit
	unsigned int pt;         //!< 7 bits, Payload Type, dynamically established
	unsigned int seq;        /*!< RTP sequence number, incremented by one for
							 each sent packet */
	unsigned int timestamp;  //!< timestamp, 27 MHz for H.264
	unsigned int ssrc;       //!< Synchronization Source, chosen randomly

	byte		offset;
	byte *       payload;    //!< the payload including payload headers
	unsigned int paylen;     //!< length of payload in bytes
	byte *       packet;     //!< complete packet including header and payload
	unsigned int packlen;    //!< length of packet, typically paylen+12

	unsigned int ident;
	int		F;				// Fragmented type
	int		VDT;
	int		pkts;

} RTPpacket;

#endif /* CHUN_DEF_H*/