#include <windows.h>
#include "parameters.h"
#include <QDebug>
#include <QString>
#include <QDate>
#include <QFile>

Parameters::Parameters() //Логика данных
    : FIO(""),
      DateBirth(1850, 1, 1),
      citizen(Russia),
      type(summer),
      YearEvent(1896),
      country(USSR),
      discipline(a),
      Place(1),
      Record(false)
{}

int Parameters::lastId = 0;

bool Parameters::operator<(const Parameters& other) const
{
       if (FIO != other.FIO)
            return FIO < other.FIO;

       if (DateBirth != other.DateBirth)
            return DateBirth < other.DateBirth;

       if (Place != other.Place)
            return Place < other.Place;

       if (YearEvent != other.YearEvent)
            return YearEvent < other.YearEvent;

       if (type != other.type)
            return type < other.type;

       return discipline < other.discipline;
}

Workbase::Workbase() : m_nextId(1), m_modified(false) {}

int Workbase::count() const
{
    return Db.size();
}

int Workbase::append(Parameters &record)
{
    record.id = m_nextId++;
    Db.insert(record.id, record);
    m_modified = true;

    return record.id;
}

bool Workbase::update(const Parameters &record)
{
    if (Db.contains(record.id)) {
        Db[record.id] = record;
        m_modified = true;
        return true;
    }
    return false;
}

void Workbase::remove(unsigned int id) //Находим и удаляем запись
{
    Db.remove(id);
    m_modified = true;
}

bool Workbase::record(unsigned int id, Parameters &result) const
{
    if (Db.contains(id)) {
        result = Db.value(id);
        return true;
    }
    return false;
}

const QVector<BrowserItem> Workbase::records() const
{
    QVector<BrowserItem> result;

    QList<Parameters> sortedList = Db.values();

    std::sort(sortedList.begin(), sortedList.end(), [](const Parameters &a, const Parameters &b) {
        return a < b;
    });

    for (const Parameters &record : sortedList)
    {
        BrowserItem item;
        item.id = record.id;
        item.YearEvent = record.YearEvent;
        item.discipline = disciplineToString(record.discipline);
        item.fio = record.FIO;
        result.append(item);
    }
    return result;
}

bool Workbase::save(QString filename) const
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly))
        return false;

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << m_nextId << "\n";

    for (const Parameters &record : Db.values())
        out << record;
    file.close();
    return true;
}

//bool Workbase::load(QString filename) //Функция загрузки
//{
//    QFile file(filename);
//    if(!file.open(QIODevice::ReadOnly)) //Не открылся только для чтения
//        return false;

//    QTextStream in(&file);
//    in.setCodec("UTF-8");

//    clear();

//    in >> m_nextId;
//    in.skipWhiteSpace();

//    bool loads = true;
//    while (!in.atEnd()) {
//        Parameters record;
//        in >> record; //выводим данные (или строки) из потока в record
//        if (in.status() == QTextStream::Ok)
//            Db.insert(record.id, record); //Добавляем данные базу Db
//        else
//            loads = false;
//    }
//    file.close();
//    m_modified = false;
//    return true;
//}

bool Workbase::load()
{
    HANDLE file = CreateFile(L"../Baza/okak.txt", GENERIC_READ, 0, NULL,
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
    buffer[fileSize] = '\0'; //+1
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
        ptr++; // Пропускаем "

        // Читаем ФИО
        char fioBuf[256] = {0};
        int i = 0;
        while (ptr < end && *ptr != '"' && i < 255)
            fioBuf[i++] = *ptr++;
        record.FIO = QString::fromUtf8(fioBuf, i);
        if (ptr < end && *ptr == '"') ptr++;

        // Пропускаем до даты
        while (ptr < end && (*ptr < '0' || *ptr > '9')) ptr++;

        // Читаем дату (10 символов: ГГГГ-ММ-ДД)
        if (ptr + 10 <= end)
        {
            char dateBuf[11] = {0};
            memcpy(dateBuf, ptr, 10);
            record.DateBirth = QDate::fromString(QString::fromUtf8(dateBuf), "yyyy-MM-dd");
            ptr += 10;
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

        Db.insert(record.id, record);
    }

    delete[] buffer;
    m_modified = false;
    return true;
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

QString Workbase::disciplineToString(Parameters::Discipline p)
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
    }
    return "";
}

QTextStream &operator<<(QTextStream &out, const Parameters &record)
{
    out << record.id << " "
        << "\"" << record.FIO << "\"" << " "
        << record.DateBirth.toString("yyyy-MM-dd") << " "
        << static_cast<int>(record.citizen) << " "
        << static_cast<int>(record.type) << " "
        << record.YearEvent << " "
        << static_cast<int>(record.country) << " "
        << static_cast<int>(record.discipline) << " "
        << record.Place << " "
        << (record.Record ? 1 : 0) << "\n";
    return out;
}

QTextStream &operator>>(QTextStream &in, Parameters &record)
{
    int citizen, type, country, discipline, recordFlag;
    QString dateStr;
    QString fio;

    in >> record.id;
    in.skipWhiteSpace();

    if (in.read(1) == '\"') {
        QChar ch;
        while (!in.atEnd()) {
            ch = in.read(1).at(0);
            if (ch == '\"') break;
            fio += ch;
        }
    } else {
        in.seek(in.pos() - 1);
        in >> fio;
    }
    record.FIO = fio.trimmed();

    in >> dateStr
       >> citizen
       >> type
       >> record.YearEvent
       >> country
       >> discipline
       >> record.Place
       >> recordFlag;

    record.DateBirth = QDate::fromString(dateStr, "yyyy-MM-dd");
    record.citizen = static_cast<Parameters::Citizen>(citizen);
    record.type = static_cast<Parameters::TypeOlympiad>(type);
    record.country = static_cast<Parameters::CountryEvent>(country);
    record.discipline = static_cast<Parameters::Discipline>(discipline);
    record.Record = (recordFlag != 0);

    in.skipWhiteSpace();

    return in;
}
