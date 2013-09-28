#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <api.h>
#include <WinSock2.h>
#include "chun_IPC.h"


ULONG ProcIDFromWnd(HWND hwnd) // Get process ID by window handle
{
	ULONG idProc;
	GetWindowThreadProcessId( hwnd, &idProc );
	return idProc;
}

HWND ExcuteDXManager(char* strDXManagerPath,char* strGRFPath, int nJitter, int nWidth, int nHeight, char* IPCParam,char* strQoSPath)
{
	const unsigned int MAX_COMMAND_SIZE			= 1024;
	const unsigned int MAX_VIDEO_INFORMATION	= 56;

	STARTUPINFO					si;
	PROCESS_INFORMATION			pi;

	char strParameter[MAX_COMMAND_SIZE]		= {0,};
	char videoWidth[MAX_VIDEO_INFORMATION]	= {0,};
	char videoHeight[MAX_VIDEO_INFORMATION]	= {0,};
	char videoJitter[MAX_VIDEO_INFORMATION]	= {0,};

	sprintf(videoWidth,	"%d",nWidth);
	sprintf(videoHeight,"%d",nHeight);
	sprintf(videoJitter,"%d",nJitter);

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	// Parameter settings
	strcat(strParameter,strDXManagerPath);
	strcat(strParameter," ");
	strcat(strParameter,strGRFPath);
	strcat(strParameter," ");
	strcat(strParameter,strQoSPath);
	strcat(strParameter," ");
	strcat(strParameter,videoWidth);
	strcat(strParameter," ");
	strcat(strParameter,videoHeight);
	strcat(strParameter," ");
	strcat(strParameter,videoJitter);
	strcat(strParameter," ");
	strcat(strParameter,IPCParam);

	printf( "\n CreateProcess exe:[%s] param:[%s]  \n", strDXManagerPath, strParameter );

	// Start the child process. 
	if( !CreateProcess( NULL,   // No module name (use command line)
			strParameter,       // Command line
				NULL,           // Process handle not inheritable
				NULL,           // Thread handle not inheritable
				FALSE,          // Set handle inheritance to FALSE
				0,              // No creation flags
				NULL,           // Use parent's environment block
				NULL,           // Use parent's starting directory 
				&si,            // Pointer to STARTUPINFO structure
				&pi )           // Pointer to PROCESS_INFORMATION structure
		) 
	{
		printf( "\n CreateProcess failed (%d)\n", GetLastError() );
		return NULL;
	}
	
	int nTimeout = 0;
	const int TRIAL_MAX = 3;

	do 
	{
		Sleep(1000);
		if(++nTimeout > TRIAL_MAX)
		{
			printf( "Creation of DXManager process is failed!!");
			return NULL;
			break;
		}
	}while(NULL == OpenMutex(SYNCHRONIZE, 0, "OEFMON_MUTEX"));
	Sleep(500);	// still need time, dunno why...

	//WaitForSingleObject(pi.hProcess,INFINITE);

	HWND tempHwnd = FindWindow(NULL,NULL); // Find most window

	while( tempHwnd != NULL )  
	{  
		if( GetParent(tempHwnd) == NULL )					// Check whether it's most window or not
			if( pi.dwProcessId == ProcIDFromWnd(tempHwnd) )  {
				printf("temporary window handle: %d \n", tempHwnd);
				return tempHwnd;  
						}
		tempHwnd = GetWindow(tempHwnd, GW_HWNDNEXT); // FInd next window handle
	} 

	printf( "\n Couldn't find process id to be executed! \n" );
	return NULL; 
}

/*
int CreateNamedVideoSendingPipe(AppDataVideoServer* pVideoServer)
{
	if(pVideoServer->hNamedPIPE){
		CloseHandle(pVideoServer->hNamedPIPE);
		pVideoServer->hNamedPIPE = NULL;
	}
	pVideoServer->hNamedPIPE = CreateNamedPipe( 
		pVideoServer->strServerNamedPIPE,		// pipe name 
		PIPE_ACCESS_DUPLEX 
		| FILE_FLAG_WRITE_THROUGH,				// read/write access 
		PIPE_TYPE_BYTE 							// message type pipe 
		| PIPE_WAIT,							// blocking mode 
		1,										// max. instances  
		PIPE_BUFFSIZE,							// output buffer size 
		PIPE_BUFFSIZE,							// input buffer size 
		NMPWAIT_USE_DEFAULT_WAIT,									// client time-out 
		NULL);									// default security attribute FILE_FLAG_WRITE_THROUGH

	if (pVideoServer->hNamedPIPE == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "[error] Named PIPE is not created [%s]",pVideoServer->strServerNamedPIPE);
		return -1;
	} 
	return 0;
}
*/