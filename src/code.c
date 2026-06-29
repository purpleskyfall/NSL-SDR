/*==============================================================================
 * code.c: Functions of GNSS spreading code generation.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2024/01/26 separated from signal.c
 *          2025/02/08 add equal13 and gencode_B3I functions
==============================================================================*/
#include "siren.h"

/* GPS L1 C/A code (IS-GPS-200) ----------------------------------------------*/
static short *gencode_L1CA(int prn, int *len, double *crate)
{
    /* G2 delay (chips) */
    const static short delay[] = {
        5,   6,   7,   8,   17,  18,  139, 140, 141, 251, /*  1 -> 10 */
        252, 254, 255, 256, 257, 258, 469, 470, 471, 472, /* 11 -> 20 */
        473, 474, 509, 512, 513, 514, 515, 516, 859, 860, /* 21 -> 30 */
        861, 862                                          /* 31 -> 32 */
    };
    char G1[LEN_L1CA], G2[LEN_L1CA], R1[10], R2[10], C1, C2;
    short *code;
    int i, j;

    trace(4, "gencode_L1CA: prn=%d\n", prn);

    if (prn < 1 || MAXPRNGPS < prn ||
        !(code = (short *)malloc(sizeof(short) * LEN_L1CA))) {
        return NULL;
    }
    for (i = 0; i < 10; i++)
        R1[i] = R2[i] = -1;
    for (i = 0; i < LEN_L1CA; i++) {
        G1[i] = R1[9];
        G2[i] = R2[9];
        C1 = R1[2] * R1[9];
        C2 = R2[1] * R2[2] * R2[5] * R2[7] * R2[8] * R2[9];
        for (j = 9; j > 0; j--) {
            R1[j] = R1[j - 1];
            R2[j] = R2[j - 1];
        }
        R1[0] = C1;
        R2[0] = C2;
    }
    for (i = 0, j = LEN_L1CA - delay[prn - 1]; i < LEN_L1CA; i++, j++) {
        code[i] = -G1[i] * G2[j % LEN_L1CA];
    }
    *len = LEN_L1CA;
    *crate = CRATE_L1CA;

    return code;
}

/* BeiDou B1I/B2I code (BeiDou SIS-ICD) -------------------------------------*/
static short *gencode_B1IB2I(int prn, int *len, double *crate)
{
    /* Phase Assignment, 1 to 63 */
    const static char phase[63][3] = {
        {1, 3, 0},   {1, 4, 0},  {1, 5, 0},  {1, 6, 0},  {1, 8, 0},  {1, 9, 0},
        {1, 10, 0},  {1, 11, 0}, {2, 7, 0},  {3, 4, 0},  {3, 5, 0},  {3, 6, 0},
        {3, 8, 0},   {3, 9, 0},  {3, 10, 0}, {3, 11, 0}, {4, 5, 0},  {4, 6, 0},
        {4, 8, 0},   {4, 9, 0},  {4, 10, 0}, {4, 11, 0}, {5, 6, 0},  {5, 8, 0},
        {5, 9, 0},   {5, 10, 0}, {5, 11, 0}, {6, 8, 0},  {6, 9, 0},  {6, 10, 0},
        {6, 11, 0},  {8, 9, 0},  {8, 10, 0}, {8, 11, 0}, {9, 10, 0}, {9, 11, 0},
        {10, 11, 0}, {1, 2, 7},  {1, 3, 4},  {1, 3, 6},  {1, 3, 8},  {1, 3, 10},
        {1, 3, 11},  {1, 4, 5},  {1, 4, 9},  {1, 5, 6},  {1, 5, 8},  {1, 5, 10},
        {1, 5, 11},  {1, 6, 9},  {1, 8, 9},  {1, 9, 10}, {1, 9, 11}, {2, 3, 7},
        {2, 5, 7},   {2, 7, 9},  {3, 4, 5},  {3, 4, 9},  {3, 5, 6},  {3, 5, 8},
        {3, 5, 10},  {3, 5, 11}, {3, 6, 9}};
    short *code;
    char G1, G2, C1, C2;
    char R1[11] = {1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1}; /* 1=>-1, 0=>1 */
    char R2[11] = {1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1}; /* 1=>-1, 0=>1 */
    int i, j;

    trace(4, "gencode_B1IB2I: prn=%d\n", prn);

    if (prn < 1 || MAXPRNBDS < prn ||
        !(code = (short *)malloc(sizeof(short) * LEN_B1IB2I))) {
        return NULL;
    }
    for (i = 0; i < LEN_B1IB2I; i++) {
        G1 = R1[10];
        G2 = R2[phase[prn - 1][0] - 1] * R2[phase[prn - 1][1] - 1] *
             (phase[prn - 1][2] ? R2[phase[prn - 1][2] - 1] : 1);
        C1 = R1[0] * R1[6] * R1[7] * R1[8] * R1[9] * R1[10];
        C2 = R2[0] * R2[1] * R2[2] * R2[3] * R2[4] * R2[7] * R2[8] * R2[10];
        for (j = 10; j > 0; j--) {
            R1[j] = R1[j - 1];
            R2[j] = R2[j - 1];
        }
        R1[0] = C1;
        R2[0] = C2;
        code[i] = -G1 * G2;
    }
    *len = LEN_B1IB2I;
    *crate = CRATE_B1IB2I;

    return code;
}

