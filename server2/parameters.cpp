#include <windows.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include "parameters.h"

// Конструктор Parameters
Parameters::Parameters()
    : FIO(""),
    DateBirth{ 1850, 1, 1 },
    citizen(Russia),
    type(summer),
    YearEvent(1896),
    country(USSR),
    discipline(a),
    Place(1),
    Record(false)
{}

// Оператор сравнения (нужен для сортировки)
bool Parameters::operator<(const Parameters& other) const
{
    if (FIO != other.FIO)
        return FIO < other.FIO;

    // Сравнение дат
    for (int i = 0; i < 3; ++i) {
        if (DateBirth[i] != other.DateBirth[i])
            return DateBirth[i] < other.DateBirth[i];
    }

    if (Place != other.Place)
        return Place < other.Place;

    if (YearEvent != other.YearEvent)
        return YearEvent < other.YearEvent;

    if (type != other.type)
        return type < other.type;

    return discipline < other.discipline;
}

// Конструктор Workbase
Workbase::Workbase() : m_nextId(1), m_modified(false) {}

int Workbase::count() const
{
    return Db.size();
}

int Workbase::append(Parameters& record)
{
    record.id = m_nextId++;
    Db[record.id] = record;  // std::map способ
    m_modified = true;
    return record.id;
}

bool Workbase::update(const Parameters& record)
{
    auto it = Db.find(record.id);
    if (it != Db.end()) {
        Db[record.id] = record;
        m_modified = true;
        return true;
    }
    return false;
}

void Workbase::remove(unsigned int id)
{
    Db.erase(id);
    m_modified = true;
}

bool Workbase::record(unsigned int id, Parameters& result) const
{
    auto it = Db.find(id);
    if (it != Db.end()) {
        result = it->second;
        return true;
    }
    return false;
}

