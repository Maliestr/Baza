#include <windows.h>
#include <string>
#include <map>

class Parameters
{
public:

    int id = 0;
    //static int lastId; //возможно удалить

    enum Citizen {Russia, Ukraine, Poland, Bulgaria, Brazil};
    enum TypeOlympiad {summer, winter};
    enum CountryEvent {USSR, China, USA, Canada};
    enum Discipline {a, b, c, d, e, f, g, i, h};

    std::string FIO;
    int DateBirth[3];
    Citizen citizen;
    TypeOlympiad type;
    int YearEvent;
    CountryEvent country;
    Discipline discipline;
    int Place;
    bool Record;
    Parameters();

    bool operator<(const Parameters& other) const;
};

struct BrowserItem
{
    int id;
    int YearEvent;
    std::string discipline;
    std::string fio;
};

class Workbase
{
public:
    Workbase();
    int count() const;
    int append(Parameters &record);
    void remove(unsigned int id);
    bool update(const Parameters &record);
    bool record(unsigned int id, Parameters &record) const;
    bool save();
    bool load();
    void clear();
    bool isModified() const;
    //QList<Parameters> values() const
    //{
    //    return Db.values();
    //}
    void printAllRecords() const;
    bool parseBuffer(const char* buffer);

private:
    std::map<int, Parameters> Db;
    unsigned int m_nextId=1;
    bool m_modified=false;

    static std::string disciplineToString(Parameters::Discipline p);
};