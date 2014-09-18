#include "blockchain_api.h"

blockchain_api::blockchain_api(QObject *parent) :
    QObject(parent)
{
    //Initalize
    tcp_socket = new QTcpSocket();
    block_hash = QStringList();
    block_tx_id = QStringList();
}

bool blockchain_api::can_connect(){
    //local vars
    bool output = false;

    //if the socket is already connected just return true
    if(tcp_socket->state() == QAbstractSocket::ConnectedState){
        output = true;
    }else{
        tcp_socket->connectToHost("127.0.0.1", 8332);
        output = tcp_socket->waitForConnected(5000);
    }

    return output;
}

void blockchain_api::blockchain_clear_for_new_request(){
    if(tcp_socket->state() == QAbstractSocket::ConnectedState){
        tcp_socket->disconnectFromHost();
    }
    if(tcp_socket->state() == QAbstractSocket::ConnectedState){
        tcp_socket->waitForDisconnected();
    }
}

void blockchain_api::scroll_and_detect_deposits(QStringList addresses){
    bool response_success_block_height = false;
    int block_height = get_highest_block_height(response_success_block_height);
    //TEMPCODE
    block_height = 2000;
    if(response_success_block_height == true){
        //pre-download hashes
        int a = 0;
        while(a < block_height){
            bool response_success_hashindex = false;
            QByteArray hash = get_hash_by_index(a, response_success_hashindex);

            if(response_success_hashindex == true){
                //record response to the list by the correct index
                block_hash.append(QString::fromUtf8(hash));

                //indicate next loop we want the next block index/height
                a = a + 1;
                qDebug() << a;
            }else{
                qDebug() << "HTTP response not 200/ok, retrying request...";

            }
        }

        //scroll the blockchain detecting deposits from the address string list
        int b = 0;
        while(b < block_hash.size()){
            bool response_success = false;
            QByteArray tx_id = get_txid_by_blockhash(block_hash.at(b), response_success);
            if(response_success == true){
                //record response to the list by the correct index
                block_tx_id.append(tx_id);

                //indicate next loop we want the next block tx information
                b = b + 1;
                qDebug() << b;
            }
        }
    }
}

int blockchain_api::get_highest_block_height(bool& http_response_successful){
    //local vars
    int output = 0;
    http_response_successful = false;

    //Get first block hash
    QByteArray http_request = QByteArray();
    QByteArray http_head = generate_http_head();
    QByteArray http_body = QByteArray();

    QJsonObject request_jsonobj = QJsonObject();
    request_jsonobj.insert(QString("jsonrpc"), QJsonValue(QString("2.0")));
    request_jsonobj.insert(QString("id"), QJsonValue(QString("1")));
    request_jsonobj.insert(QString("method"), QJsonValue(QString("getblockcount")));

    QJsonArray request_jsonarray = QJsonArray();

    request_jsonobj.insert(QString("params"), QJsonValue(request_jsonarray));

    QJsonDocument request_jsondoc = QJsonDocument(request_jsonobj);
    http_body.append(request_jsondoc.toJson(QJsonDocument::Compact));

    QString http_head_modified = QString::fromUtf8(http_head);
    http_head_modified = http_head_modified.arg(http_body.length());
    http_head = http_head_modified.toUtf8();

    //Combine head + body
    http_request.append(http_head);
    http_request.append(http_body);

    //write to blockchain network for the highest block count
    blockchain_clear_for_new_request();
    if(can_connect()){
        tcp_socket->write(http_request);

        //wait for response from blockchain
        if(tcp_socket->waitForReadyRead()){
            QByteArray blockchain_response = tcp_socket->readAll();
            QString blockchain_response_string = QString::fromUtf8(blockchain_response);
            QStringList blockchain_response_stringlist = blockchain_response_string.split(QString("\r\n\r\n"));
            QString http_body = QString();

            //Check if body exists before trying to extract the body
            if(blockchain_response_stringlist.size() >= 2){
                //HTTP body exists extract body from the message.
                http_body = blockchain_response_stringlist.at(1);
            }else{
                //Nobody, nothing to extract
                qDebug() << "nothing to extract";
            }

            //QString to QJsonDocument
            QJsonParseError jsondoc_error = QJsonParseError();
            QJsonDocument jsondoc = QJsonDocument::fromJson(http_body.toUtf8(), &jsondoc_error);

            //if no json conversion errors occured extract the "result" value
            if(jsondoc_error.error == QJsonParseError::NoError){
                QJsonObject jsonobj = jsondoc.object();
                QJsonValue jsonvalue = jsonobj.value(QString("result"));

                int height = jsonvalue.toInt();
                output = height;

                http_response_successful = true;
            }else{
                qDebug() << "-------------------";
                qDebug() << "error parsing json";
                qDebug() << http_request;
                qDebug() << "%%%%%%%%%%%%";
                qDebug() << blockchain_response;
                qDebug() << "-------------------";
            }
        }else{
            qDebug() << "no response from blockchain";
        }
    }else{
        qDebug() << "can't connect to blockchain";
    }

    return output;
}

