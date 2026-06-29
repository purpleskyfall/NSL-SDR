/*==============================================================================
 * signal.c: Functions of GNSS ephemeris operation.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/09/21 create
 *          2024/01/21 change satpos and add satvel to support BeiDou
 *          2024/02/06 move eph2sbf from signal.c to here
==============================================================================*/
#include "siren.h"

#define SQR(x) ((x) * (x))

#define GM_GPS 3.9860050e14   /* gravitational constant ref [1] */
#define GM_GAL 3.986004418e14 /* earth gravitational constant ref [7] */
#define GM_BDS 3.986004418e14 /* earth gravitational constant ref [9] */

#define OMGE_GPS 7.2921151467e-5 /* earth angular velocity (rad/s) ref [1] */
#define OMGE_GAL 7.2921151467e-5 /* earth angular velocity (rad/s) ref [7] */
#define OMGE_BDS 7.292115e-5     /* earth angular velocity (rad/s) ref [9] */

#define POW2_M5 0.03125                                /* 2^-5 */
#define POW2_M6 0.015625                               /* 2^-6 */
#define POW2_M19 1.907348632812500e-6                  /* 2^-19 */
#define POW2_M24 5.960464477539063e-8                  /* 2^-24 */
#define POW2_M27 7.450580596923828e-9                  /* 2^-27 */
#define POW2_M29 1.862645149230957e-9                  /* 2^-29 */
#define POW2_M30 9.313225746154785e-10                 /* 2^-30 */
#define POW2_M31 4.656612873077393e-10                 /* 2^-31 */
#define POW2_M33 1.164153218269348e-10                 /* 2^-33 */
#define POW2_M43 1.136868377216160e-13                 /* 2^-43 */
#define POW2_M50 8.881784197001252e-16                 /* 2^-50 */
#define POW2_M55 2.775557561562891e-17                 /* 2^-55 */
#define POW2_M66 1.3552527156068805425093160010874e-20 /* 2^-66 */

#define SIN_5 -0.0871557427476582 /* sin(-5.0 deg) */
#define COS_5 0.9961946980917456  /* cos(-5.0 deg) */

#define RTOL_KEPLER 1e-13  /* relative tolerance for Kepler equation */
#define MAX_ITER_KEPLER 30 /* max number of iteration of Kelpler */

/*!
 * @brief Compute satellite position and clock bias with ephemeris (for GECJ)
 * @param[in] time Time (GPST)
 * @param[in] eph Broadcast ephemeris
 * @param[out] pos Satellite position (ecef) {x,y,z} (m)
 * @param[out] dts Satellite clock bias (s)
 */
