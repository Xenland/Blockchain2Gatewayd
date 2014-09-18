#ifndef BLOCKCHAIN_API_H
#define BLOCKCHAIN_API_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QJsonParseError>

class blockchain_api : public QObject
{
    Q_OBJECT
public:
    explicit blockchain_api(QObject *parent = 0);

    bool can_connect();
    void blockchain_clear_for_new_request();
    void scroll_and_detect_deposits(QStringList);
    int get_highest_block_height(bool&);
    QByteArray get_hash_by_index(int, bool&);
    QByteArray get_txid_by_blockhash(QString, bool& );

private:
    QTcpSocket * tcp_socket;
    QStringList block_hash;
    QStringList block_tx_id;

    QByteArray generate_http_head();

signals:

public slots:

};

#endif // BLOCKCHAIN_API_H
