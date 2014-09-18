#include <QCoreApplication>
#include "blockchain_network.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //TEMPCODE
    blockchain_network * blockchain_net = new blockchain_network();
    blockchain_net->connect_to_blockchain();
    return a.exec();
}
