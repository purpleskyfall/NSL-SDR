/*==============================================================================
 * errors.c: Functions of errors simulation.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/09/21 create
 * 			2023/10/16 add tropdelay function
==============================================================================*/
#include "siren.h"

/*!
 * @brief Compute ionospheric delay by kolbuchar model
 * @param[in] gt GPS time at time of receiving the signal
 * @param[in] iono The ionospheric parameters {alpha0-alpha3, beta0-beta3}
 * @param[in] rcv Position of the receiver {lat,lon,h} (rad,m)
 * @param[in] azel Azimuth/elevation angle {az,el} (rad)
 * @return Ionospheric delay (m)
 */
double ionodelay(gtime_t gt, const double *iono, double *rcv, double *azel)
{
    double iono_delay = 0.0;
    double E, phi_u, lam_u, F;

    trace(3, "ionodelay: week=%d, sec=%.2f\n", gt.week, gt.sec);

    E = azel[1] / PI;
    phi_u = rcv[0] / PI;
    lam_u = rcv[1] / PI;

    /* Obliquity factor */
    F = 1.0 + 16.0 * pow((0.53 - E), 3.0);

    if (iono[0] == 0.0)
        iono_delay = F * 5.0e-9 * CLIGHT;
    else {
        double t, psi, phi_i, lam_i, phi_m, phi_m2, phi_m3;
        double A, T, X, X2, X4;

        /* Earth's central angle between the user position and the earth
           projection of ionospheric intersection point (semi-circles) */
        psi = 0.0137 / (E + 0.11) - 0.022;

        /* Geodetic latitude of the earth projection of the ionospheric
           intersection point (semi-circles) */
        phi_i = phi_u + psi * cos(azel[0]);
        if (phi_i > 0.416)
            phi_i = 0.416;
        else if (phi_i < -0.416)
            phi_i = -0.416;

        /* Geodetic longitude of the earth projection of the ionospheric
           intersection point (semi-circles) */
        lam_i = lam_u + psi * sin(azel[0]) / cos(phi_i * PI);

        /* Geomagnetic latitude of the earth projection of the ionospheric
           intersection point (mean ionospheric height assumed 350 km)
           (semi-circles) */
        phi_m = phi_i + 0.064 * cos((lam_i - 1.617) * PI);
        phi_m2 = phi_m * phi_m;
        phi_m3 = phi_m2 * phi_m;

        A = iono[0] + iono[1] * phi_m + iono[2] * phi_m2 + iono[3] * phi_m3;
        if (A < 0.0)
            A = 0.0;

        T = iono[4] + iono[5] * phi_m + iono[6] * phi_m2 + iono[7] * phi_m3;
        if (T < 72000.0)
            T = 72000.0;

        /* Local time (sec) */
        t = SECONDS_IN_DAY / 2.0 * lam_i + gt.sec;
        while (t >= SECONDS_IN_DAY)
            t -= SECONDS_IN_DAY;
        while (t < 0)
            t += SECONDS_IN_DAY;

        /* Phase (radians) */
        X = 2.0 * PI * (t - 50400.0) / T;

        if (fabs(X) < 1.57) {
            X2 = X * X;
            X4 = X2 * X2;
            iono_delay =
                F * (5.0e-9 + A * (1.0 - X2 / 2.0 + X4 / 24.0)) * CLIGHT;
        }
        else
            iono_delay = F * 5.0e-9 * CLIGHT;
    }

    return iono_delay;
}

/*!
 * @brief Compute tropospheric delay by standard atmosphere & saastamoinen model
 * @param[in] pos Receiver position {lat,lon,h} (rad,m)
 * @param[in] azel Azimuth/elevation angle {az,el} (rad)
 * @param[in] humi Relative humidity
 * @return Tropospheric delay (m)
 */
double tropdelay(const double *pos, const double *azel, double humi)
{
    const double temp0 = 15.0; /* temparature at sea level */
    double hgt, pres, temp, e, z, trph, trpw;

    trace(3, "tropdelay: pos=%.5f,%.5f,%.2f\n", pos[0] * R2D, pos[1] * R2D,
          pos[2]);

    if (-100.0 > pos[2] || 1e4 < pos[2] || azel[1] <= 0)
        return 0.0;

    /* Standard atmosphere */
    hgt = pos[2] < 0.0 ? 0.0 : pos[2];

    pres = 1013.25 * pow(1.0 - 2.2557e-5 * hgt, 5.2568);
    temp = temp0 - 6.5e-3 * hgt + 273.16;
    e = 6.108 * humi * exp((17.15 * temp - 4684.0) / (temp - 38.45));

    /* Saastamoninen model */
    z = PI / 2.0 - azel[1];
    trph = 0.0022768 * pres /
           (1.0 - 0.00266 * cos(2.0 * pos[0]) - 0.00028 * hgt / 1E3) / cos(z);
    trpw = 0.002277 * (1255.0 / temp + 0.05) * e / cos(z);

    return trph + trpw;
}
