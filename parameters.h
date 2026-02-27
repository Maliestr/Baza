#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <QString>
#include <QDate>
#include <QMetaType>
#include <QTextStream>
#include <QMap>

class Parameters
{
public:

    int id = 0;
    static int lastId; //возможно удалить

    enum Citizen {Russia, Ukraine, Poland, Bulgaria, Brazil};
    enum TypeOlympiad {summer, winter};
    enum CountryEvent {USSR, China, USA, Canada};
    enum Discipline {a, b, c, d, e, f, g, i, h};

    QString FIO;
    QDate DateBirth;
    Citizen citizen;
    TypeOlympiad type;
    int YearEvent;
    CountryEvent country;
    Discipline discipline;
    int Place;
    bool Record;
    Parameters();

    bool operator<(const Parameters& other) const;
    friend QTextStream &operator<<(QTextStream &out, const Parameters &record);
    friend QTextStream &operator>>(QTextStream &in, Parameters &record);
};

struct BrowserItem
{
    int id;
    int YearEvent;
    QString discipline;
    QString fio;
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
    const QVector<BrowserItem> records() const;
    bool save(QString filename) const;
    bool load(QString filename);
    void clear();
    bool isModified() const;
    QList<Parameters> values() const
    {
        return Db.values();
    }
//    const QVector<Record>& rawRecords();

private:
//    QVector<Parameters> rec_100;
    QMap<int, Parameters> Db;
    unsigned int m_nextId=1;
    bool m_modified=false;

    static QString disciplineToString(Parameters::Discipline p);
};

//Q_DECLARE_METATYPE(Parameters)

#endif // PARAMETERS_H
