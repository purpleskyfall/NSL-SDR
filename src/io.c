/*==============================================================================
 * io.c: Functions of input and output.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/09/21 create
 *          2023/01/18 add RINEX 3 navigation file support
==============================================================================*/
#include "siren.h"
#include <stdarg.h>

#define SQR(x) ((x) * (x))

/*!
 * @brief Decode header of navigation file
 * @param[in] buff Buffer of string line
 * @param[out] nav Navigation data
 */
static void decode_navh(char *buff, nav_t *nav)
{
    int i, j;
    char *label = buff + 60;

    trace(4, "decode_navh:\n");

    if (strstr(label, "ION ALPHA")) { /* optional in RINEX 2 */
        if (nav) {
            for (i = 0, j = 2; i < 4; i++, j += 12)
                nav->ion_gps[i] = str2num(buff, j, 12);
        }
    }
    else if (strstr(label, "ION BETA")) { /* optional in RINEX 2 */
        if (nav) {
            for (i = 0, j = 2; i < 4; i++, j += 12)
                nav->ion_gps[i + 4] = str2num(buff, j, 12);
        }
    }
    else if (strstr(label, "DELTA-UTC: A0,A1,T,W")) { /* optional in RINEX 2 */
        if (nav) {
            for (i = 0, j = 3; i < 2; i++, j += 19)
                nav->utc_gps[i] = str2num(buff, j, 19);
            for (; i < 4; i++, j += 9)
                nav->utc_gps[i] = str2num(buff, j, 9);
        }
    }
    else if (strstr(label, "IONOSPHERIC CORR")) { /* optional in RINEX 3 */
        if (nav) {
            if (!strncmp(buff, "GPSA", 4)) {
                for (i = 0, j = 5; i < 4; i++, j += 12)
                    nav->ion_gps[i] = str2num(buff, j, 12);
            }
            else if (!strncmp(buff, "GPSB", 4)) {
                for (i = 0, j = 5; i < 4; i++, j += 12)
                    nav->ion_gps[i + 4] = str2num(buff, j, 12);
            }
            else if (!strncmp(buff, "GAL", 3)) {
                for (i = 0, j = 5; i < 4; i++, j += 12)
                    nav->ion_gal[i] = str2num(buff, j, 12);
            }
            else if (!strncmp(buff, "BDSA", 4)) { /* RINEX 3.02 */
                for (i = 0, j = 5; i < 4; i++, j += 12)
                    nav->ion_bds[i] = str2num(buff, j, 12);
            }
            else if (!strncmp(buff, "BDSB", 4)) { /* RINEX 3.02 */
                for (i = 0, j = 5; i < 4; i++, j += 12)
                    nav->ion_bds[i + 4] = str2num(buff, j, 12);
            }
        }
    }
    else if (strstr(label, "TIME SYSTEM CORR")) { /* optional in RINEX 3 */
        if (nav) {
            if (!strncmp(buff, "GPUT", 4)) {
                nav->utc_gps[0] = str2num(buff, 5, 17);
                nav->utc_gps[1] = str2num(buff, 22, 16);
                nav->utc_gps[2] = str2num(buff, 38, 7);
                nav->utc_gps[3] = str2num(buff, 45, 5);
            }
            else if (!strncmp(buff, "GAUT", 4)) { /* RINEX 3.02 */
                nav->utc_gal[0] = str2num(buff, 5, 17);
                nav->utc_gal[1] = str2num(buff, 22, 16);
                nav->utc_gal[2] = str2num(buff, 38, 7);
                nav->utc_gal[3] = str2num(buff, 45, 5);
            }
            else if (!strncmp(buff, "BDUT", 4)) { /* RINEX 3.02 */
                nav->utc_bds[0] = str2num(buff, 5, 17);
                nav->utc_bds[1] = str2num(buff, 22, 16);
                nav->utc_bds[2] = str2num(buff, 38, 7);
                nav->utc_bds[3] = str2num(buff, 45, 5);
            }
        }
    }
    else if (strstr(label, "LEAP SECONDS")) { /* optional */
        if (nav) {
            nav->utc_gps[4] = str2num(buff, 0, 6);
            nav->utc_gps[7] = str2num(buff, 6, 6);
            nav->utc_gps[5] = str2num(buff, 12, 6);
            nav->utc_gps[6] = str2num(buff, 18, 6);
        }
    }
}

