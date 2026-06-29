/*==============================================================================
 * utils.c: Common function defination.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/09/21 create
 *          2023/01/16 add satsys, str2num, str2date & satid2no functions
==============================================================================*/
#include "siren.h"

#define SECONDS_IN_WEEK 604800.0
#define SECONDS_IN_HOUR 3600.0
#define SECONDS_IN_MINUTE 60.0

/*!
 * @brief Subtract two vectors of double
 * @param[in] x1 Minuend of subtraction
 * @param[in] x2 Subtrahend of subtraction
 * @param[out] y Result of subtraction
 */
void sub3(const double *x1, const double *x2, double *y)
{
    y[0] = x1[0] - x2[0];
    y[1] = x1[1] - x2[1];
    y[2] = x1[2] - x2[2];

    return;
}

/*!
 * @brief Compute euclid norm of vector
 * @param[in] x Input vector
 * @return Euclid norm of the input vector
 */
double norm3(const double *x)
{
    return sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
}

/*!
 * @brief Compute dot-product of two vectors
 * @param[in] x1 First multiplicand
 * @param[in] x2 Second multiplicand
 * @return Dot-product of both multiplicands
 */
double dot3(const double *x1, const double *x2)
{
    return x1[0] * x2[0] + x1[1] * x2[1] + x1[2] * x2[2];
}

/*!
 * @brief Convert substring in string to number
 * @param[in] str String contains numbers
 * @param[in] i Substring start position
 * @param[in] n Substring width
 * @return Converted number (0.0:error)
 */
double str2num(const char *str, int i, int n)
{
    double value;
    char s[256], *p = s;

    if (i < 0 || (int)strlen(str) < i || (int)sizeof(s) - 1 < n)
        return 0.0;

    for (str += i; *str && --n >= 0; str++)
        *p++ = *str == 'd' || *str == 'D' ? 'E' : *str;
    *p = '\0';

    return sscanf(s, "%lf", &value) == 1 ? value : 0.0;
}

/*!
 * @brief Convert a string date into a UTC date
 * @param[in] str String contains date ("yyyy mm dd hh mm ss")
 * @param[in] i Substring start position
 * @param[in] n Substring width (n < 32)
 * @return output date in UTC form
 * @note leap seconds has't added
 */
datetime_t str2date(const char *str, int i, int n)
{
    datetime_t dt = {0};
    double ep[6];
    char s[32], *p = s;

    if (i < 0 || (int)strlen(str) < i || (int)sizeof(s) - 1 < i)
        return dt;

    for (str += i; *str && --n >= 0;)
        *p++ = *str++;
    *p = '\0';

    if (sscanf(s, "%lf %lf %lf %lf %lf %lf", ep, ep + 1, ep + 2, ep + 3, ep + 4,
               ep + 5) < 6)
        return dt;

    if (ep[0] < 100.0)
        ep[0] += ep[0] < 80.0 ? 2000.0 : 1900.0;

    dt.year = (int)ep[0];
    dt.month = (int)ep[1];
    dt.day = (int)ep[2];
    dt.hour = (int)ep[3];
    dt.min = (int)ep[4];
    dt.sec = ep[5];

    return dt;
}

/*!
 * @brief Convert a UTC date into a GPS date
 * @param[in] dt input date in UTC
 * @return output date in GPS Time
 * @note leap seconds has't added
 */
