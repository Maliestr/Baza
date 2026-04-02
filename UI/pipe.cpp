#include "pipe.h"
#include <windows.h>
#include <QTimer>
#include <QDebug>


Pipe::Pipe(QObject *parent)
    : QObject(parent)
    , Chanel(INVALID_HANDLE_VALUE)
    , isRunning(false)
    , isConnected(false)
{
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, &QTimer::timeout, this, &Pipe::ConnectPipe);
}

Pipe::~Pipe()
{
    if (reconnectTimer)
    {
        reconnectTimer->stop();
    }

    if (Chanel != INVALID_HANDLE_VALUE)
    {
        CloseHandle(Chanel);
        Chanel = INVALID_HANDLE_VALUE;
    }

    qDebug() << "Pipe destroyed";
}

void Pipe::StartServer()
{
    if (isRunning) return;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    WCHAR cmdLine[] = L"D:/Projects_C/C++2/OC/Lab-2/Baza/server2/x64/Debug/server2.exe";
    bool success = CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi); //CREATE_NEW_CONSOLE

    if (!success)
    {
        DWORD error = GetLastError();
        qDebug() << "Failed to start server. Error:" << error;
//        emit connectionFailed("Не удалось запустить сервер");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    isRunning = true;
}

void Pipe::ConnectPipe()
{
    if (isConnected) return;

    Chanel = CreateFile(L"\\\\.\\pipe\\MyChanel", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (Chanel == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        qDebug() << "Не удалось подключиться к серверу. Ошибка:" << error;

        // Повторная попытка через 3 секунды
        reconnectTimer->start(3000);
        return;
    }

    qDebug() << "Подключено к серверу!";
    isConnected = true;
    emit connected();
}
