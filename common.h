#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QDateTime>
#include <QColor>
#include <QVector>
#include <QMultiMap>
#include <QDebug>
#include <QtMath>
#include <QTextCodec>
#include <QMessageBox>
#include "math.h"

#define VERSION 2.0

#define MAX_QSCD_CNT 100
#define MAX_NUM_NETWORK 100
#define MAX_NUM_STATION 1000
#define MAX_NUM_EVENTINITSTA 5

#define CIRCLE_SIZE 13

#define MAX_EVENT_DURATION 125

#define P_VEL 6.0
#define S_VEL 3.5

#define EQ_DEPTH 11

#define MAX_VELOCITY 8.0
#define MIN_VELOCITY 2.0
#define REGRESSION_PERCENT 0.9
#define VALID_DIST 50
#define THRESHOLD_FOR_DIST 400

static QString find_loc_program = "findLocC";

typedef struct _network
{
    QString netName;
    int numSta;
    int numInUse;
    int numNoUse;
    int numNormal;
    int numBad;
} _NETWORK;

typedef struct _configure
{
    QString homeDir;
    QString logDir;
    QString eventDir;
    QString db_ip, db_name, db_user, db_passwd;
    QString configFileName;

    int monChanID;  // 1. ZPGA, 2. NPGA, 3. EPGA, 4. HPGA, 5. TPGA
    QString mapType; // satellite, street, outdoors, light

    /* Parameters for Event */
    int inSeconds;
    int numSta;
    double thresholdG;
    double thresholdM;
    int distance;
    /************************/
} _CONFIGURE;

typedef struct _station
{
    int index;
    QString sta;
    QString comment;
    float lat;
    float lon;
    float elev;
    float distance;

    int maxPGATime[5];
    float maxPGA[5];

    float currentPGA[MAX_EVENT_DURATION][5];
    float stackedPGA[MAX_EVENT_DURATION][5];
    float predictedHMAXPGA;

    bool isUsed;
    bool noData;
    bool lastAlivedAfterFilter;
} _STATION;

typedef struct _event
{
    int evid;
    int eventEpochTime;
    int dataStartTime;
    int duration;
    QString eventType;

    int eewEvid;
    int eewOriginEpochTime;
    float eewLat;
    float eewLon;
    float eewMag;
    QString eewLoc;
    int eewNsta;
    bool isVoted;

    int inSeconds;
    int numStations;
    int distance;
    float thresholdG;
    float thresholdM;
} _EVENT;

typedef struct _qscd_for_bin
{
    int time;
    char sta[8];
    float pga[5];
} _QSCD_FOR_BIN;

typedef struct _qscd_struct
{
    int crc32 ;         // 4byte CRC Number
    char QFlag ;        // 1byte Quality Flag
    char DType ;        // 1byte Data Type
    char Reserved ;        // 1byte Unused
    char SSSSS[5] ;     // 5byte Station Code
    int time ;          // 4byte Data Time(EpochTime)
    float ZWMin ;     // 4byte U-D Windowed Minimum
    float ZWMax ;     // 4byte U-D Windowed Maximum
    float ZWAvg ;     // 4byte U-D Windowed Average
    float NWMin ;     // 4byte N-S Windowed Minimum
    float NWMax ;     // 4byte N-S Windowed Maximum
    float NWAvg ;     // 4byte N-S Windowed Average
    float EWMin ;     // 4byte E-W Windowed Minimum
    float EWMax ;     // 4byte E-W Windowed Maximum
    float EWAvg ;     // 4byte E-W Windowed Average
    float ZTMin ;     // 4byte U-D True Minimum
    float ZTMax ;     // 4byte U-D True Maximum
    float NTMin ;     // 4byte N-S True Minimum
    float NTMax ;     // 4byte N-S True Maximum
    float ETMin ;     // 4byte E-W True Minimum
    float ETMax ;     // 4byte E-W True Maximum
    float ZMax ;      // 4byte E-W Maximum
    float NMax ;      // 4byte N-S Maximum
    float EMax ;      // 4byte U-D Maximum
    float HPGA ;        // 4byte Horizontal PGA
    float TPGA ;        // 4byte Total PGA
    float ZSI ;       // 4byte U-D SI
    float NSI ;       // 4byte E-W SI
    float ESI ;       // 4byte N-S SI
    float HSI ;       // 4byte Horizontal SI
    float Correl ;    // 4byte Correlation
    char CN1;           // 1byte Channel Number 1
    char CN2;           // 1byte Channel Number 2
    char LO[2];     // 2byte Unused
} _QSCD_PACKET ;

static QDateTime convertKST(QDateTime oriTime)
{
    oriTime.setTimeSpec(Qt::UTC);
    oriTime = oriTime.addSecs(3600 * 9);
    return oriTime;
}

