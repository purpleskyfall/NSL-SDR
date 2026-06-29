/*==============================================================================
 * options.c: Functions of configuration file parsing.
 *
 * Copyright (C) Jon Jiang at SDAEU, 2023-2026. All rights reserved
 *
 * Author: Jon Jiang (jiangyingming@live.com)
 * History: 2023/10/10 create
 *          2023/10/19 add exclude satellite support
==============================================================================*/
#include "siren.h"
#include "toml.h"
#include <string.h>

/* Default setting */
const options_t opt_default = {
    SYS_GPS,      /* satsys */
    {0},          /* exclude satellite flags */
    CTYPE_L1CA,   /* frequency band */
    2.6e6,        /* sample frequency */
    0.0,          /* elvmask angle */
    TRUE,         /* pathloss */
    IONOOPT_BRDC, /* ionospheric delay model */
    TROPOPT_SAAS, /* tropospheric delay model */
    SC16,         /* bits */
    FALSE         /* verbose */
};

/*!
 * @brief Read options from the configuration TOML file
 * @param[in] filename File name of the configuration file
 * @param[out] opt Options of the processing
 * @return (1:successful, 0:otherwise)
 */
int readopt(const char *filename, options_t *opt)
{
    int i, prn;
    char errbuf[200], *version;
    FILE *fp;
    toml_table_t *conf, *signal, *errors, *output;
    toml_datum_t param;
    toml_array_t *array;

    trace(2, "readopt: filename=%s\n", filename);

    /* Read and parse toml file */
    if (NULL == (fp = fopen(filename, "rt")))
        return 0;

    conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!conf)
        return 0;

    /* Version of the configuration file*/
    param = toml_string_in(conf, "version");
    if (param.ok) {
        version = param.u.s;
        trace(2, "readopt: version=%s\n", version);
    }

    /* Options for signal */
    if ((signal = toml_table_in(conf, "signal"))) {
        param = toml_string_in(signal, "satsys");
        if (param.ok) {
            if (!strcmp(param.u.s, "C"))
                opt->satsys = SYS_BDS;
            else if (!strcmp(param.u.s, "E"))
                opt->satsys = SYS_GAL;
            else /* Default is GPS */
                opt->satsys = SYS_GPS;
        }

        array = toml_array_in(signal, "exclsats");
        if (!array) {
            trace(2, "readopt: cannot read exclsats\n");
        }
        for (i = 0;; i++) {
            param = toml_string_at(array, i);
            if (!param.ok)
                break;
            if ((prn = satid2no(param.u.s)))
                opt->exclsats[prn - 1] = TRUE;
        }

        param = toml_string_in(signal, "band");
        if (param.ok) {
            if (opt->satsys == SYS_BDS && !strcmp(param.u.s, "B1I"))
                opt->band = CTYPE_B1I;
            else if (opt->satsys == SYS_BDS && !strcmp(param.u.s, "B3I"))
                opt->band = CTYPE_B3I;
            else if (opt->satsys == SYS_GAL && !strcmp(param.u.s, "E1B"))
                opt->band = CTYPE_E1B;
            else
                opt->band = CTYPE_L1CA;
        }

        param = toml_double_in(signal, "sample");
        if (param.ok)
            opt->sample = param.u.d;

        param = toml_double_in(signal, "elvmask");
        if (param.ok)
            opt->elvmask = param.u.d;

        param = toml_bool_in(signal, "pathloss");
        if (param.ok)
            opt->pathloss = param.u.b;
    }

    /* Options for errors */
    if ((errors = toml_table_in(conf, "errors"))) {
        param = toml_string_in(errors, "ionospheric");
        if (param.ok) {
            if (!strcmp(param.u.s, "brdc"))
                opt->ionospheric = IONOOPT_BRDC;
            else if (!strcmp(param.u.s, "off"))
                opt->ionospheric = IONOOPT_OFF;
            else
                trace(2, "readopt: unsupport ionospheric option\n");
        }

        param = toml_string_in(errors, "tropospheric");
        if (param.ok) {
            if (!strcmp(param.u.s, "saas"))
                opt->tropospheric = TROPOPT_SAAS;
            else if (!strcmp(param.u.s, "off"))
                opt->tropospheric = TROPOPT_OFF;
            else
                trace(2, "readopt: unsupport tropospheric option\n");
        }
    }

    /* Options for output */
    if ((output = toml_table_in(conf, "output"))) {
        param = toml_int_in(output, "bits");
        if (param.ok)
            opt->bits = param.u.b;

        param = toml_bool_in(output, "verbose");
        if (param.ok)
            opt->verbose = param.u.b;
    }

    return 1;
}
