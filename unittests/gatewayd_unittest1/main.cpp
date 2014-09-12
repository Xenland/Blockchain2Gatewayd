/** Copyright 2014 Blockchain2Gatewayd Developers **/
#include <QCoreApplication>
#include <QTcpSocket>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //ASSERT Init and Define variable for the header of the HTTP request
    QString test_http_write_head = QString();
    test_http_write_head.append("POST /v1/registrations HTTP1.0\r\n"); //First line of HTTP header needs [method] [path] [http_specification]
    test_http_write_head.append("Host: localhost\r\n");                //Second line of HTTP header needs to know domain or ip address (for the reciving server information not the network connections)
    test_http_write_head.append("Content-Type: application/json\r\n"); //Content type for letting the receiver know the format of the body
    test_http_write_head.append("Content-Length: %1\r\n");             //content length for letting the receiver know when the body information is finished sending
    test_http_write_head.append("\r\n");                               //Declare end of the head

    //ASSERT Init and Define variable for the body of the HTTP request (JSON Message Format)
    QString test_http_write_body = QString();
    test_http_write_body.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"name\":\"steven@ripple.com\", \"password\":\"awholelottapizzaria\", \"ripple_address\":\"r4EwBWxrx5HxYRyisfGzMto3AT8FZiYdWk\"}");
    test_http_write_body.append("\r\n"); //Declare end of body and transmission

    //ASSERT insert information that was too unintutitve to be inserted earlier (content-length)
    test_http_write_head = test_http_write_head.arg(test_http_write_body.length());

    //ASSERT Init and Define variable for the combination/concat of head and body
    QString test_http_write_headbody = QString();
    test_http_write_headbody.append(test_http_write_head);
    test_http_write_headbody.append(test_http_write_body);

    qDebug() << test_http_write_headbody;

    //ASSERT Define TCP socket
    QTcpSocket * test_tcp_socket = new QTcpSocket();

    //ASSERT network connection to connect to the gatewayds' JSON-RPC API
    test_tcp_socket->connectToHost("localhost", 5000);

    //ASSERT network connection has been established
    Q_ASSERT(test_tcp_socket->waitForConnected(5000) == true);

    //ASSERT network has something to report back
    qDebug() << "READYREAD:" << test_tcp_socket->waitForReadyRead(5000);

    //ASSERT write to the network connection
    Q_ASSERT(test_tcp_socket->write(test_http_write_headbody.toUtf8()) > 0);

    //ASSERT network has something to report back
    qDebug() << "READYREAD:" << test_tcp_socket->waitForReadyRead(5000);

    //ASSERT read from the network
    QByteArray test_network_response = test_tcp_socket->readAll();

    //ASSERT something was read from the network
    qDebug() << "RESPONSELENGTH:" << test_network_response.length();

    //ASSERT to terminal unit test has been completed and successfull
    qDebug() << "Unit Test Success";
    return a.exec();
}
