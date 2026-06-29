/*==============================================================================
 * siren.h: Siren constants, types and function prototypes
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Options:
 *     -DDYNAMIC_MAX_DURATION change maximum duration in dynamic mode
 *     -DFLOAT_CARR_PHASE enable float carrier phase
 *     -DENABDS enable BeiDou support
 *     -DENAGAL enablbe Galileo support
 *     -DTRACE enable trace logging
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/09/21 create
 * 			2023/10/17 support tropospheric delay simulation
 *          2023/11/17 change maximum duration in dynamic mode
 *          2024/01/21 rename ephem_t to eph_t and support BeiDou ephemeris
 *          2024/02/03 extend channel_t to suite multi-system process
==============================================================================*/
#ifndef SIREN_H
#define SIREN_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION "0.1.4"  /* software version */
#define PATCH_LEVEL "a1" /* patch level */

/* Higher computational load but smoother carrier phase */
/* #define FLOAT_CARR_PHASE */

#define TRUE 1
#define FALSE 0

#define PI 3.1415926535898 /* pi in ICD-GPS-200 */
#define D2R (PI / 180.0)   /* degree to radian */
#define R2D (180.0 / PI)   /* radian to degree */
#define CLIGHT 299792458.0 /* speed of light (m/s) */

#define SYS_NONE 0x00 /* navigation system: none */
#define SYS_GPS 0x01  /* navigation system: GPS */
#define SYS_GAL 0x08  /* navigation system: Galileo */
#define SYS_BDS 0x20  /* navigation system: BeiDou */
#define SYS_ALL 0x29  /* navigation system: all */

#define TSYS_GPS 0 /* time system: GPS time */
#define TSYS_UTC 1 /* time system: UTC */
#define TSYS_GAL 3 /* time system: Galileo time */
#define TSYS_BDS 5 /* time system: BeiDou time */

#define MINPRNGPS 1  /* min satellite PRN number of GPS */
#define MAXPRNGPS 32 /* max satellite PRN number of GPS */
#define NSATGPS (MAXPRNGPS - MINPRNGPS + 1) /* number of GPS satellites */
#define NSYSGPS 1

#ifdef ENAGAL
#define MINPRNGAL 1  /* min satellite PRN number of Galileo */
#define MAXPRNGAL 36 /* max satellite PRN number of Galileo */
#define NSATGAL (MAXPRNGAL - MINPRNGAL + 1) /* number of Galileo satellites */
#define NSYSGAL 1
#else
#define MINPRNGAL 0
#define MAXPRNGAL 0
#define NSATGAL 0
#define NSYSGAL 0
#endif
#ifdef ENABDS
#define MINPRNBDS 1  /* min satellite sat number of BeiDou */
#define MAXPRNBDS 63 /* max satellite sat number of BeiDou */
#define NSATBDS (MAXPRNBDS - MINPRNBDS + 1) /* number of BeiDou satellites */
#define NSYSBDS 1
#else
#define MINPRNBDS 0
#define MAXPRNBDS 0
#define NSATBDS 0
#define NSYSBDS 0
#endif
#define NSYS (NSYSGPS + NSYSGAL + NSYSBDS)   /* number of systems */
#define MAXSAT (NSATGPS + NSATGAL + NSATBDS) /* max satellite number */

#define BD0WEEK 1356 /* GPS week number of BeiDou Time origin */

#define BDS_GEO 1  /* BeiDou GEO satellite */
#define BDS_IGSO 2 /* BeiDou IGSO satellite */
#define BDS_MEO 3  /* BeiDou MEO satellite */

#define MAXDTOE_GPS 7200.0  /* max time difference to GPS Toe (s) */
#define MAXDTOE_GAL 14400.0 /* max time difference to Galileo Toe (s) */
#define MAXDTOE_BDS 21600.0 /* max time difference to BeiDou Toe (s) */

/* Code type, keep up with GNSS-SDRLIB */
#define CTYPE_L1CA 1  /* GPS/QZSS L1C/A */
#define CTYPE_E1B 9   /* Galileo E1B (Data) */
#define CTYPE_B1I 22  /* BeiDou B1I */
#define CTYPE_B3I 24  /* BeiDou B1I */
#define CTYPE_NH20 29 /* 20 bit Neuman-Hoffman code */

