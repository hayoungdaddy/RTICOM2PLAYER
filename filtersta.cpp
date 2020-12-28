#include "filtersta.h"
#include "ui_filtersta.h"

using namespace QtCharts;

FilterSta::FilterSta(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterSta)
{
    ui->setupUi(this);

    codec = QTextCodec::codecForName("utf-8"); 
}

FilterSta::~FilterSta()
{
    delete ui;
}

QList<_STATION> FilterSta::setup(QList<_STATION> myStaList, _EVENT myEvent)
{
    staList = myStaList;
    event = myEvent;

    ui->eqInfoTB->setRowCount(0);
    ui->eqInfoTB->setSortingEnabled(false);
    ui->eqInfoTB->setRowCount(1);

    ui->eqInfoTB->setItem(0, 0, new QTableWidgetItem(QString::number(event.evid)));

    QDateTime t, tKST;
    t.setTime_t(event.eewOriginEpochTime);
    tKST = convertKST(t);
    ui->eqInfoTB->setItem(0, 1, new QTableWidgetItem(tKST.toString("yyyy-MM-dd hh:mm:ss")));

    ui->eqInfoTB->setItem(0, 2, new QTableWidgetItem(QString::number(event.eewEvid)));
    ui->eqInfoTB->setItem(0, 3, new QTableWidgetItem(QString::number(event.eewLat, 'f', 4)));
    ui->eqInfoTB->setItem(0, 4, new QTableWidgetItem(QString::number(event.eewLon, 'f', 4)));
    ui->eqInfoTB->setItem(0, 5, new QTableWidgetItem(QString::number(event.eewMag, 'f', 1)));
    ui->eqInfoTB->setItem(0, 6, new QTableWidgetItem(event.eewLoc));

    ui->eqInfoTB->setColumnWidth(1, 200);

    for(int k=0;k<ui->eqInfoTB->columnCount();k++)
    {
        ui->eqInfoTB->item(0, k)->setTextAlignment(Qt::AlignCenter);
    }

    int nUsed = drawDistTimeCurve();
    drawDistPGACurveUsingGMPE(nUsed);

    return staList;
}

