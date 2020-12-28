#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QString configFile, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if(configFile.startsWith("--version") || configFile.startsWith("-version"))
    {
        qDebug() << "--- RTICOM PLAYER (V3.0) ---";
        qDebug() << "Version " + QString::number(VERSION, 'f', 1) + " (2020-04-01)";
        qDebug() << "From KIGAM KERC.";
        exit(1);
    }

    ui->setupUi(this);

    codec = QTextCodec::codecForName("utf-8");
    cfg.configFileName = configFile;
    readCFG();
    decorationGUI();

    cfg.monChanID = 3;
    isFilter = false;
    ui->viewFilterPB->setEnabled(false);
    connect(ui->viewFilterPB, SIGNAL(clicked()), this, SLOT(viewFilterPBClicked()));

    maxPBClicked = true;
    haveEQ = false;

    connect(ui->play1PB, SIGNAL(clicked()), this, SLOT(playPBClicked()));
    connect(ui->play2PB, SIGNAL(clicked()), this, SLOT(playPBClicked()));
    connect(ui->play4PB, SIGNAL(clicked()), this, SLOT(playPBClicked()));
    connect(ui->stopPB, SIGNAL(clicked()), this, SLOT(stopPBClicked()));
    connect(ui->maxPB, SIGNAL(clicked()), this, SLOT(drawMaxCircleOnMap()));
    connect(ui->eventLoadPB, SIGNAL(clicked()), this, SLOT(eventLoadPBClicked()));
    connect(ui->monChanCB, SIGNAL(currentIndexChanged(int)), this, SLOT(changeChannel(int)));
    playTimer = new QTimer;
    playTimer->stop();
    connect(playTimer, SIGNAL(timeout()), this, SLOT(doEventPlay()));
    connect(ui->slider, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

    QVariantMap params
    {
        {"osm.mapping.highdpi_tiles", true},
        {"osm.mapping.offline.directory", cfg.mapType}
    };

    realMapView = new QQuickView();
    realMapContainer = QWidget::createWindowContainer(realMapView, this);
    realMapView->setResizeMode(QQuickView::SizeRootObjectToView);
    realMapContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    realMapContainer->setFocusPolicy(Qt::TabFocus);
    realMapView->setSource(QUrl(QStringLiteral("qrc:/ViewMap.qml")));
    ui->realMapLO->addWidget(realMapContainer);
    realObj = realMapView->rootObject();

    stackedMapView = new QQuickView();
    stackedMapContainer = QWidget::createWindowContainer(stackedMapView, this);
    stackedMapView->setResizeMode(QQuickView::SizeRootObjectToView);
    stackedMapContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stackedMapContainer->setFocusPolicy(Qt::TabFocus);
    stackedMapView->setSource(QUrl(QStringLiteral("qrc:/ViewMap.qml")));
    ui->stackedMapLO->addWidget(stackedMapContainer);
    stackedObj = stackedMapView->rootObject();

    filtersta = new FilterSta(this);

    ui->slider->setRange(0, MAX_EVENT_DURATION-1);

    QMetaObject::invokeMethod(realObj, "initializePlugin", Q_ARG(QVariant, QVariant::fromValue(params)));
    QMetaObject::invokeMethod(stackedObj, "initializePlugin", Q_ARG(QVariant, QVariant::fromValue(params)));

    QObject::connect(this->realObj, SIGNAL(sendStationIndexSignal(QString)), this, SLOT(_qmlSignalfromReplayMap(QString)));
    QObject::connect(this->stackedObj, SIGNAL(sendStationIndexSignal(QString)), this, SLOT(_qmlSignalfromStackedReplayMap(QString)));

    QMetaObject::invokeMethod(this->realObj, "createRealTimeMapObject", Q_RETURN_ARG(QVariant, returnedValue));
    QMetaObject::invokeMethod(this->stackedObj, "createStackedMapObject", Q_RETURN_ARG(QVariant, returnedValue));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if( !QMessageBox::question( this,
                                codec->toUnicode("프로그램 종료"),
                                codec->toUnicode("프로그램을 종료합니다."),
                                codec->toUnicode("종료"),
                                codec->toUnicode("취소"),
                                QString::null, 1, 1) )
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::playPBClicked()
{
    QObject* pObject = sender();
    int speed = pObject->objectName().mid(4, 1).toInt();

    if(speed == 1)
    {
        playTimer->setInterval(1000);
        ui->play1PB->setStyleSheet("background-color: green");
        ui->play2PB->setStyleSheet("background-color: white");
        ui->play4PB->setStyleSheet("background-color: white");
    }
    else if(speed == 2)
    {
        playTimer->setInterval(500);
        ui->play1PB->setStyleSheet("background-color: white");
        ui->play2PB->setStyleSheet("background-color: green");
        ui->play4PB->setStyleSheet("background-color: white");
    }
    else if(speed == 4)
    {
        playTimer->setInterval(250);
        ui->play1PB->setStyleSheet("background-color: white");
        ui->play2PB->setStyleSheet("background-color: white");
        ui->play4PB->setStyleSheet("background-color: green");
    }

    if(!playTimer->isActive())
        playTimer->start();
}

void MainWindow::stopPBClicked()
{
    ui->play1PB->setStyleSheet("background-color: white");
    ui->play2PB->setStyleSheet("background-color: white");
    ui->play4PB->setStyleSheet("background-color: white");
    playTimer->stop();
}

void MainWindow::doEventPlay()
{
    int current = ui->slider->value();
    if(current >= event.duration-1)
        stopPBClicked();
    else
        ui->slider->setValue(current + 1);
}

void MainWindow::_qmlSignalfromReplayMap(QString indexS)
{
    QString msg, msg2;
    int index = indexS.toInt();
    msg = staList.at(index).sta.left(2) + "/" + staList.at(index).sta.mid(2) + "/" + staList.at(index).comment;

    if(staList.at(index).maxPGATime[cfg.monChanID] != 0)
    {
        QDateTime dt;
        dt.setTime_t(staList.at(index).maxPGATime[cfg.monChanID]);
        dt = convertKST(dt);

        msg2 = "     Maximum PGA Time:" + dt.toString("yyyy/MM/dd hh:mm:ss") + "     Maximum PGA:" +
                QString::number(staList.at(index).maxPGA[cfg.monChanID], 'f', 6);

        msg = msg + msg2;
    }

    ui->realStatusLB->setText( msg );
}

void MainWindow::_qmlSignalfromStackedReplayMap(QString indexS)
{
    QString msg, msg2;
    int index = indexS.toInt();
    msg = staList.at(index).sta.left(2) + "/" + staList.at(index).sta.mid(2) + "/" + staList.at(index).comment;

    if(staList.at(index).maxPGATime[cfg.monChanID] != 0)
    {
        QDateTime dt;
        dt.setTime_t(staList.at(index).maxPGATime[cfg.monChanID]);
        dt = convertKST(dt);

        msg2 = "     Maximum PGA Time:" + dt.toString("yyyy/MM/dd hh:mm:ss") + "     Maximum PGA:" +
                QString::number(staList.at(index).maxPGA[cfg.monChanID], 'f', 6);

        msg = msg + msg2;
    }

    ui->stackedStatusLB->setText( msg );
}

void MainWindow::eventLoadPBClicked()
{
    EventList *eventlist = new EventList(cfg.db_ip, cfg.db_name, cfg.db_user, cfg.db_passwd, this);
    connect(eventlist, SIGNAL(sendEventNameToMain(QString)), this, SLOT(rvEventName(QString)));
    eventlist->show();
}

void MainWindow::changeCircleOnMap(QString mapName, int staIndex, int width, QColor colorName, int opacity)
{
    if(mapName.startsWith("realMap"))
    {
        QMetaObject::invokeMethod(realObj, "changeSizeAndColorForStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, staIndex), Q_ARG(QVariant, width),
                                  Q_ARG(QVariant, colorName), Q_ARG(QVariant, opacity));

    }
    else if(mapName.startsWith("stackedMap"))
    {
        QMetaObject::invokeMethod(stackedObj, "changeSizeAndColorForStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, staIndex), Q_ARG(QVariant, width),
                                  Q_ARG(QVariant, colorName),
                                  Q_ARG(QVariant, opacity));
    }
}

void MainWindow::rvEventName(QString eventName)
{
    QDir evtDir(cfg.eventDir + "/" + eventName);
    if(!evtDir.exists())
    {
        QString temp = codec->toUnicode("데이터를 요청하고 있습니다.");
        QProgressDialog progress(temp, codec->toUnicode("취소"), 0, 2, this);
        progress.setWindowTitle(codec->toUnicode("RTICOM PLAYER"));
        progress.setMinimumWidth(700);
        progress.setWindowModality(Qt::WindowModal);
        progress.setValue(0);

        evtDir.mkpath(".");
        QProcess process;
        QString cmd = "scp kisstool@" + cfg.db_ip + ":/kisstool/data/EVENTS/" + eventName + ".tar.gz " + evtDir.path();
        qDebug() << cmd;
        process.start(cmd);
        process.waitForFinished(-1);
        cmd = "tar --strip-component=6 -xzf " + evtDir.path() + "/" + eventName.section("/", -1) + ".tar.gz -C " + evtDir.path();
        progress.setValue(1);
        process.start(cmd);
        process.waitForFinished(-1);
        progress.setValue(2);
    }

    evDir = evtDir.path();
    QString evDataFileName = evtDir.path() + "/SRC/PGA_G_B.dat";
    QString evDataTxtFileName = evtDir.path() + "/SRC/PGA_G.dat";
    QString evHeaderFileName = evtDir.path() + "/SRC/header.dat";
    QString evStaFileName = evtDir.path() + "/SRC/stalist_G.dat";

    ui->maxPB->setEnabled(true);
    ui->monChanCB->setEnabled(true);
    ui->slider->setEnabled(true);
    ui->play1PB->setEnabled(true);
    ui->play2PB->setEnabled(true);
    ui->play4PB->setEnabled(true);
    ui->stopPB->setEnabled(true);

    ui->slider->setValue(0);

    loadHeaderFromFile(evHeaderFileName);
    loadStationsFromFile(evStaFileName);

    if(!loadDataFromBinFile(evDataFileName))
        loadDataFromFile(evDataTxtFileName);
    loadStackedPGA();

    staList = filtersta->setup(staList, event);

    createStaCircleOnMap();
    cfg.monChanID = 3;
    findMaxStations();
    drawMaxCircleOnMap();

    fillEVInfo();
}

void MainWindow::viewFilterPBClicked()
{
    filtersta->show();
}

void MainWindow::readCFG()
{
    QFile file(cfg.configFileName);
    if(!file.exists())
    {
        qDebug() << "Failed configuration. Parameter file doesn't exists.";

        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("파라미터 파일이 존재하지 않습니다. (") + cfg.configFileName + ")\n" + codec->toUnicode("프로그램을 종료합니다."));
        msgBox.exec();
        exit(1);
    }
    if(file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            if(_line.startsWith(" ") || _line.startsWith("#"))
                continue;
            else if(_line.startsWith("HOME_DIR"))
            {
                cfg.homeDir = _line.section("=", 1, 1);
                cfg.logDir = cfg.homeDir + "/logs";
                cfg.eventDir = cfg.homeDir + "/events";
            }
            else if(_line.startsWith("DB_IP"))
                cfg.db_ip = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_NAME"))
                cfg.db_name = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_USERNAME"))
                cfg.db_user = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_PASSWD"))
                cfg.db_passwd = _line.section("=", 1, 1);

            else if(_line.startsWith("MAP_TYPE"))
            {
                if(_line.section("=", 1, 1).startsWith("satellite")) cfg.mapType = "/.RTICOM2/map_data/mapbox-satellite";
                else if(_line.section("=", 1, 1).startsWith("outdoors")) cfg.mapType = "/.RTICOM2/map_data/mapbox-outdoors";
                else if(_line.section("=", 1, 1).startsWith("light")) cfg.mapType = "/.RTICOM2/map_data/mapbox-light";
                else if(_line.section("=", 1, 1).startsWith("street")) cfg.mapType = "/.RTICOM2/map_data/osm_street";
            }
        }
        file.close();
    }
}

