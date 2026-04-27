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
    std::string lastResponse;

public:
    explicit Pipe(QObject *parent = nullptr);
    ~Pipe();

    void startServer();
    void connectPipe();
    bool isConnectedToServer() const { return isConnected; }

    std::string sendCommand(const std::string& cmd);

signals:
    void connected();
    void connectionFailed(const QString& error);
};

#endif