bool Workbase::load()
{
    HANDLE file = CreateFile(L"../UI/okak.txt", GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    DWORD fileSize = GetFileSize(file, NULL);
    if (fileSize == INVALID_FILE_SIZE) { CloseHandle(file); return false; }

    char* buffer = new char[fileSize + 1];
    DWORD bytesRead;
    if (!ReadFile(file, buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize)
    {
        delete[] buffer;
        CloseHandle(file);
        return false;
    }
    buffer[fileSize] = '\0';
    CloseHandle(file);

    clear();

    char* ptr = buffer;
    char* end = buffer + fileSize;

    // Читаем m_nextId
    while (ptr < end && (*ptr < '0' || *ptr > '9')) ptr++;
    m_nextId = 0;
    while (ptr < end && *ptr >= '0' && *ptr <= '9')
        m_nextId = m_nextId * 10 + (*ptr++ - '0');

    // Читаем записи
    while (ptr < end)
    {
        Parameters record;

        // Пропускаем нецифровые символы до id
        while (ptr < end && (*ptr < '0' || *ptr > '9')) ptr++;
        if (ptr >= end) break;

        // Читаем id
        record.id = 0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9')
            record.id = record.id * 10 + (*ptr++ - '0');

        // Ищем открывающую кавычку
        while (ptr < end && *ptr != '"') ptr++;
        if (ptr >= end) break;
        ptr++;

        // Читаем ФИО
        char fioBuf[256] = { 0 };
        int i = 0;
        while (ptr < end && *ptr != '"' && i < 255)
            fioBuf[i++] = *ptr++;
        record.FIO = std::string(fioBuf, i);  // std::string вместо QString
        if (ptr < end && *ptr == '"') ptr++;

        // Пропускаем до даты
        while (ptr < end && (*ptr < '0' || *ptr > '9')) ptr++;

        // Читаем дату (10 символов: ГГГГ-ММ-ДД)
        if (ptr + 10 <= end)
        {
            // Парсим дату вручную
            int year = 0, month = 0, day = 0;
            for (int j = 0; j < 4; j++) year = year * 10 + (*ptr++ - '0');
            ptr++; // пропускаем '-'
            for (int j = 0; j < 2; j++) month = month * 10 + (*ptr++ - '0');
            ptr++; // пропускаем '-'
            for (int j = 0; j < 2; j++) day = day * 10 + (*ptr++ - '0');

            record.DateBirth[0] = year;
            record.DateBirth[1] = month;
            record.DateBirth[2] = day;
        }

        // Читаем остальные 7 чисел
        int vals[7];
        for (int& val : vals)
        {
            while (ptr < end && (*ptr < '0' || *ptr > '9')) ptr++;
            val = 0;
            while (ptr < end && *ptr >= '0' && *ptr <= '9')
                val = val * 10 + (*ptr++ - '0');
        }

        record.citizen = static_cast<Parameters::Citizen>(vals[0]);
        record.type = static_cast<Parameters::TypeOlympiad>(vals[1]);
        record.YearEvent = vals[2];
        record.country = static_cast<Parameters::CountryEvent>(vals[3]);
        record.discipline = static_cast<Parameters::Discipline>(vals[4]);
        record.Place = vals[5];
        record.Record = vals[6] != 0;

        Db[record.id] = record;  // std::map вставка
    }

    //delete[] buffer;
    m_modified = false;
    return true;
}

bool Workbase::save()
{
    HANDLE file = CreateFile(L"../UI/okak.txt", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    // Собираем записи в вектор для сортировки
    std::vector<Parameters> sortedList;
    for (const auto& pair : Db) {
        sortedList.push_back(pair.second);
    }
    std::sort(sortedList.begin(), sortedList.end());

    // Формируем буфер (используем std::string вместо QByteArray)
    std::string buffer;

    // Добавляем m_nextId
    buffer += std::to_string(m_nextId);
    buffer += '\n';

    // Добавляем каждую запись
    for (const Parameters& record : sortedList)
    {
        // ID
        buffer += std::to_string(record.id);
        buffer += ' ';

        // ФИО в кавычках
        buffer += '"';
        buffer += record.FIO;
        buffer += '"';
        buffer += ' ';

        // Дата
        buffer += std::to_string(record.DateBirth[0]) + "-" +
            std::to_string(record.DateBirth[1]) + "-" +
            std::to_string(record.DateBirth[2]);
        buffer += ' ';

        // Все поля
        buffer += std::to_string(static_cast<int>(record.citizen));
        buffer += ' ';
        buffer += std::to_string(static_cast<int>(record.type));
        buffer += ' ';
        buffer += std::to_string(record.YearEvent);
        buffer += ' ';
        buffer += std::to_string(static_cast<int>(record.country));
        buffer += ' ';
        buffer += std::to_string(static_cast<int>(record.discipline));
        buffer += ' ';
        buffer += std::to_string(record.Place);
        buffer += ' ';
        buffer += std::to_string(record.Record ? 1 : 0);
        buffer += '\n';
    }

    // Записываем в файл
    DWORD bytesWritten;
    BOOL writeResult = WriteFile(file, buffer.c_str(), buffer.size(), &bytesWritten, NULL);

    CloseHandle(file);

    if (bytesWritten == buffer.size())
    {
        m_modified = false;
        return true;
    }

    return false;
}

void Workbase::clear()
{
    Db.clear();
    m_nextId = 1;
    m_modified = false;
}

bool Workbase::isModified() const
{
    return m_modified;
}

std::string Workbase::disciplineToString(Parameters::Discipline p)
{
    switch (p)
    {
    case Parameters::a: return "бег на 100 м";
    case Parameters::b: return "бег с барьерами на 110 м";
    case Parameters::c: return "прыжки в высоту";
    case Parameters::d: return "прыжки с шестом";
    case Parameters::e: return "метание диска";
    case Parameters::f: return "шорт-трек 1 км";
    case Parameters::g: return "бег на коньках 5000 м";
    case Parameters::i: return "бег на лыжах 40 км";
    case Parameters::h: return "биатлон, масс-старт, 40 км";
    default: return "";
    }
}

void Workbase::printAllRecords() const
{
    std::vector<Parameters> sortedList;
    for (const auto& pair : Db) {
        sortedList.push_back(pair.second);
    }
    std::sort(sortedList.begin(), sortedList.end());

    for (const Parameters& rec : sortedList) {
        std::cout << "=================================" << std::endl;
        std::cout << "ID: " << rec.id << std::endl;
        std::cout << "ФИО: " << rec.FIO << std::endl;
        std::cout << "Дата рождения: "
            << rec.DateBirth[0] << "-"
            << rec.DateBirth[1] << "-"
            << rec.DateBirth[2] << std::endl;
        std::cout << "Гражданство: " << rec.citizen << std::endl;
        std::cout << "Тип олимпиады: " << rec.type << std::endl;
        std::cout << "Год события: " << rec.YearEvent << std::endl;
        std::cout << "Страна: " << rec.country << std::endl;
        std::cout << "Дисциплина: " << disciplineToString(rec.discipline) << std::endl;
        std::cout << "Место: " << rec.Place << std::endl;
        std::cout << "Рекорд: " << (rec.Record ? "Да" : "Нет") << std::endl;
    }
}