void MainWindow::loadStationsFromFile(QString staFileName)
{
    staList.clear();
    SSSSSList.clear();

    QFile evStaFile(staFileName);
    if(!evStaFile.exists())
    {
        qDebug() << "Stations file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("관측소 파일이 존재하지 않습니다. (") + staFileName + ")" );
        msgBox.exec();
        return;
    }

    if(evStaFile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&evStaFile);
        QString line, _line;
        int staIndex = 0;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();
            _STATION sta;
            sta.index = staIndex;
            staIndex++;
            sta.sta = _line.section("=", 0, 0);
            SSSSSList.append(_line.section("=", 0, 0));
            sta.lat = _line.section("=", 1, 1).toFloat();
            sta.lon = _line.section("=", 2, 2).toFloat();
            sta.elev = _line.section("=", 3, 3).toFloat();
            sta.comment = _line.section("=", 4, 4);

            for(int i=0;i<5;i++)
            {
                sta.maxPGA[i] = 0;
                sta.maxPGATime[i] = 0;
            }
            for(int i=0;i<MAX_EVENT_DURATION;i++)
            {
                for(int j=0;j<5;j++)
                {
                    sta.currentPGA[i][j] = 0;
                    sta.stackedPGA[i][j] = 0;
                }
            }
            if(_line.section("=", 5, 5).toInt() == 0)
                sta.isUsed = false;
            else
                sta.isUsed = true;

            sta.noData = true;
            sta.lastAlivedAfterFilter = false;

            if(haveEQ)
            {
                sta.distance = getDistance(sta.lat, sta.lon, event.eewLat, event.eewLon);
                sta.predictedHMAXPGA = getPredictedValue(sta.distance, event.eewMag);
            }
            else
                sta.distance = 0;
            staList.append(sta);
        }
        evStaFile.close();
    }
}