/*!
 * @brief Decode body of navigation
 * @param[in] sat Satellite number
 * @param[in] toc Time of clock
 * @param[in] data Data of navigation body
 * @param[out] eph Decoded ephemeris data
 * @return Status (0: error, 1: ok)
 */
static int decode_eph(int sat, gtime_t toc, const double *data, eph_t *eph)
{
    eph_t eph0 = {0};
    int sys;

    trace(4, "decode_eph: sat=%2d\n", sat);

    sys = satsys(sat, NULL);
    if (!(sys & (SYS_GPS | SYS_GAL | SYS_BDS))) {
        trace(3, "ephemeris error: invalid satellite sat=%2d\n", sat);
        return 0;
    }

    *eph = eph0;

    eph->sat = sat;
    eph->toc = toc;

    eph->f0 = data[0];
    eph->f1 = data[1];
    eph->f2 = data[2];

    eph->A = SQR(data[10]);
    eph->e = data[8];
    eph->i0 = data[15];
    eph->OMG0 = data[13];
    eph->omg = data[17];
    eph->M0 = data[6];
    eph->deln = data[5];
    eph->OMGd = data[18];
    eph->idot = data[19];
    eph->crc = data[16];
    eph->crs = data[4];
    eph->cuc = data[7];
    eph->cus = data[9];
    eph->cic = data[12];
    eph->cis = data[14];

    if (sys == SYS_GPS) {
        eph->iode = (int)data[3];  /* IODE */
        eph->iodc = (int)data[26]; /* IODC */
        eph->week = (int)data[21]; /* GPS week */
        eph->toe.week = eph->week; /* Toe */
        eph->toe.sec = data[11];
        eph->toes = data[11];
        eph->ttr.week = eph->week; /* T_trans */
        eph->ttr.sec = data[27];

        eph->code = (int)data[20]; /* GPS: codes on L2 ch */
        eph->svh = (int)data[24];  /* SV health */
        eph->sva = (int)data[23];  /* URA */
        eph->flag = (int)data[22]; /* GPS: L2 P data flag */
        eph->tgd[0] = data[25];    /* TGD */
        eph->fit = data[28];       /* Fit interval (0: 1h, 1: >2h) */
    }
    else if (sys == SYS_GAL) {     /* Galileo RINEX 3 */
        eph->iode = (int)data[3];  /* IODnav */
        eph->week = (int)data[21]; /* Galileo week = GPS week */
        eph->toe.week = eph->week; /* Toe */
        eph->toe.sec = data[11];
        eph->ttr.week = eph->week; /* T_trans */
        eph->ttr.sec = data[27];

        eph->code = (int)data[20]; /* data sources */
                                   /* bit 0 set: I/NAV E1-B */
                                   /* bit 1 set: F/NAV E5a-I */
                                   /* bit 2 set: F/NAV E5b-I */
                                   /* bit 8 set: af0-af2 toc are for E5a.E1 */
                                   /* bit 9 set: af0-af2 toc are for E5b.E1 */
        eph->svh = (int)data[24];  /* sv health */
                                   /* bit     0: E1B DVS */
                                   /* bit   1-2: E1B HS */
                                   /* bit     3: E5a DVS */
                                   /* bit   4-5: E5a HS */
                                   /* bit     6: E5b DVS */
                                   /* bit   7-8: E5b HS */
        eph->sva = (int)data[23];  /* sisa */

        eph->tgd[0] = data[25]; /* BGD E5a/E1 */
        eph->tgd[1] = data[26]; /* BGD E5b/E1 */
    }
    else if (sys == SYS_BDS) {         /* BeiDou RINEX 3.02 */
        eph->toc = bdt2gpst(eph->toc); /* BDT -> GPST */
        eph->iode = (int)data[3];      /* AODE */
        eph->iodc = (int)data[28];     /* AODC */
        eph->week = (int)data[21];     /* bdt week */
        eph->toes = data[11];          /* Toe (s) in BDT week */
        eph->toe = bdt2gpst(bdt2time(eph->week, data[11])); /* BDT -> GPST */
        eph->ttr = bdt2gpst(bdt2time(eph->week, data[27])); /* BDT -> GPST */

        eph->svh = (int)data[24]; /* satH1 */
        eph->sva = (int)data[23]; /* URA */

        eph->tgd[0] = data[25]; /* TGD1 B1/B3 */
        eph->tgd[1] = data[26]; /* TGD2 B2/B3 */
    }
    if (eph->iode < 0 || 1023 < eph->iode) {
        trace(2, "rinex nav invalid: sat=%2d iode=%d\n", sat, eph->iode);
    }
    if (eph->iodc < 0 || 1023 < eph->iodc) {
        trace(2, "rinex nav invalid: sat=%2d iodc=%d\n", sat, eph->iodc);
    }
    return 1;
}

