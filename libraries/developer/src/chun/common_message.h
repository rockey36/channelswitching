#ifndef COMMON_MESSAGE_H
#define COMMON_MESSAGE_H


//////////////////////////////////////////////////////////////////////////
/// For IPC messages

struct IPC_MESSAGE {
	short	ipc_message_ID;
	short	ipc_flow_ID;
	short	ipc_qos;
	int		ipc_packet_size;
};

#define		IPC_MESSAGE_SIZE		sizeof(IPC_MESSAGE)

#define		IPC_MESSAGE_OFFSET			0x10

#define		IPC_MESSAGE_DATA_SEND		(IPC_MESSAGE_OFFSET + 1)
/*
	0                   1
	0 1 2 3 4 5 6 7 8 9 0
	+-+-+-+-+-+-+-+-+-+-+
	MSGID|FID|QOS|PACKET SIZE
*/
#define		IPC_MESSAGE_END_FLOW		(IPC_MESSAGE_OFFSET + 2)
/*
	0                   1
	0 1 2 3 4 5 6 7 8 9 0
	+-+-+-+-+-+-+-+-+-+-+
	MSGID 0 0 0 0 0 0 0 0
*/
#define		IPC_MESSAGE_RECV_DATA		(IPC_MESSAGE_OFFSET + 3)
#define		IPC_MESSAGE_END_FRAME		(IPC_MESSAGE_OFFSET + 4)
#define		IPC_MESSAGE_MAX				IPC_MESSAGE_END_FRAME

//////////////////////////////////////////////////////////////////////////
/// For Windows messages
#ifndef WIN32
#define WM_USER                         0x0400
#endif

#define		WM_USER_RATECONTROL		(WM_USER+0x100)
#define		WM_USER_RUN				(WM_USER+0x101)
#define		WM_USER_PLAY			(WM_USER+0x102)
#define		WM_REQUEST_PROCESSING	(WM_USER+0x103)
#define		WM_REQUEST_IPCRUNNING	(WM_USER+0x104)


#endif