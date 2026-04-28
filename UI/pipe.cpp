#include "pipe.h"
#include <QDebug>
#include <sstream>
#include <QTimer>

Pipe::Pipe(QObject *parent)
    : QObject(parent)
    , channel(INVALID_HANDLE_VALUE)
    , isConnected(false)
    , dataVersion(-1)
{
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, &QTimer::timeout, this, &Pipe::connectPipe);

    // Таймер для периодической проверки версии данных
    updateTimer = new QTimer(this);
    updateTimer->setInterval(2000); // Проверяем каждые 2 секунды
    connect(updateTimer, &QTimer::timeout, this, &Pipe::checkForUpdates);
}

Pipe::~Pipe()
{
    if (reconnectTimer) reconnectTimer->stop();
    if (updateTimer) updateTimer->stop();
    if (channel != INVALID_HANDLE_VALUE) CloseHandle(channel);
}

void Pipe::startServer()
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

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

    // Запускаем таймер обновления после подключения
    updateTimer->start();

    emit connected();
}

void Pipe::checkForUpdates()
{
    if (!isConnected || channel == INVALID_HANDLE_VALUE) return;

    std::string response = sendCommand("GET_VERSION");

    if (response.substr(0, 8) == "VERSION|")
    {
        int newVersion = std::stoi(response.substr(8));

        if (newVersion != dataVersion)
        {
            qDebug() << "Data version changed from" << dataVersion << "to" << newVersion;
            dataVersion = newVersion;
            emit dataChanged();
        }
    }
    else if (response.find("ERROR") == std::string::npos)
    {
        // Сервер старой версии, все равно обновляем
        emit dataChanged();
    }
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
        updateTimer->stop();
        connectPipe();
        return "ERROR|Write failed";
    }

    if (bytesWritten != cmd.length())
    {
        qDebug() << "WriteFile: only" << bytesWritten << "of" << cmd.length() << "bytes written";
        isConnected = false;
        updateTimer->stop();
        connectPipe();
        return "ERROR|Incomplete write";
    }

    char buffer[65536];
    DWORD bytesRead;
    if (!ReadFile(channel, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0)
    {
        DWORD error = GetLastError();
        qDebug() << "Read failed:" << error;
        isConnected = false;
        updateTimer->stop();
        connectPipe();
        return "ERROR|Read failed";
    }

    buffer[bytesRead] = '\0';
    std::string response(buffer);

    // Обновляем версию данных из ответа, если есть
    size_t vPos = response.find("|V");
    if (vPos != std::string::npos)
    {
        try {
            dataVersion = std::stoi(response.substr(vPos + 2));
        } catch (...) {}
    }

    return response;
}

void Pipe::updateDataVersion(int version)
{
    dataVersion = version;
}