void MainWindow::loadHeaderFromFile(QString headerFileName)
{
    /*
    EVID=1523
    EVENT_TIME=1558902012
    DATA_START_TIME=1558902007
    DURATION=120
    EVENT_TYPE=E
    MAG_THRESHOLD=1.00
    EEW_INFO=19428:1558902012:36.1210:129.3485:1.56:3

EVENT_CONDITION=3:4:3.00:50
FIRST_EVENT_INFO=1558531133:KRSLG:역)서울역사:3.796869
FIRST_EVENT_INFO=1558531133:ACJJG:제주대학교아라):3.132810
FIRST_EVENT_INFO=1558531134:ARGOG:농)광주저수지:3.957200
FIRST_EVENT_INFO=1558531135:GKGBG:경기광백댐:3.223130
    */

    eventFirstInfo.clear();

    event.duration = MAX_EVENT_DURATION;

    QFile headerFile(headerFileName);
    if(!headerFile.exists())
    {
        qDebug() << "Header file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("이벤트 헤더 파일이 존재하지 않습니다. (") + headerFileName + ")" );
        msgBox.exec();
        return;
    }

    if(headerFile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&headerFile);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            if(_line.startsWith("EVID"))
                event.evid = _line.section("=", 1, 1).toInt();
            else if(_line.startsWith("EVENT_TIME"))
                event.eventEpochTime = _line.section("=", 1, 1).toInt();
            else if(_line.startsWith("DATA_START_TIME"))
                event.dataStartTime = _line.section("=", 1, 1).toInt();
            else if(_line.startsWith("EVENT_TYPE"))
                event.eventType = _line.section("=", 1, 1);
            else if(_line.startsWith("MAG_THRESHOLD"))
                event.thresholdM = _line.section("=", 1, 1).toFloat();
            else if(_line.startsWith("EVENT_CONDITION"))
            {
                event.inSeconds = _line.section("=", 1, 1).section(":", 0, 0).toInt();
                event.numStations = _line.section("=", 1, 1).section(":", 1, 1).toInt();
                event.thresholdG = _line.section("=", 1, 1).section(":", 2, 2).toFloat();
                event.distance = _line.section("=", 1, 1).section(":", 3, 3).toInt();
            }
            else if(_line.startsWith("EEW_INFO"))  // EEW_INFO=19428:1558902012:36.1210:129.3485:1.56:3
            {
                event.eewEvid = _line.section("=", 1, 1).section(":", 0, 0).toInt();
                event.eewOriginEpochTime = _line.section("=", 1, 1).section(":", 1, 1).toInt();
                event.eewLat = _line.section("=", 1, 1).section(":", 2, 2).toFloat();
                event.eewLon = _line.section("=", 1, 1).section(":", 3, 3).toFloat();
                event.eewMag = _line.section("=", 1, 1).section(":", 4, 4).toFloat();
                event.eewNsta = _line.section("=", 1, 1).section(":", 5, 5).toInt();
                QProcess process;
                QString cmd = cfg.homeDir + "/bin/" + find_loc_program + " " +
                        QString::number(event.eewLat, 'f', 4) + " " + QString::number(event.eewLon, 'f', 4);
                process.start(cmd);
                process.waitForFinished(-1); // will wait forever until finished

                QString stdout = process.readAllStandardOutput();
                int leng = stdout.length();
                event.eewLoc = stdout.left(leng-1);
            }
            else if(_line.startsWith("FIRST_EVENT_INFO"))
                eventFirstInfo.push_back(_line.section("=", 1, 1));
        }
        headerFile.close();
    }

    if(event.eventType.startsWith("E"))
    {
        haveEQ = true;
        ui->bFilterCB->setEnabled(true);
        ui->aFilterCB->setEnabled(true);
        ui->viewFilterPB->setEnabled(true);
    }
    else
    {
        haveEQ = false;
        ui->bFilterCB->setEnabled(false);
        ui->aFilterCB->setEnabled(false);
        ui->viewFilterPB->setEnabled(false);
    }
}

