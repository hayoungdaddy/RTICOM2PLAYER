#ifndef FILTERSTA_H
#define FILTERSTA_H

#include <QDialog>
#include <QtCharts>

#include "common.h"

namespace Ui {
class FilterSta;
}

class FilterSta : public QDialog
{
    Q_OBJECT

public:
    explicit FilterSta(QWidget *parent=nullptr);
    ~FilterSta();

    QList<_STATION> setup(QList<_STATION>, _EVENT);
    double pgaVel;

private:
    Ui::FilterSta *ui;

    QTextCodec *codec;
    _EVENT event;

    QList<_STATION> staList;

    int drawDistTimeCurve();
    void drawDistPGACurveUsingGMPE(int);
};

#endif // FILTERSTA_H
