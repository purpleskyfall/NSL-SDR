/*==============================================================================
 * siren.c: Generate emulational GNSS signal
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/09/21 create
==============================================================================*/
#include "siren.h"

#define TRACEFILE "siren.trace"

/* clang-format off */
static const char *help[] = {
    "Usage: siren <navfile> [options]",
    "",
    "Siren project for GNSS signal (GPS L1 C/A, Beidou B1I & B3I) simulation.",
    "",
    "In dynamic mode, a trajectory can be specified in either a CSV file, which",
    "contains the geodetic positions, or a NMEA GGA stream. The sampling rate of",
    "trajectory has to be 10Hz. You can also start the static mode by inputting",
    "a geodetic position. The longest simulation duration for the dynamic mode is",
    "3600s, while for the static mode, it's 86400s. The satellite constellation",
    "input through a GNSS broadcast ephemeris file in RINEX format. You can set",
    "the program by command line options. With -c option, the processing options",
    "are input from a configuration file. In this case, command line options",
    "precede options in the configuration file.",
    "",
    "Positional arguments:",
    "  <navfile>        RINEX navigation file of GNSS ephemerides",
    "",
    "Options:",
    "  -c <cfgfile>     Input options from configuration file in TOML [off]",
    "  -l <location>    Position in (lat,lon,hgt) [static mode]",
    "  -p <pathfile>    Trajectory in (time,lat,lon,hgt) or GGA [dynamic mode]",
    "  -t <date,time>   Scenario start time in (YYYY/MM/DD,hh:mm:ss)",
    "  -d <duration>    Duration of signal simulation (s) [default: 300]",
    "  -o <output>      I/Q data output (- for stdout) [default: gnsssim.iq]",
    "  -s <frequency>   Sampling frequency (Hz) [default: 2600000]",
    "  -b <bits>        I/Q data format (1/8/16) [default: 16]",
    "  -T <level>       Output trace level (valid in debug version) [off]",
    "  -v               Show details about simulated channels",
    "  -V, --version    Show program's version and exit",
    "  -h, --help       Show this help message and exit",
    "",
    "Copyright (C) Jon Jiang, 2023-2026, All rights reserved."};
/* clang-format on */

const int sys[] = {SYS_GPS, SYS_GAL, SYS_BDS, 0};
const char *sysstr[] = {"GPS", "Galileo", "BeiDou", ""};
const int band[] = {CTYPE_L1CA, CTYPE_E1B, CTYPE_B1I, CTYPE_B3I, 0};
const char *bandstr[] = {"L1CA", "E1B", "B1I", "B3I", 0};

/*! @brief Print help */
static void printhelp(void)
{
    int i;
    for (i = 0; i < (int)(sizeof(help) / sizeof(*help)); i++)
        fprintf(stderr, "%s\n", help[i]);
    return;
}

