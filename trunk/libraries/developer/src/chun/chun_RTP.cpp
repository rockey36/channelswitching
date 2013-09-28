#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <api.h>
#include <WinSock2.h>
#include "chun_RTP.h"

int RTPParsing(byte *pData,RTPpacket_t &rp)
{
	if ((pData[0] & 0xc0) != (2 << 6)){
		printf("[RTP] version is incorrect! dump = 0x%x 0x%x 0x%x 0x%x \n", pData[0], pData[1], pData[2], pData[3]);
		return 0;
	}

	/* parse RTP header */
	rp.v = (pData[0] & 0xc0) >> 6;
	rp.p = (pData[0] & 0x40) >> 5;
	rp.x = (pData[0] & 0x20) >> 4;
	rp.cc = (pData[0] & 0x0f);
	rp.m = (pData[1] & 0x80) >> 7;
	rp.pt = (pData[1] & 0x7F);
	rp.seq = ntohs (((unsigned short *) pData)[1]);
	rp.timestamp = ntohl (((unsigned int *) pData)[1]);
	rp.ssrc = ntohl (((unsigned int *) pData)[2]);

	if (rp.cc){
		for (int i = 0; i < rp.cc; i++)
		{
			//fprintf (out, " csrc: 0x%08x",ntohl (((unsigned int *) data)[3 + i]));
		}
	}
	return 0;
}
