#ifndef PIPE_H
#define PIPE_H

#include <windows.h>
#include <QObject>
#include <QTimer>
#include <string>
#include <vector>

class Pipe : public QObject {
    Q_OBJECT

private:
    HANDLE channel;
    bool isConnected;
    QTimer* reconnectTimer;
    QTimer* updateTimer;  // Таймер для периодического обновления
    std::string lastResponse;
    int dataVersion;      // Версия данных на сервере

public:
    explicit Pipe(QObject *parent = nullptr);
    ~Pipe();

    void startServer();
    void connectPipe();
    bool isConnectedToServer() const { return isConnected; }
    void checkForUpdates();  // Проверка обновлений
    void updateDataVersion(int version);

    std::string sendCommand(const std::string& cmd);

signals:
    void connected();
    void connectionFailed(const QString& error);
    void dataChanged();  // Сигнал об изменении данных на сервере
};

#endif