bool MainWindow::loadDataFromBinFile(QString dataFileName)
{
    QFile dataFile(dataFileName);
    if(!dataFile.exists())
    {
        qDebug() << "Data file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("데이터 파일이 존재하지 않습니다. (") + dataFileName + ")" );
        msgBox.exec();
        return false;
    }

    if(dataFile.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&dataFile);
        _QSCD_FOR_BIN qfb;

        while(!stream.atEnd())
        {
            int result = stream.readRawData((char *)&qfb, sizeof(_QSCD_FOR_BIN));

            QString mysta = QString(qfb.sta);
            float mypga[5] = {0x00,};
            mypga[0] = qfb.pga[0];
            mypga[1] = qfb.pga[1];
            mypga[2] = qfb.pga[2];
            mypga[3] = qfb.pga[3];
            mypga[4] = qfb.pga[4];
            int mytime = qfb.time;

            int staIndex = SSSSSList.indexOf(mysta);

            if(staIndex != -1)
            {
                _STATION sta;
                sta = staList.at(staIndex);

                int dataIndex = mytime - event.dataStartTime;
                if(dataIndex < 0 || dataIndex > MAX_EVENT_DURATION-1) continue;

                for(int i=0;i<5;i++)
                {
                    sta.currentPGA[dataIndex][i] = mypga[i];
                }

                if(dataIndex == 0)
                {
                    for(int i=0;i<5;i++)
                        sta.stackedPGA[dataIndex][i] = mypga[i];
                }

                for(int i=0;i<5;i++)
                {
                    if(sta.maxPGA[i] < mypga[i])
                    {
                        sta.maxPGA[i] = mypga[i];
                        sta.maxPGATime[i] = mytime;
                    }
                }

                sta.noData = false;

                staList.replace(staIndex, sta);
            }
        }
        dataFile.close();
    }

    return true;
}