/*!
 * @brief Read RINEX navigation data body
 * @param[in] fp The opened RINEX file
 * @param[in] ver Version of RINEX file
 * @param[in] sys System of navigation satellite
 * @param[out] eph Ephemeris of a satellite
 * @return Status (0: error, 1:ok, -1: end)
 */
static int readrnxnavb(FILE *fp, double ver, int sys, eph_t *eph)
{
    datetime_t dt;
    gtime_t toc;
    double data[64];
    int i = 0, j, prn, sat = 0, sp = 3;
    char buff[MAXCHAR], id[8] = "", *p;

    trace(4, "readrnxnavb: ver=%.2f sys=%d\n", ver, sys);

    while (fgets(buff, MAXCHAR, fp)) {
        if (i == 0) {
            /* decode satellite field */
            if (ver >= 3.0 || sys == SYS_GAL) { /* RINEX 3 or GAL/QZS */
                sprintf(id, "%.3s", buff);
                sat = satid2no(id);
                sp = 4;
                if (ver >= 3.0) {
                    sys = satsys(sat, NULL);
                    if (!sys) {
                        sys = SYS_GPS;
                    }
                }
            }
            else {
                prn = (int)str2num(buff, 0, 2);
                sat = satno(SYS_GPS, prn);
            }
            /* decode Toc field */
            dt = str2date(buff + sp, 0, 19);
            if (dt.year < 1980) {
                trace(2, "rinex nav toc error: %23.23s\n", buff);
                return 0;
            }
            toc = date2gpst(dt);
            /* decode data fields */
            for (j = 0, p = buff + sp + 19; j < 3; j++, p += 19) {
                data[i++] = str2num(p, 0, 19);
            }
        }
        else {
            /* decode data fields */
            for (j = 0, p = buff + sp; j < 4; j++, p += 19) {
                data[i++] = str2num(p, 0, 19);
            }
            /* decode ephemeris */
            if (i >= 31) {
                return decode_eph(sat, toc, data, eph);
            }
        }
    }
    return -1;
}

/*!
 * @brief Add ephemeris to navigation data
 * @param[in,out] nav Navigation data[update]
 * @param[in] eph New ephemeris
 * @return Status (0: error, 1: ok)
 */
static int add_eph(nav_t *nav, const eph_t *eph)
{
    eph_t *nav_eph;

    if (nav->nmax <= nav->n) {
        nav->nmax += 1024;
        if (!(nav_eph =
                  (eph_t *)realloc(nav->eph, sizeof(eph_t) * nav->nmax))) {
            trace(1, "decode_eph malloc error: n=%d\n", nav->nmax);
            free(nav->eph);
            nav->eph = NULL;
            nav->n = nav->nmax = 0;
            return 0;
        }
        nav->eph = nav_eph;
    }
    nav->eph[nav->n++] = *eph;
    return 1;
}

/*!
 * @brief Read ephemeris data from the RINEX Navigation file
 * @param[in] filename File name of the RINEX file
 * @param[out] nav Output satellite ephemeris data
 * @return Number of ephemerides in the file
 */
