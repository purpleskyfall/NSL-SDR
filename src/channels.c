/*==============================================================================
 * channels.c: Functions of GNSS channels operation.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/12/06 create
==============================================================================*/
#include "siren.h"

static int SinTable512[] = {
    2,    5,    8,    11,   14,   17,   20,   23,   26,   29,   32,   35,
    38,   41,   44,   47,   50,   53,   56,   59,   62,   65,   68,   71,
    74,   77,   80,   83,   86,   89,   91,   94,   97,   100,  103,  105,
    108,  111,  114,  116,  119,  122,  125,  127,  130,  132,  135,  138,
    140,  143,  145,  148,  150,  153,  155,  157,  160,  162,  164,  167,
    169,  171,  173,  176,  178,  180,  182,  184,  186,  188,  190,  192,
    194,  196,  198,  200,  202,  204,  205,  207,  209,  210,  212,  214,
    215,  217,  218,  220,  221,  223,  224,  225,  227,  228,  229,  230,
    232,  233,  234,  235,  236,  237,  238,  239,  240,  241,  241,  242,
    243,  244,  244,  245,  245,  246,  247,  247,  248,  248,  248,  249,
    249,  249,  249,  250,  250,  250,  250,  250,  250,  250,  250,  250,
    250,  249,  249,  249,  249,  248,  248,  248,  247,  247,  246,  245,
    245,  244,  244,  243,  242,  241,  241,  240,  239,  238,  237,  236,
    235,  234,  233,  232,  230,  229,  228,  227,  225,  224,  223,  221,
    220,  218,  217,  215,  214,  212,  210,  209,  207,  205,  204,  202,
    200,  198,  196,  194,  192,  190,  188,  186,  184,  182,  180,  178,
    176,  173,  171,  169,  167,  164,  162,  160,  157,  155,  153,  150,
    148,  145,  143,  140,  138,  135,  132,  130,  127,  125,  122,  119,
    116,  114,  111,  108,  105,  103,  100,  97,   94,   91,   89,   86,
    83,   80,   77,   74,   71,   68,   65,   62,   59,   56,   53,   50,
    47,   44,   41,   38,   35,   32,   29,   26,   23,   20,   17,   14,
    11,   8,    5,    2,    -2,   -5,   -8,   -11,  -14,  -17,  -20,  -23,
    -26,  -29,  -32,  -35,  -38,  -41,  -44,  -47,  -50,  -53,  -56,  -59,
    -62,  -65,  -68,  -71,  -74,  -77,  -80,  -83,  -86,  -89,  -91,  -94,
    -97,  -100, -103, -105, -108, -111, -114, -116, -119, -122, -125, -127,
    -130, -132, -135, -138, -140, -143, -145, -148, -150, -153, -155, -157,
    -160, -162, -164, -167, -169, -171, -173, -176, -178, -180, -182, -184,
    -186, -188, -190, -192, -194, -196, -198, -200, -202, -204, -205, -207,
    -209, -210, -212, -214, -215, -217, -218, -220, -221, -223, -224, -225,
    -227, -228, -229, -230, -232, -233, -234, -235, -236, -237, -238, -239,
    -240, -241, -241, -242, -243, -244, -244, -245, -245, -246, -247, -247,
    -248, -248, -248, -249, -249, -249, -249, -250, -250, -250, -250, -250,
    -250, -250, -250, -250, -250, -249, -249, -249, -249, -248, -248, -248,
    -247, -247, -246, -245, -245, -244, -244, -243, -242, -241, -241, -240,
    -239, -238, -237, -236, -235, -234, -233, -232, -230, -229, -228, -227,
    -225, -224, -223, -221, -220, -218, -217, -215, -214, -212, -210, -209,
    -207, -205, -204, -202, -200, -198, -196, -194, -192, -190, -188, -186,
    -184, -182, -180, -178, -176, -173, -171, -169, -167, -164, -162, -160,
    -157, -155, -153, -150, -148, -145, -143, -140, -138, -135, -132, -130,
    -127, -125, -122, -119, -116, -114, -111, -108, -105, -103, -100, -97,
    -94,  -91,  -89,  -86,  -83,  -80,  -77,  -74,  -71,  -68,  -65,  -62,
    -59,  -56,  -53,  -50,  -47,  -44,  -41,  -38,  -35,  -32,  -29,  -26,
    -23,  -20,  -17,  -14,  -11,  -8,   -5,   -2};

