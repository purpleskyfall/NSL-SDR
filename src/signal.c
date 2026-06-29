/*==============================================================================
 * signal.c: Functions of GNSS signal generation.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/09/21 create
 *          2024/02/01 add eph2sbf for BeiDou
 *          2024/02/16 add BeiDou B3I support
==============================================================================*/
#include "siren.h"

#define CARR2CODE_L1CA (CRATE_L1CA / FREQ_L1CA)
#define CARR2CODE_B1I (CRATE_B1IB2I / FREQ_B1I)
#define CARR2CODE_B3I (CRATE_B3I / FREQ_B3I)

/*!
 * @brief Count number of bits set to 1
 * @param[in] v long word in which bits are counted
 * @return Count of bits set to 1
 */
static unsigned int bitcount(unsigned long v)
{
    unsigned int b;
    for (b = 0; v != 0; v >>= 1)
        if (v & 01)
            b++;

    return b;
}

/*!
 * @brief Compute the Checksum for one given word of a subframe
 * @param[in] source The input data
 * @param[in] nib Does this word contain non-information-bearing bits?
 * @return Computed Checksum
 */
static unsigned long checksum(unsigned long source, int nib)
{
    /* Bits of the source data:
     * Bits 31 to 30 = 2 LSBs of the previous transmitted word, D29_ and D30_
     * Bits 29 to  6 = Source data bits, d1, d2, ..., d24
     * Bits  5 to  0 = Empty parity bits
     */

    /* Bits of the return data:
     * Bits 31 to 30 = 2 LSBs of the previous transmitted word, D29_ and D30_
     * Bits 29 to  6 = Data bits transmitted by satellite, D1, D2, ..., D24
     * Bits  5 to  0 = Computed parity bits, D25, D26, ..., D30
     */

    /* Bits used for parity computed:
     *                   1            2           3
     * bit    12 3456 7890 1234 5678 9012 3456 7890
     * ---    -------------------------------------
     * D25    11 1011 0001 1111 0011 0100 1000 0000
     * D26    01 1101 1000 1111 1001 1010 0100 0000
     * D27    10 1110 1100 0111 1100 1101 0000 0000
     * D28    01 0111 0110 0011 1110 0110 1000 0000
     * D29    10 1011 1011 0001 1111 0011 0100 0000
     * D30    00 1011 0111 1010 1000 1001 1100 0000
     */

    unsigned long bmask[6] = {0x3B1F3480UL, 0x1D8F9A40UL, 0x2EC7CD00UL,
                              0x1763E680UL, 0x2BB1F340UL, 0x0B7A89C0UL};

    unsigned long D;
    unsigned long d = source & 0x3FFFFFC0UL;
    unsigned long D29_ = (source >> 31) & 0x1UL;
    unsigned long D30_ = (source >> 30) & 0x1UL;

    trace(4, "checksum: nib=%d\n", nib);

    /* Non-information bearing bits for word 2 and 10 */
    if (nib) {
        /*
        Solve bits 23 and 24 to preserve parity check
        with zeros in bits 29 and 30.
        */
        if ((D30_ + bitcount(bmask[4] & d)) % 2)
            d ^= (0x1UL << 6);
        if ((D29_ + bitcount(bmask[5] & d)) % 2)
            d ^= (0x1UL << 7);
    }

    D = d;
    if (D30_)
        D ^= 0x3FFFFFC0UL;

    D |= ((D29_ + bitcount(bmask[0] & d)) % 2) << 5;
    D |= ((D30_ + bitcount(bmask[1] & d)) % 2) << 4;
    D |= ((D29_ + bitcount(bmask[2] & d)) % 2) << 3;
    D |= ((D30_ + bitcount(bmask[3] & d)) % 2) << 2;
    D |= ((D30_ + bitcount(bmask[4] & d)) % 2) << 1;
    D |= ((D29_ + bitcount(bmask[5] & d)) % 2);

    D &= 0x3FFFFFFFUL;

    /* Add D29* and D30* from source data bits */
    /* D |= (source & 0xC0000000UL); */

    return D;
}

/*!
 * @brief Encode an 11-bit word into a 15-bit BCH code
 * @param[in] word The input 11-bit word
 * @return The 15-bit BCH code
 */
static unsigned long bchencode(unsigned long word)
{
    /* The generator polynomial for BCH (15,11) is x^4 + x + 1, which is 10011
     * in binary, It corresponds to the integer 0x13. */
    const unsigned long generator = 0x13;
    /* Left align the word bits in the 15-bit code */
    unsigned long code = word << 4;

    /* Perform modulo-2 division by the generator polynomial */
    for (int i = 10; i >= 0; --i)
        if (code & (1 << (i + 4)))
            code ^= generator << i;

    /* The remainder after division becomes the parity bits, which are
     * placed in the lower 4 bits of the code. */
    return code | (word << 4);
}

