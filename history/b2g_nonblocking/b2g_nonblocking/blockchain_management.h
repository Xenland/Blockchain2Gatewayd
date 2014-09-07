/** Copyright 2014 Blockchain2Gatewayd Developers **/
#ifndef BLOCKCHAIN_MANAGEMENT_H
#define BLOCKCHAIN_MANAGEMENT_H

#include <QObject>
#include <QMap>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>

class blockchain_management : public QObject
{
    Q_OBJECT
public:
    explicit blockchain_management(QObject *parent = 0);

    void start_buffering();

private:
    //variables
    QTcpSocket * blockchain_network;
    int proccessing_request_queue;
    QStringList request_blockhash;
    QMap<int, QVariant> blockchain_hash_map;

    //functions
    int current_block_index;
    int block_count;
    void buffer_blockchain();
    QString generate_request_getblockcount();
    QString generate_request_getblockhash(int);
    QString generate_request_getblock(QString);
    QByteArray generate_http_head();

signals:

public slots:

private slots:
    void slot_continue_buffering();
};

#endif // BLOCKCHAIN_MANAGEMENT_H