int readnav(const char *filename, nav_t *nav)
{
    FILE *fp;
    eph_t eph;
    char buff[MAXCHAR], *label = buff + 60;
    double ver = 0.0;
    int stat, sys = SYS_GPS;

    trace(3, "readnav: %s\n", filename);

    if (NULL == (fp = fopen(filename, "rt")))
        return -1;

    /* Read header lines */
    while (fgets(buff, MAXCHAR, fp)) {
        if (strstr(label, "END OF HEADER"))
            break;

        if (strlen(buff) <= 60)
            continue;
        else if (strstr(label, "RINEX VERSION / TYPE")) {
            if ((ver = str2num(buff, 0, 9)) >= 4.0) {
                trace(2, "not supported RINEX version: %.2f", ver);
                return -2;
            }
            /* satellite system */
            switch (*(buff + 40)) {
                case ' ':
                case 'G': sys = SYS_GPS; break;
                case 'E': sys = SYS_GAL; break;  /* RINEX 2.12 */
                case 'C': sys = SYS_BDS; break;  /* RINEX 2.12 */
                case 'M': sys = SYS_NONE; break; /* mixed */
                default:
                    trace(2, "not supported satellite system: %c\n",
                          *(buff + 40));
                    break;
            }
            continue;
        }
        else if (strstr(label, "COMMENT"))
            continue;
        else if (strstr(label, "PGM / RUN BY / DATE"))
            continue;
        else
            decode_navh(buff, nav);
    }

    /* Read body lines */
    while ((stat = readrnxnavb(fp, ver, sys, &eph)) >= 0) {
        /* add ephemeris to navigation data */
        if (stat) {
            if (!add_eph(nav, &eph))
                return 0;
        }
    }

    return nav->n;
}

/*!
 * @brief Read the trace from the CSV format file
 * @param[in] filename File name of the input file
 * @param[in] length Max length of the user motion
 * @param[out] rcvpos Output array of ECEF vectors for user motion
 * @return Number of trace records read (-1: open error, 0: empty file)
 */
int readtraj_csv(const char *filename, const int length, double *rcvpos)
{
    FILE *fp;
    int numd;
    double t, llh[3];
    char str[MAXCHAR];

    trace(2, "readtraj_csv: filename=%s, length=%d\n", filename, length);

    if (NULL == (fp = fopen(filename, "rt")))
        return -1;

    for (numd = 0; numd < length; numd++) {
        if (fgets(str, MAXCHAR, fp) == NULL)
            break;
        /* Read CSV line */
        if (EOF ==
            sscanf(str, "%lf,%lf,%lf,%lf", &t, &llh[0], &llh[1], &llh[2]))
            break;

        if (llh[0] > 90.0 || llh[0] < -90.0 || llh[1] > 180.0 ||
            llh[1] < -180.0) {
            fprintf(stderr, "ERROR: Invalid file format "
                            "(time,latitude,longitude,height).\n");
            numd = 0; /* Empty user motion */
            break;
        }

        llh[0] *= D2R; /* convert to radians */
        llh[1] *= D2R; /* convert to radians */

        llh2xyz(llh, &rcvpos[numd * 3]);
    }

    fclose(fp);

    return numd;
}

/*!
 * @brief Read the trace from the GPGGA format file
 * @param[in] filename File name of the input file
 * @param[in] length Max length of the user motion
 * @param[out] rcvpos Output array of ECEF vectors for user motion
 * @return Number of trace records read (-1: open error, 0: empty file)
 */
