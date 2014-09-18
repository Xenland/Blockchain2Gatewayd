#include "blockchain_network.h"

blockchain_network::blockchain_network(QObject *parent) :
    QObject(parent)
{
    //init vars
    blockchain_tcp = new QTcpSocket();
    process_network_switch = 0;
    total_block_count = 0;
    current_block_index = 0;
    proccessing_request_queue = 0;

    //make connections
    connect(blockchain_tcp, SIGNAL(connected()), this, SLOT(slot_connected_to_blockchain()));
    connect(this, SIGNAL(keep_processing()), this, SLOT(slot_keep_processing()));
}

void blockchain_network::connect_to_blockchain(){
    blockchain_tcp->connectToHost("localhost", 8332);
}

void blockchain_network::slot_keep_processing(){
    //bitcoin api only allows one request per connection so we have to dissconnecto then reconnect (which triggers/activates processing upon connection)
    blockchain_tcp->disconnectFromHost();
    if(blockchain_tcp->state() == QAbstractSocket::ConnectedState){
        blockchain_tcp->waitForDisconnected();
    }
    blockchain_tcp->connectToHost("localhost", 8332);
}


void blockchain_network::slot_connected_to_blockchain(){
    //activate network process
    process_networking();
}

void blockchain_network::process_networking(){
    //Don't overrun
    if(proccessing_request_queue == 0){
        //Lock
        proccessing_request_queue = 1;


            //Still connected to the network?
            if(blockchain_tcp->state() == QAbstractSocket::ConnectedState && process_network_switch == 0){
                //activate - Get the block height (block count)
                process_block_height();
            }

            //Still connected to the network?
            if(blockchain_tcp->state() == QAbstractSocket::ConnectedState && process_network_switch == 1){
                //activate - Get all the block hashes and record them attributed to their index
                process_block_hash_list();
            }

            //Still connected to the network?
            if(blockchain_tcp->state() == QAbstractSocket::ConnectedState  && process_network_switch == 2){
                //activate - Get (or buffer?) all tx information per block
            }


        //Unlock
        proccessing_request_queue = 0;
    }
}

/** process block height **/
void blockchain_network::process_block_height(){
    qDebug() << "PROCESSING BLOCK HEIGHT";

    QByteArray http_request = QByteArray();
    QByteArray http_head = generate_http_head();
    QByteArray http_body = QByteArray();
    http_body.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"method\": \"getblockcount\", \"params\":[]}");
    http_body.append("\r\n"); //Declare end of body and transmission

    QString http_head_modified = QString::fromUtf8(http_head);
    http_head_modified = http_head_modified.arg(http_body.length());
    http_head = http_head_modified.toUtf8();

    //Combine head + body
    http_request.append(http_head);
    http_request.append(http_body);

    //write to blockchain network for the highest block count
    blockchain_tcp->write(http_request);

    //wait for response from blockchain
    if(blockchain_tcp->waitForReadyRead()){
        //read blockchains response
        QByteArray getblockheight_response = blockchain_tcp->readAll();

        //extract body of the whole http response
        QString getblockheight_response_string = QString::fromUtf8(getblockheight_response);
        QStringList getblockheight_response_stringlist = getblockheight_response_string.split(QString("\r\n\r\n"));
        QString getblockheight_response_body = getblockheight_response_stringlist.at(1);

        //convert bytearray to json document
        QJsonDocument getblockheight_jsondoc = QJsonDocument::fromJson(getblockheight_response_body.toUtf8());
        QJsonObject getblockheight_jsonobj = getblockheight_jsondoc.object();

        //extract blockhash result
        QJsonValue getblockheight_jsonvalue = getblockheight_jsonobj.value("result");
        int blockheight_int = getblockheight_jsonvalue.toInt();

        //set new top most blockheight
        total_block_count = blockheight_int;
        total_block_count = 10; //tempcode (to save the devs time during development/testing)

    }else{
        //blockchain didn't response after timeout range specificed.
        qDebug() << "BLOCK CHAIN TIMED OUT PROCESS_BNLOCK_HEIGHT()";
    }

    //Move on to the next process
    process_network_switch = 1;

    //emit signal to process the hash list
    emit keep_processing();
}

