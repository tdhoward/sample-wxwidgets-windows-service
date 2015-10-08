#include <windows.h>
#include "logs.h"

HANDLE g_serialHandle;

void OpenDebug()
{
    // Open serial port
    g_serialHandle = CreateFile(DEBUG_COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    // Do some basic settings
    DCB serialParams = { 0 };
    serialParams.DCBlength = sizeof(serialParams);

    GetCommState(g_serialHandle, &serialParams);
    serialParams.BaudRate = 9600;
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;
    SetCommState(g_serialHandle, &serialParams);

    // Set timeouts
    COMMTIMEOUTS timeout = { 0 };
    timeout.ReadIntervalTimeout = 50;
    timeout.ReadTotalTimeoutConstant = 50;
    timeout.ReadTotalTimeoutMultiplier = 50;
    timeout.WriteTotalTimeoutConstant = 50;
    timeout.WriteTotalTimeoutMultiplier = 10;

    SetCommTimeouts(g_serialHandle, &timeout);
}

void WriteDebug(const char *message)
{
    long unsigned int blah;
    WriteFile(g_serialHandle,message,strlen(message),&blah, NULL);  // send the message
    FlushFileBuffers(g_serialHandle);  // send it NOW!!
}

void CloseDebug()
{
    CloseHandle(g_serialHandle);
}
