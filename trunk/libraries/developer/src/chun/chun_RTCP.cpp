#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chun_RTCP.h"
#include <api.h>
#include <WinSock2.h>
/**
*
* Method Name:  RTCP_ComposeSenderReport()
*
*
* Inputs:   IGetByeInfo      *piGetByeInfo
*                         - Interface used to retrieve Bye Report information
*           IRTCPConnection  *piRTCPConnection
*                                   - Interface to associated RTCP Connection
*           IRTCPSession     *piRTCPSession
*                                      - Interface to associated RTCP Session
*
* Outputs:  None
*
* Returns:  None
*
* Description: The ByeReportReceived() event method shall inform the recipient
*              of the discontinuation of an SSRC.  This may result from an
*              SSRC collision or the termination of an associated RTP
*              connection.  The RTC Manager shall also play the role of
*              central dispatcher; forwarding the event to other subscribed
*              and interested parties.
*
* Usage Notes:
*
*/

int RTCP_ComposeSenderReport(RTCP_SenderReport rsr,byte **pData,int nLength)
{
	// Not yet implemented
	return 0;
}

int RTCP_ParsingSenderReport(byte *pData,int nLength,RTCP_SenderReport &rsr)
{
	// Not yet implemented
	return 0;
}

int RTCP_InitializeReceiverReport(RTCP_ReceiverReport& rsr)
{
	memset((void*)&rsr, 0, sizeof(RTCP_ReceiverReport));

	rsr.v = 2;
	rsr.p = 0;
	rsr.RC = 0;
	return 0;
}


int RTCP_ComposeReceiverReport(RTCP_ReceiverReport rsr,byte **pData,int nLength)
{
	unsigned int	temp32 = 0;
	unsigned short	temp16 = 0;

	memset(*pData,0,sizeof(RTCP_ReceiverReport));

	*pData[0] = (byte)( ((rsr.v  & 0x03) << 6)| ((rsr.p  & 0x01) << 5)	| ((rsr.RC  & 0x01) << 4));
	*pData[1] = rsr.PT;
	*pData[2] = htons((unsigned short)(rsr.Length));

	temp32 = htonl(rsr.SSRC);
	memcpy ((void*)*pData[2], &temp16, 2);

	temp32 = htonl(rsr.SSRCOfFirstSource);
	memcpy ((void*)*pData[4], &temp32, 4); 

	temp32 = htonl(rsr.FractLost);
	memcpy ((void*)*pData[8], &temp32, 4); 

	temp32 = htonl(rsr.CumNoOfPacketLost);
	memcpy ((void*)*pData[12], &temp32, 4);

	temp32 = htonl(rsr.HighestSeqNum);
	memcpy ((void*)*pData[16], &temp32, 4);

	temp32 = htonl(rsr.InterarrivalTime);
	memcpy ((void*)*pData[20], &temp32, 4);

	temp32 = htonl(rsr.LastSenderReportTimestamp);
	memcpy ((void*)*pData[24], &temp32, 4);

	temp32 = htonl(rsr.DelaySinceLastSenderReport);
	memcpy ((void*)*pData[28], &temp32, 4);

	return 0;
}



int RTCP_ParsingReceiverReport(byte *pData,int nLength,RTCP_ReceiverReport& rsr)
{
	if(nLength < sizeof(RTCP_ReceiverReport)){
		printf("[RTCP_ParsingReceiverReport] unexpected error 1");
		return -1;
	}

	if ((pData[0] & 0xc0) != (2 << 6)){
		printf("[RTCP_ParsingSenderReport] version is incorrect! \n");
		return 0;
	}

	rsr.v = (pData[0] & 0xc0) >> 6;
	rsr.p = (pData[0] & 0x40) >> 5;
	rsr.RC = (pData[0] & 0x20) >> 4;
	rsr.PT = pData[1];
	rsr.Length				= ntohs(((unsigned short *) pData)[1]);
	rsr.SSRC				= ntohl(((unsigned int *) pData)[2]);
	rsr.SSRCOfFirstSource	= ntohl(((unsigned int *) pData)[3]);
	rsr.FractLost			= ntohl(((unsigned int *) pData)[4]);
	rsr.CumNoOfPacketLost	= ntohl(((unsigned int *) pData)[5]);
	rsr.HighestSeqNum		= ntohl(((unsigned int *) pData)[6]);
	rsr.InterarrivalTime	= ntohl(((unsigned int *) pData)[7]);
	rsr.LastSenderReportTimestamp	= ntohl(((unsigned int *) pData)[8]);
	rsr.DelaySinceLastSenderReport	= ntohl(((unsigned int *) pData)[9]);

	return 1;
}