#pragma once
#include <Windows.h>
#define SERVICE_NAME TEXT("AudioPot Service")
#define MSG_SYSTEM_SHUTDOWN 0x1

VOID WINAPI ServiceMain(
    DWORD argc, 
    LPTSTR* argv
);

INT WINAPI ApplicationMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR pCmdLine,
	INT nCmdShow
);