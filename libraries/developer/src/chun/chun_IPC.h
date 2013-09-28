#ifndef CHUN_IPC_H
#define CHUN_IPC_H

#include "chun_Global.h"
#include "chun_def.h"
#include <windows.h>

ULONG ProcIDFromWnd(HWND hwnd);
HWND ExcuteDXManager(char* strDXManagerPath,char* strGRFPath,int nJitter, int nWidth, int nHeight, char* IPCParam,char* strQoSPath);
//int CreateNamedVideoSendingPipe(AppDataVideoServer* pVideoServer);

#endif /* CHUN_RTP_H*/