QByteArray blockchain_api::get_hash_by_index(int index, bool& http_response_successful){
    //local vars
    QByteArray output = QByteArray();
    http_response_successful = false;

    //Get first block hash
    QByteArray http_request = QByteArray();
    QByteArray http_head = generate_http_head();
    QByteArray http_body = QByteArray();

    QJsonObject request_jsonobj = QJsonObject();
    request_jsonobj.insert(QString("jsonrpc"), QJsonValue(QString("2.0")));
    request_jsonobj.insert(QString("id"), QJsonValue(QString("1")));
    request_jsonobj.insert(QString("method"), QJsonValue(QString("getblockhash")));

    QJsonArray request_jsonarray = QJsonArray();
    request_jsonarray.insert(0, QJsonValue(index));

    request_jsonobj.insert(QString("params"), QJsonValue(request_jsonarray));

    QJsonDocument request_jsondoc = QJsonDocument(request_jsonobj);
    http_body.append(request_jsondoc.toJson(QJsonDocument::Compact));

    QString http_head_modified = QString::fromUtf8(http_head);
    http_head_modified = http_head_modified.arg(http_body.length());
    http_head = http_head_modified.toUtf8();

    //Combine head + body
    http_request.append(http_head);
    http_request.append(http_body);

    //write to blockchain network for the highest block count
    blockchain_clear_for_new_request();
    if(can_connect()){
        tcp_socket->write(http_request);

        //wait for response from blockchain
        if(tcp_socket->waitForReadyRead()){
            QByteArray blockchain_response = tcp_socket->readAll();
            QString blockchain_response_string = QString::fromUtf8(blockchain_response);
            QStringList blockchain_response_stringlist = blockchain_response_string.split(QString("\r\n\r\n"));
            QString http_body = QString();

            //Check if the response what 200/Ok
            if(blockchain_response_stringlist.size() > 0){
                //HTTP head
                QString http_head = QString();
                http_head = blockchain_response_stringlist.at(0);

                //Split head so we can see what the response was
                QStringList head_split_list = http_head.split(QString("\r\n"));

                if(head_split_list.size() >= 0){
                    QString response_line = head_split_list.at(0);
                    response_line = response_line.trimmed();

                    if(response_line == QString("HTTP/1.1 200 OK")){
                        //Check if body exists before trying to extract the body
                        if(blockchain_response_stringlist.size() >= 2){
                            //HTTP body exists extract body from the message.
                            http_body = blockchain_response_stringlist.at(1);

                            //QString to QJsonDocument
                            QJsonParseError jsondoc_error = QJsonParseError();
                            QJsonDocument jsondoc = QJsonDocument::fromJson(http_body.toUtf8(), &jsondoc_error);

                            //if no json conversion errors occured extract the "result" value
                            if(jsondoc_error.error == QJsonParseError::NoError){
                                QJsonObject jsonobj = jsondoc.object();
                                QJsonValue jsonvalue = jsonobj.value(QString("result"));
                                QString genesis_hash = jsonvalue.toString();

                                //set output
                                output.clear();
                                output.append(genesis_hash);

                                //define output successful
                                http_response_successful = true;

                            }else{
                                //json parse error occurred
                                qDebug() << "-------------------";
                                qDebug() << "error parsing json";
                                qDebug() << http_request;
                                qDebug() << "%%%%%%%%%%%%";
                                qDebug() << blockchain_response;
                                qDebug() << "-------------------";
                            }
                        }else{
                            //Nobody, nothing to extract
                        }
                    }else{
                       //Not a HTTP 200/Okay response, return that we got a non successfull response
                    }
                }else{
                    //invalid http response
                }
            }else{
                //invalid http response
            }

        }else{
            qDebug() << "NOTHING TO READ";
        }
    }else{
        qDebug() << "couldn't connect to blockchain";
    }

    return output;
}

