#ifndef BLOCKCHAIN_NETWORK_H
#define BLOCKCHAIN_NETWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

class blockchain_network : public QObject
{
    Q_OBJECT
public:
    explicit blockchain_network(QObject *parent = 0);

    void connect_to_blockchain();
    void process_networking();

    void process_block_height();
    void process_block_hash_list();

    QByteArray generate_http_head();

private:
    QTcpSocket * blockchain_tcp;
    int process_network_switch;
    int total_block_count;
    int current_block_index;
    int proccessing_request_queue;

signals:
    void keep_processing();

public slots:
    void slot_keep_processing();

private slots:
    void slot_connected_to_blockchain();

};

#endif // BLOCKCHAIN_NETWORK_H
