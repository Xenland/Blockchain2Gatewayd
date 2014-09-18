#ifndef GATEWAYD_API_H
#define GATEWAYD_API_H

#include <QObject>
#include <QTcpSocket>

class gatewayd_api : public QObject
{
    Q_OBJECT
public:
    explicit gatewayd_api(QObject *parent = 0);

    bool can_connect();

private:
    QTcpSocket * tcp_socket;

signals:

public slots:

};

#endif // GATEWAYD_API_H
