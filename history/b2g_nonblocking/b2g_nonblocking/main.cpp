/** Copyright 2014 Blockchain2Gatewayd Developers **/
#include <QCoreApplication>
#include "blockchain_management.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //Begin scrolling through blockchain
    blockchain_management * blockchain = new blockchain_management();
    blockchain->start_buffering();

    return a.exec();
}