void MainWindow::loadDataFromFile(QString dataFileName)
{
    QFile dataFile(dataFileName);
    if(!dataFile.exists())
    {
        qDebug() << "Data file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("데이터 파일이 존재하지 않습니다. (") + dataFileName + ")" );
        msgBox.exec();
        return;
    }

    if(dataFile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&dataFile);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            int mytime = _line.section(" ", 0, 0).toInt();
            QString mysta = _line.section("=", 0, 0).section(" ", 1, 1);
            float mypga[5] = {0x00,};
            mypga[0] = _line.section("=", 1, 1).section(":", 0, 0).toFloat();
            mypga[1] = _line.section("=", 1, 1).section(":", 1, 1).toFloat();
            mypga[2] = _line.section("=", 1, 1).section(":", 2, 2).toFloat();
            mypga[3] = _line.section("=", 1, 1).section(":", 3, 3).toFloat();
            mypga[4] = _line.section("=", 1, 1).section(":", 4, 4).toFloat();

            int staIndex = SSSSSList.indexOf(mysta);

            if(staIndex != -1)
            {
                _STATION sta;
                sta = staList.at(staIndex);

                int dataIndex = mytime - event.dataStartTime;
                if(dataIndex < 0 || dataIndex > MAX_EVENT_DURATION-1) continue;

                for(int i=0;i<5;i++)
                {
                    sta.currentPGA[dataIndex][i] = mypga[i];
                }

                if(dataIndex == 0)
                {
                    for(int i=0;i<5;i++)
                        sta.stackedPGA[dataIndex][i] = mypga[i];
                }

                for(int i=0;i<5;i++)
                {
                    if(sta.maxPGA[i] < mypga[i])
                    {
                        sta.maxPGA[i] = mypga[i];
                        sta.maxPGATime[i] = mytime;
                    }
                }

                sta.noData = false;

                staList.replace(staIndex, sta);
            }
        }
        dataFile.close();
    }
}

void MainWindow::loadStackedPGA()
{
    for(int i=0;i<staList.size();i++)
    {
        _STATION sta = staList.at(i);

        if(sta.noData || !sta.isUsed) continue;

        for(int j=1;j<MAX_EVENT_DURATION;j++)
        {
            for(int k=0;k<5;k++)
            {
                if(sta.currentPGA[j][k] > sta.stackedPGA[j-1][k])
                    sta.stackedPGA[j][k] = sta.currentPGA[j][k];
                else
                    sta.stackedPGA[j][k] = sta.stackedPGA[j-1][k];
                staList.replace(i, sta);
            }
        }
    }
}

