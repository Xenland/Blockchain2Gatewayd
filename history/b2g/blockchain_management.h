/** Copyright 2014 Blockchain2Gatewayd Developers **/
#ifndef BLOCKCHAIN_MANAGEMENT_H
#define BLOCKCHAIN_MANAGEMENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

class blockchain_management : public QObject
{
    Q_OBJECT
public:
    explicit blockchain_management(QObject *parent = 0);

    void start();

private:
    //variables
    QTcpSocket * blockchain_api_netconnection;
    QByteArray current_block_hash;
    int proccessing_request_queue;

    //functions
    void process_blockchain();
    void api_get_first_hash();
    bool open_managed_connection();
    QByteArray api_response_generate_head();
    QByteArray api_genjson_getfirsthash();
    QByteArray api_genjson_gettxbyhash(QString);
    void api_get_block_tx_data(QString);
    QString extract_blockhash();

signals:
    void begin_blockchain_buffer();

public slots:

private slots:
    void slot_activate_management();


};

#endif // BLOCKCHAIN_MANAGEMENT_H