/*!
 * @brief Interleave of two input word
 * @param[in] word1 The low word
 * @param[in] word2 The high word
 * @return The interleaved word
 */
static unsigned long interleave(unsigned long word1, unsigned long word2)
{
    unsigned long bit1, bit2, result = 0;
    /* Iterate only through the low 15 bits */
    for (int i = 0; i < 15; i++) {
        /* Isolate the bit at position i from each word */
        bit1 = (word1 >> i) & 1;
        bit2 = (word2 >> i) & 1;

        result |= (bit1 << (2 * i)) | (bit2 << ((2 * i) + 1));
    }
    return result;
}

/*!
 * @brief Compute range between a satellite and the receiver
 * @param[in] gt GPS time of receiving the signal
 * @param[in] eph Ephemeris data of the satellite
 * @param[in] iono Ionosphere parameters (double*8)
 * @param[in] rcv Position of the receiver (ecef)
 * @param[in] opt Option setting
 * @param[out] rho The computed range
 */
void calcrange(gtime_t gt, const eph_t *eph, const double *iono, double rcv[],
               options_t opt, range_t *rho)
{
    double pos[3], vel[3], clk[2];
    double los[3];
    double tau;
    double range, rate;
    double xrot, yrot;

    double llh[3], neu[3];
    double tmat[3][3];

    trace(4, "calcrange: week=%d, sec=%.2f, iono=%d, trop=%d\n", gt.week,
          gt.sec, opt.ionospheric, opt.tropospheric);

    /* Satellite position at time of the pseudorange observation */
    satpos(gt, eph, pos, &clk[0]);
    satvel(gt, eph, vel, &clk[1]);

    /* Receiver to satellite vector and light-time */
    sub3(pos, rcv, los);
    tau = norm3(los) / CLIGHT;

    /* Extrapolate the satellite position backwards to the transmission time */
    pos[0] -= vel[0] * tau;
    pos[1] -= vel[1] * tau;
    pos[2] -= vel[2] * tau;

    /* Earth rotation correction. The change in velocity can be neglected */
    xrot = pos[0] + pos[1] * OMEGA_WGS84 * tau;
    yrot = pos[1] - pos[0] * OMEGA_WGS84 * tau;
    pos[0] = xrot;
    pos[1] = yrot;

    /* New receiver to satellite vector and satellite range */
    sub3(pos, rcv, los);
    range = norm3(los);
    rho->d = range;

    /* Pseudorange, add satellite's clock bias effect */
    rho->range = range - CLIGHT * clk[0];

    /* Relative velocity of satellite and receiver */
    rate = dot3(vel, los) / range;

    /* Pseudorange rate, satellite's clock drift not added */
    rho->rate = rate; /* - CLIGHT*clk[1]; */

    /* Time of application */
    rho->gt = gt;

    /* Azimuth and elevation angles */
    xyz2llh(rcv, llh);
    ltcmat(llh, tmat);
    ecef2neu(los, tmat, neu);
    neu2azel(neu, rho->azel);

    /* Calculate ionospheric delay */
    if (opt.ionospheric)
        rho->iono_delay = ionodelay(gt, iono, llh, rho->azel);
    else
        rho->iono_delay = 0.0;

    /* Calculate tropospheric delay */
    if (opt.tropospheric)
        rho->trop_delay = tropdelay(llh, rho->azel, 0.7);
    else
        rho->trop_delay = 0.0;

    rho->range += rho->iono_delay + rho->trop_delay;

    return;
}

/*!
 * @brief Compute the code phase for a given channel (satellite)
 * @param[in] rho1 Current range, after dt has expired
 * @param[in] dt delta-t (time difference) in seconds
 * @param[out] chan Channel on which we operate (is updated)
 * @note `rho1`: current pseudorange, `chan->rho0`: pseudorange at last epoch
 */
