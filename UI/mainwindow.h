#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QVector>
#include <QDate>
#include <string>
#include <vector>
#include "pipe.h"
#include "parameters.h"

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

private slots:
    void onConnected();
    void onSaveData();
    void onShowCurrentRecord(int row);
    void onCreate();
    void onDelete();
    void onFill();
    void onYearEventChanged(int value);
    void onPlaceChanged(int value);
    void onOlympiadChanged(int row);
    void saveFile();
    void loadFile();
    void onDataChanged();

private:
    Ui::MainWindow *ui;
    Pipe pipe;
    QRegExpValidator valFio;
    QVector<Parameters> displayedRecords;  // Изменено с RecordData на Parameters
    int currentId = -1;
    bool loadingInProgress = false;
    bool dataModified = false;

    void setupConnections();
    void loadAllRecords();
    void updateList();
    void sendRecordToServer(const Parameters& rec, bool isUpdate);
    QString disciplineToString(int discipline) const;
};

#endif