/* Determine whether two arrays (length of 13) are completely identical ------*/
static int equal13(const short *reg, const short *ref)
{
    for (int i = 0; i < 13; i++) {
        if (reg[i] != ref[i]) {
            return 0; /* Not equal */
        }
    }
    return 1; /* Equal */
}

/* BeiDou B3I code (BeiDou SIS-ICD) ------------------------------------------*/
static short *gencode_B3I(int prn, int *len, double *crate)
{
    /* Phase Assignment, 1 to 63 */
    const static int G2_phase[63] = {
        4,    11,   13,   22,   30,   36,   44,   48,   88,   104,  116,
        129,  376,  418,  458,  682,  696,  707,  1078, 2069, 2248, 2574,
        2596, 2731, 4294, 4436, 4647, 4978, 4986, 1,    5209, 5539, 6061,
        6488, 7130, 7165, 7403, 5879, 1681, 5080, 5938, 3983, 6208, 7223,
        2996, 1814, 6906, 6144, 4713, 7406, 7264, 1766, 5347, 3515, 7951,
        7054, 3884, 6067, 4230, 3803, 869,  3683, 1205};

    int reset_pos = G2_phase[prn - 1];

    /* Feedback positions of G1, G2 */
    int ca_feedback_pos[4] = {0, 2, 3, 12};
    int cb_feedback_pos[8] = {0, 4, 5, 6, 8, 9, 11, 12};

    int i, j, k;

    short ca_reg[13], cb_reg[13], reset_state[13];
    short feedback, feedback_ca, feedback_cb, ca_val, cb_val;

    short *code = (short *)malloc(sizeof(short) * LEN_B3I);
    if (!code)
        return NULL;

    trace(4, "gencode_B3I: prn=%d\n", prn);

    /* Initialize the CA & CB register, and the reset reference state of CA */
    memset(ca_reg, -1, sizeof(ca_reg));
    memset(cb_reg, -1, sizeof(cb_reg));
    memset(reset_state, -1, 11 * sizeof(short));
    reset_state[11] = 1;
    reset_state[12] = 1;

    for (i = 0; i < reset_pos; i++) {
        feedback = 1;
        for (k = 0; k < 8; k++)
            feedback *= cb_reg[cb_feedback_pos[k]];

        for (j = 12; j > 0; j--)
            cb_reg[j] = cb_reg[j - 1];

        cb_reg[0] = feedback;
    }

    for (i = 0; i < LEN_B3I; i++) {
        ca_val = ca_reg[12];

        if (equal13(ca_reg, reset_state)) {
            memset(ca_reg, -1, sizeof(ca_reg));
        }
        else {
            feedback_ca = 1;
            for (k = 0; k < 4; k++)
                feedback_ca *= ca_reg[ca_feedback_pos[k]];

            for (j = 12; j > 0; j--)
                ca_reg[j] = ca_reg[j - 1];

            ca_reg[0] = feedback_ca;
        }

        cb_val = cb_reg[12];

        feedback_cb = 1;
        for (k = 0; k < 8; k++)
            feedback_cb *= cb_reg[cb_feedback_pos[k]];

        for (j = 12; j > 0; j--)
            cb_reg[j] = cb_reg[j - 1];

        cb_reg[0] = feedback_cb;

        code[i] = (short)(ca_val * cb_val);
    }

    *len = LEN_B3I;
    *crate = CRATE_B3I;
    return code;
}

/* Neuman-Hoffman code (20bit) -----------------------------------------------*/
static short *gencode_NH20(int *len, double *crate)
{
    short *code;

    trace(4, "gencode_NH20:\n");

    if (!(code = (short *)calloc(LEN_NH20, sizeof(short)))) {
        return NULL;
    }

    code[0] = -1;
    code[1] = -1;
    code[2] = -1;
    code[3] = -1;
    code[4] = -1;
    code[5] = 1;
    code[6] = -1;
    code[7] = -1;
    code[8] = 1;
    code[9] = 1;
    code[10] = -1;
    code[11] = 1;
    code[12] = -1;
    code[13] = 1;
    code[14] = -1;
    code[15] = -1;
    code[16] = 1;
    code[17] = 1;
    code[18] = 1;
    code[19] = -1;

    if (len)
        *len = LEN_NH20;
    if (crate)
        *crate = CRATE_NH20;

    return code;
}

/*!
 * @brief Generate spreading code
 * @param[in] prn Satellite PRN
 * @param[in] ctype Code type (CTYPE_*)
 * @param[out] len Code length
 * @param[out] crate Code chip rate (chip/s)
 * @return Pointer of generated code
 */
short *gencode(int prn, int ctype, int *len, double *crate)
{
    trace(3, "gencode: prn=%d, ctype=%d\n", prn, ctype);

    switch (ctype) {
        case CTYPE_L1CA: return gencode_L1CA(prn, len, crate);
        case CTYPE_B1I: return gencode_B1IB2I(prn, len, crate);
        case CTYPE_B3I: return gencode_B3I(prn, len, crate);
        case CTYPE_NH20: return gencode_NH20(len, crate);
        default:
            trace(2, "Error: not support ctype %d for PRN %d\n", ctype, prn);
            return NULL;
    }
}