void satpos(gtime_t time, const eph_t *eph, double *pos, double *dts)
{
    double tk, M, E, Ek, sinE, cosE, u, r, i, O, sin2u, cos2u, x, y, sinO, cosO,
        cosi, mu, omge;
    double xg, yg, zg, sino, coso;
    int n, sys, prn;

    trace(4, "satpos : time=%d,%.2f sat=%2d\n", time.week, time.sec, eph->sat);

    if (eph->A <= 0.0) {
        pos[0] = pos[1] = pos[2] = *dts = 0.0;
        return;
    }
    tk = subgtime(time, eph->toe);

    switch ((sys = satsys(eph->sat, &prn))) {
        case SYS_GAL:
            mu = GM_GAL;
            omge = OMGE_GAL;
            break;
        case SYS_BDS:
            mu = GM_BDS;
            omge = OMGE_BDS;
            break;
        default:
            mu = GM_GPS;
            omge = OMGE_GPS;
            break;
    }
    M = eph->M0 + (sqrt(mu / (eph->A * eph->A * eph->A)) + eph->deln) * tk;

    for (n = 0, E = M, Ek = 0.0;
         fabs(E - Ek) > RTOL_KEPLER && n < MAX_ITER_KEPLER; n++) {
        Ek = E;
        E -= (E - eph->e * sin(E) - M) / (1.0 - eph->e * cos(E));
    }
    if (n >= MAX_ITER_KEPLER) {
        trace(2, "sat2pos: kepler iteration overflow sat=%2d\n", eph->sat);
        return;
    }
    sinE = sin(E);
    cosE = cos(E);

    trace(4, "kepler: sat=%2d e=%8.5f n=%2d del=%10.3e\n", eph->sat, eph->e, n,
          E - Ek);

    u = atan2(sqrt(1.0 - eph->e * eph->e) * sinE, cosE - eph->e) + eph->omg;
    r = eph->A * (1.0 - eph->e * cosE);
    i = eph->i0 + eph->idot * tk;
    sin2u = sin(2.0 * u);
    cos2u = cos(2.0 * u);
    u += eph->cus * sin2u + eph->cuc * cos2u;
    r += eph->crs * sin2u + eph->crc * cos2u;
    i += eph->cis * sin2u + eph->cic * cos2u;
    x = r * cos(u);
    y = r * sin(u);
    cosi = cos(i);

    /* BeiDou GEO satellite */
    if (sys == SYS_BDS && bdsorbit(prn) == BDS_GEO) { /* ref [9] table 4-1 */
        O = eph->OMG0 + eph->OMGd * tk - omge * eph->toe.sec;
        sinO = sin(O);
        cosO = cos(O);
        xg = x * cosO - y * cosi * sinO;
        yg = x * sinO + y * cosi * cosO;
        zg = y * sin(i);
        sino = sin(omge * tk);
        coso = cos(omge * tk);
        pos[0] = xg * coso + yg * sino * COS_5 + zg * sino * SIN_5;
        pos[1] = -xg * sino + yg * coso * COS_5 + zg * coso * SIN_5;
        pos[2] = -yg * SIN_5 + zg * COS_5;
    }
    else {
        O = eph->OMG0 + (eph->OMGd - omge) * tk - omge * eph->toes;
        sinO = sin(O);
        cosO = cos(O);
        pos[0] = x * cosO - y * cosi * sinO;
        pos[1] = x * sinO + y * cosi * cosO;
        pos[2] = y * sin(i);
    }
    tk = subgtime(time, eph->toc);
    dts[0] = eph->f0 + eph->f1 * tk + eph->f2 * tk * tk;

    /* relativity correction */
    dts[0] -= 2.0 * sqrt(mu * eph->A) * eph->e * sinE / SQR(CLIGHT);
}

/*!
 * @brief Compute satellite velocities and clock drift with ephemeris (for GECJ)
 * @param[in] time Time (GPST)
 * @param[in] eph Broadcast ephemeris
 * @param[out] vel Satellite velocities (ecef) {x,y,z} (m/s)
 * @param[out] ddts Satellite clock drift (s/s)
 */
void satvel(gtime_t time, const eph_t *eph, double *vel, double *ddts)
{
    int i;
    double pos0[3], pos1[3], dts0, dts1, tt = 1e-3;

    satpos(time, eph, pos0, &dts0);
    time = addgtime(time, tt);
    satpos(time, eph, pos1, &dts1);

    for (i = 0; i < 3; i++)
        vel[i] = (pos1[i] - pos0[i]) / tt;
    *ddts = (dts1 - dts0) / tt;
}

/*!
 * @brief Check satellite visibility
 * @param[in] gt GPS time of receiving the signal
 * @param[in] eph Broadcast ephemeris
 * @param[in] rcv position of the receiver (ecef) {x,y,z} (m)
 * @param[in] elvmask elevation mask (degree)
 * @param[out] azel azimuth & elevation of satellite (radian)
 * @return visibility (0:invisible, 1:visible)
 */
int satvisible(gtime_t gt, eph_t *eph, const double *rcv, double elvmask,
               double *azel)
{
    double llh[3], neu[3];
    double pos[3], clk[3], los[3];
    double tmat[3][3];

    trace(3, "satvisible: week=%d, sec=%.2f, elvmask=%.3f\n", gt.week, gt.sec,
          elvmask);

    xyz2llh(rcv, llh);
    ltcmat(llh, tmat);

    satpos(gt, eph, pos, clk);
    sub3(pos, rcv, los);
    ecef2neu(los, tmat, neu);
    neu2azel(neu, azel);

    if (azel[1] * R2D > elvmask) /* Visible */
        return TRUE;

    return FALSE; /* Invisible */
}