static int CosTable512[] = {
    250,  250,  250,  250,  250,  249,  249,  249,  249,  248,  248,  248,
    247,  247,  246,  245,  245,  244,  244,  243,  242,  241,  241,  240,
    239,  238,  237,  236,  235,  234,  233,  232,  230,  229,  228,  227,
    225,  224,  223,  221,  220,  218,  217,  215,  214,  212,  210,  209,
    207,  205,  204,  202,  200,  198,  196,  194,  192,  190,  188,  186,
    184,  182,  180,  178,  176,  173,  171,  169,  167,  164,  162,  160,
    157,  155,  153,  150,  148,  145,  143,  140,  138,  135,  132,  130,
    127,  125,  122,  119,  116,  114,  111,  108,  105,  103,  100,  97,
    94,   91,   89,   86,   83,   80,   77,   74,   71,   68,   65,   62,
    59,   56,   53,   50,   47,   44,   41,   38,   35,   32,   29,   26,
    23,   20,   17,   14,   11,   8,    5,    2,    -2,   -5,   -8,   -11,
    -14,  -17,  -20,  -23,  -26,  -29,  -32,  -35,  -38,  -41,  -44,  -47,
    -50,  -53,  -56,  -59,  -62,  -65,  -68,  -71,  -74,  -77,  -80,  -83,
    -86,  -89,  -91,  -94,  -97,  -100, -103, -105, -108, -111, -114, -116,
    -119, -122, -125, -127, -130, -132, -135, -138, -140, -143, -145, -148,
    -150, -153, -155, -157, -160, -162, -164, -167, -169, -171, -173, -176,
    -178, -180, -182, -184, -186, -188, -190, -192, -194, -196, -198, -200,
    -202, -204, -205, -207, -209, -210, -212, -214, -215, -217, -218, -220,
    -221, -223, -224, -225, -227, -228, -229, -230, -232, -233, -234, -235,
    -236, -237, -238, -239, -240, -241, -241, -242, -243, -244, -244, -245,
    -245, -246, -247, -247, -248, -248, -248, -249, -249, -249, -249, -250,
    -250, -250, -250, -250, -250, -250, -250, -250, -250, -249, -249, -249,
    -249, -248, -248, -248, -247, -247, -246, -245, -245, -244, -244, -243,
    -242, -241, -241, -240, -239, -238, -237, -236, -235, -234, -233, -232,
    -230, -229, -228, -227, -225, -224, -223, -221, -220, -218, -217, -215,
    -214, -212, -210, -209, -207, -205, -204, -202, -200, -198, -196, -194,
    -192, -190, -188, -186, -184, -182, -180, -178, -176, -173, -171, -169,
    -167, -164, -162, -160, -157, -155, -153, -150, -148, -145, -143, -140,
    -138, -135, -132, -130, -127, -125, -122, -119, -116, -114, -111, -108,
    -105, -103, -100, -97,  -94,  -91,  -89,  -86,  -83,  -80,  -77,  -74,
    -71,  -68,  -65,  -62,  -59,  -56,  -53,  -50,  -47,  -44,  -41,  -38,
    -35,  -32,  -29,  -26,  -23,  -20,  -17,  -14,  -11,  -8,   -5,   -2,
    2,    5,    8,    11,   14,   17,   20,   23,   26,   29,   32,   35,
    38,   41,   44,   47,   50,   53,   56,   59,   62,   65,   68,   71,
    74,   77,   80,   83,   86,   89,   91,   94,   97,   100,  103,  105,
    108,  111,  114,  116,  119,  122,  125,  127,  130,  132,  135,  138,
    140,  143,  145,  148,  150,  153,  155,  157,  160,  162,  164,  167,
    169,  171,  173,  176,  178,  180,  182,  184,  186,  188,  190,  192,
    194,  196,  198,  200,  202,  204,  205,  207,  209,  210,  212,  214,
    215,  217,  218,  220,  221,  223,  224,  225,  227,  228,  229,  230,
    232,  233,  234,  235,  236,  237,  238,  239,  240,  241,  241,  242,
    243,  244,  244,  245,  245,  246,  247,  247,  248,  248,  248,  249,
    249,  249,  249,  250,  250,  250,  250,  250};

