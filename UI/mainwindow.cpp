#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <sstream>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , valFio(QRegExp("^([А-ЯЁA-Z]{1})([а-яёa-z]*)\\s([А-ЯЁA-Z]{1})([а-яёa-z]*)\\s([А-ЯЁA-Z]{1})([а-яёa-z]*)$"))
{
    ui->setupUi(this);
    ui->le_fio->setValidator(&valFio); //валидатор для ФИО

    // Настройка валидации и ограничений
    ui->le_fio->setMaxLength(45);
    ui->sb_year_event->setRange(1896, 2100);
    ui->sb_year_event->setSingleStep(2);
    ui->sb_place->setRange(1, 3);
    ui->de_birth->setMinimumDate(QDate(1850, 1, 1));
    ui->de_birth->setMaximumDate(QDate(1896, 1, 1).addYears(-14));

    setupConnections();

    pipe.startServer();
    QTimer::singleShot(1500, &pipe, &Pipe::connectPipe);
    connect(&pipe, &Pipe::connected, this, &MainWindow::onConnected);
    connect(&pipe, &Pipe::connectionFailed, this, [this](const QString& err) {
        QMessageBox::warning(this, "Ошибка", err);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections()
{
    // Используем старый синтаксис SIGNAL/SLOT для совместимости
    connect(ui->le_fio, SIGNAL(textChanged(QString)), this, SLOT(onSaveData()));
    connect(ui->de_birth, SIGNAL(dateChanged(QDate)), this, SLOT(onSaveData()));
    connect(ui->lw_citizen, SIGNAL(currentRowChanged(int)), this, SLOT(onSaveData()));
    connect(ui->lw_olympiad, SIGNAL(currentRowChanged(int)), this, SLOT(onSaveData()));
    connect(ui->sb_year_event, SIGNAL(valueChanged(int)), this, SLOT(onSaveData()));
    connect(ui->lw_country, SIGNAL(currentRowChanged(int)), this, SLOT(onSaveData()));
    connect(ui->lw_discipline, SIGNAL(currentRowChanged(int)), this, SLOT(onSaveData()));
    connect(ui->sb_place, SIGNAL(valueChanged(int)), this, SLOT(onSaveData()));
    connect(ui->cb_have_record, SIGNAL(clicked(bool)), this, SLOT(onSaveData()));

    connect(ui->sb_year_event, SIGNAL(valueChanged(int)), this, SLOT(onYearEventChanged(int)));
    connect(ui->sb_place, SIGNAL(valueChanged(int)), this, SLOT(onPlaceChanged(int)));
    connect(ui->lw_olympiad, SIGNAL(currentRowChanged(int)), this, SLOT(onOlympiadChanged(int)));

    connect(ui->pb_create, SIGNAL(clicked()), this, SLOT(onCreate()));
    connect(ui->pb_remove, SIGNAL(clicked()), this, SLOT(onDelete()));
    connect(ui->pb_fill, SIGNAL(clicked()), this, SLOT(onFill()));
    connect(ui->lw_browser, SIGNAL(currentRowChanged(int)), this, SLOT(onShowCurrentRecord(int)));
}

void MainWindow::onConnected()
{
    // Загружаем только список (без полных данных записей)
    std::string response = pipe.sendCommand("GET_ALL");
    if (response.substr(0, 6) != "ITEMS|")
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные: " + QString::fromStdString(response));
        return;
    }

    displayedRecords.clear();

    std::string data = response.substr(6);

    // Разбиваем всю строку по разделителю '|'
    std::vector<std::string> allTokens;
    std::stringstream ss(data);
    std::string token;
    while (std::getline(ss, token, '|'))
        allTokens.push_back(token);

    if (allTokens.empty()) return;

    // Первый токен - это количество записей
    int count = std::stoi(allTokens[0]);

    // Остальные токены - это данные записей (4 поля на запись)
    int index = 1; // начинаем со следующего после count
    for (int i = 0; i < count; i++)
    {
        // Проверяем, что у нас есть достаточно токенов для записи
        if (index + 3 >= allTokens.size()) break;

        Parameters rec;
        rec.id = std::stoi(allTokens[index++]);      // id
        rec.YearEvent = std::stoi(allTokens[index++]); // YearEvent
        rec.discipline = static_cast<Parameters::Discipline>(std::stoi(allTokens[index++])); // discipline
        rec.FIO = QString::fromStdString(allTokens[index++]); // FIO

        displayedRecords.append(rec);
    }

    updateList();

    if (displayedRecords.size() > 0)
        ui->lw_browser->setCurrentRow(0);  // 0 - заголовок

    ui->pb_fill->setVisible(true);
}

void MainWindow::updateList()
{
    qDebug() << "updateList called, records:" << displayedRecords.size();

    ui->lw_browser->clear();
    ui->lw_browser->addItem("Год проведения\tДисциплина\t\tФИО");

    for (const Parameters& rec : displayedRecords)
    {
        QString text = QString("%1\t\t%2\t\t%3")
            .arg(rec.YearEvent)
            .arg(disciplineToString(static_cast<int>(rec.discipline)))
            .arg(rec.FIO);
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, rec.id);
        ui->lw_browser->addItem(item);
    }

    qDebug() << "List widget count:" << ui->lw_browser->count();
}

void MainWindow::onShowCurrentRecord(int row)
{
    if (row <= 0 || row - 1 >= displayedRecords.size())
    {
        if (row == 0)
        {
            // Очищаем поля при выборе заголовка
            loadingInProgress = true;
            ui->le_fio->clear();
            ui->de_birth->setDate(QDate(1850, 1, 1));
            ui->lw_citizen->setCurrentRow(-1);
            ui->lw_olympiad->setCurrentRow(-1);
            ui->sb_year_event->setValue(1896);
            ui->lw_country->setCurrentRow(-1);
            ui->lw_discipline->setCurrentRow(-1);
            ui->sb_place->setValue(1);
            ui->cb_have_record->setChecked(false);
            currentId = -1;
            loadingInProgress = false;
        }
        return;
    }

    loadingInProgress = true;

    const Parameters& rec = displayedRecords[row - 1];
    currentId = rec.id;

    // Загружаем полные данные с сервера
    std::string cmd = "GET:" + std::to_string(rec.id);
    std::string response = pipe.sendCommand(cmd);

    if (response.substr(0, 7) == "RECORD|")
    {
        std::string data = response.substr(7);
        std::vector<std::string> tokens;
        std::stringstream ss(data);
        std::string token;
        while (std::getline(ss, token, '|'))
            tokens.push_back(token);

        if (tokens.size() >= 12)
        {
            ui->le_fio->setText(QString::fromStdString(tokens[1]));

            int year = std::stoi(tokens[2]);
            int month = std::stoi(tokens[3]);
            int day = std::stoi(tokens[4]);
            ui->de_birth->setDate(QDate(year, month, day));

            ui->lw_citizen->setCurrentRow(std::stoi(tokens[5]));
            ui->lw_olympiad->setCurrentRow(std::stoi(tokens[6]));
            ui->sb_year_event->setValue(std::stoi(tokens[7]));
            ui->lw_country->setCurrentRow(std::stoi(tokens[8]));
            ui->lw_discipline->setCurrentRow(std::stoi(tokens[9]));
            ui->sb_place->setValue(std::stoi(tokens[10]));
            ui->cb_have_record->setChecked(std::stoi(tokens[11]) != 0);
        }
    }
    else
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить запись");
    }

    loadingInProgress = false;
}

