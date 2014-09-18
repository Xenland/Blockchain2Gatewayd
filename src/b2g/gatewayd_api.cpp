#include "gatewayd_api.h"

gatewayd_api::gatewayd_api(QObject *parent) :
    QObject(parent)
{
    //Initalize
    tcp_socket = new QTcpSocket();
}

bool gatewayd_api::can_connect(){
    tcp_socket->connectToHost("127.0.0.1", 5990);
    return tcp_socket->waitForConnected(5000);
}