static QDateTime convertUTC(QDateTime oriTime)
{
    oriTime = oriTime.addSecs( - (3600 * 9) );
    return oriTime;
}

static double getPredictedValue(float dist, float mag)
{
    double y = (dist * dist) + (EQ_DEPTH * EQ_DEPTH);
    double R = sqrt(y);
    double c0, c1, c2;

    /* 2001  */
    double ksai0[4] = {0.1250737E+02, 0.4874629E+00, -0.2940726E-01, 0.1737204E-01};
    double ksai1[4] = {-0.1928185E-02, 0.2251016E-03, -0.6378615E-04, 0.6967121E-04};
    double ksai2[4] = {-0.5795112E+00, 0.1138817E+00, -0.1162326E-01, -0.3646674E-02};

    c0 = ksai0[0] + ksai0[1]*(mag-6) + ksai0[2]*pow((mag-6), 2) + ksai0[3]*pow((mag-6), 3);
    c1 = ksai1[0] + ksai1[1]*(mag-6) + ksai1[2]*pow((mag-6), 2) + ksai1[3]*pow((mag-6), 3);
    c2 = ksai2[0] + ksai2[1]*(mag-6) + ksai2[2]*pow((mag-6), 2) + ksai2[3]*pow((mag-6), 3);


    /* 2003
    double ksai0[4] = {0.1073829E+02 , 0.5909022E+00, -0.5622945E-01, 0.2135007E-01};
    double ksai1[4] = {-0.2379955E-02, 0.2081359E-03, -0.2046806E-04, 0.4192630E-04};
    double ksai2[4] = {-0.2437218E+00, 0.9498274E-01, -0.8804236E-02, -0.3302350E-02};

    c0 = ksai0[0] + (ksai0[1]*(mag-6)) + pow((ksai0[2]*(mag-6)), 2) + pow((ksai0[3]*(mag-6)), 3);
    c1 = ksai1[0] + (ksai1[1]*(mag-6)) + pow((ksai1[2]*(mag-6)), 2) + pow((ksai1[3]*(mag-6)), 3);
    c2 = ksai2[0] + (ksai2[1]*(mag-6)) + pow((ksai2[2]*(mag-6)), 2) + pow((ksai2[3]*(mag-6)), 3);
     */

    double lnSA;
    if(R < 100)
        lnSA = c0 + c1*R + c2*log(R) - log(R) - 0.5*log(100);
    else if(R > 100)
        lnSA = c0 + c1*R + c2*log(R) - log(100) - 0.5*log(R);
    else if(R == 100)
        lnSA = c0 + c1*R + c2*log(R) - log(100) - 0.5*log(100);

    //qDebug() << c0 << c1 << c2 << R << exp(lnSA) << qLn(lnSA);

    return exp(lnSA);
}

static unsigned int GetBits(unsigned x, int p, int n) { return (x >> (p-n+1)) & ~(~0 << n) ; }

static std::uint32_t Swap32(std::uint32_t x)
{
    return static_cast<std::uint32_t>((x << 24) | ((x << 8) & 0x00FF0000) |
                                     ((x >>  8) & 0x0000FF00) | (x >> 24));
}

static float SwapFloat(float x)
{
    union
    {
        float f;
        std::uint32_t ui32;
    } swapper;
    swapper.f = x;
    swapper.ui32 = Swap32(swapper.ui32);
    return swapper.f;
}

inline static double sqr(double x)
{
    return x*x;
}

/*
static int linreg(int n, const double x[], const double y[], double* m, double* b, double* r)
{
    double   sumx = 0.0;
    double   sumx2 = 0.0;
    double   sumxy = 0.0;
    double   sumy = 0.0;
    double   sumy2 = 0.0;

    for (int i=0;i<n;i++)
    {
        sumx  += x[i];
        sumx2 += sqr(x[i]);
        sumxy += x[i] * y[i];
        sumy  += y[i];
        sumy2 += sqr(y[i]);
    }

    double denom = (n * sumx2 - sqr(sumx));
    if (denom == 0)
    {
        // singular matrix. can't solve the problem.
        *m = 0;
        *b = 0;
        if (r) *r = 0;
            return 1;
    }

    *m = (n * sumxy  -  sumx * sumy) / denom;
    *b = (sumy * sumx2  -  sumx * sumxy) / denom;
    if (r!=NULL)
    {
        *r = (sumxy - sumx * sumy / n) /
              sqrt((sumx2 - sqr(sumx)/n) *
              (sumy2 - sqr(sumy)/n));
    }

    return 0;
}*/

