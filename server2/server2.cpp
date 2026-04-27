#include <iostream>
#include <windows.h>
#include "parameters.h"
using namespace std;

Workbase workbase;
HANDLE pipe = INVALID_HANDLE_VALUE;

// Протокол команд:
// "GET_ALL" - получить все записи
// "GET:id" - получить одну запись
// "SAVE" - сохранить файл
// "LOAD" - загрузить файл
// "ADD:FIO|год|мес|день|citizen|type|year|country|disc|place|record" - добавить
// "UPDATE:id|FIO|год|мес|день|citizen|type|year|country|disc|place|record" - обновить
// "DELETE:id" - удалить
// "COUNT" - количество записей
// "MODIFIED" - есть ли изменения

string serializeRecord(const Parameters& p)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%d|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d",
        p.id, p.FIO.c_str(),
        p.DateBirth[0], p.DateBirth[1], p.DateBirth[2],
        p.citizen, p.type, p.YearEvent, p.country,
        p.discipline, p.Place, p.Record ? 1 : 0);
    return string(buf);
}

string serializeBrowserItem(const BrowserItem& item)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%d|%d|%d|%s",
        item.id, item.YearEvent, item.disciplineInt, item.fio.c_str());
    return string(buf);
}

string handleCommand(const string& cmd)
{
    if (cmd == "GET_ALL")
    {
        auto items = workbase.getBrowserItems();
        string result = "ITEMS|" + to_string(items.size());
        for (const auto& item : items)
            result += "|" + serializeBrowserItem(item);
        return result;
    }
    else if (cmd.substr(0, 4) == "GET:")
    {
        int id = stoi(cmd.substr(4));
        Parameters p;
        if (workbase.record(id, p))
            return "RECORD|" + serializeRecord(p);
        return "ERROR|Record not found";
    }
    else if (cmd == "SAVE")
    {
        if (workbase.save())
            return "OK|Saved";
        return "ERROR|Save failed";
    }
    else if (cmd == "LOAD")
    {
        if (workbase.load())
            return "OK|Loaded";
        return "ERROR|Load failed";
    }
    else if (cmd == "COUNT")
    {
        return "COUNT|" + to_string(workbase.count());
    }
    else if (cmd == "MODIFIED")
    {
        return "MODIFIED|" + to_string(workbase.isModified() ? 1 : 0);
    }
    else if (cmd.substr(0, 4) == "ADD:")
    {
        string data = cmd.substr(4);
        Parameters p;
        // Парсим: FIO|год|мес|день|citizen|type|year|country|disc|place|record
        size_t pos = 0;
        int field = 0;
        size_t start = 0;
        while (pos <= data.length() && field <= 10)
        {
            if (pos == data.length() || data[pos] == '|')
            {
                string token = data.substr(start, pos - start);

                switch (field)
                {
                case 0: p.FIO = token; break;
                case 1: p.DateBirth[0] = stoi(token); break;
                case 2: p.DateBirth[1] = stoi(token); break;
                case 3: p.DateBirth[2] = stoi(token); break;
                case 4: p.citizen = static_cast<Parameters::Citizen>(stoi(token)); break;
                case 5: p.type = static_cast<Parameters::TypeOlympiad>(stoi(token)); break;
                case 6: p.YearEvent = stoi(token); break;
                case 7: p.country = static_cast<Parameters::CountryEvent>(stoi(token)); break;
                case 8: p.discipline = static_cast<Parameters::Discipline>(stoi(token)); break;
                case 9: p.Place = stoi(token); break;
                case 10: p.Record = stoi(token) != 0; break;
                }
                field++;
                start = pos + 1;
            }
            pos++;
        }
        int newId = workbase.append(p);
        return "ADDED|" + to_string(newId);
    }
    else if (cmd.substr(0, 7) == "UPDATE:")
    {
        string data = cmd.substr(7);
        Parameters p;
        size_t pos = 0;
        int field = 0;
        size_t start = 0;
        while (pos <= data.length() && field <= 11)
        {
            if (pos == data.length() || data[pos] == '|')
            {
                string token = data.substr(start, pos - start);

                switch (field)
                {
                case 0: p.id = stoi(token); break;
                case 1: p.FIO = token; break;
                case 2: p.DateBirth[0] = stoi(token); break;
                case 3: p.DateBirth[1] = stoi(token); break;
                case 4: p.DateBirth[2] = stoi(token); break;
                case 5: p.citizen = static_cast<Parameters::Citizen>(stoi(token)); break;
                case 6: p.type = static_cast<Parameters::TypeOlympiad>(stoi(token)); break;
                case 7: p.YearEvent = stoi(token); break;
                case 8: p.country = static_cast<Parameters::CountryEvent>(stoi(token)); break;
                case 9: p.discipline = static_cast<Parameters::Discipline>(stoi(token)); break;
                case 10: p.Place = stoi(token); break;
                case 11: p.Record = stoi(token) != 0; break;
                }
                field++;
                start = pos + 1;
            }
            pos++;
        }
        if (workbase.update(p))
            return "UPDATED|" + to_string(p.id);
        return "ERROR|Update failed";
    }
    else if (cmd.substr(0, 7) == "DELETE:")
    {
        int id = stoi(cmd.substr(7));
        workbase.remove(id);
        return "DELETED|" + to_string(id);
    }

    return "ERROR|Unknown command";
}

void RunServer()
{
    pipe = CreateNamedPipeA("\\\\.\\pipe\\OlimpPipe",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        65536, 65536, 5000, NULL);

    if (pipe == INVALID_HANDLE_VALUE)
    {
        cerr << "CreateNamedPipe failed: " << GetLastError() << endl;
        return;
    }

    cout << "Server started, waiting for connection..." << endl;

    while (true)
    {
        if (!ConnectNamedPipe(pipe, NULL))
        {
            DWORD err = GetLastError();
            if (err != ERROR_PIPE_CONNECTED)
            {
                cerr << "ConnectNamedPipe failed: " << err << endl;
                continue;
            }
        }

        cout << "Client connected" << endl;

        char buffer[65536];
        DWORD bytesRead;

        while (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            string command(buffer);
            cout << "Received: " << command.substr(0, 50) << (command.length() > 50 ? "..." : "") << endl;

            string response = handleCommand(command);

            DWORD bytesWritten;
            WriteFile(pipe, response.c_str(), response.length(), &bytesWritten, NULL);
        }

        DisconnectNamedPipe(pipe);
        cout << "Client disconnected" << endl;
    }
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (!workbase.load()) {
        cerr << "Warning: Could not load data from file" << endl;
    }
    cout << "Loaded " << workbase.count() << " records" << endl;

    RunServer();
    return 0;
}