void calccaphase(range_t rho1, double dt, channel_t *chan)
{
    int ims;
    double ms, rhorate, lambda, carr2code;

    trace(4, "calccaphase: rho1=%.2f, dt=%.2f\n", rho1.range, dt);

    switch (chan->ctype) {
        case CTYPE_B1I:
            lambda = LAMBDA_B1I;
            carr2code = CARR2CODE_B1I;
            break;
        case CTYPE_B3I:
            lambda = LAMBDA_B3I;
            carr2code = CARR2CODE_B3I;
            break;
        default: /* default is GPS L1 C/A */
            lambda = LAMBDA_L1CA;
            carr2code = CARR2CODE_L1CA;
            break;
    }

    /* Pseudorange rate */
    rhorate = (rho1.range - chan->rho0.range) / dt;

    /* Carrier and code frequency */
    chan->f_carr = -rhorate / lambda;
    chan->f_code = chan->f_code0 + chan->f_carr * carr2code;

    /* Initial code phase and data bit counters */
    ms = ((subgtime(chan->rho0.gt, chan->g0) + 6.0) -
          chan->rho0.range / CLIGHT) *
         1000.0;

    ims = (int)ms;
    chan->code_phase = (ms - (double)ims) * chan->clen; /* in chip */

    chan->iword = ims / 600; /* 1 word = 30 bits = 600 ms */
    ims -= chan->iword * 600;

    chan->ibit = ims / 20; /* 1 bit = 20 code = 20 ms */
    ims -= chan->ibit * 20;

    chan->icode = ims; /* 1 code = 1 ms */

    /* Set code chip, data bit and Neuman-Hoffman bit */
    chan->codechip = (int)chan->code[(int)chan->code_phase];
    chan->databit =
        1 - 2 * (int)((chan->dwrd[chan->iword] >> (29 - chan->ibit)) & 0x1UL);
    if (chan->ctype == CTYPE_B1I || chan->ctype == CTYPE_B3I)
        chan->nhbit = (int)chan->nh[chan->icode];
    else
        chan->nhbit = 1;

    /* Save current pseudorange */
    chan->rho0 = rho1;

    return;
}

/* GPS navigation message generation -----------------------------------------*/
static int gennavmsg_gps(gtime_t gt, channel_t *chan, int init)
{
    int iwrd, isbf;
    gtime_t g0;
    unsigned long wn, tow;
    unsigned sbfwrd;
    unsigned long prevwrd;
    int nib;

    trace(4, "gennavmsg_gps: week=%d, sec=%.2f, sat=%c%d\n", gt.week, gt.sec,
          syschr(chan->sys), chan->prn);

    g0.week = gt.week;
    /* Align with the full frame length = 30 sec */
    g0.sec = (double)(((unsigned long)(gt.sec + 0.5)) / 30UL) * 30.0;
    chan->g0 = g0; /* Data bit reference time */

    wn = (unsigned long)(g0.week % 1024);
    tow = ((unsigned long)g0.sec) / 6UL;

    /* Subframe 5 of last frame */
    if (init) {
        prevwrd = 0UL;

        for (iwrd = 0; iwrd < N_DWRD_SBF; iwrd++) {
            sbfwrd = chan->sbf[4][iwrd];

            /* Add TOW-count message into HOW */
            if (iwrd == 1)
                sbfwrd |= ((tow & 0x1FFFFUL) << 13);

            /* Compute checksum, 2 LSBs of the previous transmitted word */
            sbfwrd |= (prevwrd << 30) & 0xC0000000UL;
            /* Non-information bearing bits for word 2 and 10 */
            nib = ((iwrd == 1) || (iwrd == 9)) ? 1 : 0;
            chan->dwrd[iwrd] = checksum(sbfwrd, nib);

            prevwrd = chan->dwrd[iwrd];
        }
    }
    else {
        for (iwrd = 0; iwrd < N_DWRD_SBF; iwrd++) {
            chan->dwrd[iwrd] = chan->dwrd[N_DWRD_SBF * N_SBF + iwrd];

            prevwrd = chan->dwrd[iwrd];
        }
        /* Sanity check */
        /*
        if (((chan->dwrd[1])&(0x1FFFFUL<<13)) != ((tow&0x1FFFFUL)<<13)) {
            fprintf(stderr, "\nWARNING: Invalid TOW in subframe 5.\n");
            return(0);
        }
        */
    }

    /* Subframe 1->5 of this frame */
    for (isbf = 0; isbf < N_SBF; isbf++) {
        tow++;

        for (iwrd = 0; iwrd < N_DWRD_SBF; iwrd++) {
            sbfwrd = chan->sbf[isbf][iwrd];

            /* Add transmission week number to Subframe 1 */
            if ((isbf == 0) && (iwrd == 2))
                sbfwrd |= (wn & 0x3FFUL) << 20;

            /* Add TOW-count message into HOW */
            if (iwrd == 1)
                sbfwrd |= ((tow & 0x1FFFFUL) << 13);

            /* Compute checksum, 2 LSBs of the previous transmitted word */
            sbfwrd |= (prevwrd << 30) & 0xC0000000UL;
            /* Non-information bearing bits for word 2 and 10 */
            nib = ((iwrd == 1) || (iwrd == 9)) ? 1 : 0;
            chan->dwrd[(isbf + 1) * N_DWRD_SBF + iwrd] = checksum(sbfwrd, nib);

            prevwrd = chan->dwrd[(isbf + 1) * N_DWRD_SBF + iwrd];
        }
    }

    return 1;
}

