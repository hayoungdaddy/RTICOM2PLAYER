#ifndef EVENTLIST_H
#define EVENTLIST_H

#include <QDialog>
#include <QDir>
#include <QDateTime>
#include <QDebug>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QSqlError>

#include <QButtonGroup>

namespace Ui {
class EventList;
}

class EventList : public QDialog
{
    Q_OBJECT

public:
    explicit EventList(QString db_ip, QString db_name, QString db_user, QString db_passwd, QWidget *parent = nullptr);
    ~EventList();

private:
    Ui::EventList *ui;

    QButtonGroup *conBG;

    void openDB();
    QSqlDatabase mydb;
    QSqlQueryModel *eventModel;

    void getEventList();

private slots:
    void researchPBClicked(bool);
    void twDoubleClicked(int, int);

signals:
    void sendEventNameToMain(QString);
};

#endif // EVENTLIST_H
