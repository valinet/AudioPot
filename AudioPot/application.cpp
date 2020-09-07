#define _WIN32_DCOM
#include <Windows.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <conio.h>
#include <Dbt.h>

#define BUFFER_SIZE 6

#define APPLICATION_NAME TEXT("AudioPot")

//BOOL filterFirstMute = TRUE;

// https://stackoverflow.com/questions/50722026/how-to-get-and-set-system-volume-in-windows
float GetSystemVolume() {
    HRESULT hr;

    // -------------------------
    CoInitialize(NULL);
    IMMDeviceEnumerator* deviceEnumerator = NULL;
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),
        (LPVOID*)&deviceEnumerator
    );
    IMMDevice* defaultDevice = NULL;

    hr = deviceEnumerator->GetDefaultAudioEndpoint(
        eRender,
        eConsole,
        &defaultDevice
    );
    deviceEnumerator->Release();
    deviceEnumerator = NULL;

    IAudioEndpointVolume* endpointVolume = NULL;
    hr = defaultDevice->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_INPROC_SERVER,
        NULL,
        (LPVOID*)&endpointVolume
    );
    defaultDevice->Release();
    defaultDevice = NULL;

    float currentVolume = 0;
    //Current volume as a scalar
    hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
    endpointVolume->Release();
    CoUninitialize();

    return currentVolume;
}

void process(char* szBuffer)
{
    //printf("%s\n", szBuffer);
    // I have to use ProgMan so that the changes work when the user is
    // switched to another desktop as well - keybd_event does not work
    // when the keyboard is active on another desktop, for example
    LPARAM lParam;
    WPARAM wParam;
    if (szBuffer[0] != 0 && szBuffer[0] != 'm')
    {
        DWORD dwVal = 100 * atoi(szBuffer) / 1023;
        float dwVol = GetSystemVolume() * 100;
        int dwCnt = (dwVal - dwVol) / 2;
        if (dwCnt < 0)
        {
            lParam = APPCOMMAND_VOLUME_DOWN;
            dwCnt = 0 - dwCnt;
        }
        else
        {
            lParam = APPCOMMAND_VOLUME_UP;
        }
        wParam = (WPARAM)FindWindow(L"ProgMan", NULL);
        for (DWORD i = 0; i < dwCnt; ++i)
        {
            SendMessage(
                (HWND)wParam,
                WM_APPCOMMAND,
                wParam,
                lParam * 65536
            );
        }
    }
    else if (szBuffer[0] != 0)
    {
        //if (!filterFirstMute)
        //{
        wParam = (WPARAM)FindWindow(L"ProgMan", NULL);
        SendMessage(
            (HWND)wParam,
            WM_APPCOMMAND,
            wParam,
            APPCOMMAND_MEDIA_PLAY_PAUSE * 65536
        );
        //}
        //filterFirstMute = FALSE;
    }
}

