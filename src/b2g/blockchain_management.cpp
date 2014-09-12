/** Copyright 2014 Blockchain2Gatewayd Developers **/
#include "blockchain_management.h"

blockchain_management::blockchain_management(QObject *parent) :
    QObject(parent)
{
    //Init variables
    current_block_hash = QByteArray();
    proccessing_request_queue = 0;
    blockchain_api_netconnection = new QTcpSocket();

    //Make connections for signals/slots
    connect(this, SIGNAL(begin_blockchain_buffer()), this, SLOT(slot_activate_management()));
}

void blockchain_management::start(){
    //start buffering blockchain information
    emit begin_blockchain_buffer();
}


void blockchain_management::slot_activate_management(){
    //(re)activate management for blockchain
    process_blockchain();
}

void blockchain_management::process_blockchain(){
    //Don't overrun
    if(proccessing_request_queue == 0){
        //Lock
        proccessing_request_queue = 1;

        //-------------------------------
        //Do we have the first hash?
            //No retrieve it, loop again

            //Yes, get tx information, add to buffer for gatewayd to call, loop again but next time go to next blockhash
        //-------------------------------

        //Is the hash set?
        if(current_block_hash.isEmpty() == true){
            //No, retrieve the first hash
            api_get_first_hash();
            //api_get_block_tx_data(QString("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"));
        }else if(current_block_hash.isEmpty() == false){
            //Yes, get tx information

            //[todo]Get next block hash
        }

        //Unlock
        proccessing_request_queue = 0;
    }
}

void blockchain_management::api_get_first_hash(){
    //Define local variables
    QByteArray http_request = QByteArray();

    //open the connection [managed]
    if(open_managed_connection() == true){
        /** clear network buffer **/
        blockchain_api_netconnection->readAll();

        /* ask the blockchain connection for the first hash of the block */
        //Generate the head of the HTTP JSON-RPC request
        QByteArray http_head = api_response_generate_head();

        //Generate the body of the HTTP JSON-RPC request
        QByteArray http_body = api_genjson_getfirsthash();

        //Insert the content-length value in the head
        http_head = QString::fromUtf8(http_head).arg(http_body.length()).toUtf8();

        //Generate the whole http request
        http_request.append(http_head);
        http_request.append(http_body);

        //Submit request to the blockchain network
        qint64 amount_written = blockchain_api_netconnection->write(http_request);
        if(amount_written >= http_request.length()){
            //successfully written to the blockchain network now wait for the response
            if(blockchain_api_netconnection->waitForReadyRead(30000)){
                QByteArray getblockhash_response = blockchain_api_netconnection->readAll();

                //get body of the response
                QString getblockhash_response_string = QString::fromUtf8(getblockhash_response);
                QStringList getblockhash_response_stringlist = getblockhash_response_string.split(QString("\r\n\r\n"));
                QString getblockhash_response_body = getblockhash_response_stringlist.at(1);
                qDebug() << " body: " << getblockhash_response_body;
                //convert bytearray to json document
                QJsonDocument getblockhash_jsondoc = QJsonDocument::fromJson(getblockhash_response_body.toUtf8());
                qDebug() << "doc:" << getblockhash_jsondoc.toJson();
                QJsonObject getblockhash_jsonobj = getblockhash_jsondoc.object();
                //extract blockhash result
                QJsonValue getblockhash_jsonvalue = getblockhash_jsonobj.value("result");
                QString blockhash_string = getblockhash_jsonvalue.toString();

                //ask block chain for tx information from this block/selected hash
                api_get_block_tx_data(blockhash_string);

            }else{
                qDebug() << "1NO RESPONSE FROM BLOCKCHAIN NETWORK";
            }
        }else{
            qDebug() << "1FAILED TO SEND/WRITE, amount: " << amount_written;
        }

    }else{
        //Connection couldn't be opened with the blockchain api network
        qDebug() << "1blockchain connection closed";
    }
}

void blockchain_management::api_get_block_tx_data(QString blockhash){
    qDebug() << "getting block tx data";
    //Define local variables
    QByteArray http_request = QByteArray();
    //open the connection [managed]
    if(open_managed_connection() == true){
        /* ask the blockchain for the tx data from the requested blockhash */
        //Generate the head of the HTTP JSON-RPC request
        QByteArray http_head = api_response_generate_head();

        //Generate the body of the HTTP JSON-RPC request
        QByteArray http_body = api_genjson_gettxbyhash(blockhash);

        //Insert the content-length value in the head
        http_head = QString::fromUtf8(http_head).arg(http_body.length()).toUtf8();

        //Generate the whole http request
        http_request.append(http_head);
        http_request.append(http_body);

        qDebug() << http_request;

        //Clear read buffer
        blockchain_api_netconnection->waitForReadyRead();

        //Submit request to the blockchain network
        qint64 amount_written = blockchain_api_netconnection->write(http_request);
        if(amount_written >= http_request.length()){
            //successfully written to the blockchain network now wait for the response
            if(blockchain_api_netconnection->waitForReadyRead(30000)){
                QByteArray getblockhash_response = blockchain_api_netconnection->readAll();
                qDebug() << getblockhash_response;
            }else{
                qDebug() << "2No response";
            }
        }else{
            qDebug() << "2FAILED TO SEND/WRITE, amount sent:" << amount_written;
        }

    }else{
        qDebug() << "2connection failed";
    }
}

bool blockchain_management::open_managed_connection(){
    //Define local variables
    bool output = false;

    if(blockchain_api_netconnection->isValid() == false){
        //open connection
        blockchain_api_netconnection->connectToHost("localhost", 8332);

        //wait for connection to open before returning status of the connection
        if(blockchain_api_netconnection->waitForConnected(30000) == true){
            qDebug() << "NETCONNECTION OPEN";
            //connection is now open, return true
            output = true;
        }else{
            //connection couldn't be opened, return false
            output = false;
        }
    }else{
        //connection already open return true
        output = true;
    }

    return output;
}

//This will generate a HTTP header with the credentials inputted
QByteArray blockchain_management::api_response_generate_head(){
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


//This will generate a HTTP body with the json structure asking for the first block hash
QByteArray blockchain_management::api_genjson_getfirsthash(){
    //Define local variables
    QByteArray output = QByteArray();

    output.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"method\": \"getblockhash\", \"params\":[0]}");
    output.append("\r\n\r\n"); //Declare end of body and transmission

    return output;
}

//This will generate a HTTP body with the json structure asking for the tx data from the selected blockhash
QByteArray blockchain_management::api_genjson_gettxbyhash(QString blockhash){
    //Define local variables
    QByteArray output = QByteArray();

    QString pre_output = QString();
    pre_output.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"method\": \"getblock\", \"params\":[\"%1\"]}");
    pre_output.append("\r\n\r\n"); //Declare end of body and transmission
    pre_output = pre_output.arg(blockhash);

    output.append(pre_output);

    return output;
}

//Extract hash from the HTTP response
QString blockchain_management::extract_blockhash(){

}