/* GPS subframe parameters compution -----------------------------------------*/
static void eph2sbf_gps(const eph_t *eph, const double *utc, const double *iono,
                        unsigned long sbf[5][N_DWRD_SBF])
{
    unsigned long wn, toe, toc, iode, iodc;
    long deltan, cic, cis, crc, crs, cuc, cus;
    unsigned long ecc, sqrta;
    long m0, omg0, i0, omega, omgdot, idot, af0, af1, af2, tgd;
    int svhlth, codeL2;

    unsigned long ura = 0UL, dataId = 1UL;
    unsigned long sbf4_page18_svId = 56UL, sbf4_page25_svId = 63UL;
    unsigned long sbf5_page25_svId = 51UL;

    unsigned long wna, toa;

    signed long alpha0, alpha1, alpha2, alpha3;
    signed long beta0, beta1, beta2, beta3;
    signed long A0, A1, dtls, dtlsf;
    unsigned long tot, wnt, wnlsf, dn;

    trace(4, "eph2sbf_gps: eph.sat=%d\n", eph->sat);

    /* FIXED: This has to be the "transmission" week number, not for the
              ephemeris reference time
    wn = (unsigned long)(eph->toe.week%1024); */
    wn = 0UL;
    toe = (unsigned long)(eph->toe.sec / 16.0);
    toc = (unsigned long)(eph->toc.sec / 16.0);
    iode = (unsigned long)(eph->iode);
    iodc = (unsigned long)(eph->iodc);
    deltan = (long)(eph->deln / POW2_M43 / PI);
    cuc = (long)(eph->cuc / POW2_M29);
    cus = (long)(eph->cus / POW2_M29);
    cic = (long)(eph->cic / POW2_M29);
    cis = (long)(eph->cis / POW2_M29);
    crc = (long)(eph->crc / POW2_M5);
    crs = (long)(eph->crs / POW2_M5);
    ecc = (unsigned long)(eph->e / POW2_M33);
    sqrta = (unsigned long)(sqrt(eph->A) / POW2_M19);
    m0 = (long)(eph->M0 / POW2_M31 / PI);
    omg0 = (long)(eph->OMG0 / POW2_M31 / PI);
    i0 = (long)(eph->i0 / POW2_M31 / PI);
    omega = (long)(eph->omg / POW2_M31 / PI);
    omgdot = (long)(eph->OMGd / POW2_M43 / PI);
    idot = (long)(eph->idot / POW2_M43 / PI);
    af0 = (long)(eph->f0 / POW2_M31);
    af1 = (long)(eph->f1 / POW2_M43);
    af2 = (long)(eph->f2 / POW2_M55);
    tgd = (long)(eph->tgd[0] / POW2_M31);
    svhlth = (unsigned long)(eph->svh);
    codeL2 = (unsigned long)(eph->code);

    wna = (unsigned long)(eph->toe.week % 256);
    toa = (unsigned long)(eph->toe.sec / 4096.0);

    alpha0 = (signed long)round(iono[0] / POW2_M30);
    alpha1 = (signed long)round(iono[1] / POW2_M27);
    alpha2 = (signed long)round(iono[2] / POW2_M24);
    alpha3 = (signed long)round(iono[3] / POW2_M24);
    beta0 = (signed long)round(iono[4] / 2048.0);
    beta1 = (signed long)round(iono[5] / 16384.0);
    beta2 = (signed long)round(iono[6] / 65536.0);
    beta3 = (signed long)round(iono[7] / 65536.0);
    A0 = (signed long)round(utc[0] / POW2_M30);
    A1 = (signed long)round(utc[1] / POW2_M50);
    tot = (unsigned long)(utc[2] / 4096);
    wnt = (unsigned long)((int)utc[3] % 256);
    dtls = (signed long)(utc[4]);
    /* TO DO: Specify scheduled leap seconds in command options */
    /* 2016/12/31 (Sat) -> WNlsf = 1929, DN = 7
    (http://navigationservices.agi.com/GNSSWeb/) Days are counted from 1 to 7
    (Sunday is 1). */
    wnlsf = 1929 % 256;
    dn = 7;
    dtlsf = 18;

    /* Subframe 1 */
    sbf[0][0] = 0x8B0000UL << 6;
    sbf[0][1] = 0x1UL << 8;
    sbf[0][2] = ((wn & 0x3FFUL) << 20) | ((codeL2 & 0x3UL) << 18) |
                ((ura & 0xFUL) << 14) | ((svhlth & 0x3FUL) << 8) |
                (((iodc >> 8) & 0x3UL) << 6);
    sbf[0][3] = 0UL;
    sbf[0][4] = 0UL;
    sbf[0][5] = 0UL;
    sbf[0][6] = (tgd & 0xFFUL) << 6;
    sbf[0][7] = ((iodc & 0xFFUL) << 22) | ((toc & 0xFFFFUL) << 6);
    sbf[0][8] = ((af2 & 0xFFUL) << 22) | ((af1 & 0xFFFFUL) << 6);
    sbf[0][9] = (af0 & 0x3FFFFFUL) << 8;

    /* Subframe 2 */
    sbf[1][0] = 0x8B0000UL << 6;
    sbf[1][1] = 0x2UL << 8;
    sbf[1][2] = ((iode & 0xFFUL) << 22) | ((crs & 0xFFFFUL) << 6);
    sbf[1][3] = ((deltan & 0xFFFFUL) << 14) | (((m0 >> 24) & 0xFFUL) << 6);
    sbf[1][4] = (m0 & 0xFFFFFFUL) << 6;
    sbf[1][5] = ((cuc & 0xFFFFUL) << 14) | (((ecc >> 24) & 0xFFUL) << 6);
    sbf[1][6] = (ecc & 0xFFFFFFUL) << 6;
    sbf[1][7] = ((cus & 0xFFFFUL) << 14) | (((sqrta >> 24) & 0xFFUL) << 6);
    sbf[1][8] = (sqrta & 0xFFFFFFUL) << 6;
    sbf[1][9] = (toe & 0xFFFFUL) << 14;

    /* Subframe 3 */
    sbf[2][0] = 0x8B0000UL << 6;
    sbf[2][1] = 0x3UL << 8;
    sbf[2][2] = ((cic & 0xFFFFUL) << 14) | (((omg0 >> 24) & 0xFFUL) << 6);
    sbf[2][3] = (omg0 & 0xFFFFFFUL) << 6;
    sbf[2][4] = ((cis & 0xFFFFUL) << 14) | (((i0 >> 24) & 0xFFUL) << 6);
    sbf[2][5] = (i0 & 0xFFFFFFUL) << 6;
    sbf[2][6] = ((crc & 0xFFFFUL) << 14) | (((omega >> 24) & 0xFFUL) << 6);
    sbf[2][7] = (omega & 0xFFFFFFUL) << 6;
    sbf[2][8] = (omgdot & 0xFFFFFFUL) << 6;
    sbf[2][9] = ((iode & 0xFFUL) << 22) | ((idot & 0x3FFFUL) << 8);

    if (iono[0] != 0.0) {
        /* Subframe 4, page 18 */
        sbf[3][0] = 0x8B0000UL << 6;
        sbf[3][1] = 0x4UL << 8;
        sbf[3][2] = (dataId << 28) | (sbf4_page18_svId << 22) |
                    ((alpha0 & 0xFFUL) << 14) | ((alpha1 & 0xFFUL) << 6);
        sbf[3][3] = ((alpha2 & 0xFFUL) << 22) | ((alpha3 & 0xFFUL) << 14) |
                    ((beta0 & 0xFFUL) << 6);
        sbf[3][4] = ((beta1 & 0xFFUL) << 22) | ((beta2 & 0xFFUL) << 14) |
                    ((beta3 & 0xFFUL) << 6);
        sbf[3][5] = (A1 & 0xFFFFFFUL) << 6;
        sbf[3][6] = ((A0 >> 8) & 0xFFFFFFUL) << 6;
        sbf[3][7] = ((A0 & 0xFFUL) << 22) | ((tot & 0xFFUL) << 14) |
                    ((wnt & 0xFFUL) << 6);
        sbf[3][8] = ((dtls & 0xFFUL) << 22) | ((wnlsf & 0xFFUL) << 14) |
                    ((dn & 0xFFUL) << 6);
        sbf[3][9] = (dtlsf & 0xFFUL) << 22;
    }
    else {
        /* Subframe 4, page 25 */
        sbf[3][0] = 0x8B0000UL << 6;
        sbf[3][1] = 0x4UL << 8;
        sbf[3][2] = (dataId << 28) | (sbf4_page25_svId << 22);
        sbf[3][3] = 0UL;
        sbf[3][4] = 0UL;
        sbf[3][5] = 0UL;
        sbf[3][6] = 0UL;
        sbf[3][7] = 0UL;
        sbf[3][8] = 0UL;
        sbf[3][9] = 0UL;
    }

    /* Subframe 5, page 25 */
    sbf[4][0] = 0x8B0000UL << 6;
    sbf[4][1] = 0x5UL << 8;
    sbf[4][2] = (dataId << 28) | (sbf5_page25_svId << 22) |
                ((toa & 0xFFUL) << 14) | ((wna & 0xFFUL) << 6);
    sbf[4][3] = 0UL;
    sbf[4][4] = 0UL;
    sbf[4][5] = 0UL;
    sbf[4][6] = 0UL;
    sbf[4][7] = 0UL;
    sbf[4][8] = 0UL;
    sbf[4][9] = 0UL;

    return;
}