gtime_t date2gpst(const datetime_t dt)
{
    gtime_t g1;
    int doy[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int yr, de, lpdays;

    yr = dt.year - 1980;

    /* Compute the number of leap days since Jan 5/Jan 6, 1980 */
    lpdays = yr / 4 + 1;
    if ((yr % 4) == 0 && dt.month <= 2)
        lpdays--;

    /* Compute the number of days elapsed since Jan 5/Jan 6, 1980 */
    de = yr * 365 + doy[dt.month - 1] + dt.day + lpdays - 6;

    /* Convert time to GPS weeks and seconds */
    g1.week = de / 7;
    g1.sec = (double)(de % 7) * SECONDS_IN_DAY + dt.hour * SECONDS_IN_HOUR +
             dt.min * SECONDS_IN_MINUTE + dt.sec;

    return g1;
}

/*!
 * @brief Convert a GPS date into a UTC date
 * @param[in] gt input date in GPS form
 * @return output date in UTC form
 * @note leap seconds has't added
 */
datetime_t gpst2date(const gtime_t gt)
{
    datetime_t dt;
    /* Convert Julian day number to calendar date */
    int c = (int)(7 * gt.week + floor(gt.sec / 86400.0) + 2444245.0) + 1537;
    int d = (int)((c - 122.1) / 365.25);
    int e = 365 * d + d / 4;
    int f = (int)((c - e) / 30.6001);

    dt.day = c - e - (int)(30.6001 * f);
    dt.month = f - 1 - 12 * (f / 14);
    dt.year = d - 4715 - ((7 + dt.month) / 10);

    dt.hour = ((int)(gt.sec / 3600.0)) % 24;
    dt.min = ((int)(gt.sec / 60.0)) % 60;
    dt.sec = gt.sec - 60.0 * floor(gt.sec / 60.0);

    return dt;
}

/*!
 * @brief Subtract of GPS time: dt = g1 - g0
 * @param g1 GPS time
 * @param g0 GPS time
 * @return Time delta (dt) in seconds
 */
double subgtime(gtime_t g1, gtime_t g0)
{
    double dt;

    dt = g1.sec - g0.sec;
    dt += (double)(g1.week - g0.week) * SECONDS_IN_WEEK;

    return dt;
}

/*!
 * @brief Increase of GPS time: g1 = g0 + dt
 * @param g0 GPS time
 * @param dt Time in seconds
 * @return New GPS time (g1)
 */
gtime_t addgtime(gtime_t g0, double dt)
{
    gtime_t g1;

    g1.week = g0.week;
    g1.sec = g0.sec + dt;

    g1.sec = round(g1.sec * 1000.0) / 1000.0; /* Avoid rounding error */

    while (g1.sec >= SECONDS_IN_WEEK) {
        g1.sec -= SECONDS_IN_WEEK;
        g1.week++;
    }

    while (g1.sec < 0.0) {
        g1.sec += SECONDS_IN_WEEK;
        g1.week--;
    }

    return g1;
}

/*!
 * @brief Convert bdt (beidou navigation satellite system time) to gpstime
 * @param[in] t0 Time expressed in bdt
 * @return Time expressed in gpstime
 * @note Slight time offset under 100 ns were ignored
 */
gtime_t bdt2gpst(gtime_t t0) { return addgtime(t0, 14.0); }

/*!
 * @brief Convert bdt (beidou navigation satellite system time) to gpstime
 * @param[in] t0 Time expressed in gpstime
 * @return Time expressed in bdt
 * @note Slight time offset under 100 ns were ignored
 */
gtime_t gpst2bdt(gtime_t t0) { return addgtime(t0, -14.0); }

/*!
 * @brief Convert week and sow in BeiDou Time (bdt) to gtime_t struct
 * @param week Week number in bdt
 * @param sec Seconds of week in bdt (s)
 * @return BeiDou Time in gtime_t struct
 */
gtime_t bdt2time(int week, double sec)
{
    gtime_t g1;

    g1.week = week + BD0WEEK;
    g1.sec = sec;

    return g1;
}

/*!
 * @brief Convert satellite system+prn/slot number to satellite number
 * @param[in] sys Satellite system (SYS_GPS, SYS_GAL, SYS_BDS)
 * @param[in] prn Satellite prn/slot number
 * @return Satellite number (0:error)
 */
int satno(int sys, int prn)
{
    if (prn <= 0)
        return 0;
    switch (sys) {
        case SYS_GPS:
            if (prn < MINPRNGPS || MAXPRNGPS < prn)
                return 0;
            return prn - MINPRNGPS + 1;
        case SYS_GAL:
            if (prn < MINPRNGAL || MAXPRNGAL < prn)
                return 0;
            return NSATGPS + prn - MINPRNGAL + 1;
        case SYS_BDS:
            if (prn < MINPRNBDS || MAXPRNBDS < prn)
                return 0;
            return NSATGPS + NSATGAL + prn - MINPRNBDS + 1;
    }
    return 0;
}

/*!
 * @brief Convert satellite id to satellite number
 * @param[in] id Satellite id (nn, Gnn, Enn, Cnn)
 * @return Satellite number (0: error)
 */
int satid2no(const char *id)
{
    int sys, prn;
    char code;

    if (sscanf(id, "%d", &prn) == 1) {
        if (MINPRNGPS <= prn && prn <= MAXPRNGPS)
            sys = SYS_GPS;
        else
            return 0;
        return satno(sys, prn);
    }
    if (sscanf(id, "%c%d", &code, &prn) < 2)
        return 0;

    switch (code) {
        case 'G':
            sys = SYS_GPS;
            prn += MINPRNGPS - 1;
            break;
        case 'E':
            sys = SYS_GAL;
            prn += MINPRNGAL - 1;
            break;
        case 'C':
            sys = SYS_BDS;
            prn += MINPRNBDS - 1;
            break;
        default: return 0;
    }
    return satno(sys, prn);
}

/*!
 * @brief Convert satellite number to satellite system
 * @param[in] sat Satellite number (1 -> MAXSAT)
 * @param[out] prn Satellite prn/slot number (NULL: no output)
 * @return Satellite system (SYS_GPS, SYS_GAL, SYS_BDS)
 */
int satsys(int sat, int *prn)
{
    int sys = SYS_NONE;

    if (sat <= 0 || MAXSAT < sat)
        sat = 0;
    else if (sat <= NSATGPS) {
        sys = SYS_GPS;
        sat += MINPRNGPS - 1;
    }
    else if ((sat -= NSATGPS) <= NSATGAL) {
        sys = SYS_GAL;
        sat += MINPRNGAL - 1;
    }
    else if ((sat -= NSATGAL) <= NSATBDS) {
        sys = SYS_BDS;
        sat += MINPRNBDS - 1;
    }
    else
        sat = 0;

    if (prn)
        *prn = sat;

    return sys;
}

/*!
 * @brief Convert satellite system to char
 * @param[in] sys Satellite system (SYS_GPS, SYS_GAL, SYS_BDS)
 * @return Satellite system by a char (G, E, C)
 */
char syschr(int sys)
{
    switch (sys) {
        case SYS_GAL: return 'E';
        case SYS_BDS: return 'C';
        default: return 'G';
    }
}

/*!
 * @brief Get orbit type of BeiDou satellite
 * @param prn Satellite prn/slot number
 * @return Orbit type (BDS_GEO, BDS_IGSO, BDS_MEO), 0 for error or unknown
 */
int bdsorbit(int prn)
{
    /* Satellite status, https://www.csno-tarc.cn/system/constellation */
    const static int bds_status[] = {
        BDS_GEO,  BDS_GEO,  BDS_GEO,  BDS_GEO,  BDS_GEO,  /* 1 ~ 5 */
        BDS_IGSO, BDS_IGSO, BDS_IGSO, BDS_IGSO, BDS_IGSO, /* 6 ~ 10 */
        BDS_MEO,  BDS_MEO,  BDS_IGSO, BDS_MEO,  0,        /* 11 ~ 15 */
        BDS_IGSO, 0,        0,        BDS_MEO,  BDS_MEO,  /* 16 ~ 20 */
        BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  /* 21 ~ 25 */
        BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  /* 26 ~ 30 */
        BDS_IGSO, BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  /* 31 ~ 35 */
        BDS_MEO,  BDS_MEO,  BDS_IGSO, BDS_IGSO, BDS_IGSO, /* 36 ~ 40 */
        BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  /* 41 ~ 45 */
        BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  BDS_MEO,  /* 46 ~ 50 */
        0,        0,        0,        0,        0,        /* 51 ~ 55 */
        BDS_IGSO, BDS_MEO,  BDS_MEO,  BDS_GEO,  BDS_GEO,  /* 56 ~ 60 */
        BDS_GEO,  BDS_GEO};                               /* 61 ~ 62 */

    if (prn < 1 || prn > 62)
        return 0;
    else
        return bds_status[prn - 1];
}

/*!
 * @brief Convert Earth-centered Earth-fixed (ECEF) into Lat-Long-Height
 * @param[in] xyz Input Array of X, Y and Z ECEF coordinates
 * @param[out] llh Output Array of Latitude, Longitude and Height
 */
void xyz2llh(const double *xyz, double *llh)
{
    double a, eps, e, e2;
    double x, y, z;
    double rho2, dz, zdz, nh, slat, n, dz_new;

    a = RADIUS_WGS84;
    e = ECCENTRICITY_WGS84;

    eps = 1.0e-3;
    e2 = e * e;

    if (norm3(xyz) < eps) {
        /* Invalid ECEF vector */
        llh[0] = 0.0;
        llh[1] = 0.0;
        llh[2] = -a;

        return;
    }

    x = xyz[0];
    y = xyz[1];
    z = xyz[2];

    rho2 = x * x + y * y;
    dz = e2 * z;

    while (TRUE) {
        zdz = z + dz;
        nh = sqrt(rho2 + zdz * zdz);
        slat = zdz / nh;
        n = a / sqrt(1.0 - e2 * slat * slat);
        dz_new = n * e2 * slat;

        if (fabs(dz - dz_new) < eps)
            break;

        dz = dz_new;
    }

    llh[0] = atan2(zdz, sqrt(rho2));
    llh[1] = atan2(y, x);
    llh[2] = nh - n;

    return;
}

/*!
 * @brief Convert Lat-Long-Height into Earth-centered Earth-fixed (ECEF)
 * @param[in] llh Input Array of Latitude, Longitude and Height
 * @param[out] xyz Output Array of X, Y and Z ECEF coordinates
 */
void llh2xyz(const double *llh, double *xyz)
{
    double n, a, e, e2;
    double clat, slat, clon, slon;
    double d, nph, tmp;

    a = RADIUS_WGS84;
    e = ECCENTRICITY_WGS84;
    e2 = e * e;

    clat = cos(llh[0]);
    slat = sin(llh[0]);
    clon = cos(llh[1]);
    slon = sin(llh[1]);
    d = e * slat;

    n = a / sqrt(1.0 - d * d);
    nph = n + llh[2];

    tmp = nph * clat;
    xyz[0] = tmp * clon;
    xyz[1] = tmp * slon;
    xyz[2] = ((1.0 - e2) * n + llh[2]) * slat;

    return;
}

/*!
 * @brief Compute the intermediate matrix from ECEF to NEU
 * @param[in] llh Input position in Lat-Long-Height format
 * @param[out] t Three-by-Three output matrix
 */
void ltcmat(const double *llh, double t[3][3])
{
    double slat, clat;
    double slon, clon;

    slat = sin(llh[0]);
    clat = cos(llh[0]);
    slon = sin(llh[1]);
    clon = cos(llh[1]);

    t[0][0] = -slat * clon;
    t[0][1] = -slat * slon;
    t[0][2] = clat;
    t[1][0] = -slon;
    t[1][1] = clon;
    t[1][2] = 0.0;
    t[2][0] = clat * clon;
    t[2][1] = clat * slon;
    t[2][2] = slat;

    return;
}

/*!
 * @brief Convert Earth-centered Earth-Fixed to topocentric
 * @param[in] xyz Input position as vector in ECEF format
 * @param[in] t Intermediate matrix computed by @ref ltcmat
 * @param[out] neu Output position as North-East-Up format
 */
void ecef2neu(const double *xyz, double t[3][3], double *neu)
{
    neu[0] = t[0][0] * xyz[0] + t[0][1] * xyz[1] + t[0][2] * xyz[2];
    neu[1] = t[1][0] * xyz[0] + t[1][1] * xyz[1] + t[1][2] * xyz[2];
    neu[2] = t[2][0] * xyz[0] + t[2][1] * xyz[1] + t[2][2] * xyz[2];

    return;
}

/*!
 * @brief Convert North-East-Up to Azimuth + Elevation
 * @param[in] neu Input position in North-East-Up format
 * @param[out] azel Output array of azimuth + elevation as double
 */
void neu2azel(const double *neu, double *azel)
{
    double ne;

    azel[0] = atan2(neu[1], neu[0]);
    if (azel[0] < 0.0)
        azel[0] += 2.0 * PI;

    ne = sqrt(neu[0] * neu[0] + neu[1] * neu[1]);
    azel[1] = atan2(neu[2], ne);

    return;
}
