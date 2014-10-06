#include "blockchain_api.h"

blockchain_api::blockchain_api(QObject *parent) :
    QObject(parent)
{
    //Initalize
    tcp_socket = new QTcpSocket();
    block_hash = QStringList();
    block_tx_id = QStringList();
    raw_txid_representation = QStringList();

    //init tx_list as an array
    tx_list = QJsonArray();
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

void blockchain_api::scroll_and_detect_deposits(QMap<QString, QString> addresses){
    while(1==1){//Note: remember not to leave this as 1==1 change to a boolean variable
        bool continue_process = false;
        int block_height = 0;
        while(continue_process == false){ //guarantee we have retrieved the block height
            bool response_success_block_height = false;
            block_height = get_highest_block_height(response_success_block_height);

            if(response_success_block_height == true){
                //We have successfully conneected to the block chain and got the first piece of information we needed, continue with process
                continue_process = true;
            }
        }

        qDebug() << "retrieved the highest block height";
        bool response_success_hashindex = false;
        QByteArray hash = get_hash_by_index(0, response_success_hashindex);

        qDebug() << "retrieved all known block hashes";

        //get all the txs from the gensis block till the known block height
        bool response_success = false;
        QByteArray json_tx_since_block = get_listsinceblock(hash, response_success);

        if(response_success == true){
            //scroll through tx list and catch all new balances
            //convert to jsondocument
            QJsonParseError * json_tx_list_error = new QJsonParseError();
            QJsonDocument json_tx_list = QJsonDocument::fromJson(json_tx_since_block, json_tx_list_error);

            //make sure conversion was a success
            if(json_tx_list_error->error == QJsonParseError::NoError){
                //success
                QJsonObject json_tx_list_obj = json_tx_list.object();

                //get the "result" object
                QJsonObject result = json_tx_list_obj.value(QString("result")).toObject();

                //get the "transactions" array
                QJsonArray transactions = result.value(QString("transactions")).toArray();

                //get size of transactions array
                int transactions_size = transactions.size();
                if(transactions_size > 0){
                    for(int a = 0; a < transactions_size; ++a){
                        QJsonObject tx_object = transactions.at(a).toObject();

                        /** Check if the tx (tx_object) has already been deposited **/
                        bool already_deposited = false;
                        for(int b = 0; b < tx_list.size(); ++b){
                            //check if this tx matches the one we are looking at from the incoming tx list
                            if(tx_list.at(b).toObject().value(QString("txid")) == tx_object.value(QString("txid")).toString()){
                                already_deposited = true;

                                //break loop
                                b = tx_list.size();
                            }
                        }
                        qDebug() << "already deposited";
                        if(already_deposited == false){
                            /*The tx has not been deposited send a deposit signal to gatewayd and record the tx to the memory*/
                            //Generate an object to add to the tx_list json document
                            QJsonObject new_tx_object = QJsonObject();

                            //add the txid
                            new_tx_object.insert(QString("txid"), tx_object.value(QString("txid")));
                            new_tx_object.insert(QString("address"), tx_object.value(QString("address")));

                            //add the new tx object to the tx list
                            tx_list.append(new_tx_object);
                        }
                    }
                }else{
                    qDebug() << "Nothing in the transactions list to parse";
                }
            }else{
                qDebug() << "json parse error";
            }
        }else{

        }
        qDebug() << "retrieved all txs from the genesis block to the height of:" << block_height;
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

QByteArray blockchain_api::get_listsinceblock(QString hash, bool& http_response_successful){
    qDebug() << "geting tx";
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
    request_jsonobj.insert(QString("method"), QJsonValue(QString("listsinceblock")));

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
                                output.clear();
                                output.append(jsondoc.toJson(QJsonDocument::Compact));

                                http_response_successful = true;
                            }else{
                                qDebug() << "json error";
                            }
                        }else{
                            qDebug() << "body dosen't exist";
                        }
                    }else{
                        qDebug() << "body";
                    }
                }
            }
        }
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
        //qDebug() << "couldn't connect to blockchain";
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
    }else{
        //qDebug() << " CANT CONNECT ";
    }

    return output;
}

QByteArray blockchain_api::get_rawtxinfo_by_txid(QString txid, bool& http_response_successful){
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
    request_jsonobj.insert(QString("method"), QJsonValue(QString("getrawtransaction")));

    QJsonArray request_jsonarray = QJsonArray();
    request_jsonarray.insert(0, QJsonValue(txid));
    request_jsonarray.insert(1, QJsonValue((int)1));

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
                                QJsonObject jsonobj_result = jsonvalue.toObject();
                                QJsonValue jsonvalue_hex = jsonobj_result.value(QString("hex"));
                                QString raw_tx_representation = jsonvalue_hex.toString();

                                //set output
                                output.clear();
                                output.append(raw_tx_representation);

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
                        }
                    }

                    //If 500 internal server error, than there is no information on this tx, return true
                    if(response_line == QString("HTTP/1.1 500 Internal Server Error")){
                        //set output
                        output.clear();
                        output.append(QString(""));

                        //define output successful
                        http_response_successful = true;
                    }
                }
            }
        }
    }else{
        //qDebug() << "cannot connect to blockchain";
    }

    return output;
}

void blockchain_api::get_tx_list_info(QString raw_tx_hex, bool& http_response_successful){
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
    request_jsonobj.insert(QString("method"), QJsonValue(QString("decoderawtransaction")));

    QJsonArray request_jsonarray = QJsonArray();
    request_jsonarray.insert(0, QJsonValue(raw_tx_hex));

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
                            }
                        }
                    }
                }
            }
        }
    }
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