static int linregVector(int n, QVector<double> x, QVector<double> y, double* m, double* b, double* r)
{
    double   sumx = 0.0;                      /* sum of x     */
    double   sumx2 = 0.0;                     /* sum of x**2  */
    double   sumxy = 0.0;                     /* sum of x * y */
    double   sumy = 0.0;                      /* sum of y     */
    double   sumy2 = 0.0;                     /* sum of y**2  */

    for (int i=0;i<n;i++)
    {
        sumx  += x.at(i);
        sumx2 += sqr(x.at(i));
        sumxy += x.at(i) * y.at(i);
        sumy  += y.at(i);
        sumy2 += sqr(y.at(i));
    }

    double denom = (n * sumx2 - sqr(sumx));
    if (denom == 0)
    {
        // singular matrix. can't solve the problem.
        *m = 0;
        *b = 0;
        if (r) *r = 0;
            return 1;
    }

    *m = (n * sumxy  -  sumx * sumy) / denom;
    *b = (sumy * sumx2  -  sumx * sumxy) / denom;
    if (r!=NULL)
    {
        *r = (sumxy - sumx * sumy / n) /    /* compute correlation coeff */
              sqrt((sumx2 - sqr(sumx)/n) *
              (sumy2 - sqr(sumy)/n));
    }

    return 0;
}

#define PI 3.14159265358979323846

static double myRound(double n, unsigned int c)
{
    double marge = pow(10, c);
    double up = n * marge;
    double ret = round(up) / marge;
    return ret;
}

static int geo_to_km(double lat1,double lon1,double lat2,double lon2,double* dist,double* azm)
{
    double a, b;
    double semi_major=a=6378.160;
    double semi_minor=b=6356.775;
    double torad, todeg;
    double aa, bb, cc, dd, top, bottom, lambda12, az, temp;
    double v1, v2;
    double fl, e, e2, eps, eps0;
    double b0, x2, y2, z2, z1, u1p, u2p, xdist;
    double lat1rad, lat2rad, lon1rad, lon2rad;
    double coslon1, sinlon1, coslon2, sinlon2;
    double coslat1, sinlat1, coslat2, sinlat2;
    double tanlat1, tanlat2, cosazm, sinazm;

    double c0, c2, c4, c6;

    double c00=1.0, c01=0.25, c02=-0.046875, c03=0.01953125;
    double c21=-0.125, c22=0.03125, c23=-0.014648438;
    double c42=-0.00390625, c43=0.0029296875;
    double c63=-0.0003255208;

    if( lat1 == lat2 && lon1 == lon2 ) {
        *azm = 0.0;
        *dist= 0.0;
        return(1);
    }

    torad = PI / 180.0;
    todeg = 1.0 / torad;
    fl = ( a - b ) / a;
    e2 = 2.0*fl - fl*fl;
    e  = sqrt(e2);
    eps = e2 / ( 1.0 - e2);

    temp=lat1;
    if(temp == 0.) temp=1.0e-08;
    lat1rad=torad*temp;
    lon1rad=torad*lon1;

    temp=lat2;
    if(temp == 0.) temp=1.0e-08;
    lat2rad=torad*temp;
    lon2rad=torad*lon2;

    coslon1 = cos(lon1rad);
    sinlon1 = sin(lon1rad);
    coslon2 = cos(lon2rad);
    sinlon2 = sin(lon2rad);
    tanlat1 = tan(lat1rad);
    tanlat2 = tan(lat2rad);
    sinlat1 = sin(lat1rad);
    coslat1 = cos(lat1rad);
    sinlat2 = sin(lat2rad);
    coslat2 = cos(lat2rad);

    v1 = a / sqrt( 1.0 - e2*sinlat1*sinlat1 );
    v2 = a / sqrt( 1.0 - e2*sinlat2*sinlat2 );
    aa = tanlat2 / ((1.0+eps)*tanlat1);
    bb = e2*(v1*coslat1)/(v2*coslat2);
    lambda12 = aa + bb;
    top = sinlon2*coslon1 - coslon2*sinlon1;
    bottom = lambda12*sinlat1-coslon2*coslon1*sinlat1-sinlon2*sinlon1*sinlat1;
    az = atan2(top,bottom)*todeg;
    if( az < 0.0 ) az = 360 + az;
    *azm = az;
    az = az * torad;
    cosazm = cos(az);
    sinazm = sin(az);

    if( lat2rad < 0.0 ) {
        temp = lat1rad;
        lat1rad = lat2rad;
        lat2rad = temp;
        temp = lon1rad;
        lon1rad = lon2rad;
        lon2rad = temp;

        coslon1 = cos(lon1rad);
        sinlon1 = sin(lon1rad);
        coslon2 = cos(lon2rad);
        sinlon2 = sin(lon2rad);
        tanlat1 = tan(lat1rad);
        tanlat2 = tan(lat2rad);
        sinlat1 = sin(lat1rad);
        coslat1 = cos(lat1rad);
        sinlat2 = sin(lat2rad);
        coslat2 = cos(lat2rad);

        v1 = a / sqrt( 1.0 - e2*sinlat1*sinlat1 );
        v2 = a / sqrt( 1.0 - e2*sinlat2*sinlat2 );

        aa = tanlat2 / ((1.0+eps)*tanlat1);
        bb = e2*(v1*coslat1)/(v2*coslat2);
        lambda12 = aa + bb;

        top = sinlon2*coslon1 - coslon2*sinlon1;
        bottom =lambda12*sinlat1-coslon2*coslon1*sinlat1-
            sinlon2*sinlon1*sinlat1;
        az = atan2(top,bottom);
        cosazm = cos(az);
        sinazm = sin(az);

    }

    eps0 = eps * ( coslat1*coslat1*cosazm*cosazm + sinlat1*sinlat1 );
    b0 = (v1/(1.0+eps0)) * sqrt(1.0+eps*coslat1*coslat1*cosazm*cosazm);

    x2 = v2*coslat2*(coslon2*coslon1+sinlon2*sinlon1);
    y2 = v2*coslat2*(sinlon2*coslon1-coslon2*sinlon1);
    z2 = v2*(1.0-e2)*sinlat2;
    z1 = v1*(1.0-e2)*sinlat1;

    c0 = c00 + c01*eps0 + c02*eps0*eps0 + c03*eps0*eps0*eps0;
    c2 =       c21*eps0 + c22*eps0*eps0 + c23*eps0*eps0*eps0;
    c4 =                  c42*eps0*eps0 + c43*eps0*eps0*eps0;
    c6 =                                  c63*eps0*eps0*eps0;

    bottom = cosazm*sqrt(1.0+eps0);
    u1p = atan2(tanlat1,bottom);

    top = v1*sinlat1+(1.0+eps0)*(z2-z1);
    bottom = (x2*cosazm-y2*sinlat1*sinazm)*sqrt(1.0+eps0);
    u2p = atan2(top,bottom);

    aa = c0*(u2p-u1p);
    bb = c2*(sin(2.0*u2p)-sin(2.0*u1p));
    cc = c4*(sin(4.0*u2p)-sin(4.0*u1p));
    dd = c6*(sin(6.0*u2p)-sin(6.0*u1p));

    xdist = fabs(b0*(aa+bb+cc+dd));
    *dist = xdist;
    return(1);
}