/* Code chip rate (chip/s) */
#define CRATE_L1CA 1.023e6   /* GPS/QZSS L1C/A */
#define CRATE_E1B 1.023e6    /* Galileo E1B (Data) */
#define CRATE_B1IB2I 2.046e6 /* BeiDou B1I/B2I */
#define CRATE_B3I 10.23e6    /* BeiDou B3I */
#define CRATE_NH20 500       /* 20 bit Neuman-Hoffman code */

/* Code chip length (chip) */
#define LEN_L1CA 1023   /* GPS/QZSS L1C/A */
#define LEN_E1B 4092    /* Galileo E1B (Data) */
#define LEN_B1IB2I 2046 /* BeiDou B1I/B2I */
#define LEN_B3I 10230   /* BeiDou B3I */
#define LEN_NH20 20     /* 20 bit Neuman-Hoffman code */

/* Conventional values employed in GPS ephemeris model (ICD-GPS-200) */
#define GM_WGS84 3.986005e14
#define OMEGA_WGS84 7.2921151467e-5 /* earth angular velocity (rad/s) */
#define RADIUS_WGS84 6378137.0      /* earth semimajor axis (WGS84) (m) */
#define ECCENTRICITY_WGS84 0.0818191908426 /* earth eccentricity */

#define FREQ_L1CA 1575.42e6 /* GPS L1 C/A carrier frequency (Hz) */
#define FREQ_E1B 1575.42e6  /* Galileo E1 carrier frequency (Hz) */
#define FREQ_B1I 1561.098e6 /* BeiDou B1I carrier frequency (Hz) */
#define FREQ_B3I 1268.520e6 /* BeiDou B3I carrier frequency (Hz) */

#define LAMBDA_L1CA (CLIGHT / FREQ_L1CA) /* GPS L1 C/A carrier lambda (m) */
#define LAMBDA_E1B (CLIGHT / FREQ_E1B)   /* Galileo E1 carrier lambda (m) */
#define LAMBDA_B1I (CLIGHT / FREQ_B1I)   /* BeiDou B1I carrier lambda (m) */
#define LAMBDA_B3I (CLIGHT / FREQ_B3I)   /* BeiDou B3I carrier lambda (m) */

#define MAXCHAR 100 /* Maximum length of a line in text file */
#define MAXCHAN 32  /* Maximum number of channels we simulate */

#ifndef DYNAMIC_MAX_DURATION
#define DYNAMIC_MAX_DURATION 3600 /* Maximum duration for dynamic mode (s) */
#endif

#define STATIC_MAX_DURATION 86400 /* Maximum duration for static mode (s) */

#define N_SBF 5                           /* Number of subframes */
#define N_DWRD_SBF 10                     /* Number of words per subframe */
#define N_DWRD ((N_SBF + 1) * N_DWRD_SBF) /* Number of words, buffer size */

#define SECONDS_IN_DAY 86400.0

#define IONOOPT_OFF 0  /* Ionosphere option: correction off */
#define IONOOPT_BRDC 1 /* Ionosphere option: broadcast model */

#define TROPOPT_OFF 0  /* Troposphere option: correction off */
#define TROPOPT_SAAS 1 /* Troposphere option: Saastamoinen model */

#define SC01 1  /* Sampling data format, 1 bit */
#define SC08 8  /* Sampling data format, 8 bit */
#define SC16 16 /* Sampling data format, 16 bit */

/*! @brief Structure representing GNSS time */
typedef struct
{
    int week;   /* GPS week number (since 1980/01/06) */
    double sec; /* second inside the GPS week */
} gtime_t;

/*! @brief Structure repreenting UTC time */
typedef struct
{
    int year;   /* Calendar year */
    int month;  /* Calendar month */
    int day;    /* Calendar day */
    int hour;   /* Calendar hour */
    int min;    /* Calendar minutes */
    double sec; /* Calendar seconds */
} datetime_t;