int main(int argc, char *argv[])
{
    clock_t tstart, tend;

    FILE *fp;

    int sv, neph, iephs[MAXSAT];
    int eph_shift, eph_selected;
    double maxdtoe;
    nav_t nav = {0};
    gtime_t g0;

    double llh[3];

    char *p;
    int i, ipar;

    int iq_buff_size;
    short *iq_buff = NULL;

    gtime_t grx;
    double delt;

    char ptfile[MAXCHAR], navfile[MAXCHAR], outfile[MAXCHAR];

    int iumd, numd;
    double *rcvpos = NULL; /* receiver position(s) in ECEF */

    datetime_t t0, tmin, tmax = {0};
    gtime_t gmin, gmax = {0};
    double dt;
    int flushnav;

    double duration;
    int iduration;

    int trace = 0;           /* Trace level */
    int static_mode = FALSE; /* Enable static mode */

    options_t opt;

    /* ============================ Read options ============================ */

    /* Default options */
    opt = opt_default;
    navfile[0] = 0;
    ptfile[0] = 0;
    strcpy(outfile, "gnsssim.iq");
    g0.week = -1; /* Invalid start time */
    iduration = 3000;
    duration = (double)iduration / 10.0; /* Default duration */

    /* load options from configuration file */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            if (!readopt(argv[++i], &opt)) {
                fprintf(stderr, "Error: Bad configuration file.\n");
                exit(1);
            }
        }
    }
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p") && i + 1 < argc)
            strcpy(ptfile, argv[++i]);
        else if (!strcmp(argv[i], "-l") && i + 1 < argc) {
            static_mode = TRUE;
            sscanf(argv[++i], "%lf,%lf,%lf", &llh[0], &llh[1], &llh[2]);
            llh[0] = llh[0] * D2R; /* Convert to radians */
            llh[1] = llh[1] * D2R; /* Convert to radians */
        }
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
            sscanf(argv[++i], "%d/%d/%d,%d:%d:%lf", &t0.year, &t0.month,
                   &t0.day, &t0.hour, &t0.min, &t0.sec);
            if (t0.year <= 1980 || t0.month < 1 || t0.month > 12 ||
                t0.day < 1 || t0.day > 31 || t0.hour < 0 || t0.hour > 23 ||
                t0.min < 0 || t0.min > 59 || t0.sec < 0.0 || t0.sec >= 60.0) {
                fprintf(stderr, "Error: Invalid date and time.\n");
                exit(1);
            }
            t0.sec = floor(t0.sec);
            g0 = date2gpst(t0);
        }
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            ++i;
            continue;
        }
        else if (!strcmp(argv[i], "-d") && i + 1 < argc)
            duration = atof(argv[++i]);
        else if (!strcmp(argv[i], "-o") && i + 1 < argc)
            strcpy(outfile, argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
            opt.sample = atof(argv[++i]);
            if (opt.sample < 1.0e6) {
                fprintf(stderr, "Error: Invalid sampling frequency.\n");
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "-b") && i + 1 < argc) {
            opt.bits = atoi(argv[++i]);
            if (opt.bits != SC01 && opt.bits != SC08 && opt.bits != SC16) {
                fprintf(stderr, "Error: Invalid I/Q data format.\n");
                exit(1);
            }
        }
        else if (!strcmp(argv[i], "-T") && i + 1 < argc)
            trace = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-v"))
            opt.verbose = TRUE;
        else if (!strcmp(argv[i], "--ion")) {
            ipar = ++i;
            if (!strcmp(argv[ipar], "brdc"))
                opt.ionospheric = IONOOPT_BRDC;
            else if (!strcmp(argv[ipar], "off"))
                opt.ionospheric = IONOOPT_OFF;
            else
                fprintf(stderr, "Error: Unsupported model '%s'\n", argv[ipar]);
        }
        else if (!strcmp(argv[i], "--tro")) {
            ipar = ++i;
            if (!strcmp(argv[ipar], "saas"))
                opt.tropospheric = TROPOPT_SAAS;
            else if (!strcmp(argv[ipar], "off"))
                opt.tropospheric = TROPOPT_OFF;
            else
                fprintf(stderr, "Error: Unsupported model '%s'\n", argv[ipar]);
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printhelp();
            exit(0);
        }
        else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
            fprintf(stderr, "Siren software %s%s\n", VERSION, PATCH_LEVEL);
            exit(0);
        }
        else if (!strncmp(argv[i], "-", 1) && strlen(argv[i]) > 1) {
            fprintf(stderr, "Error: Unsupported option '%s'.\n", argv[i]);
            exit(1);
        }
        else
            strcpy(navfile, argv[i]);
    }

    /* ============================ Check options =========================== */

    if (navfile[0] == 0) {
        fprintf(stderr, "Error: GPS ephemeris file is not specified.\n");
        exit(1);
    }

    if (ptfile[0] == 0 && !static_mode) {
        /* Default static location: Zibo, China */
        static_mode = TRUE;
        llh[0] = 36.846297 * D2R;
        llh[1] = 117.924797 * D2R;
        llh[2] = 10.0;
    }

    if (duration < 0.0 || (duration > DYNAMIC_MAX_DURATION && !static_mode) ||
        (duration > STATIC_MAX_DURATION && static_mode)) {
        fprintf(stderr, "Error: Invalid duration.\n");
        exit(1);
    }
    iduration = (int)(duration * 10.0 + 0.5);

    if (trace > 0) {
        traceopen(TRACEFILE);
        tracelevel(trace);
    }

    /* User position(s) array */
    if (static_mode)
        rcvpos = (double *)calloc(3, sizeof(double));
    else
        rcvpos = (double *)calloc(iduration * 3, sizeof(double));

    /* Buffer size */
    opt.sample = floor(opt.sample / 10.0);
    iq_buff_size = (int)opt.sample; /* Samples per 0.1 second */
    opt.sample *= 10.0;

    delt = 1.0 / opt.sample;

    /* ========================== Receiver position ==========================*/

    if (!static_mode) {
        /* Read user motion file, detect file type by extension name */
        p = strrchr(ptfile, '.');
        if (!strcmp(p, ".GGA") || !strcmp(p, ".gga"))
            numd = readtraj_gga(ptfile, iduration, rcvpos);
        else if (!strcmp(p, ".CSV") || !strcmp(p, ".csv"))
            numd = readtraj_csv(ptfile, iduration, rcvpos);
        else {
            fprintf(stderr,
                    "Error: Unsupported trajectory format, use CSV or GGA.\n");
            exit(1);
        }

        if (numd == -1) {
            fprintf(stderr, "Error: Failed to open trajectory file.\n");
            exit(1);
        }
        if (numd == 0) {
            fprintf(stderr, "Error: Failed to read trajectory data.\n");
            exit(1);
        }
        if (numd < iduration) {
            fprintf(stderr, "Error: Insufficient trajectory points.\n");
            exit(1);
        }

        fprintf(stdout, "Location mode: dynamic\n");

        /* Set initial position */
        xyz2llh(rcvpos, llh);
    }
    else {
        fprintf(stdout, "Location mode: static\n");

        /* Set simulation duration */
        numd = iduration;
        /* Set initial position */
        llh2xyz(llh, rcvpos);
    }

    /* Print constellation and location options */
    fprintf(stdout, "GNSS signal:");
    for (i = 0; sys[i]; i++) {
        if (opt.satsys & sys[i])
            fprintf(stdout, " %s", sysstr[i]);
    }
    for (i = 0; band[i]; i++) {
        if (opt.band == band[i])
            fprintf(stdout, " %s", bandstr[i]);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "Location: %.7f, %.7f, %.1f\n", llh[0] * R2D, llh[1] * R2D,
            llh[2]);

    /* =========================== Read ephemeris =========================== */

    neph = readnav(navfile, &nav);

    if (neph == 0) {
        fprintf(stderr, "Error: No ephemeris available.\n");
        exit(1);
    }
    if (neph == -1) {
        fprintf(stderr, "Error: ephemeris file not found.\n");
        exit(1);
    }
    if (neph == -2) {
        fprintf(stderr, "Error: RINEX version not supported.\n");
        exit(1);
    }

    if ((opt.verbose == TRUE) && (opt.ionospheric != IONOOPT_OFF)) {
        fprintf(stdout, "GPS Iono:   alpha0    alpha1     alpha2    alpha3\n");
        fprintf(stdout, "          %9.3e %9.3e %9.3e %9.3e\n", nav.ion_gps[0],
                nav.ion_gps[1], nav.ion_gps[2], nav.ion_gps[3]);
        fprintf(stdout, "            beta0     beta1      beta2     beta3\n");
        fprintf(stdout, "          %9.3e %9.3e %9.3e %9.3e\n", nav.ion_gps[4],
                nav.ion_gps[5], nav.ion_gps[6], nav.ion_gps[7]);
        fprintf(stdout,
                "GPS-UTC:       A0             A1        tot    wnt  dtls\n");
        fprintf(stdout, "          %12.7e %12.7e %.0f %.0f %.0f\n",
                nav.utc_gps[0], nav.utc_gps[1], nav.utc_gps[2], nav.utc_gps[3],
                nav.utc_gps[4]);
    }

    /* Get the start & end time of ephemeris */
    for (i = 0; i < neph; i++) {
        if (nav.eph[i].sat > 0) {
            gmin = nav.eph[i].toc;
            tmin = gpst2date(gmin);
            break;
        }
    }
    for (i = neph - 1; i > 0; i--) {
        if (nav.eph[i].sat > 0) {
            gmax = nav.eph[i].toc;
            tmax = gpst2date(gmax);
            break;
        }
    }

    /* Scenario start time has been set */
    if (g0.week >= 0) {
        if (subgtime(g0, gmin) < 0.0 || subgtime(gmax, g0) < 0.0) {
            fprintf(stderr, "Error: Invalid start time.\n");
            fprintf(stderr, "tmin = %4d/%02d/%02d,%02d:%02d:%02.0f (%d:%.0f)\n",
                    tmin.year, tmin.month, tmin.day, tmin.hour, tmin.min,
                    tmin.sec, gmin.week, gmin.sec);
            fprintf(stderr, "tmax = %4d/%02d/%02d,%02d:%02d:%02.0f (%d:%.0f)\n",
                    tmax.year, tmax.month, tmax.day, tmax.hour, tmax.min,
                    tmax.sec, gmax.week, gmax.sec);
            exit(1);
        }
    }
    else {
        g0 = gmin;
        t0 = tmin;
    }

    fprintf(stdout, "Start time: %4d/%02d/%02d, %02d:%02d:%02.0f (%d:%.0f)\n",
            t0.year, t0.month, t0.day, t0.hour, t0.min, t0.sec, g0.week,
            g0.sec);
    fprintf(stdout, "Sampling frequency: %.2f MHz\n", opt.sample / 1.0e6);
    fprintf(stdout, "Duration: %.1f s\n", ((double)numd) / 10.0);

    if (opt.ionospheric == IONOOPT_BRDC)
        fprintf(stdout, "Ionospheric model: brdc\n");
    else
        fprintf(stdout, "Ionospheric model: off\n");

    if (opt.tropospheric == TROPOPT_SAAS)
        fprintf(stdout, "Tropospheric model: saas\n");
    else
        fprintf(stdout, "Tropospheric model: off\n");

    if (opt.pathloss)
        fprintf(stdout, "Pathloss model: on\n");
    else
        fprintf(stdout, "Pathloss model: off\n");

    /* Select ephemerides for satellites */
    for (sv = 0; sv < MAXSAT; sv++)
        iephs[sv] = -1;

    for (sv = 0; sv < MAXSAT; sv++) {
        switch (satsys(sv + 1, NULL)) {
            case SYS_GAL: maxdtoe = MAXDTOE_GAL; break;
            case SYS_BDS: maxdtoe = MAXDTOE_BDS; break;
            default: maxdtoe = MAXDTOE_GPS; break;
        }
        for (i = 0; i < nav.n; i++) {
            if (nav.eph[i].sat != sv + 1)
                continue;
            dt = subgtime(g0, nav.eph[i].toe);
            if (-30 <= dt && dt < maxdtoe) {
                iephs[sv] = i;
                break;
            }
        }
    }

    /* Check selected ephemerides */
    for (eph_selected = FALSE, sv = 0; sv < MAXSAT; sv++) {
        if (iephs[sv] != -1) {
            eph_selected = TRUE;
            break;
        }
    }
    if (!eph_selected) {
        fprintf(stderr,
                "Error: No current set of ephemerides has been found.\n");
        exit(1);
    }

    /* =============== Baseband signal buffer and output file =============== */

    /* Allocate I/Q buffer */
    iq_buff = (short *)calloc(2 * iq_buff_size, 2);

    if (iq_buff == NULL) {
        fprintf(stderr, "Error: Failed to allocate 16-bit I/Q buffer.\n");
        exit(1);
    }

    /* Open output file, "-" can be used as name for stdout */
    if (strcmp("-", outfile)) {
        if (NULL == (fp = fopen(outfile, "wb"))) {
            fprintf(stderr, "Error: Failed to open output file.\n");
            exit(1);
        }
    }
    else {
        fp = stdout;
    }

    /* ======================== Initialize channels ======================== */

    initchan();              /* Initial channels */
    grx = addgtime(g0, 0.0); /* Initial reception time */

    /* Allocate channel for visible satellites */
    allocchan(grx, &nav, iephs, rcvpos, opt);
    printchan(stdout, TRUE); /* Print channels */

    /* ===================== Generate baseband signals ===================== */

    tstart = clock();

    /* Update receiver time */
    grx = addgtime(grx, 0.1);

    for (iumd = 1; iumd < numd; iumd++) {
        /* Update channals by new receiver position */
        if (static_mode)
            updatechan(grx, &nav, iephs, rcvpos, opt);
        else
            updatechan(grx, &nav, iephs, &rcvpos[iumd * 3], opt);

        /* Signal generation */
        gensignal(iq_buff, iq_buff_size, delt);

        /* Signal output */
        if (writeiq(iq_buff, iq_buff_size, fp, opt.bits)) {
            fprintf(stderr, "Error: failed to output I/Q file\n");
            exit(1);
        }

        /* Update navigation message and channel allocation every 30 seconds */
        if (opt.satsys == SYS_GPS)
            flushnav = ((int)(grx.sec * 10.0 + 0.5)) % 300 == 0;
        else if (opt.satsys == SYS_BDS)
            flushnav = ((int)(gpst2bdt(grx).sec * 10 + 0.5)) % 300 == 0;
        else
            flushnav = FALSE;

        if (flushnav) {
            /* Check if the next set of ephemeris should be used */
            for (sv = 0, eph_shift = FALSE; sv < MAXSAT; sv++) {
                if ((iephs[sv] + 1 >= nav.n) ||
                    (nav.eph[iephs[sv] + 1].sat != sv + 1))
                    continue;
                if (subgtime(grx, nav.eph[iephs[sv] + 1].toc) >= 0.0) {
                    iephs[sv]++;
                    eph_shift = TRUE;
                }
            }

            /* Flush navigation message in channels */
            flushchan(grx, &nav, iephs, eph_shift);

            /* Update channel allocation, maybe new satellites visible */
            if (static_mode)
                allocchan(grx, &nav, iephs, rcvpos, opt);
            else
                allocchan(grx, &nav, iephs, &rcvpos[iumd * 3], opt);

            /* Show details about simulated channels */
            if (opt.verbose == TRUE) {
                fprintf(stderr, "\nChannels have been updated.\n");
                printchan(stderr, FALSE);
            }
        }

        /* Update receiver time */
        grx = addgtime(grx, 0.1);

        /* Update time counter */
        fprintf(stderr, "\rTime into run: %4.1f", subgtime(grx, g0));
        fflush(stdout);
    }

    tend = clock();

    fprintf(stderr, "\nDone!\n");
    fprintf(stderr, "Process time: %.1fs\n",
            (double)(tend - tstart) / CLOCKS_PER_SEC);

    free(iq_buff); /* Free I/Q buffer */
    fclose(fp);    /* Close file */
    traceclose();  /* Close trace */

    return 0;
}
