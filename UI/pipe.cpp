#include "pipe.h"
#include <QDebug>
#include <sstream>

Pipe::Pipe(QObject *parent)
    : QObject(parent)
    , channel(INVALID_HANDLE_VALUE)
    , isConnected(false)
{
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, &QTimer::timeout, this, &Pipe::connectPipe);
}

Pipe::~Pipe()
{
    if (reconnectTimer) reconnectTimer->stop();
    if (channel != INVALID_HANDLE_VALUE) CloseHandle(channel);
}

void Pipe::startServer()
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    // Путь к серверу (изменяйте под свою структуру)
    WCHAR cmdLine[] = L"../server2/x64/Debug/server2.exe";

    if (!CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        DWORD error = GetLastError();
        qDebug() << "Failed to start server. Error:" << error;
        emit connectionFailed("Не удалось запустить сервер");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void Pipe::connectPipe()
{
    if (isConnected) return;

    channel = CreateFileA("\\\\.\\pipe\\OlimpPipe",
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);

    if (channel == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        qDebug() << "Connection failed. Error:" << error;
        reconnectTimer->start(2000);
        return;
    }

    qDebug() << "Connected to server!";
    isConnected = true;
    emit connected();
}

std::string Pipe::sendCommand(const std::string& cmd)
{
    if (!isConnected || channel == INVALID_HANDLE_VALUE)
        return "ERROR|Not connected";

    DWORD bytesWritten;
    if (!WriteFile(channel, cmd.c_str(), cmd.length(), &bytesWritten, NULL))
    {
        DWORD error = GetLastError();
        qDebug() << "Write failed:" << error;
        isConnected = false;
        connectPipe();
        return "ERROR|Write failed";
    }

    char buffer[65536];
    DWORD bytesRead;
    if (!ReadFile(channel, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0)
    {
        DWORD error = GetLastError();
        qDebug() << "Read failed:" << error;
        isConnected = false;
        connectPipe();
        return "ERROR|Read failed";
    }

    buffer[bytesRead] = '\0';
    return std::string(buffer);
}
