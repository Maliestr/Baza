#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <QMainWindow>
#include <QLineEdit>
#include <QDate>
#include <QMessageBox>
#include <QValidator>
#include <QMap>
#include "parameters.h"

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    Workbase workbase;
//    QMap<int, Parameters> Db;
    QRegExpValidator valFio;
    int currentId = -1;
    int nextId = 1;

    bool loadingInProgress = false;
    QString currentFile;

    void setupConnections();
    void sortingList();
    QString disciplineToString(Parameters::Discipline discipline) const;
    void updateList(int index);
    int generateId();
    void updateSingleItem(int recordId);

private slots:
    void sl_Year_Event(int value);
    void sl_have_record(int value);
    void sl_choose_discipline(int value);
    void sl_saveData();
    void showCurrentRecord(int value);
    void sl_fill();
    void sl_create();
    void sl_delete();
    void save_File();
    void save_File_how();
    void open_File();
    void create_File();
};
#endif // MAINWINDOW_H