/* BeiDou D1 navigation message subframe parameters compution ----------------*/
static void eph2sbf_bds(const eph_t *eph, const double *utc, const double *iono,
                        unsigned long sbf[5][N_DWRD_SBF])
{
    unsigned long ura, wn, toc, toe, aodc, aode;
    long deltan, cic, cis, crc, crs, cuc, cus;
    unsigned long ecc, sqrta;
    long m0, omg0, i0, omega, omgdot, idot, af0, af1, af2, tgd1, tgd2;

    signed long alpha0, alpha1, alpha2, alpha3;
    signed long beta0, beta1, beta2, beta3;

    trace(4, "eph2sbf_bds: eph.sat=%d\n", eph->sat);

    wn = (unsigned long)(eph->toc.week - BD0WEEK);
    toe = (unsigned long)(gpst2bdt(eph->toe).sec / 8.0);
    toc = (unsigned long)(gpst2bdt(eph->toc).sec / 8.0);
    aode = (unsigned long)(eph->iode);
    aodc = (unsigned long)(eph->iodc);
    deltan = (long)round((eph->deln / POW2_M43 / PI));
    cuc = (long)round(eph->cuc / POW2_M31);
    cus = (long)round(eph->cus / POW2_M31);
    cic = (long)round(eph->cic / POW2_M31);
    cis = (long)round(eph->cis / POW2_M31);
    crc = (long)round(eph->crc / POW2_M6);
    crs = (long)round(eph->crs / POW2_M6);
    ecc = (unsigned long)round(eph->e / POW2_M33);
    sqrta = (unsigned long)round(sqrt(eph->A) / POW2_M19);
    m0 = (long)round((eph->M0 / POW2_M31 / PI));
    omg0 = (long)round(eph->OMG0 / POW2_M31 / PI);
    i0 = (long)round(eph->i0 / POW2_M31 / PI);
    omega = (long)round(eph->omg / POW2_M31 / PI);
    omgdot = (long)round(eph->OMGd / POW2_M43 / PI);
    idot = (long)round(eph->idot / POW2_M43 / PI);
    af0 = (long)round((eph->f0 / POW2_M33));
    af1 = (long)round((eph->f1 / POW2_M50));
    af2 = (long)round((eph->f2 / POW2_M66));
    tgd1 = (long)(eph->tgd[0] / 1e-10);
    tgd2 = (long)(eph->tgd[1] / 1e-10 + 0.5);

    ura = (unsigned long)(eph->sva);
    alpha0 = (signed long)round(iono[0] / POW2_M30);
    alpha1 = (signed long)round(iono[1] / POW2_M27);
    alpha2 = (signed long)round(iono[2] / POW2_M24);
    alpha3 = (signed long)round(iono[3] / POW2_M24);
    beta0 = (signed long)round(iono[4] / 2048.0);
    beta1 = (signed long)round(iono[5] / 16384.0);
    beta2 = (signed long)round(iono[6] / 65536.0);
    beta3 = (signed long)round(iono[7] / 65536.0);

    /* Subframe 1 */
    sbf[0][0] = (0x712 << 19) | (0x1 << 12) | (((toc >> 12) & 0xFF) << 3);
    sbf[0][1] = (toc & 0xFFF) << 10 | (aodc << 4) | ura;
    sbf[0][2] = (wn << 9) | ((toc >> 8) & 0x1FF);
    sbf[0][3] =
        ((toc & 0xFF) << 14) | ((tgd1 & 0x3FF) << 4) | ((tgd2 >> 6) & 0xF);
    sbf[0][4] = (tgd2 & 0x3F) << 16 | ((alpha0 & 0xFF) << 8) | (alpha1 & 0xFF);
    sbf[0][5] = ((alpha2 & 0xFF) << 14) | ((alpha3 & 0xFF) << 6) |
                ((beta0 >> 2) & 0x3F);
    sbf[0][6] = ((beta0 & 0x3) << 20) | ((beta1 & 0xFF) << 12) |
                ((beta2 & 0xFF) << 4) | ((beta3 >> 4) & 0xF);
    sbf[0][7] =
        (beta3 & 0xF) << 18 | ((af2 & 0x7FF) << 7) | ((af0 >> 17) & 0x7F);
    sbf[0][8] = (af0 & 0x1FFFF) << 5 | ((af1 >> 17) & 0x1F);
    sbf[0][9] = ((af1 & 0x1FFFF) << 5) | aode;

    /* Subframe 2 */
    sbf[1][0] = (0x712 << 19) | (0x2 << 12) | (((toc >> 12) & 0xFF) << 3);
    sbf[1][1] = (toc & 0xFFF) << 10 | ((deltan >> 6) & 0x3FF);
    sbf[1][2] = (deltan & 0x3F) << 16 | ((cuc >> 2) & 0xFFFF);
    sbf[1][3] = (cuc & 0x3) << 20 | ((m0 >> 12) & 0xFFFFF);
    sbf[1][4] = (m0 & 0xFFF) << 10 | ((ecc >> 22) & 0x3FF);
    sbf[1][5] = (ecc & 0x3FFFFF);
    sbf[1][6] = (cus & 0x3FFFF) << 4 | ((crc >> 14) & 0xF);
    sbf[1][7] = (crc & 0x3FFF) << 8 | ((crs >> 10) & 0xFF);
    sbf[1][8] = (crs & 0x3FF) << 12 | ((sqrta >> 20) & 0xFFF);
    sbf[1][9] = (sqrta & 0xFFFFF) << 2 | ((toe >> 15) & 0x3);

    /* Subframe 3 */
    sbf[2][0] = (0x712 << 19) | (0x3 << 12) | (((toc >> 12) & 0xFF) << 3);
    sbf[2][1] = (toc & 0xFFF) << 10 | ((toe >> 5) & 0x3FF);
    sbf[2][2] = (toe & 0x1F) << 17 | ((i0 >> 15) & 0x1FFFF);
    sbf[2][3] = (i0 & 0x7FFF) << 7 | ((cic >> 11) & 0x7F);
    sbf[2][4] = (cic & 0x7FF) << 11 | ((omgdot >> 13) & 0x7FF);
    sbf[2][5] = (omgdot & 0x1FFF) << 9 | ((cis >> 9) & 0x1FF);
    sbf[2][6] = (cis & 0x1FF) << 13 | ((idot >> 1) & 0x1FFF);
    sbf[2][7] = (idot & 0x1) << 21 | ((omg0 >> 11) & 0x1FFFFF);
    sbf[2][8] = (omg0 & 0x7FF) << 11 | ((omega >> 21) & 0x7FF);
    sbf[2][9] = (omega & 0x1FFFFF) << 1;

    /* Subframe 4 */
    sbf[3][0] = (0x712 << 19) | (0x4 << 12) | (((toc >> 12) & 0xFF) << 3);
    sbf[3][1] = (toc & 0xFFF) << 10;
    sbf[3][2] = 0UL;
    sbf[3][3] = 0UL;
    sbf[3][4] = 0UL;
    sbf[3][5] = 0UL;
    sbf[3][6] = 0UL;
    sbf[3][7] = 0UL;
    sbf[3][8] = 0UL;
    sbf[3][9] = 0UL;

    /* Subframe 5 */
    sbf[4][0] = (0x712 << 19) | (0x5 << 12) | (((toc >> 12) & 0xFF) << 3);
    sbf[4][1] = (toc & 0xFFF) << 10;
    sbf[4][2] = 0UL;
    sbf[4][3] = 0UL;
    sbf[4][4] = 0UL;
    sbf[4][5] = 0UL;
    sbf[4][6] = 0UL;
    sbf[4][7] = 0UL;
    sbf[4][8] = 0UL;
    sbf[4][9] = 0UL;

    return;
}

/*!
 * @brief Compute Subframe from Ephemeris, valid for GPS & BeiDou
 * @param[in] sys Satellite system
 * @param[in] eph Ephemeris of given satellite
 * @param[in] utc GPS-UTC parameters (double*5)
 * @param[in] iono Ionosphere parameters (double*8)
 * @param[out] sbf Array of 5 sub-frames, 10 long words each
 */
void eph2sbf(int sys, const eph_t *eph, const double *utc, const double *iono,
             unsigned long sbf[5][N_DWRD_SBF])
{
    trace(3, "eph2sbf: sat=%c%d\n", syschr(sys), eph->sat);

    switch (sys) {
        case SYS_GPS: eph2sbf_gps(eph, utc, iono, sbf); break;
        case SYS_BDS: eph2sbf_bds(eph, utc, iono, sbf); break;
        default:
            trace(2, "Error: not support satellite system: %c\n", syschr(sys));
            break;
    }
    return;
}