void MainWindow::fillEVInfo()
{
    QDateTime t, tKST;
    t.setTime_t(event.eventEpochTime);
    tKST = convertKST(t);
    ui->eventTimeLB->setText(tKST.toString("yyyy-MM-dd hh:mm:ss") + "(KST)");

    ui->monChanCB->setCurrentIndex(cfg.monChanID);

    if(haveEQ)
    {
        ui->eventCondition1LB->hide();
        ui->eventCondition2LB->hide();
        ui->eventCondition3LB->hide();
        ui->eventCondition4LB->hide();
        ui->con2LB->hide();
        ui->con3LB->hide();
        ui->con4LB->hide();
        ui->eewFR->show();
        ui->galFR->hide();

        ui->con1LB->setText(codec->toUnicode("규모 ") + QString::number(event.thresholdM, 'f', 1) + codec->toUnicode(" 이상 조기경보 정보 수신 시"));

        ui->eqTimeLB->clear();
        ui->eqLocLB->clear();
        ui->eqLoc2LB->clear();
        ui->eqMagLB->clear();

        QDateTime eqTimeD, eqTimeDKST;
        eqTimeD.setTime_t(event.eewOriginEpochTime);
        eqTimeDKST = convertKST(eqTimeD);
        ui->eqTimeLB->setText(eqTimeDKST.toString("yyyy/MM/dd hh:mm:ss"));

        /*
        QProcess process;
        QString cmd = cfg.homeDir + "/bin/" + find_loc_program + " " +
                QString::number(event.eewLat, 'f', 4) + " " + QString::number(event.eewLon, 'f', 4);
        process.start(cmd);
        process.waitForFinished(-1); // will wait forever until finished

        QString stdout = process.readAllStandardOutput();
        int leng = stdout.length();
        event.eewLoc = stdout.left(leng-1);
        */

        ui->eqLocLB->setText(event.eewLoc);
        ui->eqLoc2LB->setText("LAT:"+QString::number(event.eewLat, 'f', 4) +
                              "    LON:"+QString::number(event.eewLon, 'f', 4));
        ui->eqMagLB->setText(QString::number(event.eewMag, 'f', 1));

        QString tempMag = QString::number(event.eewMag, 'f', 1);
        QMetaObject::invokeMethod(realObj, "showEEWStarMarker",
                                  Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, event.eewLat), Q_ARG(QVariant, event.eewLon),
                                  Q_ARG(QVariant, tempMag));
        QMetaObject::invokeMethod(stackedObj, "showEEWStarMarker",
                                  Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, event.eewLat), Q_ARG(QVariant, event.eewLon),
                                  Q_ARG(QVariant, tempMag));
    }
    else
    {
        ui->eventCondition1LB->show();
        ui->eventCondition2LB->show();
        ui->eventCondition3LB->show();
        ui->eventCondition4LB->show();
        ui->con2LB->show();
        ui->con3LB->show();
        ui->con4LB->show();
        ui->eewFR->hide();
        ui->galFR->show();

        ui->con1LB->setText(codec->toUnicode("초 안에, "));

        ui->eventCondition1LB->setText(QString::number(event.inSeconds));
        ui->eventCondition2LB->setText(QString::number(event.numStations));
        ui->eventCondition3LB->setText(QString::number(event.thresholdG, 'f', 1));
        ui->eventCondition4LB->setText(QString::number(event.distance));

        for(int j=0;j<MAX_NUM_EVENTINITSTA;j++)
        {
            eventNetLB[j]->clear();
            eventStaLB[j]->clear();
            eventComLB[j]->clear();
            eventTimeLB[j]->clear();
            eventPGALB[j]->clear();
        }

        //FIRST_EVENT_INFO=1558531133:KRSLG:역)서울역사:3.796869
        for(int j=0;j<eventFirstInfo.count();j++) // FIRST_EVENT_INFO=1551749617:GS:HJG:(가)합정관리소:48.789825
        {
            if(eventFirstInfo.at(j).section(":", 1, 1) != "")
            {
                eventNetLB[j]->setText(eventFirstInfo.at(j).section(":", 1, 1).left(2));
                eventStaLB[j]->setText(eventFirstInfo.at(j).section(":", 1, 1));
                eventComLB[j]->setText(eventFirstInfo.at(j).section(":", 2, 2));
                t.setTime_t(eventFirstInfo.at(j).section(":", 0, 0).toInt());
                tKST = convertKST(t);
                eventTimeLB[j]->setText(tKST.toString("hh:mm:ss"));
                eventPGALB[j]->setText(eventFirstInfo.at(j).section(":", 3, 3));
            }
        }
    }
}

void MainWindow::changeChannel(int monChan)
{
    cfg.monChanID = monChan; 

    findMaxStations();

    if(maxPBClicked)
        drawMaxCircleOnMap();
    else
        valueChanged(ui->slider->value());
}

void MainWindow::valueChanged(int value)
{
    maxPBClicked = false;

    ui->maxPB->setStyleSheet("background-color: white");

    int dataTime = event.dataStartTime + value;
    int dataIndex = value;

    QDateTime time, timeKST;
    time.setTime_t(dataTime);
    timeKST = convertKST(time);

    ui->dataTimeLCD->display(timeKST.toString("hh:mm:ss"));

    if(haveEQ)
    {
        int diffTime = dataTime - event.eewOriginEpochTime;
        if(diffTime > 0)
        {
            double radiusP = diffTime * P_VEL * 1000;
            double radiusS = diffTime * filtersta->pgaVel * 1000;

            QMetaObject::invokeMethod(this->realObj, "showCircleForAnimation",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, event.eewLat), Q_ARG(QVariant, event.eewLon),
                                      Q_ARG(QVariant, radiusP), Q_ARG(QVariant, radiusS), Q_ARG(QVariant, 1));
            QMetaObject::invokeMethod(this->stackedObj, "showCircleForAnimation",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, event.eewLat), Q_ARG(QVariant, event.eewLon),
                                      Q_ARG(QVariant, radiusP), Q_ARG(QVariant, radiusS), Q_ARG(QVariant, 1));
        }
    }

    int realstanum = 0;

    for(int i=0;i<staList.size();i++)
    {
        _STATION sta = staList.at(i);

        if(!sta.isUsed || sta.noData) continue;

        float cpga = sta.currentPGA[dataIndex][cfg.monChanID];
        float spga = sta.stackedPGA[dataIndex][cfg.monChanID];

        if(isFilter)
        {
            if(!sta.lastAlivedAfterFilter)
            {
                changeCircleOnMap("realMap", sta.index, CIRCLE_SIZE, "white", 0);
                changeCircleOnMap("stackedMap", sta.index, CIRCLE_SIZE, "white", 0);
                continue;
            }
        }

        if(cpga != 0)
        {
            if(cpga >= 9.9 && cpga <= 10.1)
            {
                changeCircleOnMap("realMap", sta.index, getCircleSize(cpga), QColor(255, 0, 0), 1);
                realstanum++;
            }
            else
            {
                changeCircleOnMap("realMap", sta.index, getCircleSize(cpga),
                                  QColor(redColor(cpga),
                                    greenColor(cpga),
                                    blueColor(cpga)), 1);
                realstanum++;
            }
        }
        else
        {
            changeCircleOnMap("realMap", sta.index, CIRCLE_SIZE, "white", 1);
        }

        if(spga >= 9.9 && spga <= 10.1)
        {
            changeCircleOnMap("stackedMap", sta.index, getCircleSize(spga), QColor(255, 0, 0), 1);
        }
        else
        {
            changeCircleOnMap("stackedMap", sta.index, getCircleSize(spga),
                              QColor(redColor(spga),
                                greenColor(spga),
                                blueColor(spga)), 1);
        }
    }

    ui->staNumLB->setText(QString::number(realstanum));
}

