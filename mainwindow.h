#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QQuickView>
#include <QtQuick>
#include <QtConcurrent>

#include <QTimer>
#include <QProgressDialog>
#include <QProcess>

#include "common.h"
#include "eventlist.h"
#include "filtersta.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString configFile = nullptr, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

protected:

private:
    Ui::MainWindow *ui;

    QTextCodec *codec;
    QTimer *playTimer;

    FilterSta *filtersta;

    QButtonGroup *conBG;
    bool isFilter;

    _CONFIGURE cfg;
    void readCFG();
    void decorationGUI();

    _EVENT event;

    QList<_STATION> staList;
    QList<QString> SSSSSList;

    QVector<QString> eventFirstInfo;

    bool haveEQ;
    QString evDir;

    void loadStationsFromFile(QString);
    void loadDataFromFile(QString);
    bool loadDataFromBinFile(QString);
    void loadHeaderFromFile(QString);
    void loadStackedPGA();
    void fillEVInfo();
    void findMaxStations();

    QQuickView *realMapView;
    QQuickView *stackedMapView;
    QObject *realObj;
    QWidget *realMapContainer;
    QObject *stackedObj;
    QWidget *stackedMapContainer;
    QVariant returnedValue;

    void createStaCircleOnMap();
    void changeCircleOnMap(QString, int, int, QColor, int);
    int getCircleSize(float);

    bool maxPBClicked;

    QLabel *eventNetLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventStaLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventComLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventTimeLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventPGALB[MAX_NUM_EVENTINITSTA];

private slots:
    void doEventPlay();
    void eventLoadPBClicked();
    void rvEventName(QString);
    void changeChannel(int);
    void valueChanged(int);

    void drawMaxCircleOnMap();

    void _qmlSignalfromReplayMap(QString);
    void _qmlSignalfromStackedReplayMap(QString);
    void playPBClicked();
    void stopPBClicked();

    void filterChanged(int);
    void viewFilterPBClicked();
};

#endif // MAINWINDOW_H