DWORD WINAPI WorkerThread
(
    LPVOID lpParam
)
{
    HANDLE hComm;
    hComm = CreateFile(
        L"\\\\.\\COM240",
        GENERIC_READ,
        0,
        0,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        0
    );
    if (hComm == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %d\n", GetLastError());
        return NULL;
    }
    //printf("opening serial port successful.\n");

    // Retrieve and configure configuration parameters
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hComm, &dcbSerialParams))
    {
        printf("GetCommState: %d\n", GetLastError());
        CloseHandle(hComm);
        return NULL;
    }
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hComm, &dcbSerialParams))
    {
        printf("SetCommState: %d\n", GetLastError());
        CloseHandle(hComm);
        return NULL;
    }

    COMMTIMEOUTS comTimeOut = { 0 };
    comTimeOut.ReadIntervalTimeout = 0;
    comTimeOut.ReadTotalTimeoutMultiplier = 0;
    comTimeOut.ReadTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(hComm, &comTimeOut))
    {
        printf("SetCommTimeouts: %d\n", GetLastError());
        CloseHandle(hComm);
        return NULL;
    }

    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL
    );
    if (!overlapped.hEvent)
    {
        printf("CreateEvent: %d\n", GetLastError());
        CloseHandle(hComm);
        return NULL;
    }

    char szBuffer[BUFFER_SIZE];
    ZeroMemory(szBuffer, BUFFER_SIZE);
    DWORD dwPos = 0;
    while (TRUE)
    {
        // https://stackoverflow.com/questions/15136645/serialports-and-waitformultipleobjects
        // http://www.egmont.com.pl/addi-data/instrukcje/standard_driver.pdf

        BOOL bRet;
        DWORD dwBytesRead, dwRet;
        char chRd;

        bRet = ReadFile(
            hComm,
            &chRd,
            1,
            NULL,
            &overlapped);
        DWORD err = GetLastError();
        if (!bRet && err != ERROR_IO_PENDING)
        {
            printf("ReadFile: %d\n", err);
            break;
        }
        dwRet = WaitForSingleObject(
            overlapped.hEvent,
            INFINITE
        );
        if (dwRet == WAIT_OBJECT_0 + 0)
        {
            if (!GetOverlappedResult(
                hComm,
                &overlapped,
                &dwBytesRead,
                FALSE
            ))
            {
                printf("GetOverlappedResult: %d\n", GetLastError());
                break;
            }
            if (chRd == '\r')
            {
                process(szBuffer);
                ZeroMemory(szBuffer, BUFFER_SIZE);
                dwPos = 0;
            }
            else if (chRd != '\n')
            {
                szBuffer[dwPos] = chRd;
                if (dwPos < BUFFER_SIZE - 1) dwPos++;
            }
        }
        else
        {
            printf("WaitForSingleObject: %d\n", GetLastError());
            break;
        }
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(hComm);
    return NULL;
}

LRESULT CALLBACK WindowProc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_DEVICECHANGE:
    {
        switch (wParam)
        {
        case DBT_DEVICEARRIVAL:
        {
            DEV_BROADCAST_HDR* hdr = (DEV_BROADCAST_HDR*)lParam;
            if (hdr->dbch_devicetype == DBT_DEVTYP_PORT)
            {
                DEV_BROADCAST_PORT* port = (DEV_BROADCAST_PORT*)lParam;
                if (!wcscmp(port->dbcp_name, TEXT("COM240")))
                {
                    wprintf(TEXT("%s\n"), port->dbcp_name);
                    PostQuitMessage(0);
                }
            }
            break;
        }
        }
        break;
    }
    }
    return DefWindowProc(
        hwnd,
        uMsg,
        wParam,
        lParam
    );
}

INT WINAPI ApplicationMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    INT nCmdShow
)
{
#ifdef _DEBUG
    FILE* conout;
    AllocConsole();
    freopen_s(
        &conout,
        "CONOUT$",
        "w",
        stdout
    );
#else
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f, * g;
        freopen_s(
            &f,
            "CONOUT$",
            "w",
            stdout
        );
        freopen_s(
            &g,
            "CONOUT$",
            "w",
            stderr
        );
    }
#endif

    WNDCLASS wndClass = { 0 };
    wndClass.lpfnWndProc = WindowProc;
    wndClass.hInstance = hInstance;
    wndClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND);
    wndClass.lpszClassName = APPLICATION_NAME;

    HWND hWnd = CreateWindowEx(
        NULL,
        reinterpret_cast<LPCWSTR>(
            MAKEINTATOM(
                RegisterClass(&wndClass)
            )
            ),
        APPLICATION_NAME,
        NULL,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    MSG msg;
    BOOL bRet;
    while (TRUE)
    {
        //filterFirstMute = TRUE;
        // Try to communicate with audio pot
        HANDLE handle = CreateThread(
            NULL,
            0,
            WorkerThread,
            NULL,
            0,
            NULL
        );
        if (!handle)
        {
            break;
        }
        DWORD dwResult = WaitForSingleObject(
            handle,
            INFINITE
        );
        // Communication ended, check if error
        if (dwResult == WAIT_FAILED)
        {
            break;
        }
        // If not, wait until the device is plugged into the system
        while ((bRet = GetMessage(
            &msg,
            NULL,
            0,         // should work with WM_DEVICECHANGE here
            0)) != 0)  // and here, but it does not...
        {
            // An error occured
            if (bRet == -1)
            {
                break;
            }
            // Device connected, will send WM_QUIT to retry communication
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return 0;
}