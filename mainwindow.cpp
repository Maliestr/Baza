#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QCloseEvent>
#include "parameters.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , valFio(QRegExp("^([А-ЯЁA-Z]{1})([а-яёa-z]*)\\s([А-ЯЁA-Z]{1})([а-яёa-z]*)\\s([А-ЯЁA-Z]{1})([а-яёa-z]*)$")) //пробел вначале нельзя поставить
    , currentFile("")
    , nextId(1)
{
    ui->setupUi(this);

    ui->lw_browser->setFocus();

    setupConnections();

    ui->le_fio->setValidator(&valFio); //валидатор для ФИО
    ui->le_fio->setMaxLength(45);

    int year = 1896;
    QDate p(1850, 01, 01);
    ui->de_birth->setMinimumDate(p);
    ui->de_birth->setDate(p);
    QDate t(1896, 01, 01);
    ui->de_birth->setMaximumDate(t.addYears(-14)); //задаёт каждый раз

    ui->sb_year_event->setRange(year, 2100);
    ui->sb_year_event->setSingleStep(2);

    ui->sb_place->setMinimum(1);
    ui->sb_place->setMaximum(3);

    ui->lw_citizen->setCurrentRow(-1);
    ui->lw_olympiad->setCurrentRow(-1);
    ui->lw_country->setCurrentRow(-1);
    ui->lw_discipline->setCurrentRow(-1);

    ui->cb_have_record->setEnabled(true);

    QListWidgetItem* item = new QListWidgetItem("Год проведения\tДисциплина\t\tФИО");
    ui->lw_browser->addItem(item);
}

void MainWindow::setupConnections() //Обработка сигналов (кликов)
{
    connect(ui->le_fio, SIGNAL(textChanged(QString)), this, SLOT(sl_saveData()));
    connect(ui->de_birth, SIGNAL(editingFinished()), this, SLOT(sl_saveData()));
    connect(ui->lw_citizen, SIGNAL(currentRowChanged(int)), this, SLOT(sl_saveData()));
    connect(ui->lw_olympiad, SIGNAL(currentRowChanged(int)), this, SLOT(sl_saveData()));
    connect(ui->sb_year_event, SIGNAL(editingFinished()), this, SLOT(sl_saveData()));
    connect(ui->lw_country, SIGNAL(currentRowChanged(int)), this, SLOT(sl_saveData()));
    connect(ui->lw_discipline, SIGNAL(currentRowChanged(int)), this, SLOT(sl_saveData()));
    connect(ui->sb_place, SIGNAL(editingFinished()), this, SLOT(sl_saveData()));
    connect(ui->cb_have_record, SIGNAL(clicked()), this, SLOT(sl_saveData()));

    connect(ui->sb_place, SIGNAL(valueChanged(int)), this, SLOT(sl_have_record(int)));
    connect(ui->sb_year_event, SIGNAL(valueChanged(int)), this, SLOT(sl_Year_Event(int)));
    connect(ui->lw_olympiad, SIGNAL(currentRowChanged(int)), this, SLOT(sl_choose_discipline(int)));

    connect(ui->pb_fill, SIGNAL(clicked()), this, SLOT(sl_fill()));
    connect(ui->lw_browser, SIGNAL(currentRowChanged(int)), this, SLOT(showCurrentRecord(int)));
    connect(ui->pb_create, SIGNAL(clicked()), this, SLOT(sl_create()));
    connect(ui->pb_remove, SIGNAL(clicked()), this, SLOT(sl_delete()));

    connect(ui->create_file, SIGNAL(triggered()), this, SLOT(create_File()));
    connect(ui->open_file, SIGNAL(triggered()), this, SLOT(open_File()));
    connect(ui->save_file, SIGNAL(triggered()), this, SLOT(save_File()));
    connect(ui->save_file_how, SIGNAL(triggered()), this, SLOT(save_File_how()));
}

void MainWindow::sl_Year_Event(int value) //Валидатор для дат
{
    if (value % 2 != 0)
        ui->sb_year_event->setValue(value - 1);
    QDate t(value, 01, 01);
    ui->de_birth->setMaximumDate(t.addYears(-14));
}

