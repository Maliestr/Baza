#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <windows.h>
#include <string>
#include <map>
#include <vector>

class Parameters
{
public:
    int id = 0;

    enum Citizen { Russia, Ukraine, Poland, Bulgaria, Brazil };
    enum TypeOlympiad { summer, winter };
    enum CountryEvent { USSR, China, USA, Canada };
    enum Discipline { a, b, c, d, e, f, g, i, h };

    std::string FIO;
    int DateBirth[3];  // год, месяц, день
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
    int disciplineInt;  // Изменено: теперь int вместо string
    std::string fio;
};

class Workbase
{
public:
    Workbase();
    int count() const;
    int append(Parameters& record);
    void remove(unsigned int id);
    bool update(const Parameters& record);
    bool record(unsigned int id, Parameters& result) const;
    bool load();
    bool save();
    void clear();
    bool isModified() const;
    std::vector<BrowserItem> getBrowserItems() const;

private:
    std::map<int, Parameters> Db;
    unsigned int m_nextId = 1;
    bool m_modified = false;

    static std::string disciplineToString(Parameters::Discipline p);
};

#endif