/* Receiver antenna attenuation in dB for boresight angle = 0:5:180 [deg] */
static double ant_pat_db[37] = {
    0.00,  0.00,  0.22,  0.44,  0.67,  1.11,  1.56,  2.00,  2.44,  2.89,
    3.56,  4.22,  4.89,  5.56,  6.22,  6.89,  7.56,  8.22,  8.89,  9.78,
    10.67, 11.56, 12.44, 13.33, 14.44, 15.56, 16.67, 17.78, 18.89, 20.00,
    21.33, 22.67, 24.00, 25.56, 27.33, 29.33, 31.56};

static channel_t chan[MAXCHAN];   /* Assignable channels of signal */
static int gain[MAXCHAN];         /* Signal gain of channels */
static int allocated_sat[MAXSAT]; /* Allocated channel index for satellite */

/*!
 * @brief Initialize satellite channels for the receiver
 */
void initchan(void)
{
    int i;

    trace(3, "initchan\n");

    /* Clear all channels */
    for (i = 0; i < MAXCHAN; i++)
        chan[i].prn = 0;

    /* Clear satellite allocation flag */
    for (i = 0; i < MAXSAT; i++)
        allocated_sat[i] = -1;
}

/*!
 * @brief Print satellite channels for the receiver
 * @param fp The pointer of output file
 * @param head If print head of records
 */
void printchan(FILE *fp, const int head)
{
    int i;

    if (head)
        fprintf(fp, "Sat Azimu Elev Pseudorange Ionos Tropos\n");

    for (i = 0; i < MAXCHAN; i++) {
        if (chan[i].prn > 0) {
            fprintf(fp, "%c%02d %5.1f %4.1f %11.2f %5.2f %6.2f\n",
                    syschr(chan[i].sys), chan[i].prn, chan[i].azel[0] * R2D,
                    chan[i].azel[1] * R2D, chan[i].rho0.range,
                    chan[i].rho0.iono_delay, chan[i].rho0.trop_delay);
        }
    }
}

/*!
 * @brief Allocate satellite channels for the receiver
 * @param[in] gt GPS time of receiver
 * @param[in] nav Satellite navigation data
 * @param[in] iephs Index of selected ephemeris
 * @param[in] rcv Receiver position (ecef) {x,y,z} (m)
 * @param[in] opt Options setting
 * @return Number of visible satellites
 */