/*! @brief GPS/BDS/QZS/GAL broadcast ephemeris type */
typedef struct
{
    int sat;  /* satellite number */
    int iode; /* Issue of Data, Clock */
    int iodc; /* Isuse of Data, Ephemeris */
    int sva;  /* SV accuracy (URA index) */
    int svh;  /* SV health (0:ok) */
    int week; /* GPS/QZS: gps week, GAL: galileo week */
    int code; /* GPS/QZS: code on L2 */
              /* GAL: data source defined as rinex 3.03 */
              /* BDS: source (0:unknown,1:B1I,2:B1Q,3:B2I,4:B2Q,5:B3I,6:B3Q) */
    int flag; /* GPS/QZS: L2 P data flag */
              /* BDS: nav type (0:unknown,1:IGSO/MEO,2:GEO) */
    gtime_t toc;   /* Time of Clock */
    gtime_t toe;   /* Time of Ephemeris */
    gtime_t ttr;   /* T_trans */
                   /* SV orbit parameters */
    double A;      /* Semi-major axis */
    double e;      /* Eccentricity */
    double i0;     /* Inclination (radians) */
    double idot;   /* IDOT (radians/s) */
    double deln;   /* Delta-N (radians/sec) */
    double M0;     /* Mean anamoly (radians) */
    double omg;    /* omega (radians) */
    double OMG0;   /* Longitude of the ascending node (radians) */
    double OMGd;   /* Omega dot (radians/s) */
    double cuc;    /* Cuc (radians) */
    double cus;    /* Cus (radians) */
    double cic;    /* Correction to inclination cos (radians) */
    double cis;    /* Correction to inclination sin (radians) */
    double crc;    /* Correction to radius cos (meters) */
    double crs;    /* Correction to radius sin (meters) */
    double toes;   /* Toe (s) in week */
    double fit;    /* fit interval (h) */
    double f0;     /* Clock offset (seconds) */
    double f1;     /* rate (sec/sec) */
    double f2;     /* acceleration (sec/sec^2) */
    double tgd[6]; /* group delay parameters */
                   /* GPS/QZS: tgd[0]=TGD */
                   /* GAL:tgd[0]=BGD_E1E5a,tgd[1]=BGD_E1E5b */
                   /* BDS:tgd[0]=TGD_B1I,tgd[1]=TGD_B2I/B2b */
                   /*     tgd[2]=TGD_B1Cp,tgd[3]=TGD_B2ap */
                   /*     tgd[4]=ISC_B1Cd,tgd[5]=ISC_B2ad */
    double Adot;   /* Adot for CNAV */
    double ndot;   /* ndot for CNAV */
} eph_t;

typedef struct
{                      /* navigation data type */
    int n, nmax;       /* number of broadcast ephemeris */
    eph_t *eph;        /* GPS/QZS/GAL/BDS/IRN ephemeris */
    double utc_gps[8]; /* GPS UTC parameters */
                       /* {A0,A1,Tot,WNt,dtLS,WNLSF,DN,dtLSF} */
    double utc_gal[8]; /* Galileo UTC parameters */
    double utc_bds[8]; /* BeiDou UTC parameters */
    double ion_gps[8]; /* GPS iono parameters {a0,a1,a2,a3,b0,b1,b2,b3} */
    double ion_gal[4]; /* Galileo iono parameters {ai0,ai1,ai2,0} */
    double ion_bds[8]; /* BeiDou iono parameters {a0,a1,a2,a3,b0,b1,b2,b3} */
} nav_t;

/*! @brief Range between satellite and receiver */
typedef struct
{
    gtime_t gt;        /* Receiving time */
    double range;      /* Pseudorange */
    double rate;       /* Pseudorange rate */
    double d;          /* Geometric distance */
    double azel[2];    /* Azimuth and elevation angles */
    double iono_delay; /* Ionospheric delay */
    double trop_delay; /* Tropospheric delay */
} range_t;

/*! @brief Structure representing a Channel */
typedef struct
{
    int sys;        /* Satellite system */
    int prn;        /* Satellite number */
    int ctype;      /* Code type: CTYPE_L1CA, ... */
    short *code;    /* Spreading code Sequence */
    int clen;       /* Length of code Sequence */
    short *nh;      /* Neuman-Hoffman code */
    int nhlen;      /* Length of Neuman-Hoffman code */
    double f_carr;  /* Carrier frequency change by Doppler effect (Hz) */
    double f_code;  /* Code frequency (chip/s) */
    double f_code0; /* Base code frequency (chip/s) */
#ifdef FLOAT_CARR_PHASE
    double carr_phase;
#else
    unsigned int carr_phase; /* Carrier phase */
    int carr_phasestep;      /* Carrier phasestep */
#endif
    double code_phase;                /* Code phase */
    gtime_t g0;                       /* GPS time at full frame start */
    unsigned long sbf[5][N_DWRD_SBF]; /* current subframe */
    unsigned long dwrd[N_DWRD];       /* Data words of subframe:
                                       *  0->10: subframe 5 of last frame
                                       * 11->60: subframe 1->5 of this frame */
    int iword;                        /* initial word */
    int ibit;                         /* initial bit */
    int icode;                        /* initial code */
    int codechip;                     /* current C/A chip, electrical value */
    int databit;                      /* current data bit, electrical value */
    int nhbit;                        /* current NH bit, electrical value */
    double azel[2];
    range_t rho0;
} channel_t;

