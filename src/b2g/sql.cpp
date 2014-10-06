#include "sql.h"

sql::sql(QObject *parent) :
    QObject(parent)
{
    //We are simulating a database for now, but we don't want to lose our progress so we keep the lines below.
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setPort(5432);
    db.setDatabaseName("gatewayd_db");
    db.setUserName("gatewayd_user");
    db.setPassword("mkr5UyTnbG");
    bool ok = db.open();
    qDebug() << "DB: " << ok;
}