void MainWindow::findMaxStations()
{
    ui->eventMaxPGATW->setRowCount(0);
    ui->eventMaxPGATW->setSortingEnabled(false);

    for(int i=0;i<staList.size();i++)
    {
        _STATION sta = staList.at(i);

        if(!sta.isUsed || sta.noData) continue;

        if(isFilter)
        {
            if(!sta.lastAlivedAfterFilter)
                continue;
        }

        QDateTime t;
        t.setTime_t(sta.maxPGATime[cfg.monChanID]);
        t = convertKST(t);

        ui->eventMaxPGATW->setRowCount(ui->eventMaxPGATW->rowCount()+1);
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 0, new QTableWidgetItem(sta.sta));

        if(haveEQ)
        {
            QTableWidgetItem *distItem = new QTableWidgetItem;
            distItem->setData(Qt::EditRole, staList.at(i).distance);
            ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 1, distItem);
        }
        else
            ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 1, new QTableWidgetItem("nan"));

        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 2, new QTableWidgetItem(sta.comment));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 3, new QTableWidgetItem(t.toString("hh:mm:ss")));

        QTableWidgetItem *pgaItem = new QTableWidgetItem;
        pgaItem->setData(Qt::EditRole, myRound(sta.maxPGA[cfg.monChanID], 6));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 4, pgaItem);
    }

    for(int j=0;j<ui->eventMaxPGATW->rowCount();j++)
    {
        for(int k=0;k<ui->eventMaxPGATW->columnCount();k++)
        {
            ui->eventMaxPGATW->item(j, k)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->eventMaxPGATW->setSortingEnabled(true);

    if(haveEQ)
        ui->eventMaxPGATW->sortByColumn(1, Qt::AscendingOrder);
    else
        ui->eventMaxPGATW->sortByColumn(4, Qt::DescendingOrder);
}

void MainWindow::createStaCircleOnMap()
{
    // create staCircle
    QMetaObject::invokeMethod(realObj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));
    QMetaObject::invokeMethod(stackedObj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));

    // make all station circle
    for(int i=0;i<staList.size();i++)
    {
        _STATION sta = staList.at(i);
        QMetaObject::invokeMethod(this->realObj, "createStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, sta.index), Q_ARG(QVariant, sta.lat),
                                  Q_ARG(QVariant, sta.lon), Q_ARG(QVariant, CIRCLE_SIZE), Q_ARG(QVariant, "white"),
                                  Q_ARG(QVariant, sta.sta.left(2) + "/" + sta.sta.mid(2)),
                                  Q_ARG(QVariant, 0));

        QMetaObject::invokeMethod(this->stackedObj, "createStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, sta.index), Q_ARG(QVariant, sta.lat),
                                  Q_ARG(QVariant, sta.lon), Q_ARG(QVariant, CIRCLE_SIZE), Q_ARG(QVariant, "white"),
                                  Q_ARG(QVariant, sta.sta.left(2) + "/" + sta.sta.mid(2)),
                                  Q_ARG(QVariant, 0));
    }
}

int MainWindow::getCircleSize(float pga)
{
    int size;
    if(pga < 0.2)
        size = 12;
    else if(pga < 0.5)
        size = 14;
    else if(pga < 1.0)
        size = 16;
    else if(pga < 2.0)
        size = 18;
    else if(pga < 5.0)
        size = 20;
    else
        size =21;

    return size;
}