int readtraj_gga(const char *filename, const int length, double *rcvpos)
{
    FILE *fp;
    int numd = 0;
    char str[MAXCHAR];
    char *token;
    double llh[3];
    char tmp[8];

    trace(2, "readtraj_gga: filename=%s, length=%d\n", filename, length);

    if (NULL == (fp = fopen(filename, "rt")))
        return -1;

    for (numd = 0; numd < length; numd++) {
        if (fgets(str, MAXCHAR, fp) == NULL)
            break;

        token = strtok(str, ",");

        if (strncmp(token + 3, "GGA", 3) == 0) {
            token = strtok(NULL, ","); /* Date and time */

            token = strtok(NULL, ","); /* Latitude */
            strncpy(tmp, token, 2);
            tmp[2] = 0;

            llh[0] = atof(tmp) + atof(token + 2) / 60.0;

            token = strtok(NULL, ","); /* North or south */
            if (token[0] == 'S')
                llh[0] *= -1.0;

            llh[0] *= D2R; /* in radian */

            token = strtok(NULL, ","); /* Longitude */
            strncpy(tmp, token, 3);
            tmp[3] = 0;

            llh[1] = atof(tmp) + atof(token + 3) / 60.0;

            token = strtok(NULL, ","); /* East or west */
            if (token[0] == 'W')
                llh[1] *= -1.0;

            llh[1] *= D2R; /* in radian */

            token = strtok(NULL, ","); /* GPS fix */
            token = strtok(NULL, ","); /* Number of satellites */
            token = strtok(NULL, ","); /* HDOP */

            token = strtok(NULL, ","); /* Altitude above mean sea level */

            llh[2] = atof(token);

            token = strtok(NULL, ","); /* in meter */

            token =
                strtok(NULL, ","); /* Geodetic height above WGS84 ellipsoid */

            llh[2] += atof(token);

            /* Convert geodetic position into ECEF coordinates */
            llh2xyz(llh, &rcvpos[numd * 3]);
        }
    }

    fclose(fp);

    return numd;
}

/*!
 * @brief Write IQ signal buffer into an opening file
 * @param[in] buff Buffer of IQ data
 * @param[in] buff_size IQ buffer size
 * @param[in] fp The pointer of output file
 * @param[in] format Output bits option, in [SC01, SC08, SC16]
 * @return Status (0: successful, 1: error)
 */
int writeiq(short *buff, const int buff_size, FILE *fp, const int format)
{
    int i;
    signed char *iq8_buff = NULL;

    trace(3, "writeiq: buff_size=%d, format=%d\n", buff_size, format);

    if (format == SC01) {
        /* 1 byte = {I0, Q0, I1, Q1, I2, Q2, I3, Q3} */
        iq8_buff = (signed char *)calloc(buff_size / 4, 1);
        if (iq8_buff == NULL) {
            trace(2, "failed to allocate 1-bit I/Q buffer.\n");
            return 1;
        }
    }
    else if (format == SC08) {
        iq8_buff = (signed char *)calloc(2 * buff_size, 1);
        if (iq8_buff == NULL) {
            trace(2, "failed to allocate 8-bit I/Q buffer.\n");
            return 1;
        }
    }

    if (format == SC01) {
        for (i = 0; i < 2 * buff_size; i++) {
            if (i % 8 == 0)
                iq8_buff[i / 8] = 0x00;

            iq8_buff[i / 8] |= (buff[i] > 0 ? 0x01 : 0x00) << (7 - i % 8);
        }

        fwrite(iq8_buff, 1, buff_size / 4, fp);
    }
    else if (format == SC08) {
        for (i = 0; i < 2 * buff_size; i++)
            /* 12-bit bladeRF -> 8-bit HackRF */
            iq8_buff[i] = buff[i] >> 4;

        fwrite(iq8_buff, 1, 2 * buff_size, fp);
    }
    /* format == SC16 */
    else {
        fwrite(buff, 2, 2 * buff_size, fp);
    }

    return 0;
}

#ifdef TRACE

static FILE *fp_trace = NULL;    /* file pointer of trace */
static char file_trace[MAXCHAR]; /* trace file */
static int level_trace = 0;      /* level of trace */

void traceopen(const char *file)
{

    if (!(fp_trace = fopen(file, "w")))
        fp_trace = stderr;
    strcpy(file_trace, file);
}
void traceclose(void)
{
    if (fp_trace && fp_trace != stderr)
        fclose(fp_trace);
    fp_trace = NULL;
    file_trace[0] = '\0';
}
void tracelevel(int level) { level_trace = level; }
void trace(int level, const char *format, ...)
{
    va_list ap;

    /* print error message to stderr */
    if (level <= 1) {
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
    }
    if (!fp_trace || level > level_trace)
        return;
    fprintf(fp_trace, "%d ", level);
    va_start(ap, format);
    vfprintf(fp_trace, format, ap);
    va_end(ap);
    fflush(fp_trace);
}
#else
void traceopen(const char *file) {}
void traceclose(void) {}
void tracelevel(int level) {}
void trace(int level, const char *format, ...) {}
#endif
