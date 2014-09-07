/** Copyright 2014 Blockchain2Gatewayd Developers **/
#include "blockchain_management.h"

blockchain_management::blockchain_management(QObject *parent) :
    QObject(parent)
{
    //Init and Define variables
    proccessing_request_queue = 0;
    current_block_index = 0;
    block_count = 0;
    request_blockhash = QStringList();
    blockchain_network = new QTcpSocket();
}

void blockchain_management::start_buffering(){
    emit slot_continue_buffering();
}

void blockchain_management::slot_continue_buffering(){
    //(re)activate buffering of blockchain
    buffer_blockchain();
}

void blockchain_management::buffer_blockchain(){
    //Don't overrun
    if(proccessing_request_queue == 0){
        //Lock
        proccessing_request_queue = 1;

        //Loop through each request as they come in (loadup all the blockhashes into a list)
        while(request_blockhash.size() > 0){
            //ASSERT still connected to the blockchain by network
            blockchain_network->connectToHost("localhost", 8332);
            if(blockchain_network->waitForConnected(5000)){
                //connected

                //send zero index request to the blockchain, wait for a response and process that response.
                QByteArray request_to_write = request_blockhash.at(0).toUtf8();
                blockchain_network->write(request_to_write);

                //wait for response from the blockchain
                if(blockchain_network->waitForReadyRead(30000)){
                    //record response temporarily
                    QByteArray blockchain_response = blockchain_network->readAll();
                    QString blockchain_response_string = QString::fromUtf8(blockchain_response);
                    QString blockchain_response_body = blockchain_response_string.split(QString("\r\n\r\n")).at(1);
                    blockchain_response_body.replace(QString("\r\n"), QString(""));

                    /* extract blockhash */
                    QJsonDocument blockchain_response_jsondoc = QJsonDocument::fromJson(blockchain_response_body.toUtf8());
                    QJsonObject blockchain_response_jsonobj = blockchain_response_jsondoc.object();

                    //extract blockhash from the "result"
                    QJsonValue blockhash_jsonvalue = blockchain_response_jsonobj.value("result");
                    QString blockhash_string = blockhash_jsonvalue.toString();

                    //record response into the buffer
                    blockchain_hash_map.insert(current_block_index, blockhash_string);

                    //set the next block index to retrieve
                    current_block_index = current_block_index + 1;

                    //add to buffer to process request
                    if(current_block_index < block_count){
                        request_blockhash.append(generate_request_getblockhash(current_block_index));
                    }
                    //disconnect
                    blockchain_network->disconnectFromHost();

                }else{
                    //Nothing to read after 30seconds
                    qDebug() << "timeout156";
                }
            }else{
                qDebug() << "couldn't connect to host";
            }

            //delete the request we just worked with
            request_blockhash.removeAt(0);
        }

        //Loop through each request as they come in (gatewayd wants all the tx information from each blockhash retrieved)
        int next_block_index = 0;
        while(next_block_index > -1){
            qDebug() << "WHILE NEXTBLOCKCINDEX";
            //Make sure we have a block count before we begin scrolling the block chain
            if(block_count > 0){
                qDebug() << "WORKING";
                //ASSERT still connected to the blockchain by network
                blockchain_network->connectToHost("localhost", 8332);
                if(blockchain_network->waitForConnected(5000)){
                    //connected

                    //send blockhash for the block info we want, wait for a response and process that response.
                    QString request_to_write = generate_request_getblock(blockchain_hash_map.value(next_block_index).toString());
                    qDebug() << "WRITING\n" << request_to_write;

                    blockchain_network->write(request_to_write.toUtf8());

                    //wait for response from the blockchain
                    if(blockchain_network->waitForReadyRead(30000)){
                        //record response temporarily
                        QByteArray blockchain_response = blockchain_network->readAll();
                        qDebug() << blockchain_response;
                        QString blockchain_response_string = QString::fromUtf8(blockchain_response);
                        QString blockchain_response_body = blockchain_response_string.split(QString("\r\n\r\n")).at(1);
                        blockchain_response_body.replace(QString("\r\n"), QString(""));

                        //disconnect
                        blockchain_network->disconnectFromHost();
                    }else{
                        //Nothing to read after 30seconds
                        qDebug() << "timeout156";
                    }
                }else{
                    qDebug() << "couldn't connect to host";
                }

                //Increment next block index to request tx data from
                next_block_index = next_block_index + 1;

                //Do we break the while loop yet?
                if(next_block_index > block_count){
                    next_block_index = -1; //break while loop
                }
            }else{
                //No blocks to get tx data from yet, Break the while() loop
                next_block_index = -1; //break while loop
            }
        }


        //Update total block count
        //ASSERT still connected to the blockchain by network
        blockchain_network->connectToHost("localhost", 8332);
        if(blockchain_network->waitForConnected(5000)){
            //connected
            QString request_to_write = generate_request_getblockcount();
            qDebug() << blockchain_network->write(request_to_write.toUtf8());

            //wait for response from the blockchain
            if(blockchain_network->waitForReadyRead(30000)){
                //record response temporarily
                QByteArray blockchain_response = blockchain_network->readAll();
                QString blockchain_response_string = QString::fromUtf8(blockchain_response);
                QStringList blockchain_response_stringlist = blockchain_response_string.split(QString("\r\n\r\n"));
                QString blockchain_response_body = blockchain_response_stringlist.at(1);
                blockchain_response_body = blockchain_response_body.replace(QString("\r\n"), QString(""));

                //extract block count from the "result"
                QJsonDocument blockchain_response_jsondoc = QJsonDocument::fromJson(blockchain_response_body.toUtf8());
                QJsonObject blockchain_response_jsonobj = blockchain_response_jsondoc.object();
                QJsonValue blockcount_jsonvalue = blockchain_response_jsonobj.value("result");
                QString blockcount_string = QString::number(blockcount_jsonvalue.toDouble());

                //convert to int
                int blockcount_int = blockcount_string.toInt();

                //TEMPCODE: force max blockcount
                blockcount_int = 10000;

                //set new max blockcount
                block_count = blockcount_int;
                qDebug() << "BLOCKCOUNT:" << block_count;

                //create request to get the current block hash
                request_blockhash.append(generate_request_getblockhash(current_block_index));

                //set the next block index to retrieve
                current_block_index = current_block_index + 1;

                //disconnect
                blockchain_network->disconnectFromHost();
            }
        }

        //Unlock
        proccessing_request_queue = 0;
    }

    //Retrigger this function if there was another request
    emit slot_continue_buffering();
}