void MainWindow::drawMaxCircleOnMap()
{
    maxPBClicked = true;
    ui->maxPB->setStyleSheet("background-color: green");
    ui->play1PB->setStyleSheet("background-color: white");
    ui->play2PB->setStyleSheet("background-color: white");
    ui->play4PB->setStyleSheet("background-color: white");
    ui->stopPB->setStyleSheet("background-color: white");

    for(int i=0;i<staList.size();i++)
    {
        _STATION sta = staList.at(i);
        if(!sta.isUsed || sta.noData) continue;

        float mypga = sta.maxPGA[cfg.monChanID];

        if(sta.maxPGA[cfg.monChanID] < 0.2)

        if(isFilter)
        {
            if(!sta.lastAlivedAfterFilter)
            {
                changeCircleOnMap("realMap", sta.index, CIRCLE_SIZE, "white", 0);
                changeCircleOnMap("stackedMap", sta.index, CIRCLE_SIZE, "white", 0);
                continue;
            }
        }

        if(mypga >= 9.9 && mypga <= 10.1)
        {
            changeCircleOnMap("realMap", sta.index, getCircleSize(mypga), QColor(255, 0, 0), 1);
            changeCircleOnMap("stackedMap", sta.index, getCircleSize(mypga), QColor(255, 0, 0), 1);
        }
        else
        {
            changeCircleOnMap("realMap", sta.index, getCircleSize(mypga),
                              QColor(redColor(mypga),
                                greenColor(mypga),
                                blueColor(mypga)), 1);
            changeCircleOnMap("stackedMap", sta.index, getCircleSize(mypga),
                              QColor(redColor(mypga),
                                greenColor(mypga),
                                blueColor(mypga)), 1);
        }
    }
}

void MainWindow::filterChanged(int state)
{
    if(conBG->checkedId() == 0)
        isFilter = false;
    else
        isFilter = true;

    findMaxStations();

    if(maxPBClicked)
        drawMaxCircleOnMap();
    else
        valueChanged(ui->slider->value());
}

void MainWindow::decorationGUI()
{
    conBG = new QButtonGroup(this);
    conBG->addButton(ui->bFilterCB); conBG->setId(ui->bFilterCB, 0);
    conBG->addButton(ui->aFilterCB); conBG->setId(ui->aFilterCB, 1);

    connect(ui->bFilterCB, SIGNAL(stateChanged(int)), this, SLOT(filterChanged(int)));

    ui->bFilterCB->setEnabled(false);
    ui->aFilterCB->setEnabled(false);
    ui->viewFilterPB->setEnabled(false);

    QPixmap titlePX("/.RTICOM2/images/RTICOM2PlayerLogo.png");
    ui->titleLB->setPixmap(titlePX.scaled(ui->titleLB->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    for(int i=0;i<MAX_NUM_EVENTINITSTA;i++)
    {
        eventNetLB[i] = new QLabel;
        eventNetLB[i]->setFixedSize(65, 21);
        eventNetLB[i]->setAlignment(Qt::AlignCenter);
        eventNetLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventNetLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventNetLB[i], i+1, 0);
        eventStaLB[i] = new QLabel;
        eventStaLB[i]->setFixedSize(70, 21);
        eventStaLB[i]->setAlignment(Qt::AlignCenter);
        eventStaLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventStaLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventStaLB[i], i+1, 1);
        eventComLB[i] = new QLabel;
        eventComLB[i]->setFixedSize(163, 21);
        eventComLB[i]->setAlignment(Qt::AlignCenter);
        eventComLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventComLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventComLB[i], i+1, 2);
        eventTimeLB[i] = new QLabel;
        eventTimeLB[i]->setFixedSize(80, 21);
        eventTimeLB[i]->setAlignment(Qt::AlignCenter);
        eventTimeLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventTimeLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventTimeLB[i], i+1, 3);
        eventPGALB[i] = new QLabel;
        eventPGALB[i]->setFixedSize(80, 21);
        eventPGALB[i]->setAlignment(Qt::AlignCenter);
        eventPGALB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventPGALB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventPGALB[i], i+1, 4);
    }

    ui->eventMaxPGATW->setColumnWidth(0, 70);
    ui->eventMaxPGATW->setColumnWidth(1, 90);
    ui->eventMaxPGATW->setColumnWidth(2, 140);
    ui->eventMaxPGATW->setColumnWidth(3, 70);

    QLabel *legendLE[47];
    float pga = 0;

    for(int i=0;i<47;i++)
    {
        legendLE[i] = new QLabel;
        legendLE[i]->setFrameShape(QFrame::NoFrame);

        if(i < 10)
            pga = pga + 0.01;
        else if(i < 19)
            pga = pga + 0.1;
        else if(i < 28)
            pga = pga + 1;
        else
            pga = pga + 10;


        QString sheet = "background-color: rgb(" + QString::number(redColor(pga)) +
                                             "," + QString::number(greenColor(pga)) +
                                             "," + QString::number(blueColor(pga)) + ");";

        legendLE[i]->setStyleSheet(sheet);

        ui->legendLO->addWidget(legendLE[i]);
    }
}