int allocchan(gtime_t gt, nav_t *nav, int *iephs, double *rcv, options_t opt)
{
    int nsat = 0;
    int i, sv, ieph, sys, prn;
    double azel[2];

    range_t rho;
    double ref[3] = {0.0};
    double r_ref, r_xyz;
    double phase_ini;

    trace(3, "allocchan: week=%d, sec=%.2f\n", gt.week, gt.sec);

    for (sv = 0; sv < MAXSAT; sv++) {
        if (opt.exclsats[sv])
            continue;
        if ((ieph = iephs[sv]) < 0)
            continue;
        /* Check satellite system with user option */
        sys = satsys(nav->eph[ieph].sat, &prn);
        if (opt.satsys != sys)
            continue;
        /* Skip process BeiDou GEO satellites */
        if (sys == SYS_BDS && bdsorbit(prn) == BDS_GEO)
            continue;
        if (!satvisible(gt, &nav->eph[ieph], rcv, opt.elvmask, azel)) {
            /* Not visible but allocated */
            if (allocated_sat[sv] >= 0) {
                trace(3, "free channel for sat: %c%02d\n", syschr(sys), prn);
                chan[allocated_sat[sv]].prn = 0;
                allocated_sat[sv] = -1;
            }
            continue;
        }
        nsat++;
        /* Visible and allocated */
        if (allocated_sat[sv] != -1)
            continue;
        /* Visible but not allocated */
        for (i = 0; i < MAXCHAN; i++) {
            if (chan[i].prn != 0)
                continue;
            /* Initialize channel */
            trace(3, "allocate channel for sat: %c%02d\n", syschr(sys), prn);
            chan[i].sys = sys;
            chan[i].prn = prn;
            chan[i].ctype = opt.band;
            chan[i].azel[0] = azel[0];
            chan[i].azel[1] = azel[1];
            /* Spreading code generation */
            chan[i].code = gencode(chan[i].prn, chan[i].ctype, &chan[i].clen,
                                   &chan[i].f_code0);
            /* Neuman-Hoffman code generation */
            if (chan->ctype == CTYPE_B1I || chan->ctype == CTYPE_B3I)
                chan[i].nh =
                    gencode(chan[i].prn, CTYPE_NH20, &chan[i].nhlen, NULL);
            /* Generate subframe */
            eph2sbf(chan[i].sys, &nav->eph[ieph], nav->utc_gps, nav->ion_gps,
                    chan[i].sbf);
            /* Generate navigation message */
            gennavmsg(gt, &chan[i], 1);
            /* Initialize pseudorange */
            calcrange(gt, &nav->eph[ieph], nav->ion_gps, rcv, opt, &rho);
            chan[i].rho0 = rho;
            /* Initialize carrier phase */
            r_xyz = rho.range;
            calcrange(gt, &nav->eph[ieph], nav->ion_gps, ref, opt, &rho);
            r_ref = rho.range;
            if (chan->ctype == CTYPE_B1I)
                phase_ini = (2.0 * r_ref - r_xyz) / LAMBDA_B1I;
            else if (chan->ctype == CTYPE_B3I)
                phase_ini = (2.0 * r_ref - r_xyz) / LAMBDA_B3I;
            else
                phase_ini = (2.0 * r_ref - r_xyz) / LAMBDA_L1CA;
#ifdef FLOAT_CARR_PHASE
            chan[i].carr_phase = phase_ini - floor(phase_ini);
#else
            phase_ini -= floor(phase_ini);
            chan[i].carr_phase = (unsigned int)(512.0 * 65536.0 * phase_ini);
#endif
            break; /* Done */
        }
        /* Set satellite allocation channel */
        if (i < MAXCHAN)
            allocated_sat[sv] = i;
        else
            trace(3, "%c%02d visiable but no channel left\n", syschr(sys), prn);
    }

    return nsat;
}

/*!
 * @brief Update satellite channel status by a new receiver position
 * @param[in] gt GPS time of receiver
 * @param[in] nav Satellite navigation data
 * @param[in] iephs Index of selected ephemeris
 * @param[in] rcv Receiver position (ecef) {x,y,z} (m)
 * @param[in] opt Options setting
 * @return Number of updated channels
 */
int updatechan(gtime_t gt, nav_t *nav, int *iephs, double *rcv, options_t opt)
{
    int nch = 0;
    int i, ibs, sv;
    double path_loss, ant_gain;
    double delt = 1.0 / opt.sample;

    range_t rho;

    trace(3, "updatechan: week=%d, sec=%.2f\n", gt.week, gt.sec);

    for (i = 0; i < MAXCHAN; i++) {
        if (chan[i].prn == 0) /* channel not used */
            continue;

        /* Refresh code phase and data bit counters */
        sv = satno(chan[i].sys, chan[i].prn) - 1;

        /* Current pseudorange */
        calcrange(gt, &nav->eph[iephs[sv]], nav->ion_gps, rcv, opt, &rho);

        chan[i].azel[0] = rho.azel[0];
        chan[i].azel[1] = rho.azel[1];

        /* Update code phase and data bit counters */
        calccaphase(rho, 0.1, &chan[i]);
#ifndef FLOAT_CARR_PHASE
        chan[i].carr_phasestep =
            (int)round(512.0 * 65536.0 * chan[i].f_carr * delt);
#endif

        if (opt.pathloss) {
            path_loss = (nav->eph[iephs[sv]].A - 6371004.0) / rho.d;

            /* Receiver antenna gain, convert elevation to boresight */
            ibs = (int)((90.0 - rho.azel[1] * R2D) / 5.0);
            ant_gain = pow(10.0, -ant_pat_db[ibs] / 20.0);

            /* Signal gain, scaled by 2^7 */
            gain[i] = (int)(path_loss * ant_gain * 128.0);
        }
        else
            gain[i] = 128; /* hold the power level constant */

        nch++;
    }

    return nch;
}

/*!
 * @brief Flush navigation message of channels
 * @param[in] gt GPS time of receiver
 * @param[in] nav Satellite navigation data
 * @param[in] iephs Index of selected ephemeris
 * @param[in] neweph Change to new ephemeris
 */
