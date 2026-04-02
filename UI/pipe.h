#ifndef PIPE_H
#define PIPE_H

#include "windows.h"
#include <QObject>
#include "QTimer"


class Pipe : public QObject {
    Q_OBJECT

private:
    HANDLE Chanel;
    bool isRunning;
    bool isConnected;
    QTimer *reconnectTimer;
public:
    void StartServer();
    void ConnectPipe();
    explicit Pipe(QObject *parent = nullptr);
    ~Pipe();
signals:
    void connected();
};

#endif // PIPE_H