void MainWindow::onSaveData()
{
    if (loadingInProgress) return;
    if (currentId == -1) return;

    Parameters rec;
    rec.id = currentId;
    rec.FIO = ui->le_fio->text();
    rec.DateBirth = ui->de_birth->date();
    rec.citizen = static_cast<Parameters::Citizen>(ui->lw_citizen->currentRow());
    rec.type = static_cast<Parameters::TypeOlympiad>(ui->lw_olympiad->currentRow());
    rec.YearEvent = ui->sb_year_event->value();
    rec.country = static_cast<Parameters::CountryEvent>(ui->lw_country->currentRow());
    rec.discipline = static_cast<Parameters::Discipline>(ui->lw_discipline->currentRow());
    rec.Place = ui->sb_place->value();
    rec.Record = ui->cb_have_record->isChecked();

    sendRecordToServer(rec, true);

    // Обновляем отображение
    for (int i = 0; i < displayedRecords.size(); i++)
    {
        if (displayedRecords[i].id == currentId)
        {
            displayedRecords[i].YearEvent = rec.YearEvent;
            displayedRecords[i].discipline = rec.discipline;
            displayedRecords[i].FIO = rec.FIO;
            break;
        }
    }
    updateList();

    // Восстанавливаем выделение
    for (int i = 1; i < ui->lw_browser->count(); i++)
    {
        if (ui->lw_browser->item(i)->data(Qt::UserRole).toInt() == currentId)
        {
            ui->lw_browser->setCurrentRow(i);
            break;
        }
    }
}