void flushchan(gtime_t gt, nav_t *nav, int *iephs, const int neweph)
{
    int i, ieph;

    trace(3, "flushchan: week=%d, sec=%.2f\n", gt.week, gt.sec);

    /* Refresh ephemeris and subframes */
    if (neweph) {
        trace(3, "Flush navigation message of channels by new ephemris\n");
        for (i = 0; i < MAXCHAN; i++) {
            if (chan[i].prn == 0)
                continue;
            /* Generate new subframes if allocated */
            ieph = iephs[chan[i].prn - 1];
            eph2sbf(chan[i].sys, &nav->eph[ieph], nav->utc_gps, nav->ion_gps,
                    chan[i].sbf);
        }
    }

    /* Update navigation message */
    for (i = 0; i < MAXCHAN; i++)
        if (chan[i].prn > 0)
            gennavmsg(gt, &chan[i], 0);

    return;
}

/*!
 * @brief Generate signal I/Q bits by a input interval
 * @param[in,out] buff Buffer of signal I/Q values (updated)
 * @param[in] buff_size Size of buffer
 * @param[in] interval Interval of signal, interval = 1/sampling
 */
void gensignal(short *buff, const int buff_size, const double interval)
{
    int i, isamp, itable;
    int ip, qp, i_acc, q_acc;

    trace(3, "gensignal: buff_size=%d\n", buff_size);

    for (isamp = 0; isamp < buff_size; isamp++) {
        i_acc = 0;
        q_acc = 0;

        for (i = 0; i < MAXCHAN; i++) {
            if (chan[i].prn == 0) /* channel not used */
                continue;

#ifdef FLOAT_CARR_PHASE
            itable = (int)floor(chan[i].carr_phase * 512.0);
#else
            itable = (chan[i].carr_phase >> 16) & 0x1FF; /* 9-bit index */
#endif
            /* Generate I/Q bytes of channel */
            ip = chan[i].nhbit * chan[i].databit * chan[i].codechip *
                 CosTable512[itable] * gain[i];
            qp = chan[i].nhbit * chan[i].databit * chan[i].codechip *
                 SinTable512[itable] * gain[i];

            /* Accumulate for all visible satellites */
            i_acc += ip;
            q_acc += qp;

            /* Update code phase */
            chan[i].code_phase += chan[i].f_code * interval;

            if (chan[i].code_phase >= chan[i].clen) {
                chan[i].code_phase -= chan[i].clen;
                chan[i].icode++;

                /* 20 C/A codes = 1 navigation data bit */
                if (chan[i].icode >= 20) {
                    chan[i].icode -= 20;
                    chan[i].ibit++;

                    /* 30 navigation data bits = 1 word */
                    if (chan[i].ibit >= 30) {
                        chan[i].ibit -= 30;
                        chan[i].iword++;
                    }

                    /* Update navigation data bit */
                    /* clang-format off */
                    chan[i].databit =
                        1-2*(int)((chan[i].dwrd[chan[i].iword]>>(29-chan[i].ibit))&0x1UL);
                    /* clang-format on */
                }

                /* Update Neuman-Hoffman bit */
                if (chan[i].ctype == CTYPE_B1I || chan[i].ctype == CTYPE_B3I)
                    chan[i].nhbit = (int)chan[i].nh[chan[i].icode];
            }

            /* Update spreading code chip */
            chan[i].codechip = (int)chan[i].code[(int)chan[i].code_phase];

            /* Update carrier phase */
#ifdef FLOAT_CARR_PHASE
            chan[i].carr_phase += chan[i].f_carr * interval;

            if (chan[i].carr_phase >= 1.0)
                chan[i].carr_phase -= 1.0;
            else if (chan[i].carr_phase < 0.0)
                chan[i].carr_phase += 1.0;
#else
            chan[i].carr_phase += chan[i].carr_phasestep;
#endif
        }

        /* Scaled by 2^7, for 12-bit bladeRF */
        i_acc = (i_acc + 64) >> 7;
        q_acc = (q_acc + 64) >> 7;

        /* Store I/Q samples into buffer */
        buff[isamp * 2] = (short)i_acc;
        buff[isamp * 2 + 1] = (short)q_acc;
    }

    return;
}