void FilterSta::drawDistPGACurveUsingGMPE(int nTotal)
{
    QLayoutItem *child;
    while ((child = ui->eqGraph2LO->takeAt(0)) != nullptr) {
        delete child;
    }

    int nUsed = 0;

    QChartView *distPGACurveCV = new QChartView();
    QChart *distPGACurveC = new QChart();

    QList<_STATION> outBaundStaList;

    QScatterSeries *realDataSeries = new QScatterSeries();
    QScatterSeries *outDataSeries = new QScatterSeries();
    QLineSeries *predDataSeries = new QLineSeries();
    QLineSeries *predDataSeriesUp = new QLineSeries();
    QLineSeries *predDataSeriesDown = new QLineSeries();

    realDataSeries->setName("Alived Stations");
    realDataSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    realDataSeries->setMarkerSize(10.0);

    outDataSeries->setName("Out Baund");
    outDataSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    outDataSeries->setMarkerSize(10.0);

    predDataSeries->setName("Pred.");
    predDataSeriesUp->setName("Pred.(+" +
                            QString::number(REGRESSION_PERCENT*100) + "%)");
    predDataSeriesDown->setName("Pred.(-" +
                            QString::number(REGRESSION_PERCENT*100) + "%)");

    QVector<double> predVector;
    QVector<double> predVectorUp;
    QVector<double> predVectorDown;

    for(int i=1;i<=THRESHOLD_FOR_DIST;i++)
    {
        predVector.push_back(getPredictedValue(i, event.eewMag));
        predVectorUp.push_back(predVector.at(i-1) + (predVector.at(i-1)*REGRESSION_PERCENT));
        predVectorDown.push_back(predVector.at(i-1) - (predVector.at(i-1)*REGRESSION_PERCENT));
    }

    for(int i=0;i<THRESHOLD_FOR_DIST;i++)
    {
        predDataSeries->append(i+1, predVector.at(i));
        predDataSeriesUp->append(i+1, predVectorUp.at(i));
        predDataSeriesDown->append(i+1, predVectorDown.at(i));
    }

    for(int i=0;i<staList.size();i++)
    {
        _STATION sta = staList.at(i);

        if(!sta.lastAlivedAfterFilter) continue;

        double upBoundPGA = predVectorUp.at((int)sta.distance-1);
        double downBoundPGA = predVectorDown.at((int)sta.distance-1);

        if(sta.maxPGA[3] <= upBoundPGA && sta.maxPGA[3] >= downBoundPGA)
        {
            realDataSeries->append(sta.distance, sta.maxPGA[3]);
            nUsed++;
        }
        else
        {
            outDataSeries->append(sta.distance, sta.maxPGA[3]);
            outBaundStaList.append(sta);
            sta.lastAlivedAfterFilter = false;
            staList.replace(i, sta);
        }
    }

    ui->total4LB->setText(QString::number(nTotal));
    ui->error4LB->setText(QString::number(outBaundStaList.size()));
    ui->nused4LB->setText(QString::number(nUsed));

    distPGACurveC->addSeries(realDataSeries);
    distPGACurveC->addSeries(outDataSeries);
    distPGACurveC->addSeries(predDataSeries);
    distPGACurveC->addSeries(predDataSeriesUp);
    distPGACurveC->addSeries(predDataSeriesDown);

    QLogValueAxis *axisLogX = new QLogValueAxis();
    axisLogX->setTitleText("Distance (Km)");
    axisLogX->setLabelFormat("%i");
    axisLogX->setMinorTickCount(-1);

    QLogValueAxis *axisLogY = new QLogValueAxis();
    axisLogY->setTitleText("PGA (gal)");
    axisLogY->setLabelFormat("%f");
    axisLogY->setMinorTickCount(-1);

    distPGACurveC->addAxis(axisLogX, Qt::AlignBottom);
    realDataSeries->attachAxis(axisLogX);
    outDataSeries->attachAxis(axisLogX);
    predDataSeries->attachAxis(axisLogX);
    predDataSeriesUp->attachAxis(axisLogX);
    predDataSeriesDown->attachAxis(axisLogX);

    distPGACurveC->addAxis(axisLogY, Qt::AlignLeft);
    realDataSeries->attachAxis(axisLogY);
    outDataSeries->attachAxis(axisLogY);
    predDataSeries->attachAxis(axisLogY);
    predDataSeriesUp->attachAxis(axisLogY);
    predDataSeriesDown->attachAxis(axisLogY);

    distPGACurveCV->setRenderHint(QPainter::Antialiasing);
    distPGACurveCV->setChart(distPGACurveC);
    ui->eqGraph2LO->addWidget(distPGACurveCV);
}