void MainWindow::sendRecordToServer(const Parameters& rec, bool isUpdate)
{
    std::stringstream cmd;

    if (isUpdate)
    {
        cmd << "UPDATE:" << rec.id << "|"
            << rec.FIO.toStdString() << "|"
            << rec.DateBirth.year() << "|"
            << rec.DateBirth.month() << "|"
            << rec.DateBirth.day() << "|"
            << static_cast<int>(rec.citizen) << "|"
            << static_cast<int>(rec.type) << "|"
            << rec.YearEvent << "|"
            << static_cast<int>(rec.country) << "|"
            << static_cast<int>(rec.discipline) << "|"
            << rec.Place << "|"
            << (rec.Record ? 1 : 0);
    }
    else
    {
        cmd << "ADD:" << rec.FIO.toStdString() << "|"
            << rec.DateBirth.year() << "|"
            << rec.DateBirth.month() << "|"
            << rec.DateBirth.day() << "|"
            << static_cast<int>(rec.citizen) << "|"
            << static_cast<int>(rec.type) << "|"
            << rec.YearEvent << "|"
            << static_cast<int>(rec.country) << "|"
            << static_cast<int>(rec.discipline) << "|"
            << rec.Place << "|"
            << (rec.Record ? 1 : 0);
    }

    pipe.sendCommand(cmd.str());
    dataModified = true;
}

void MainWindow::onCreate()
{
    std::string response = pipe.sendCommand(
        "ADD:|1850|1|1|0|0|1896|0|0|1|0"
    );

    if (response.substr(0, 6) == "ADDED|")
    {
        int newId = std::stoi(response.substr(6));

        Parameters newRec;
        newRec.id = newId;
        newRec.FIO = "";
        newRec.YearEvent = 1896;
        newRec.discipline = Parameters::a;
        displayedRecords.append(newRec);
        updateList();

        // Находим и выделяем новую запись
        for (int i = 1; i < ui->lw_browser->count(); i++)
        {
            if (ui->lw_browser->item(i)->data(Qt::UserRole).toInt() == newId)
            {
                ui->lw_browser->setCurrentRow(i);
                break;
            }
        }
    }

    ui->pb_fill->setVisible(false);
}

void MainWindow::onDelete()
{
    int row = ui->lw_browser->currentRow();
    if (row <= 0) return;

    int id = ui->lw_browser->item(row)->data(Qt::UserRole).toInt();

    pipe.sendCommand("DELETE:" + std::to_string(id));

    for (int i = 0; i < displayedRecords.size(); i++)
    {
        if (displayedRecords[i].id == id)
        {
            displayedRecords.remove(i);
            break;
        }
    }

    updateList();
    dataModified = true;

    if (displayedRecords.size() > 0)
        ui->lw_browser->setCurrentRow(1);
    else
        currentId = -1;
}

