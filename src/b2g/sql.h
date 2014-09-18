#ifndef SQL_H
#define SQL_H

#include <QDebug>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlDriver>

class sql : public QObject
{
    Q_OBJECT
public:
    explicit sql(QObject *parent = 0);

signals:

public slots:

};

#endif // SQL_H
