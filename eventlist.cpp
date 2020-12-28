#include "eventlist.h"
#include "ui_eventlist.h"

EventList::EventList(QString db_ip, QString db_name, QString db_user, QString db_passwd, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventList)
{
    ui->setupUi(this);

    connect(ui->eventListTW, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(twDoubleClicked(int,int)));

    mydb = QSqlDatabase::addDatabase("QMYSQL");
    mydb.setHostName(db_ip);
    mydb.setDatabaseName(db_name);
    mydb.setUserName(db_user);
    mydb.setPassword(db_passwd);

    eventModel = new QSqlQueryModel();

    conBG = new QButtonGroup(this);
    conBG->addButton(ui->numEventCB); conBG->setId(ui->numEventCB, 0);
    conBG->addButton(ui->numDayCB); conBG->setId(ui->numDayCB, 1);
    conBG->addButton(ui->rangeDayCB); conBG->setId(ui->rangeDayCB, 2);
    conBG->addButton(ui->rangeMagCB); conBG->setId(ui->rangeMagCB, 3);

    ui->eDayDE->setDate(QDate::currentDate());
    ui->sDayDE->setDate(QDate::currentDate().addDays(-5));
    ui->sMagLE->setText("3.0");
    ui->eMagLE->setText("6.0");

    ui->eventListTW->setColumnWidth(0, 60);
    ui->eventListTW->setColumnWidth(1, 180);
    ui->eventListTW->setColumnWidth(2, 60);
    ui->eventListTW->setColumnWidth(3, 60);
    ui->eventListTW->setColumnWidth(4, 110);
    ui->eventListTW->setColumnWidth(5, 80);
    ui->eventListTW->setColumnWidth(6, 80);
    ui->eventListTW->setColumnWidth(7, 80);

    getEventList();

    connect(ui->reSearchPB, SIGNAL(clicked(bool)), this, SLOT(researchPBClicked(bool)));
}

EventList::~EventList()
{
    delete ui;
}

void EventList::openDB()
{
    mydb.open();

    qDebug() << "Connect to the Database (" + mydb.hostName() + " (" + mydb.databaseName() + "))";

    if(!mydb.open())
    {
        qDebug() << "Error connecting to DB: " + mydb.lastError().text();
    }
}

void EventList::researchPBClicked(bool)
{
    getEventList();
}

void EventList::getEventList()
{
    ui->eventListTW->setRowCount(0);

    QString query;
    openDB();
    query = "SELECT * FROM EVENT ";

    int option = conBG->checkedId();

    if(option == 0)
    {
        if(ui->votedOnlyCB->isChecked())
            query = query + "WHERE vote=1 ORDER BY evid DESC LIMIT " + ui->numEventSB->text();
        else
            query = query + "ORDER BY evid DESC LIMIT " + ui->numEventSB->text();
    }
    else if(option == 1)
    {
        QDate day = QDate::currentDate();
        day = day.addDays(- ui->numDaySB->text().toInt());
        query = query + "WHERE lddate >= '" + day.toString("yyyy-MM-dd") + "' ORDER BY evid DESC";
    }
    else if(option == 2)
    {
        QDate sday, eday;
        sday = ui->sDayDE->date();
        eday = ui->eDayDE->date();
        query = query + "WHERE lddate <= '" + eday.toString("yyyy-MM-dd") + "' and lddate >= '" + sday.toString("yyyy-MM-dd") + "' ORDER BY evid DESC";
    }
    else if(option == 3)
    {
        QString smag, emag;
        smag = ui->sMagLE->text();
        emag = ui->eMagLE->text();

        if(ui->mVotedOnlyCB->isChecked())
            query = query + "WHERE mag <= " + emag + " and mag >= " + smag + " and vote=1 ORDER BY evid DESC";
        else
            query = query + "WHERE mag <= " + emag + " and mag >= " + smag + " ORDER BY evid DESC";
    }

    eventModel->setQuery(query);

    for(int i=0;i<eventModel->rowCount();i++)
    {
        ui->eventListTW->setRowCount(ui->eventListTW->rowCount()+1);
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 0, new QTableWidgetItem(eventModel->record(i).value("evid").toString()));
        QString time = eventModel->record(i).value("eventtime").toString();
        time.replace("T", " ");
        time.replace(".000", "");
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 1, new QTableWidgetItem(time));
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 2, new QTableWidgetItem(eventModel->record(i).value("eventtype").toString()));

        QString eewevid = eventModel->record(i).value("eewevid").toString();
        if(eewevid.startsWith("0")) eewevid = "-";
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 3, new QTableWidgetItem(eewevid));

        QString origintime = eventModel->record(i).value("origintime").toString();
        if(origintime.startsWith("0")) origintime = "-";
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 4, new QTableWidgetItem(origintime));

        QString lat = eventModel->record(i).value("lat").toString();
        if(lat.startsWith("0")) lat = "-";
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 5, new QTableWidgetItem(lat));

        QString lon = eventModel->record(i).value("lon").toString();
        if(lon.startsWith("0")) lon = "-";
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 6, new QTableWidgetItem(lon));

        QString mag = eventModel->record(i).value("mag").toString();
        if(mag.startsWith("0")) mag = "-";
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 7, new QTableWidgetItem(mag));

        QString vote = eventModel->record(i).value("vote").toString();
        if(vote.startsWith("1"))
            vote = "YES";
        else vote = "NO";
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 8, new QTableWidgetItem(vote));

        if(vote.startsWith("YES"))
            ui->eventListTW->item(ui->eventListTW->rowCount()-1, 8)->setBackground(Qt::green);

        QString eventname = eventModel->record(i).value("eventname").toString();
        if(eventname.startsWith(" ")) eventname = "  -";
        ui->eventListTW->setItem(ui->eventListTW->rowCount()-1, 9, new QTableWidgetItem("  " + eventname));
    }

    for(int j=0;j<ui->eventListTW->rowCount();j++)
    {
        for(int k=0;k<ui->eventListTW->columnCount()-1;k++)
        {
            ui->eventListTW->item(j, k)->setTextAlignment(Qt::AlignCenter);
        }
    }
}

void EventList::twDoubleClicked(int row, int col)
{
    QString evidS = ui->eventListTW->item(row, 0)->text();
    QString timeS = ui->eventListTW->item(row, 1)->text();
    QDateTime tt = QDateTime::fromString(timeS, "yyyy-MM-dd hh:mm:ss");

    emit sendEventNameToMain(tt.toString("/yyyy/MM/") + evidS);
    accept();
}
