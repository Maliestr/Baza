#include <iostream>
#include <windows.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <string>
#include <map>
#include <algorithm>
#include "parameters.h"

using namespace std;

Workbase workbase;
mutex workbaseMutex;
atomic<int> dataVersion(0);
atomic<bool> serverRunning(true);

// Безопасная функция преобразования строки в число
int safe_stoi(const string& str, int defaultValue = 0)
{
    if (str.empty()) return defaultValue;
    try {
        return stoi(str);
    }
    catch (...) {
        return defaultValue;
    }
}

// Протокол команд:
// "GET_ALL" - получить все записи
// "GET:id" - получить одну запись
// "SAVE" - сохранить файл
// "LOAD" - загрузить файл
// "GET_VERSION" - получить версию данных
// "ADD:FIO|год|мес|день|citizen|type|year|country|disc|place|record" - добавить
// "UPDATE:id|FIO|год|мес|день|citizen|type|year|country|disc|place|record" - обновить
// "DELETE:id" - удалить
// "COUNT" - количество записей
// "MODIFIED" - есть ли изменения

string serializeRecord(const Parameters& p)
{
    char buf[2048];
    snprintf(buf, sizeof(buf), "%d|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d",
        p.id, p.FIO.c_str(),
        p.DateBirth[0], p.DateBirth[1], p.DateBirth[2],
        static_cast<int>(p.citizen),
        static_cast<int>(p.type),
        p.YearEvent,
        static_cast<int>(p.country),
        static_cast<int>(p.discipline),
        p.Place,
        p.Record ? 1 : 0);
    return string(buf);
}

string serializeBrowserItem(const BrowserItem& item)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%d|%d|%d|%s",
        item.id, item.YearEvent, item.disciplineInt, item.fio.c_str());
    return string(buf);
}