/*! @brief Options of the software */
typedef struct
{
    int satsys;           /* Satellite system */
    int exclsats[MAXSAT]; /* Exclude satellite flag */
    int band;             /* Frequency band */
    double sample;        /* Sampling frequency (Hz) */
    double elvmask;       /* Elevation mask angle (degree) */
    int pathloss;         /* Path loss */
    int ionospheric;      /* Ionospheric delay model */
    int tropospheric;     /* Tropospheric delay model */
    int bits;             /* I/Q data format [1/8/16] */
    int verbose;          /* Verbose mode enable */
} options_t;

extern const options_t opt_default; /* Defaults options */

/* Basic functions, for math, time and coordinate convert --------------------*/

void sub3(const double *x1, const double *x2, double *y);
double norm3(const double *x);
double dot3(const double *x1, const double *x2);

double str2num(const char *str, int i, int n);
datetime_t str2date(const char *str, int i, int n);

int satno(int sys, int prn);
int satid2no(const char *id);
int satsys(int sat, int *prn);
char syschr(int sys);

gtime_t bdt2time(int week, double sec);
gtime_t bdt2gpst(gtime_t t0);
gtime_t gpst2bdt(gtime_t t0);

int bdsorbit(int prn);

gtime_t date2gpst(const datetime_t dt);
datetime_t gpst2date(const gtime_t gt);
gtime_t addgtime(gtime_t g0, double dt);
double subgtime(gtime_t g1, gtime_t g0);

void xyz2llh(const double *xyz, double *llh);
void llh2xyz(const double *llh, double *xyz);
void ltcmat(const double *llh, double t[3][3]);
void ecef2neu(const double *xyz, double t[3][3], double *neu);
void neu2azel(const double *neu, double *azel);

/* Functions for signal simulation -------------------------------------------*/

short *gencode(int prn, int ctype, int *len, double *crate);
void eph2sbf(int sys, const eph_t *eph, const double *utc, const double *iono,
             unsigned long sbf[5][N_DWRD_SBF]);
int gennavmsg(gtime_t gt, channel_t *chan, int init);
void calcrange(gtime_t gt, const eph_t *eph, const double *iono, double xyz[],
               options_t opt, range_t *rho);
void calccaphase(range_t rho1, double dt, channel_t *chan);

/* Functions for satellite's position and velocity compution -----------------*/

void satpos(gtime_t time, const eph_t *eph, double *pos, double *dts);
void satvel(gtime_t time, const eph_t *eph, double *vel, double *ddts);

int satvisible(gtime_t gt, eph_t *eph, const double *rcv, double elvmask,
               double *azel);

/* Functions for data input/output (I/O) -------------------------------------*/

int readopt(const char *filename, options_t *opt);
int readnav(const char *filename, nav_t *nav);
int readtraj_csv(const char *filename, const int length, double *rcvpos);
int readtraj_gga(const char *filename, const int length, double *rcvpos);
int writeiq(short *buff, const int buff_size, FILE *fp, const int format);

/* Functions for error models ------------------------------------------------*/

double ionodelay(gtime_t gt, const double *iono, double *rcv, double *azel);
double tropdelay(const double *pos, const double *azel, double humi);

/* Functions for channel operation of signal ---------------------------------*/

void initchan(void);
void printchan(FILE *fp, const int head);
int allocchan(gtime_t gt, nav_t *nav, int *iephs, double *rcv, options_t opt);
int updatechan(gtime_t gt, nav_t *nav, int *iephs, double *rcv, options_t opt);
void flushchan(gtime_t gt, nav_t *nav, int *iephs, const int neweph);
void gensignal(short *buff, const int buff_size, const double interval);

/* Functions for trace print -------------------------------------------------*/

void traceopen(const char *file);
void traceclose(void);
void tracelevel(int level);
void trace(int level, const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif /* SIREN_H */