static double getDistance(double lat1, double lon1, double lat2, double lon2)
{
    double dist, azim;
    int rtn = geo_to_km(lat1, lon1, lat2, lon2, &dist, &azim);
    return dist;
}

static int redColor(float gal)
{
    int color ;

    // red color value
    if( gal <= 0.0098 )
    {
      color = 191 ;
    }
    else if (gal > 0.0098 && gal <= 0.0392)
    {
      color = gal * (-3265.31) + 223 ;
    }
    else if (gal > 0.0392 && gal <= 0.0784)
    {
      color = 95 ;
    }
    else if (gal > 0.0784 && gal <= 0.098)
    {
      color = gal * 3265.31 - 161 ;
    }
    else if (gal > 0.098 && gal <= 0.98)
    {
      color = gal * 103.82 + 148.497 ;
    }
    else if (gal > 0.98 && gal <= 147)
    {
      color = 255 ;
    }
    else if (gal > 147 && gal <= 245)
    {
      color = -0.00333195 * pow(gal,2) + 0.816327 * gal + 207 ;
    }
    else if (gal > 245)
      color = 207 ;

    return color ;
}

static int greenColor(float gal)
{
    int color ;
    // red color value
    if( gal <= 0.98 )
    {
      color = 255 ;
    }
    else if (gal > 0.98 && gal <= 9.8)
    {
      color = -0.75726 * gal * gal - 0.627943 * gal + 255.448 ;
    }
    else if (gal > 0.98 && gal <= 245)
    {
      color = 0.00432696 * gal * gal - 1.84309 * gal + 192.784 ;
      if(color < 0)
        color = 0 ;
    }
    else if (gal > 245)
      color = 0 ;

    return color ;
}

static int blueColor(float gal)
{
    int color ;

    // red color value
    if( gal <= 0.0098 )
    {
      color = 255 ;
    }
    else if (gal > 0.0098 && gal <= 0.098)
    {
      color = -19799.2 * gal * gal + 538.854 * gal + 260.429 ;
    }
    else if (gal > 0.098 && gal <= 0.98)
    {
      color = -35.4966 * gal * gal - 65.8163 * gal + 116.264 ;
    }
    else if (gal > 0.98 && gal <= 3.92)
    {
      color = -5.10204 * gal + 20 ;
    }
    else if (gal > 3.92)
    {
      color = 0 ;
    }

    if(color > 255)
      color = 255 ;

    return color ;
}

#endif // COMMON_H