QString blockchain_management::generate_request_getblockcount(){
    //Define local variables
    QString output = QString();

    //gen http head
    QByteArray http_head = generate_http_head();
    QString http_head_string = QString::fromUtf8(http_head);

    //gen http body, contains the request (json format)
    QString http_body = QString();
    http_body.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"method\": \"getblockcount\", \"params\":[]}\r\n");

    //calculate content-length of body and insert into the http head
    http_head_string = http_head_string.arg(http_body.length());

    //redefine http_head with new information
    http_head.clear();
    http_head.append(http_head_string.toUtf8());

    //render output
    output.append(http_head);
    output.append(http_body);

    return output;
}

QString blockchain_management::generate_request_getblockhash(int block_index){
    //Define local variables
    QString output = QString();

    //gen http head
    QByteArray http_head = generate_http_head();
    QString http_head_string = QString::fromUtf8(http_head);

    //gen http body, contains the request (json format)
    QString http_body = QString();
    http_body.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"method\": \"getblockhash\", \"params\":[%1]}\r\n");
    http_body = http_body.arg(block_index);

    //calculate content-length of body and insert into the http head
    http_head_string = http_head_string.arg(http_body.length());

    //redefine http_head with new information
    http_head.clear();
    http_head.append(http_head_string.toUtf8());

    //render output
    output.append(http_head);
    output.append(http_body);
    return output;
}

QString blockchain_management::generate_request_getblock(QString block_hash){
    //Define local vairables
    QString output = QString();
    //gen http head
    QByteArray http_head = generate_http_head();
    QString http_head_string = QString::fromUtf8(http_head);

    //gen http body, contains the request (json format)
    QString http_body = QString();
    http_body.append("{\"jsonrpc\":\"1.0\", \"id\":\"1\", \"method\": \"getblock\", \"params\":[\"%1\"]}\r\n");
    http_body = http_body.arg(block_hash);

    //calculate content-length of body and insert into the http head
    http_head_string = http_head_string.arg(http_body.length());

    //redefine http_head with new information
    http_head.clear();
    http_head.append(http_head_string.toUtf8());

    //render output
    output.append(http_head);
    output.append(http_body);

    return output;
}

QByteArray blockchain_management::generate_http_head(){
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
    output.append(QString("Authorization: Basic %1\r\n").arg(QString::fromUtf8(base64_auth_answer)));
    output.append("Content-Type: application/json\r\n");
    output.append("Content-Length: %1\r\n"); //content length is produced and inserted later when it is known
    output.append("\r\n");

    //return head
    return output.toUtf8();
}