void MainWindow::sl_choose_discipline(int value) //Выбор дисциплины
{
    if (value == 1)
    {
        ui->lw_discipline->setRowHidden(0, true);
        ui->lw_discipline->setRowHidden(1, true);
        ui->lw_discipline->setRowHidden(2, true);
        ui->lw_discipline->setRowHidden(3, true);
        ui->lw_discipline->setRowHidden(4, true);
        ui->lw_discipline->setRowHidden(5, 0);
        ui->lw_discipline->setRowHidden(6, 0);
        ui->lw_discipline->setRowHidden(7, 0);
        ui->lw_discipline->setRowHidden(8, 0);
    }
    else if (value == 0)
    {
        ui->lw_discipline->setRowHidden(0, 0);
        ui->lw_discipline->setRowHidden(1, 0);
        ui->lw_discipline->setRowHidden(2, 0);
        ui->lw_discipline->setRowHidden(3, 0);
        ui->lw_discipline->setRowHidden(4, 0);
        ui->lw_discipline->setRowHidden(5, true);
        ui->lw_discipline->setRowHidden(6, true);
        ui->lw_discipline->setRowHidden(7, true);
        ui->lw_discipline->setRowHidden(8, true);
    }
}

void MainWindow::sl_saveData()
{
    if (loadingInProgress) return;

    int row = ui->lw_browser->currentRow();
    if (row <= 0) return;

    int index = ui->lw_browser->item(row)->data(Qt::UserRole).toInt();
    Parameters s;
    if (!workbase.record(index, s))
    {
        qDebug() << "Запись не найдена";
        return;
    }

    s.FIO = ui->le_fio->text();
    s.DateBirth = ui->de_birth->date();

    if (ui->lw_citizen->currentRow() >= (0))
        s.citizen = static_cast<Parameters::Citizen>(ui->lw_citizen->currentRow());

    if (ui->lw_olympiad->currentRow() >= (0))
        s.type = static_cast<Parameters::TypeOlympiad>(ui->lw_olympiad->currentRow());

    s.YearEvent = ui->sb_year_event->value();

    if (ui->lw_country->currentRow() >= (0))
        s.country = static_cast<Parameters::CountryEvent>(ui->lw_country->currentRow());

    if (ui->lw_discipline->currentRow() >= (0))
        s.discipline = static_cast<Parameters::Discipline>(ui->lw_discipline->currentRow());

    s.Place = ui->sb_place->value();
    s.Record = ui->cb_have_record->isChecked();

    workbase.update(s);

//    updateSingleItem(index);

    updateList(index);
    sortingList();
}

void MainWindow::updateSingleItem(int recordId)
{
    Parameters record;
    if (!workbase.record(recordId, record)) return;

    for (int i = 1; i < ui->lw_browser->count(); ++i) {
        if (ui->lw_browser->item(i)->data(Qt::UserRole).toInt() == recordId) {
            ui->lw_browser->item(i)->setText(
                QString("%1\t\t%2\t\t%3")
                    .arg(record.YearEvent)
                    .arg(disciplineToString(record.discipline))
                    .arg(record.FIO));
            return;
        }
    }
}

void MainWindow::updateList(int index)
{
    Parameters up;
    if (!workbase.record(index, up)) return;

    QListWidgetItem* item = nullptr;

    const int itemCount = ui->lw_browser->count();
    for (int i = 0; i < itemCount && !item; ++i)
    {
        QListWidgetItem* itemz = ui->lw_browser->item(i);
        if (itemz->data(Qt::UserRole).toInt() == index)
        {
            item = itemz;
        }
    }

    if (item)
    {
        item->setText(QString("%1\t\t%2\t\t%3")
                .arg(up.YearEvent)
                .arg(disciplineToString(up.discipline))
                .arg(up.FIO));
    }
}

void MainWindow::sl_delete()
{
    int row = ui->lw_browser->currentRow();
    if (row <= 0) return;

    int index = ui->lw_browser->item(row)->data(Qt::UserRole).toInt();
    workbase.remove(index);

    delete ui->lw_browser->takeItem(row);

    if (workbase.count() == 0)
        currentId = -1;
    else
    {
        if (row >= ui->lw_browser->count())
            row = ui->lw_browser->count() - 1;
        if (row > 0)
        {
            currentId = ui->lw_browser->item(row)->data(Qt::UserRole).toInt();
            ui->lw_browser->setCurrentRow(row);
        }
        else if (ui->lw_browser->count() > 1)
        {
            currentId = ui->lw_browser->item(1)->data(Qt::UserRole).toInt();
            ui->lw_browser->setCurrentRow(1);
        }
        else
            currentId = -1;
    }
}

