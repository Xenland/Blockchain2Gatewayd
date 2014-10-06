/** Copyright 2014 Blockchain2Gatewayd Developers **/
#include <QCoreApplication>
#include <QMap>

#include "sql.h"
#include "gatewayd_api.h"
#include "blockchain_api.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //init sql
    sql * db = new sql();

    //[todo]Step 1, Init gatewayd api
    gatewayd_api * gatewayd = new gatewayd_api();

    //[todo]Step 2, Init blockchain api
    blockchain_api * blockchain = new blockchain_api();

    //[todo]Step 3, ping gatewayd to check if network is online, if yes continue
    bool can_connect_to_gatewayd = gatewayd->can_connect();
    qDebug() << "gatewayd connected:" << can_connect_to_gatewayd;

    if(can_connect_to_gatewayd == true){

        /*
        //[todo]Step 4, ping blockchain to check if network is online and ready to scroll, if yes continue
        bool can_connect_to_blockchain = blockchain->can_connect();
        qDebug() << "blockchain connected: " << can_connect_to_blockchain;

        //[todo]Step 5, ask postgresql for gatewayds bitcoin addresses and associated account ids
        qDebug() << "asking gatewayd/db for blockchain addresses";
        //Simulate asking the database for gatewayds bitcoin addresses and associated account ids
        QMap<QString, QString> bitcoin_addresses = QMap<QString, QString>();
        bitcoin_addresses.insert(QString("1"), QString("1KFHE7w8BhaENAswwryaoccDb6qcT6DbYY"));

        //[todo]Step 6, scroll through the blockchain and detect known addreses as the processor scrolls
        qDebug() << "scrolling through blockchain, detecting deposits...";
        blockchain->scroll_and_detect_deposits(bitcoin_addresses);
        //[todo]Step 7, when a bitcoin address is detected, send/queue a request to gatewayd to submit a deposit.

        //[todo]Step 8, repeat process at step 6
        */
    }

    return a.exec();
}