// Разбиение строки на токены по разделителю
vector<string> split(const string& str, char delimiter)
{
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

string handleCommand(const string& cmd)
{
    // GET_VERSION - получение версии данных
    if (cmd == "GET_VERSION")
    {
        return "VERSION|" + to_string(dataVersion.load());
    }
    // GET_ALL - получить все записи для списка
    else if (cmd == "GET_ALL")
    {
        lock_guard<mutex> lock(workbaseMutex);
        auto items = workbase.getBrowserItems();
        string result = "ITEMS|" + to_string(items.size()) + "|V" + to_string(dataVersion.load());
        for (const auto& item : items)
            result += "|" + serializeBrowserItem(item);
        return result;
    }
    // GET:id - получить полную запись по id
    else if (cmd.substr(0, 4) == "GET:")
    {
        string idStr = cmd.substr(4);
        int id = safe_stoi(idStr, -1);
        if (id == -1) return "ERROR|Invalid ID";

        lock_guard<mutex> lock(workbaseMutex);
        Parameters p;
        if (workbase.record(id, p))
            return "RECORD|" + serializeRecord(p);
        return "ERROR|Record not found";
    }
    // SAVE - сохранить данные в файл
    else if (cmd == "SAVE")
    {
        lock_guard<mutex> lock(workbaseMutex);
        if (workbase.save())
            return "OK|Saved";
        return "ERROR|Save failed";
    }
    // LOAD - загрузить данные из файла
    else if (cmd == "LOAD")
    {
        lock_guard<mutex> lock(workbaseMutex);
        if (workbase.load())
        {
            dataVersion++;
            return "OK|Loaded|V" + to_string(dataVersion.load());
        }
        return "ERROR|Load failed";
    }
    // COUNT - количество записей
    else if (cmd == "COUNT")
    {
        lock_guard<mutex> lock(workbaseMutex);
        return "COUNT|" + to_string(workbase.count());
    }
    // MODIFIED - есть ли несохраненные изменения
    else if (cmd == "MODIFIED")
    {
        lock_guard<mutex> lock(workbaseMutex);
        return "MODIFIED|" + to_string(workbase.isModified() ? 1 : 0);
    }
    // ADD: - добавление новой записи
    else if (cmd.substr(0, 4) == "ADD:")
    {
        string data = cmd.substr(4);
        vector<string> tokens = split(data, '|');

        // Должно быть 11 полей (FIO + 10 чисел)
        // Если полей меньше, дополняем значениями по умолчанию
        while (tokens.size() < 11)
            tokens.push_back("");

        Parameters p;
        p.FIO = tokens[0];
        p.DateBirth[0] = safe_stoi(tokens[1], 1850);
        p.DateBirth[1] = safe_stoi(tokens[2], 1);
        p.DateBirth[2] = safe_stoi(tokens[3], 1);
        p.citizen = static_cast<Parameters::Citizen>(safe_stoi(tokens[4], 0));
        p.type = static_cast<Parameters::TypeOlympiad>(safe_stoi(tokens[5], 0));
        p.YearEvent = safe_stoi(tokens[6], 1896);
        p.country = static_cast<Parameters::CountryEvent>(safe_stoi(tokens[7], 0));
        p.discipline = static_cast<Parameters::Discipline>(safe_stoi(tokens[8], 0));
        p.Place = safe_stoi(tokens[9], 1);
        p.Record = safe_stoi(tokens[10], 0) != 0;

        lock_guard<mutex> lock(workbaseMutex);
        int newId = workbase.append(p);
        dataVersion++;
        return "ADDED|" + to_string(newId) + "|V" + to_string(dataVersion.load());
    }
    // UPDATE: - обновление существующей записи
    else if (cmd.substr(0, 7) == "UPDATE:")
    {
        string data = cmd.substr(7);
        vector<string> tokens = split(data, '|');

        // Должно быть 12 полей (id + FIO + 10 чисел)
        if (tokens.size() < 12)
            return "ERROR|Invalid UPDATE format";

        Parameters p;
        p.id = safe_stoi(tokens[0], -1);
        if (p.id == -1) return "ERROR|Invalid ID";

        p.FIO = tokens[1];
        p.DateBirth[0] = safe_stoi(tokens[2], 1850);
        p.DateBirth[1] = safe_stoi(tokens[3], 1);
        p.DateBirth[2] = safe_stoi(tokens[4], 1);
        p.citizen = static_cast<Parameters::Citizen>(safe_stoi(tokens[5], 0));
        p.type = static_cast<Parameters::TypeOlympiad>(safe_stoi(tokens[6], 0));
        p.YearEvent = safe_stoi(tokens[7], 1896);
        p.country = static_cast<Parameters::CountryEvent>(safe_stoi(tokens[8], 0));
        p.discipline = static_cast<Parameters::Discipline>(safe_stoi(tokens[9], 0));
        p.Place = safe_stoi(tokens[10], 1);
        p.Record = safe_stoi(tokens[11], 0) != 0;

        lock_guard<mutex> lock(workbaseMutex);
        if (workbase.update(p))
        {
            dataVersion++;
            return "UPDATED|" + to_string(p.id) + "|V" + to_string(dataVersion.load());
        }
        return "ERROR|Update failed - record not found";
    }
    // DELETE: - удаление записи
    else if (cmd.substr(0, 7) == "DELETE:")
    {
        string idStr = cmd.substr(7);
        int id = safe_stoi(idStr, -1);
        if (id == -1) return "ERROR|Invalid ID";

        lock_guard<mutex> lock(workbaseMutex);
        workbase.remove(id);
        dataVersion++;
        return "DELETED|" + to_string(id) + "|V" + to_string(dataVersion.load());
    }

    return "ERROR|Unknown command: " + cmd.substr(0, 50);
}

// Функция обработки клиента в отдельном потоке
void handleClient(HANDLE clientPipe, DWORD threadId)
{
    char buffer[65536];
    DWORD bytesRead;
    DWORD bytesWritten;

    cout << "[Thread " << threadId << "] Starting client handler" << endl;

    while (true)
    {
        // Чтение команды от клиента
        memset(buffer, 0, sizeof(buffer));
        if (!ReadFile(clientPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0)
        {
            DWORD err = GetLastError();
            if (err != ERROR_BROKEN_PIPE && err != ERROR_NO_DATA)
            {
                cerr << "[Thread " << threadId << "] ReadFile error: " << err << endl;
            }
            break;
        }

        buffer[bytesRead] = '\0';
        string command(buffer);

        // Удаляем лишние символы новой строки
        while (!command.empty() && (command.back() == '\n' || command.back() == '\r' || command.back() == ' '))
            command.pop_back();

        if (command.empty()) continue;

        cout << "[Thread " << threadId << "] Received: " << command.substr(0, 100)
            << (command.length() > 100 ? "..." : "") << endl;

        // Обработка команды
        string response = handleCommand(command);

        // Отправка ответа с проверкой количества записанных байт
        if (!WriteFile(clientPipe, response.c_str(), static_cast<DWORD>(response.length()), &bytesWritten, NULL))
        {
            DWORD err = GetLastError();
            cerr << "[Thread " << threadId << "] WriteFile error: " << err << endl;
            break;
        }

        if (bytesWritten != response.length())
        {
            cerr << "[Thread " << threadId << "] Incomplete write: " << bytesWritten
                << " of " << response.length() << " bytes" << endl;
            break;
        }

        cout << "[Thread " << threadId << "] Response: " << response.substr(0, 100)
            << (response.length() > 100 ? "..." : "") << endl;
    }

    // Закрытие соединения с клиентом
    FlushFileBuffers(clientPipe);
    DisconnectNamedPipe(clientPipe);
    CloseHandle(clientPipe);
    cout << "[Thread " << threadId << "] Client disconnected" << endl;
}

// Запуск сервера
void RunServer()
{
    vector<thread> clientThreads;
    DWORD threadCounter = 0;

    cout << "Server started, waiting for connections..." << endl;

    while (serverRunning)
    {
        // Создание именованного канала
        HANDLE pipe = CreateNamedPipeA(
            "\\\\.\\pipe\\OlimpPipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,  // Неограниченное количество экземпляров для многопоточности
            65536, 65536,
            5000,
            NULL);

        if (pipe == INVALID_HANDLE_VALUE)
        {
            DWORD err = GetLastError();
            cerr << "CreateNamedPipe failed: " << err << endl;
            Sleep(1000);
            continue;
        }

        // Ожидание подключения клиента
        if (!ConnectNamedPipe(pipe, NULL))
        {
            DWORD err = GetLastError();
            if (err != ERROR_PIPE_CONNECTED)
            {
                cerr << "ConnectNamedPipe failed: " << err << endl;
                CloseHandle(pipe);
                continue;
            }
        }

        cout << "New client connected, creating handler thread #" << ++threadCounter << endl;

        // Создание отдельного потока для обработки клиента
        clientThreads.emplace_back(handleClient, pipe, threadCounter);

        // Очистка завершенных потоков (неблокирующая)
        for (auto it = clientThreads.begin(); it != clientThreads.end();)
        {
            if (it->joinable())
            {
                // Проверяем, не завершился ли поток
                HANDLE h = it->native_handle();
                if (WaitForSingleObject(h, 0) == WAIT_OBJECT_0)
                {
                    it->join();
                    it = clientThreads.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    // Ожидание завершения всех потоков при остановке сервера
    cout << "Shutting down server, waiting for " << clientThreads.size() << " clients..." << endl;
    for (auto& t : clientThreads)
    {
        if (t.joinable())
            t.join();
    }
    cout << "All client threads finished" << endl;
}

int main()
{
    // Настройка кодировки для корректного отображения русских символов
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    cout << "Database Server (Multi-threaded)" << endl << endl;

    // Загрузка данных из файла
    if (!workbase.load()) {
        cerr << "Warning: Could not load data from file" << endl;
    }
    cout << "Loaded " << workbase.count() << " records" << endl;
    cout << "Data version: " << dataVersion.load() << endl;

    // Запуск сервера
    RunServer();

    return 0;
}