void MainWindow::showCurrentRecord(int current)
{
    if (current <= 0) return;

    QListWidgetItem* item = ui->lw_browser->item(current);
    if (!item) return;

    int index = item->data(Qt::UserRole).toInt();
    Parameters rec;
    if (workbase.record(index, rec))
    {
        currentId = index;

        loadingInProgress = true;

        ui->le_fio->setText(rec.FIO);
        ui->de_birth->setDate(rec.DateBirth);
        ui->lw_citizen->setCurrentRow(rec.citizen);
        ui->lw_olympiad->setCurrentRow(rec.type);
        ui->sb_year_event->setValue(rec.YearEvent);
        ui->lw_country->setCurrentRow(rec.country);
        ui->lw_discipline->setCurrentRow(rec.discipline);
        ui->sb_place->setValue(rec.Place);
        ui->cb_have_record->setChecked(rec.Record);

        loadingInProgress = false;
    }
}

void MainWindow::sl_have_record(int value)
{
    if (value == 1)
    {
        ui->cb_have_record->setEnabled(true);
    }
    else
    {
        ui->cb_have_record->setChecked(false);
        ui->cb_have_record->setEnabled(false);
    }
}

void MainWindow::sl_fill()
{
    QStringList name =
    {
        "Путин Владимир Владимирович", "Мусагалиев Азамат Тахирович", "Гейтс Уильям Генри", "Геннадий", "Зубенко Михаил Петрович",
        "Чоу Юнь Фат", "Чехов Капибар Капибарыч", "Сын Васи Березуцкого", "Чемпион", "Зинаида Петровна"
    };

    for (int i = 0; i < 10; i++)
    {
        Parameters ten;
        ten.id = generateId();
        ten.FIO = QString("%1").arg(name[i]);
        ten.DateBirth = QDate(1850 + i, (i % 12) + 1, 1);
        ten.citizen = static_cast<Parameters::Citizen>(i % 5);
        ten.YearEvent = 1896 + (i * 2);
        ten.country = static_cast<Parameters::CountryEvent>(i % 4);
        ten.discipline = static_cast<Parameters::Discipline>(i % 9);
        if (ten.discipline < 5)
            ten.type = static_cast<Parameters::TypeOlympiad>(0);
        else
            ten.type = static_cast<Parameters::TypeOlympiad>(1);
        if (i > 0 && i < 4)
            ten.Place = i;
        else if (i == 0)
            ten.Place = 1;
        else
            ten.Place = 3;
        if (ten.Place == 1)
            ten.Record = 1;
        else
            ten.Record = 0;
        workbase.append(ten);
    }
    sortingList();

    ui->pb_fill->setVisible(false); //кнопка исчезает
}

void MainWindow::sortingList()
{
    QSignalBlocker blocker(ui->lw_browser);

    int currentRow = ui->lw_browser->currentRow();
    int saveId = -1;
    if (currentRow > 0)
        saveId = ui->lw_browser->item(currentRow)->data(Qt::UserRole).toInt();

    while (ui->lw_browser->count() > 1) //
        delete ui->lw_browser->takeItem(1);

    QVector<BrowserItem> items = workbase.records();

    for (const BrowserItem &item : items)
    {
        Parameters ten;
        if (workbase.record(item.id, ten))
        {
            QString write = QString("%1\t\t%2\t\t%3")
                    .arg(ten.YearEvent)
                    .arg(disciplineToString(ten.discipline))
                    .arg(ten.FIO);

            QListWidgetItem* wr = new QListWidgetItem(write);
            wr->setData(Qt::UserRole, ten.id);
            ui->lw_browser->addItem(wr);
        }
    }

    if (saveId != -1)
    {
           for (int i = 1; i < ui->lw_browser->count(); ++i)
           {
               if (ui->lw_browser->item(i)->data(Qt::UserRole).toInt() == saveId)
               {
                   ui->lw_browser->setCurrentRow(i);
//                   currentId = saveId;
                   return;
               }
           }
    }
    else if (ui->lw_browser->count() > 1) {
        ui->lw_browser->setCurrentRow(1);
    }
}