/** process block hash list **/
void blockchain_network::process_block_hash_list(){
    //Get first block hash
    QByteArray http_request = QByteArray();
    QByteArray http_head = generate_http_head();
    QByteArray http_body = QByteArray();

    QJsonObject request_jsonobj = QJsonObject();
    request_jsonobj.insert(QString("jsonrpc"), QJsonValue(QString("1.0")));
    request_jsonobj.insert(QString("id"), QJsonValue(QString("1.0")));
    request_jsonobj.insert(QString("method"), QJsonValue(QString("getblockhash")));

    QJsonArray request_jsonarray = QJsonArray();
    request_jsonarray.insert(0, QJsonValue(current_block_index));

    request_jsonobj.insert(QString("params"), QJsonValue(request_jsonarray));

    QJsonDocument request_jsondoc = QJsonDocument(request_jsonobj);
    http_body.append(request_jsondoc.toJson());

    QString http_head_modified = QString::fromUtf8(http_head);
    http_head_modified = http_head_modified.arg(http_body.length());
    http_head = http_head_modified.toUtf8();

    //Combine head + body
    http_request.append(http_head);
    http_request.append(http_body);

    //write to blockchain network for the highest block count
    blockchain_tcp->write(http_request);

    //wait for response from blockchain
    if(blockchain_tcp->waitForReadyRead()){
        qDebug() << "response from blockchain";
        QByteArray blockchain_response = blockchain_tcp->readAll();
        qDebug() << blockchain_response;
        QString blockchain_response_string = QString::fromUtf8(blockchain_response);
        QString blockchain_response_body = blockchain_response_string.split(QString("\r\n\r\n")).at(1);
        blockchain_response_body.replace(QString("\r\n"), QString(""));

        /* extract blockhash */
        QJsonDocument blockchain_response_jsondoc = QJsonDocument::fromJson(blockchain_response_body.toUtf8());
        QJsonObject blockchain_response_jsonobj = blockchain_response_jsondoc.object();

        //extract blockhash from the "result"
        QJsonValue blockhash_jsonvalue = blockchain_response_jsonobj.value("result");
        QString blockhash_string = blockhash_jsonvalue.toString();

        qDebug() << blockhash_string;

        //Is there a next block?
        if(current_block_index < total_block_count){
            //yes, there is a next block, check it on the next round of processing by incrementing current block index
            current_block_index = current_block_index + 1;
        }else{
            //no there is not a next block hash to retrieve
            qDebug() << "got all the block hashes";

            //Move on to the next process
            process_network_switch = 2;
        }

        //keep processing
        emit keep_processing();

    }else{
        qDebug() << "TIMED OUT";
    }
}

/** Helper Functions **/
//This will generate a HTTP header with the credentials inputted
QByteArray blockchain_network::generate_http_head(){
    //Init local variables
    QString output = QString();
    QByteArray plaintext_username = QByteArray();
    QByteArray plaintext_password = QByteArray();
    QByteArray base64_auth_answer = QByteArray();


    //Define username in plaintext
    plaintext_username.append("somereallylongusername");

    //Define password in plaintext
    plaintext_password.append("alsoareallylongpassword");

    //Define auth answer [username:password] output => base64
    base64_auth_answer.append(plaintext_username);
    base64_auth_answer.append(":");
    base64_auth_answer.append(plaintext_password);
    base64_auth_answer = QByteArray(base64_auth_answer.toBase64());

    //Define http head
    output.append("POST / HTTP1.1\r\n");
    output.append("Host: localhost\r\n");
    output.append("Connection: Keep-Alive\r\n");
    output.append(QString("Authorization: Basic %1\r\n").arg(QString::fromUtf8(base64_auth_answer)));
    output.append("Content-Type: application/json\r\n");
    output.append("Content-Length: %1\r\n"); //content length is produced and inserted later when it is known
    output.append("\r\n");

    //return head
    return output.toUtf8();
}