int FilterSta::drawDistTimeCurve()
{
    QLayoutItem *child;
    while ((child = ui->eqGraph1LO->takeAt(0)) != nullptr) {
        delete child;
    }

    int nUsed = 0;

    QChartView *distTimeCurveCV = new QChartView();
    QChart *distTimeCurveC = new QChart();

    QList<_STATION> notUsedStaList;
    QList<_STATION> noDataStaList;
    QList<_STATION> errorInfoStaList;
    QList<_STATION> outDistanceStaList;
    QList<_STATION> outVelStaList;

    QScatterSeries *realDataSeries = new QScatterSeries();
    QScatterSeries *outVelSeries = new QScatterSeries();
    QLineSeries *maxWaveSeries = new QLineSeries();
    QLineSeries *minWaveSeries = new QLineSeries();
    QLineSeries *linearRegressionSeries = new QLineSeries();

    realDataSeries->setName("Alived Stations");
    realDataSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    realDataSeries->setMarkerSize(10.0);

    outVelSeries->setName("Out Baund");
    outVelSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    outVelSeries->setMarkerSize(10.0);

    maxWaveSeries->setName("MAX Velocity (" + QString::number(MAX_VELOCITY, 'f', 1) + "Km/s)");
    minWaveSeries->setName("MIN Velocity (" + QString::number(MIN_VELOCITY, 'f', 1) + "Km/s)");

    for(int i=0;i<staList.size();i++)
    {
        _STATION sta = staList.at(i);
        int difftime = sta.maxPGATime[3] - event.eewOriginEpochTime;

        if(sta.isUsed == false)
        {
            notUsedStaList.append(sta);
        }
        else if(sta.noData == true)
        {
            noDataStaList.append(sta);
        }
        else if(sta.distance >= 1000)
            errorInfoStaList.append(sta);
        else if(sta.distance > THRESHOLD_FOR_DIST)
            outDistanceStaList.append(sta);
        else
        {
            float maxSec = sta.distance / MAX_VELOCITY;
            float minSec = sta.distance / MIN_VELOCITY;

            if(sta.distance <= VALID_DIST)
            {
                realDataSeries->append(sta.distance, difftime);
                sta.lastAlivedAfterFilter = true;
                nUsed++;
                staList.replace(i, sta);
            }
            else if(difftime > maxSec && difftime < minSec)
            {
                realDataSeries->append(sta.distance, difftime);
                sta.lastAlivedAfterFilter = true;
                nUsed++;
                staList.replace(i, sta);
            }
            else
            {
                outVelSeries->append(sta.distance, difftime);
                outVelStaList.append(sta);
            }
        }
    }

    QVector<double> x;
    QVector<double> y;

    for(int i=0;i<realDataSeries->count();i++)
    {
        x.push_back(realDataSeries->at(i).x());
        y.push_back(realDataSeries->at(i).y());
    }

    double m, b, r;
    linregVector(realDataSeries->count(), x, y, &m, &b, &r);  // (m*x)+b = regression

    ui->slopeLB->setText(QString::number(m, 'f', 5));
    ui->interceptLB->setText(QString::number(b, 'f', 5));
    ui->coeffLB->setText(QString::number(r, 'f', 5));

    linearRegressionSeries->append(1, (m*1)+b);
    linearRegressionSeries->append(THRESHOLD_FOR_DIST, (m*THRESHOLD_FOR_DIST)+b);

    pgaVel = 1 / (((m*5)+b) - ((m*4)+b)); // if x = 5
    ui->pgaVelLB->setText(QString::number(pgaVel, 'f', 2) + "Km/s");
    linearRegressionSeries->setName("Linear Regression (" + QString::number(pgaVel, 'f', 2) + "Km/s)");

    maxWaveSeries->append(VALID_DIST, VALID_DIST / MAX_VELOCITY);
    maxWaveSeries->append(THRESHOLD_FOR_DIST, THRESHOLD_FOR_DIST / MAX_VELOCITY);
    minWaveSeries->append(VALID_DIST, VALID_DIST / MIN_VELOCITY);
    minWaveSeries->append(THRESHOLD_FOR_DIST, THRESHOLD_FOR_DIST / MIN_VELOCITY);

    ui->total1LB->setText(QString::number(staList.size()));
    ui->notUsedLB->setText(QString::number(notUsedStaList.size()));
    ui->noDataLB->setText(QString::number(noDataStaList.size()));
    ui->nErrorLB->setText(QString::number(errorInfoStaList.size()));
    ui->nOverDistLB->setText(QString::number(outDistanceStaList.size()));
    ui->nOutVelLB->setText(QString::number(outVelStaList.size()));
    ui->used1LB->setText(QString::number(nUsed));

    distTimeCurveC->addSeries(realDataSeries);
    distTimeCurveC->addSeries(outVelSeries);
    distTimeCurveC->addSeries(maxWaveSeries);
    distTimeCurveC->addSeries(minWaveSeries);
    distTimeCurveC->addSeries(linearRegressionSeries);

    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();
    axisX->setTitleText("DISTANCE (Km)");
    axisY->setTitleText("TIME (s)");
    axisX->setLabelFormat("%i");
    axisY->setLabelFormat("%i");

    axisX->setRange(0, THRESHOLD_FOR_DIST);
    axisY->setRange(0, THRESHOLD_FOR_DIST / MIN_VELOCITY);
    axisX->setTickCount(THRESHOLD_FOR_DIST/50 + 1);
    axisY->setTickCount((THRESHOLD_FOR_DIST / MIN_VELOCITY)/20 + 1);

    distTimeCurveC->addAxis(axisX, Qt::AlignBottom);
    distTimeCurveC->addAxis(axisY, Qt::AlignLeft);
    realDataSeries->attachAxis(axisX);
    realDataSeries->attachAxis(axisY);
    maxWaveSeries->attachAxis(axisX);
    maxWaveSeries->attachAxis(axisY);
    minWaveSeries->attachAxis(axisX);
    minWaveSeries->attachAxis(axisY);
    outVelSeries->attachAxis(axisX);
    outVelSeries->attachAxis(axisY);
    linearRegressionSeries->attachAxis(axisX);
    linearRegressionSeries->attachAxis(axisY);

    distTimeCurveCV->setRenderHint(QPainter::Antialiasing);
    distTimeCurveCV->setChart(distTimeCurveC);
    ui->eqGraph1LO->addWidget(distTimeCurveCV);

    return nUsed;
}