QString MainWindow::disciplineToString(Parameters::Discipline discipline) const
{
    switch (discipline)
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

void MainWindow::sl_create()
{
    if (ui->lw_browser->count() == 0)
    {
        ui->lw_browser->addItem("Год проведения\tДисциплина\t\tФИО"); //добавить строку
    }
    Parameters n;
    n.id = generateId();
    n.FIO = "";
    n.YearEvent = 1896;
    n.discipline = static_cast<Parameters::Discipline>(0);
    if (n.discipline < 5)
        n.type = static_cast<Parameters::TypeOlympiad>(0);
    else
        n.type = static_cast<Parameters::TypeOlympiad>(1);
    n.DateBirth = QDate(1850, 1, 1);
    n.citizen = static_cast<Parameters::Citizen>(0);
    n.country = static_cast<Parameters::CountryEvent>(0);
    n.Place = 2;
    n.Record = false;

    workbase.append(n);
    currentId = n.id;
    sortingList();

    ui->pb_fill->setVisible(false); //кнопка исчезает
}

int MainWindow::generateId()
{
    return nextId++;
}

void MainWindow::save_File()
{
    if (currentFile.isEmpty()) {
        save_File_how();
        return;
    }

    if (!workbase.save(currentFile)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл");
    }
}

void MainWindow::save_File_how() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");

    if (fileName.isEmpty()) return;

    currentFile = fileName;
    save_File();
}

void MainWindow::open_File()
{
    if (workbase.isModified()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Несохраненные изменения",
             "У вас есть несохраненные изменения. Хотите сохранить перед открытием другого файла?",
             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            save_File(); // Сохраняем текущий файл
        }
        else if (reply == QMessageBox::Cancel) {
            return; // Отменяем создание нового файла
        }
    }

    QString fileName = QFileDialog::getOpenFileName(this,
        "Открыть файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");

    if (fileName.isEmpty()) return;

    currentFile = fileName;

    workbase.clear();
    while (ui->lw_browser->count() > 1) {
        delete ui->lw_browser->takeItem(1);
    }

    loadingInProgress = true;

    if (!workbase.load(currentFile)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить файл");
        loadingInProgress = false;
        return;
    }

    nextId = 1;
    QVector<BrowserItem> items = workbase.records();
    for (const BrowserItem &item : items) {
        if (item.id >= nextId) {
            nextId = item.id + 1;
        }
    }

    sortingList();

    loadingInProgress = false;

    if (workbase.count() > 0) {
        currentId = workbase.records().first().id;
    }
    else
        currentId = -1;
}

void MainWindow::create_File()
{
    if (workbase.isModified()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Несохраненные изменения",
             "У вас есть несохраненные изменения. Хотите сохранить перед созданием нового файла?",
             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            save_File(); // Сохраняем текущий файл
        }
        else if (reply == QMessageBox::Cancel) {
            return; // Отменяем создание нового файла
        }
    }

    workbase.clear();
    nextId = 1;
    currentFile.clear();

    while (ui->lw_browser->count() > 1) {
        delete ui->lw_browser->takeItem(1);
    }

    ui->le_fio->clear();
    ui->de_birth->setDate(QDate(1850, 1, 1));
    ui->lw_citizen->setCurrentRow(-1);
    ui->lw_olympiad->setCurrentRow(-1);
    ui->sb_year_event->setValue(1896);
    ui->lw_country->setCurrentRow(-1);
    ui->lw_discipline->setCurrentRow(-1);
    ui->sb_place->setValue(1);
    ui->cb_have_record->setChecked(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (workbase.isModified()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Несохраненные изменения",
                                     "У вас есть несохраненные изменения. Хотите сохранить перед выходом?",
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            save_File();
            event->accept(); // Закрываем программу
        }
        else if (reply == QMessageBox::No) {
            event->accept(); // Закрываем без сохранения
        }
        else {
            event->ignore(); // Отменяем закрытие
        }
    } else {
        event->accept(); // Закрываем если нет изменений
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