void MainWindow::onFill()
{
    QStringList names = {
        "Путин Владимир Владимирович", "Мусагалиев Азамат Тахирович", "Гейтс Уильям Генри", "Геннадий", "Зубенко Михаил Петрович",
        "Чоу Юнь Фат", "Чехов Капибар Капибарыч", "Сын Васи Березуцкого", "Чемпион", "Зинаида Петровна"
    };

    for (int i = 0; i < 10; i++)
    {
        Parameters rec;
        rec.FIO = names[i];
        rec.DateBirth = QDate(1850 + i, (i % 12) + 1, 1);
        rec.citizen = static_cast<Parameters::Citizen>(i % 5);
        rec.YearEvent = 1896 + i * 2;
        rec.country = static_cast<Parameters::CountryEvent>(i % 4);
        rec.discipline = static_cast<Parameters::Discipline>(i % 9);
        rec.type = (static_cast<int>(rec.discipline) < 5) ? Parameters::summer : Parameters::winter;
        rec.Place = (i == 0) ? 1 : 3;
        rec.Record = (rec.Place == 1);

        sendRecordToServer(rec, false);
    }

    // Пауза для обработки сервером
    Sleep(500);

    loadAllRecords();
    ui->pb_fill->setVisible(false);
}

void MainWindow::loadAllRecords()
{
    std::string response = pipe.sendCommand("GET_ALL");
    if (response.substr(0, 6) != "ITEMS|")
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные");
        return;
    }

    displayedRecords.clear();

    std::string data = response.substr(6);

    // Разбиваем всю строку по разделителю '|'
    std::vector<std::string> allTokens;
    std::stringstream ss(data);
    std::string token;
    while (std::getline(ss, token, '|'))
        allTokens.push_back(token);

    if (allTokens.empty()) return;

    // Первый токен - это количество записей
    int count = std::stoi(allTokens[0]);

    // Остальные токены - это данные записей (4 поля на запись)
    int index = 1;
    for (int i = 0; i < count; i++)
    {
        if (index + 3 >= allTokens.size()) break;

        Parameters rec;
        rec.id = std::stoi(allTokens[index++]);
        rec.YearEvent = std::stoi(allTokens[index++]);
        rec.discipline = static_cast<Parameters::Discipline>(std::stoi(allTokens[index++]));
        rec.FIO = QString::fromStdString(allTokens[index++]);

        displayedRecords.append(rec);
    }

    updateList();
}

void MainWindow::onYearEventChanged(int value)
{
    if (value % 2 != 0)
        ui->sb_year_event->setValue(value - 1);

    QDate maxDate(value, 1, 1);
    ui->de_birth->setMaximumDate(maxDate.addYears(-14));
}

void MainWindow::onPlaceChanged(int value)
{
    ui->cb_have_record->setEnabled(value == 1);
    if (value != 1)
        ui->cb_have_record->setChecked(false);
}

void MainWindow::onOlympiadChanged(int row)
{
    bool isWinter = (row == 1);
    for (int i = 0; i < ui->lw_discipline->count(); i++)
    {
        bool hide = (isWinter && i < 5) || (!isWinter && i >= 5);
        ui->lw_discipline->setRowHidden(i, hide);
    }
}

void MainWindow::saveFile()
{
    std::string response = pipe.sendCommand("SAVE");
    if (response.substr(0, 2) == "OK")
    {
        dataModified = false;
        QMessageBox::information(this, "Сохранение", "Данные сохранены");
    }
    else
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить данные");
    }
}

void MainWindow::loadFile()
{
    std::string response = pipe.sendCommand("LOAD");
    if (response.substr(0, 2) == "OK")
    {
        loadAllRecords();
        QMessageBox::information(this, "Загрузка", "Данные загружены");
    }
    else
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные");
    }
}

QString MainWindow::disciplineToString(int discipline) const
{
    switch (discipline)
    {
        case 0: return "бег на 100 м";
        case 1: return "бег с барьерами на 110 м";
        case 2: return "прыжки в высоту";
        case 3: return "прыжки с шестом";
        case 4: return "метание диска";
        case 5: return "шорт-трек 1 км";
        case 6: return "бег на коньках 5000 м";
        case 7: return "бег на лыжах 40 км";
        case 8: return "биатлон, масс-старт, 40 км";
        default: return "";
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (dataModified)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Сохранение",
            "Данные не сохранены. Сохранить перед выходом?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
            saveFile();
        else if (reply == QMessageBox::Cancel)
        {
            event->ignore();
            return;
        }
    }
    event->accept();
}
