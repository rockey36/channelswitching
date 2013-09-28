#ifndef CHUN_RTCP_H
#define CHUN_RTCP_H
#include "chun_Global.h"
#include "chun_def.h"

int RTCP_ComposeSenderReport(RTCP_SenderReport rsr,byte **pData,int nLength);
int RTCP_ParsingSenderReport(byte *pData,int nLength,RTCP_SenderReport &rsr);
int RTCP_ComposeReceiverReport(RTCP_ReceiverReport rsr,byte **pData,int nLength);
int RTCP_ParsingReceiverReport(byte *pData,int nLength,RTCP_ReceiverReport &rsr);

int RTCP_InitializeReceiverReport(RTCP_ReceiverReport& rsr);

#endif /* CHUN_RTCP_H*/