/** Copyright 2014 Blockchain2Gatewayd Developers **/
#include <QCoreApplication>
#include <QTcpSocket>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    /** Begin Unit Test **/
    //ASSERT Init and Define variable username as plaintext
    QByteArray test_plaintext_username = QByteArray("username");

    //ASSERT Init and Define variable password as plaintext
    QByteArray test_plaintext_password = QByteArray("password");

    //ASSERT Init and Define variable base64 authentication answer
    QByteArray test_base64_auth_answer = QByteArray();
    test_base64_auth_answer.append(test_plaintext_username);
    test_base64_auth_answer.append(":");
    test_base64_auth_answer.append(test_plaintext_password);
    test_base64_auth_answer = QByteArray(test_base64_auth_answer.toBase64());

    //ASSERT Init and Define variable for the header of the HTTP request
    QString test_http_write_head = QString();
    test_http_write_head.append("POST / HTTP1.1\r\n");                  //First line of HTTP header needs [method] [path] [http_specification]
    test_http_write_head.append("Host: localhost\r\n");                 //Second line of HTTP header needs to know domain or ip address (for the server not the network connection)
    test_http_write_head.append("Authorization: Basic %1\r\n");         //Auhtorization challange,  username:password => base64 output goes here
    test_http_write_head.append("Content-Type: application/json\r\n");  //Content type for letting the receiver know the format of the body
    test_http_write_head.append("Content-Length: %2\r\n");              //content length for letting the receiver know when the body information is finished sending
    test_http_write_head.append("\r\n");                                //Declare end of the head

    //ASSERT Init and Define variable for the body of the HTTP request (JSON Message Format)
    QString test_http_write_body = QString();
    test_http_write_body.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"method\": \"getblock\", \"params\":[\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"]}");
    test_http_write_body.append("\r\n\r\n"); //Declare end of body and transmission

    //ASSERT insert information that was too unintutitve to be inserted earlier
    test_http_write_head = test_http_write_head.arg(QString::fromUtf8(test_base64_auth_answer)).arg(test_http_write_body.length());

    //ASSERT Init and Define variable for the combination/concat of head and body
    QString test_http_write_headbody = QString();
    test_http_write_headbody.append(test_http_write_head);
    test_http_write_headbody.append(test_http_write_body);

    //ASSERT Define TCP socket
    QTcpSocket * test_tcp_socket = new QTcpSocket();

    //ASSERT network connection to connect to the blockchains JSON-RPC API
    test_tcp_socket->connectToHost("localhost", 8332);

    //ASSERT network connection has been established
    Q_ASSERT(test_tcp_socket->waitForConnected(30000) == true);

    //ASSERT write to the network connection
    Q_ASSERT(test_tcp_socket->write(test_http_write_headbody.toUtf8()) > 0);

    //ASSERT network has something to report back
    Q_ASSERT(test_tcp_socket->waitForReadyRead() == true);

    //ASSERT read from the network
    QByteArray test_network_response = test_tcp_socket->readAll();

    //ASSERT something was read from the network
    Q_ASSERT(test_network_response.length() > 0);

    //ASSERT to terminal unit test has been completed and successfull
    qDebug() << "Unit Test Success";

    return a.exec();
}
