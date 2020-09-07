#pragma once
#define SERVICE_NAME TEXT("AudioPot Service")

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