/* BeiDou navigation message generation --------------------------------------*/
static int gennavmsg_bds(gtime_t gt, channel_t *chan, int init)
{
    int iwrd, isbf;
    unsigned long sbfwrd, word1, word2;
    unsigned long wn, sow;
    gtime_t g0 = gpst2bdt(gt);

    trace(4, "gennavmsg_bds: week=%d, sec=%.2f, sat=%c%d\n", gt.week, gt.sec,
          syschr(chan->sys), chan->prn);

    /* Align with the full frame length = 30 sec */
    g0.sec = (double)(((unsigned long)(g0.sec + 0.5)) / 30UL) * 30.0;
    chan->g0 = bdt2gpst(g0); /* Data bit reference time */

    wn = (unsigned long)((g0.week - BD0WEEK) % 8192);
    sow = (unsigned long)(g0.sec - 6.0);

    /* Subframe 5 of last frame */
    if (init) {
        for (iwrd = 0; iwrd < N_DWRD_SBF; iwrd++) {
            sbfwrd = chan->sbf[4][iwrd];

            /* Add SOW into the first two words */
            if (iwrd == 0)
                sbfwrd = (sbfwrd & ~0xFFF) | (((sow >> 12) & 0xFF) << 4);
            else if (iwrd == 1)
                sbfwrd = (sow & 0xFFF) << 10 | (sbfwrd & 0x3FF);

            /* BCH and interleave encode */
            if (iwrd == 0) {
                word1 = (sbfwrd >> 4) & 0x7FF;
                chan->dwrd[iwrd] = (sbfwrd & ~0x7FFF) | bchencode(word1);
            }
            else {
                word1 = bchencode(sbfwrd & 0x7FF);
                word2 = bchencode((sbfwrd >> 11) & 0x7FF);
                chan->dwrd[iwrd] = interleave(word1, word2);
            }
        }
    }
    else {
        for (iwrd = 0; iwrd < N_DWRD_SBF; iwrd++) {
            chan->dwrd[iwrd] = chan->dwrd[N_DWRD_SBF * N_SBF + iwrd];
        }
    }

    /* Subframe 1->5 of this frame */
    for (isbf = 0; isbf < N_SBF; isbf++) {
        sow += 6UL;

        for (iwrd = 0; iwrd < N_DWRD_SBF; iwrd++) {
            sbfwrd = chan->sbf[isbf][iwrd];

            /* Add transmission week number to Subframe 1 */
            if ((isbf == 0) && (iwrd == 2))
                sbfwrd = (wn << 9) | (sbfwrd & 0x1FF);

            /* Add SOW into the first two words */
            if (iwrd == 0)
                sbfwrd = (sbfwrd & ~0xFFF) | (((sow >> 12) & 0xFF) << 4);
            else if (iwrd == 1)
                sbfwrd = (sow & 0xFFF) << 10 | (sbfwrd & 0x3FF);

            /* BCH and interleave encode */
            if (iwrd == 0) {
                word1 = (sbfwrd >> 4) & 0x7FF;
                chan->dwrd[(isbf + 1) * N_DWRD_SBF + iwrd] =
                    (sbfwrd & ~0x7FFF) | bchencode(word1);
            }
            else {
                word1 = bchencode(sbfwrd & 0x7FF);
                word2 = bchencode((sbfwrd >> 11) & 0x7FF);
                chan->dwrd[(isbf + 1) * N_DWRD_SBF + iwrd] =
                    interleave(word1, word2);
            }
        }
    }

    return 0;
}

/*!
 * @brief Generate navigation message by subframe words, valid for GPS & BeiDou
 * @param[in] gt GPS time
 * @param[in,out] chan Channel on which we operate (is updated)
 * @param[in] init If initialize navigation message
 * @return Status (0: successful, 1: otherwise)
 */
int gennavmsg(gtime_t gt, channel_t *chan, int init)
{
    trace(3, "gennavmsg: week=%d, sec=%.2f, sat=%c%d\n", gt.week, gt.sec,
          syschr(chan->sys), chan->prn);

    switch (chan->sys) {
        case SYS_GPS: return gennavmsg_gps(gt, chan, init);
        case SYS_BDS: return gennavmsg_bds(gt, chan, init);
        default:
            trace(2, "Error: not support satellite system: %c\n",
                  syschr(chan->sys));
            return 1;
    }
}