QByteArray blockchain_api::get_txid_by_blockhash(QString hash, bool& http_response_successful){
    //local vars
    QByteArray output = QByteArray();
    http_response_successful = false;

    //Get first block hash
    QByteArray http_request = QByteArray();
    QByteArray http_head = generate_http_head();
    QByteArray http_body = QByteArray();

    QJsonObject request_jsonobj = QJsonObject();
    request_jsonobj.insert(QString("jsonrpc"), QJsonValue(QString("2.0")));
    request_jsonobj.insert(QString("id"), QJsonValue(QString("1")));
    request_jsonobj.insert(QString("method"), QJsonValue(QString("getblock")));

    QJsonArray request_jsonarray = QJsonArray();
    request_jsonarray.insert(0, QJsonValue(hash));

    request_jsonobj.insert(QString("params"), QJsonValue(request_jsonarray));

    QJsonDocument request_jsondoc = QJsonDocument(request_jsonobj);
    http_body.append(request_jsondoc.toJson(QJsonDocument::Compact));

    QString http_head_modified = QString::fromUtf8(http_head);
    http_head_modified = http_head_modified.arg(http_body.length());
    http_head = http_head_modified.toUtf8();

    //Combine head + body
    http_request.append(http_head);
    http_request.append(http_body);

    //write to blockchain network for the highest block count
    blockchain_clear_for_new_request();
    if(can_connect()){
        tcp_socket->write(http_request);

        //wait for response from blockchain
        if(tcp_socket->waitForReadyRead()){
            QByteArray blockchain_response = tcp_socket->readAll();
            QString blockchain_response_string = QString::fromUtf8(blockchain_response);
            QStringList blockchain_response_stringlist = blockchain_response_string.split(QString("\r\n\r\n"));
            QString http_body = QString();
            qDebug() << blockchain_response;
            //Check if the response what 200/Ok
            if(blockchain_response_stringlist.size() > 0){
                //HTTP head
                QString http_head = QString();
                http_head = blockchain_response_stringlist.at(0);

                //Split head so we can see what the response was
                QStringList head_split_list = http_head.split(QString("\r\n"));

                if(head_split_list.size() >= 0){
                    QString response_line = head_split_list.at(0);
                    response_line = response_line.trimmed();

                    if(response_line == QString("HTTP/1.1 200 OK")){
                        http_response_successful = true;
                        //Check if body exists before trying to extract the body
                        if(blockchain_response_stringlist.size() >= 2){
                            //HTTP body exists extract body from the message.
                            http_body = blockchain_response_stringlist.at(1);

                            //QString to QJsonDocument
                            QJsonParseError jsondoc_error = QJsonParseError();
                            QJsonDocument jsondoc = QJsonDocument::fromJson(http_body.toUtf8(), &jsondoc_error);

                            //if no json conversion errors occured extract the "result" value
                            if(jsondoc_error.error == QJsonParseError::NoError){
                                QJsonObject jsonobj = jsondoc.object();
                                QJsonValue result = jsonobj.value(QString("result"));
                                QJsonObject result_obj = result.toObject();
                                QJsonValue tx_jsonvalue = result_obj.value(QString("tx"));
                                QJsonArray tx_array = tx_jsonvalue.toArray();
                                QJsonValue tx_value = tx_array.at(0);
                                QString tx_id = tx_value.toString();
                                //set output
                                output.clear();
                                output.append(tx_id.toUtf8());

                                //define output successful
                                http_response_successful = true;
                            }
                        }
                    }
                }
            }
        }
    }

    return output;
}


/** Helper Functions **/
//This will generate a HTTP header with the credentials inputted
QByteArray blockchain_api::generate_http_head(){
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
    output.append("POST / HTTP/1.1\r\n");
    output.append("Host: localhost\r\n");
    output.append("Connection: Keep-Alive\r\n");
    output.append(QString("Authorization: Basic %1\r\n").arg(QString::fromUtf8(base64_auth_answer)));
    output.append("Content-Type: application/json\r\n");
    output.append("Content-Length: %1\r\n"); //content length is produced and inserted later when it is known
    output.append("\r\n");

    //return head
    return output.toUtf8();
}
