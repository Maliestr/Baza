#include <iostream>
#include <windows.h>
#include "parameters.h"

using namespace std;

Workbase workbase;

int RunServer()
{
    char* buffer = new char[2048];
    DWORD bytesRead;

    HANDLE Chanel = CreateNamedPipe(L"\\\\.\\pipe\\MyChanel", PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 1024, 1024, 5000, NULL); //Проверить

    if (Chanel == INVALID_HANDLE_VALUE)
    {
        fprintf(stdout, "CreateNamedPipe: Error %ld\n", GetLastError());
        return 0;
    }

    fprintf(stdout, "Waiting for connect...\n");

    bool fConnected = ConnectNamedPipe(Chanel, NULL);

    if (fConnected)
    {
        cout << "The client is connected.\n";

        ReadFile(Chanel, buffer, 512, &bytesRead, NULL);
        cout << "The data has been read.\n";
        for (int i = 0; i < 2048; i++) { cout << buffer[i]; }

        //loadFromFile();

        //        WriteFile();
        cout << "The data was successfully transmitted.\n";

    }
    else
    {
        DWORD error = GetLastError();
        if (error == ERROR_PIPE_CONNECTED)
        {
            std::cout << "Client already connected!" << std::endl;

            DisconnectNamedPipe(Chanel);
        }
        else
            std::cerr << "ConnectNamedPipe failed: " << error << std::endl;
    }
    return 0;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    workbase.load();
    workbase.printAllRecords();
    RunServer();
    
    return 0;
}
