# User Manual

## Introduction

Siren is a GNSS signal simulation software based on Software-Defined Radio (SDR) technology. It can simulate signals transmitted by satellites in the Global Positioning System (GPS) and the BeiDou Navigation Satellite System (BDS), and generate standard IQ signal files. These files can be used to test the performance of GNSS receivers or transmitted via SDR hardware such as USRP and HackRF.

Siren’s main features include:

- Multi-system support: GPS L1 C/A, BeiDou B1I, BeiDou B3I.
- Flexible configuration: Customize parameters via RINEX broadcast ephemeris files and TOML configuration files.
- Error simulation: Employs the Klobuchar ionospheric model and the Saastamoinen tropospheric model.
- Dynamic trajectories: Supports motion trajectory files in CSV or NMEA GGA format.
- Standard output: Binary IQ files (1/8/16-bit).
- Modular design: Clear code structure for easy extensibility.

The signals supported by Siren include:

| Signal     | Carrier frequency | PRN code rate | PRN code length | Modulation scheme |
| ---------- | ----------------- | ------------- | --------------- | ----------------- |
| GPS L1 C/A | 1575.42 MHz       | 1.023 Mcps    | 1023            | BPSK              |
| BDS B1I    | 1561.098 MHz      | 2.046 Mcps    | 2046            | BPSK              |
| BDS B3I    | 1268.52 MHz       | 10.23 Mcps    | 10230           | BPSK              |

## Quick Start

### Compile

The Siren core algorithm is written in C (C90 standard), and the required compilation environment is:

- Compiler: GCC（Linux / MinGW-w64）。
- Build tool: Make。

Users can compile using the following command:

```sh
cd siren/
make          # Linux/Mac
mingw32-make  # Windows (MinGW)
```

After successful compilation, an executable file named siren will be generated in the current directory.

### Instructions for use

Siren is a command-line program that you use with the following command:

```sh
siren <navfile> [options]
```

The required parameters include:

- `<navfile>`: RINEX navigation file (version 2.x or 3.x).

Commonly used optional parameters include:

- `-c <cfgfile>`: Path to the TOML configuration file.
- `-l <lat,lon,hgt>`: Static receiver position.
- `-p <path>`: Dynamic trajectory file (CSV or GGA).
- `-t <date,time>`: Simulation start time; defaults to the start time of the navigation file.
- `-d <duration>`: Simulation duration (seconds); defaults to 300.
- `-s <frequency>`: Sampling rate (Hz); defaults to 2.6e6.
- `-b <bits>`: IQ bit width (1/8/16); defaults to 16.
- `-o <path>`: Output IQ file path; defaults to gnsssim.iq.

Either the static receiver position or the dynamic trajectory file must be specified; these two parameters set the simulation mode to either static or dynamic. A complete command example is as follows:

```sh
siren example/brdc3110.23p -c config.toml -l 35.9,120.1,30.0 -d 55 -o bds_b1i_231107_55s_8m_8bit.iq
```

Here, `config.toml` is used to configure simulation signals, sampling rate, error models, signal bit width, and other parameters. A detailed configuration file structure is shown below (as an example):

```toml
version = "0.1.0"

[signal]
satsys = 'C'                   # GNSS system [G:GPS, C:BDS]
exclsats = ['G01']             # exclude satellites
band = 'B1I'                   # GNSS frequency band [L1CA, B1I, B3I]
sample = 5.2e+6                # sampling frequency (Hz)
elvmask = 0                    # mask of elevation angle (degree)
snrmask = 0                    # mask of signal SNR (dBHz)
pathloss = true                # enable path loss

[errors]
ionospheric = 'brdc'           # ionospheric delay model [off, brdc]
tropospheric = 'saas'          # tropospheric delay model [off, saas]

[output]
bits = 8                       # I/Q data format [1/8/16]
verbose = false                # verbose mode enable
```

Command-line options take precedence over configuration files.

## Program Architecture and Core Concepts

### Code structure

Siren’s core code is located in the src folder, and the tools folder contains several helper scripts written in Python.

```sh
siren/
├── src/                 # C core algorithms
│   ├── channels.c       # Channel management
│   ├── code.c           # Pseudocode generation
│   ├── ephemeris.c      # Ephemeris processing，satellite positioning
│   ├── errors.c         # Ionospheric/tropospheric error models
│   ├── io.c             # File I/O (RINEX, trajectory, IQ writing)
│   ├── options.c        # TOML configuration file parsing
│   ├── signal.c         # Navigation message generation, pseudorange and code phase calculation
│   ├── siren.c          # Main program entry
│   ├── siren.h          # Main header file
│   ├── toml.c/h         # TOML parsing library (third-party)
│   └── utils.c          # General utilities
├── tools/               # Python helper scripts
│   ├── combine_l1b1i.py # Merge GPS L1C/A & BDS B1I signals using FDM
│   ├── genpath.py       # Generate dynamic trajectory files
│   ├── plotpath.py      # Plot trajectories on a map
│   ├── plotskymap.py    # Plot simulated satellite sky map
│   ├── samebin.py       # Check if two binary files are identical
│   └── trans_via_uhd.py # Transmit signals via UHD devices
└── makefile
```

### Core Data Structures

#### gtime_t – GNSS time

| Field | Type   | Description                        |
| ----- | ------ | ---------------------------------- |
| week  | int    | GPS week number (since 1980-01-06) |
| sec   | double | Seconds of week (seconds)          |

#### datetime_t – UTC time

| Field | Type   | Description |
| ----- | ------ | ----------- |
| year  | int    | Year        |
| month | int    | Month       |
| day   | int    | Day         |
| hour  | int    | Hour        |
| min   | int    | Minute      |
| sec   | double | Second      |

#### eph_t – Broadcast Ephemeris

| Field     | Type    | Description                                                  |
| --------- | ------- | ------------------------------------------------------------ |
| sat       | int     | Satellite number                                             |
| iode      | int     | Issue of ephemeris data (IODE/AODE)                          |
| iodc      | int     | Issue of clock data (IODC/AODC)                              |
| sva       | int     | Accuracy index (URA)                                         |
| svh       | int     | Health status (0 = normal)                                   |
| week      | int     | GPS week (BDS week for BeiDou)                               |
| code      | int     | GPS: L2 code type / BDS: Signal source                       |
| flag      | int     | GPS: L2P flag / BDS: Navigation type                         |
| toc       | gtime_t | Clock reference time                                         |
| toe       | gtime_t | Ephemeris reference time                                     |
| ttr       | gtime_t | Transmission time                                            |
| A         | double  | Semi-major axis of orbit (meters)                            |
| e         | double  | Eccentricity                                                 |
| i0        | double  | Inclination angle at reference time (radians)                |
| idot      | double  | Rate of change of inclination angle (radians/sec)            |
| deln      | double  | Mean motion difference from computed value (radians/sec)     |
| M0        | double  | Mean anomaly at reference time (radians)                     |
| omg       | double  | Argument of perigee (radians)                                |
| OMG0      | double  | Longitude of ascending node at reference time (radians)      |
| OMGd      | double  | Rate of change of right ascension (radians/sec)              |
| cuc/cus   | double  | Amplitude of cosine/sine correction term for argument of latitude (radians) |
| cic/cis   | double  | Amplitude of cosine/sine correction term for inclination angle (radians) |
| crc/crs   | double  | Amplitude of cosine/sine correction term for orbit radius (meters) |
| toes      | double  | Ephemeris reference time (seconds of week)                   |
| fit       | double  | Fit interval (hours)                                         |
| f0/f1/f2  | double  | Clock bias, drift, and drift rate                            |
| tgd\[6]   | double  | Group delay parameters (meanings vary by system)             |
| Adot/ndot | double  | Rate of change of semi-major axis and mean motion in CNAV    |

#### nav_t – Navigation Data Collection

| Field       | Type   | Description                    |
| ----------- | ------ | ------------------------------ |
| n           | int    | Number of ephemerides          |
| nmax        | int    | Allocated capacity             |
| eph         | eph_t* | Ephemeris array                |
| utc_gps\[8] | double | GPS-UTC parameters             |
| utc_gal\[8] | double | Galileo-UTC parameters         |
| utc_bds\[8] | double | BDS-UTC parameters             |
| ion_gps\[8] | double | GPS ionospheric parameters     |
| ion_gal\[4] | double | Galileo ionospheric parameters |
| ion_bds\[8] | double | BDS ionospheric parameters     |

### range_t – geodetic distance

| Field      | Type    | Description                  |
| ---------- | ------- | ---------------------------- |
| gt         | gtime_t | Reception time               |
| range      | double  | Pseudorange (m, with errors) |
| rate       | double  | Pseudorange rate (m/s)       |
| d          | double  | Geometric range (m)          |
| azel\[2]   | double  | Azimuth/elevation (radians)  |
| iono_delay | double  | Ionospheric delay (m)        |
| trop_delay | double  | Tropospheric delay (m)       |

#### channel_t – signal channel

| Field            | Type                | Description                                                |
| ---------------- | ------------------- | ---------------------------------------------------------- |
| sys              | int                 | Satellite system                                           |
| prn              | int                 | PRN number                                                 |
| ctype            | int                 | Code type (CTYPE_L1CA/CTYPE_B1I/CTYPE_B3I)                 |
| code             | short*              | Ranging code sequence                                      |
| clen             | int                 | Ranging code length                                        |
| nh               | short*              | NH code sequence                                           |
| nhlen            | int                 | NH code length                                             |
| f_carr           | double              | Carrier Doppler frequency (Hz)                             |
| f_code           | double              | Code frequency (chips/s)                                   |
| f_code0          | double              | Baseband code frequency (chips/s)                          |
| carr_phase       | double/unsigned int | Carrier phase (floating-point or fixed-point)              |
| carr_phasestep   | int                 | Carrier phase step (fixed-point mode)                      |
| code_phase       | double              | Code phase (chips)                                         |
| g0               | gtime_t             | GPS time at the start of the current frame                 |
| sbf\[5]\[10]     | unsigned long       | 5 subframes with 10 words each (raw content)               |
| dwrd\[60]        | unsigned long       | 60 data words (including subframe from the previous frame) |
| iword/ibit/icode | int                 | Current word/bit/code index                                |
| codechip         | int                 | Current code chip level (±1)                               |
| databit          | int                 | Current data bit level (±1)                                |
| nhbit            | int                 | Current NH code bit level (±1)                             |
| azel[2]          | double              | Satellite azimuth/elevation (radians)                      |
| rho0             | range_t             | Pseudorange information from the previous epoch            |

#### options_t – Configuration options

| Field        | Type   | Description                                               |
| ------------ | ------ | --------------------------------------------------------- |
| satsys       | int    | Simulated satellite system (SYS_GPS/SYS_BDS)              |
| exclsats\[]   | int    | Satellite exclusion flags (by satellite number)           |
| band         | int    | Simulated frequency band (CTYPE_L1CA/CTYPE_B1I/CTYPE_B3I) |
| sample       | double | Sampling rate (Hz)                                        |
| elvmask      | double | Elevation mask (degrees)                                  |
| pathloss     | int    | Path loss simulation switch                               |
| ionospheric  | int    | Ionospheric model (IONOOPT_BRDC/IONOOPT_OFF)              |
| tropospheric | int    | Tropospheric model (TROPOPT_SAAS/TROPOPT_OFF)             |
| bits         | int    | IQ bit width (SC01/SC08/SC16)                             |
| verbose      | int    | Verbose output switch                                     |

### Signal Generation Process

1. Load ephemerides: `readnav()` → Parses the RINEX file and populates the `nav_t` structure.
2. Select ephemerides: Selects the currently valid ephemeris for each satellite based on the simulation start time.
3. Initialize channels: `initchan()` → Clears the channels; `allocchan()` → Calculates visible satellites, allocates channels, and generates ranging codes, NH codes, subframes, initial pseudoranges, and carrier phases.
4. Signal generation loop (every 0.1 seconds):
   - `updatechan()` → Updates pseudoranges, code phases, carrier frequencies, and gains.
   - `gensignal()` → Accumulates the I/Q values of all visible satellites for each sample point.
   - `writeiq()` → Writes the data to the file.
   - Every 30 seconds, `flushchan()` is called to update the navigation messages and check whether an ephemeris switch is required.
5. Termination: Releases resources and closes files.

## API Reference Manual

### Basic functions (utils.c)

| Function Prototype                                       | Description                                   |
| -------------------------------------------------------- | --------------------------------------------- |
| void sub3(const double *x1, const double *x2, double *y) | 3D vector subtraction                         |
| double norm3(const double *x)                            | 3D vector norm (magnitude)                    |
| double dot3(const double *x1, const double *x2)          | 3D vector dot product                         |
| double str2num(const char *str, int i, int n)            | Convert string to number                      |
| datetime_t str2date(const char *str, int i, int n)       | Convert string to UTC date                    |
| int satno(int sys, int prn)                              | System + PRN → Satellite number               |
| int satid2no(const char *id)                             | Satellite ID (e.g., "G01") → Satellite number |
| int satsys(int sat, int *prn)                            | Satellite number → System and PRN             |
| char syschr(int sys)                                     | System identifier character (G/E/C)           |
| gtime_t date2gpst(const datetime_t dt)                   | UTC date → GPS time                           |
| datetime_t gpst2date(const gtime_t gt)                   | GPS time → UTC date                           |
| gtime_t addgtime(gtime_t g0, double dt)                  | GPS time addition                             |
| double subgtime(gtime_t g1, gtime_t g0)                  | GPS time difference (seconds)                 |
| gtime_t bdt2gpst(gtime_t t0)                             | BDT → GPST (+14 seconds)                      |
| gtime_t gpst2bdt(gtime_t t0)                             | GPST → BDT (-14 seconds)                      |
| gtime_t bdt2time(int week, double sec)                   | BDS week + seconds → gtime_t                  |
| int bdsorbit(int prn)                                    | BDS satellite orbit type (GEO/IGSO/MEO)       |
| void xyz2llh(const double *xyz, double *llh)             | ECEF → Latitude/Longitude/Height              |
| void llh2xyz(const double *llh, double *xyz)             | Latitude/Longitude/Height → ECEF              |
| void ltcmat(const double *llh, double t $3] $3])         | Compute NEU rotation matrix                   |
| void ecef2neu(...)                                       | ECEF → NEU                                    |
| void neu2azel(const double *neu, double *azel)           | NEU → Azimuth/Elevation                       |

### Pseudo-code generation (code.c)

| Function Prototype                                           | Description                                                  |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| short *gencode(int prn, int ctype, int *len, double *crate)  | General pseudocode generation entry; calls specific implementation based on ctype |
| static short *gencode_L1CA(int prn, int *len, double *crate) | GPS L1 C/A code (G1/G2 shift registers, 1023 chips)          |
| static short *gencode_B1IB2I(int prn, int *len, double *crate) | BDS B1I/B2I code (11-stage shift registers, 2046 chips)      |
| static short *gencode_B3I(int prn, int *len, double *crate)  | BDS B3I code (13-stage shift registers, 10230 chips, with reset mechanism) |
| static short *gencode_NH20(int *len, double *crate)          | 20-bit Neuman-Hoffman code (fixed sequence)                  |
| static int equal13(const short *reg, const short *ref)       | Check if two arrays of length 13 are equal (B3I specific)    |

### Ephemeris and Satellite Position (ephemeris.c)

| Function Prototype                                           | Description                                                  |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| void satpos(gtime_t time, const eph_t *eph, double *pos, double *dts) | Computes satellite ECEF position and clock bias              |
| void satvel(gtime_t time, const eph_t *eph, double *vel, double *ddts) | Computes satellite velocity and clock drift using numerical differentiation |
| int satvisible(gtime_t gt, eph_t *eph, const double *rcv, double elvmask, double *azel) | Determines satellite visibility                              |
|                                                              | Encodes ephemeris parameters into subframes                  |
| static void eph2sbf_gps(...)                                 | GPS subframe encoding (includes ionospheric/UTC pages)       |
| static void eph2sbf_bds(...)                                 | BDS D1 navigation message subframe encoding                  |

#### Error Model (errors.c)

| Function Prototype                                           | Description                              |
| ------------------------------------------------------------ | ---------------------------------------- |
| double ionodelay(gtime_t gt, const double *iono, double *rcv, double *azel) | Klobuchar ionospheric delay (meters)     |
| double tropdelay(const double *pos, const double *azel, double humi) | Saastamoinen tropospheric delay (meters) |

### Input/Output (io.c)

| Function Prototype                                           | Description                          |
| ------------------------------------------------------------ | ------------------------------------ |
| int readnav(const char *filename, nav_t *nav)                | Reads RINEX navigation file          |
| int readtraj_csv(const char *filename, const int length, double *rcvpos) | Reads CSV trajectory file            |
| int readtraj_gga(const char *filename, const int length, double *rcvpos) | Reads NMEA GGA trajectory file       |
| int writeiq(short *buff, const int buff_size, FILE *fp, const int format) | Writes IQ data (supports 1/8/16-bit) |
| void traceopen(const char *file)                             | Opens trace log file                 |
| void traceclose(void)                                        | Closes trace log file                |
| void tracelevel(int level)                                   | Sets trace level                     |
| void trace(int level, const char *format, ...)               | Outputs trace messages               |

### Configuration Parsing (options.c)

| Function Prototype                                | Description                    |
| ------------------------------------------------- | ------------------------------ |
| int readopt(const char *filename, options_t *opt) | Parses TOML configuration file |

### Channel operations (channels.c)

| Function Prototype                                           | Description                                   |
| ------------------------------------------------------------ | --------------------------------------------- |
| void initchan(void)                                          | Initializes all channels                      |
| void printchan(FILE *fp, const int head)                     | Prints channel status                         |
| int allocchan(gtime_t gt, nav_t *nav, int *iephs, double *rcv, options_t opt) | Allocates channels for visible satellites     |
| int updatechan(gtime_t gt, nav_t *nav, int *iephs, double *rcv, options_t opt) | Updates channel parameters (code phase, gain) |
| void flushchan(gtime_t gt, nav_t *nav, int *iephs, const int neweph) | Flushes navigation messages                   |
| void gensignal(short *buff, const int buff_size, const double interval) | Generates IQ signal samples                   |

### Signal Generation (signal.c)

| Function Prototype                                           | Description                                                  |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| void calcrange(gtime_t gt, const eph_t *eph, const double *iono, double rcv[], options_t opt, range_t *rho) | Computes pseudorange (including Earth rotation correction and errors) |
| void calccaphase(range_t rho1, double dt, channel_t *chan)   | Computes code phase, carrier frequency, and data bit counter |
| int gennavmsg(gtime_t gt, channel_t *chan, int init)         | Generates navigation messages (GPS/BDS)                      |
| static int gennavmsg_gps(...)                                | GPS navigation message generation (including parity check)   |
| static int gennavmsg_bds(...)                                | BDS navigation message generation (including BCH encoding and interleaving) |
| static unsigned long checksum(...)                           | GPS parity check (Hamming code)                              |
| static unsigned long bchencode(...)                          | BCH (15,11) encoding                                         |
| static unsigned long interleave(...)                         | Interleaving of two 15-bit words                             |

## Detailed Explanation of the Internal Algorithm

### Pseudo-code generation

- GPS L1 C/A: Employs two 10-level Gold code generators, G1 and G2. The phase offset of G2 is determined by the PRN; the final chip value is given by -G1[i] * G2[j], with a length of 1023.
- BeiDou B1I/B2I: Uses two 11-level linear feedback shift registers. The tap positions of G2 are obtained from a lookup table based on the PRN; the length is 2046.
- BeiDou B3I: Employs two 13-level registers. The initial phase of G2 is determined by the reset position corresponding to the PRN. The CA register automatically resets when it reaches a specific state; the length is 10230.
- NH Code: A fixed 20-bit sequence, 0x1A4C1 (binary 00001010011000001, mapped to ±1).

### Navigation message generation

GPS subframe structure:

- Each frame consists of 5 subframes, with each subframe containing 10 characters (30 bits), for a total duration of 30 seconds.
- Subframe 1 contains the day of the week and clock parameters; subframes 2–3 contain ephemeris parameters; subframes 4–5 contain ionospheric data, UTC, astronomical almanac, and other relevant information.
- Each character undergoes Hamming code parity checking (`checksum()`), where the 2nd and 10th characters are non-information-bearing bits that require special handling.

Beidou D1 Navigation Message:

- Each frame consists of 5 subframes, with each subframe containing 10 characters, for a total duration of 30 seconds.
- Subframe 1 includes weekly count, clock offset, ionospheric parameters, and more; subframes 2–3 contain ephemeris data; subframes 4–5 are reserved for backup use.
- Each character is first encoded using BCH(15,11), and then the characters from the 2nd to the 10th are subjected to two-way interleaving.

### Carrier phase calculation

- Use `calccaphase()` to calculate the Doppler frequency `f_carr` and the code frequency `f_code` based on the rate of change of pseudorange.
- The initial value of the carrier phase is set according to the geometric relationship between the reference point (origin) and the receiver’s position, ensuring that the phase is aligned when the signal reaches the receiver.
- Supports two modes: floating-point phase (`FLOAT_CARR_PHASE` macro) and fixed-point phase (a 32-bit unsigned integer, with the top 9 bits used for table lookup).

### Signal synthesis

Within `gensignal()`, for each sample point:

1. Iterate through all active channels to compute `ip = nhbit * databit * codechip * CosTable[itable] * gain[i]`. Compute `qp` similarly using `SinTable`.
2. Accumulate the I and Q values across all channels.
3. Right-shift by 7 bits for scaling (to adapt to the 12-bit bladeRF).
4. Update the code phase, carrier phase, data bit counter, and NH code bit.

### Satellite Position Calculation

The siren.h file defines the satpos() function, which implements an iterative solution to the classical Kepler equation and supports GPS, Galileo, and BeiDou. For BeiDou GEO satellites, a special coordinate rotation process is applied (a rotation of -5° around the X-axis). The satvel() function calculates velocity using numerical differentiation with a step size of 1 ms.

## Detailed Explanation of Auxiliary Tools

This chapter provides a detailed introduction to the Python utility scripts in the tools/ directory, which help users perform tasks such as trajectory generation, signal merging, map plotting, sky map creation, binary comparison, and signal transmission via USRP.

### genpath.py

Interpolate to generate a continuous dynamic trajectory file based on the given waypoint coordinates (time, latitude, longitude, altitude) for use by Siren. The output is in CSV format with a sampling rate of 10 Hz. Usage instructions:

```sh
python genpath.py <csvfile> -out <pathfile> [-len <duration>]
```

The parameters are:

- `<csvfile>`: The input CSV file. Each line must contain "Time (s), Latitude (deg),  Longitude (deg), Altitude (m)". At least two waypoints are required.
- `-out`: Output path for the trajectory CSV file.
- `-len`: Total duration of the trajectory (in seconds). Defaults to 300 seconds.

For example, suppose the content of waypoints.csv is as follows:

```
0,39.9,116.4,50
600,39.95,116.5,100
```

Use the following command to generate the trajectory file:

```sh
python genpath.py waypoints.csv -out trajectory.csv -len 600
```

This command will generate a trajectory lasting 600 seconds, starting at (39.9, 116.4, 50) and ending at (39.95, 116.5, 100), with linear interpolation used to fill in the intermediate points.

### plotpath.py

Plot trajectory files (CSV, NMEA GGA, or GeoJSON) on a map, supporting multiple base map styles (satellite, street view, terrain, etc.). The output is saved as a JPG image. To use:

```sh
python plotpath.py <path> [-out <jpgpath>] [-style <style>]
```

The parameters are:

- `<path>`: Supports `.csv`, `.gga`, and `.geojson` formats.
- `-out`: Output image filename. Defaults to the trajectory filename with the extension changed to `.jpg`.
- `-style`: Base map style. Available options are `satellite`, `street`, `terrain`, and `only_streets`. Defaults to `satellite`.

Running this script requires installing `cartopy`, `matplotlib`, and `pynmea2` (for GGA format parsing):

```sh
pip install cartopy matplotlib pynmea2
```

Usage example:

```sh
python plotpath.py trajectory.csv -out track.jpg -style street
```

### plotskymap.py

Plot the satellite information file output by Siren (in text format, containing satellite ID, elevation angle, and azimuth angle) as a polar-coordinate sky map to visually display the distribution of satellites. The output will be in JPG image format. To use this tool:

```sh
python plotskymap.py <csvfile> [-out <jpgpath>]
```

The parameters are:

- `<csvfile>`: The satellite list outputted during Siren runtime.
- `-out`: Output image filename. Defaults to the input filename appended with `_skymap.jpg`.

Example usage: Suppose the satellite information output by the siren is saved in a file named satlist.txt. The format can be referenced as follows:

```
Sat Elev Azimu
G01 45.2 120.3
C06 62.1 80.5
```

Run the following command to generate the plot:

```sh
python plotskymap.py satlist.txt -out sky.jpg
```

### samebin.py

Compare two binary files byte by byte to check whether they are exactly identical; this method is often used to verify the consistency between IQ files generated by Siren and the expected results. To use it:

```sh
python samebin.py <file1> <file2>
```

Usage example:

```sh
python samebin.py output.iq reference.iq
```

program will output `NO` or `OK`.

### combine_l1b1i.py

Merge the two IQ files—GPS L1C/A and BeiDou B1I—that were independently generated by Siren into a single composite signal file, suitable for simultaneous transmission in dual frequency bands (e.g., when using HackRF). During the merging process, upconvert each of the two signals to frequencies on either side of the center frequency to avoid spectral overlap. The usage method is as follows:

```sh
python combine_l1b1i.py --gps <file1> --bds <file2> [-o <file3>] [-s <SAMPLE>]
```

The parameters are:

- `--gps <file>`: File path to the GPS L1C/A signal.
- `--bds <file>`: File path to the BDS B1I signal.
- `-s <SAMPLE>`: Sampling rate of the signal file. Defaults to 8 MHz.
- `-o <file>`: Output file path. Defaults to `combined_signal.bin`.

After running, output the merged file and prompt the HackRF transmit command.

Precautions:

- The input files must be in 8-bit interleaved IQ format (int8).
- The sampling rates of both signals must be identical.
- The output file is in 8-bit interleaved IQ format, with a center frequency  equal to the average of the GPS and BDS frequencies (approximately  1568.259 MHz).

## Module Dependency Relationships

```sh
siren.c (main)
├── options.c → toml.c/toml.h
├── io.c
│ ├── readnav() → Parses RINEX
│ ├── readtraj_csv/gga()
│ └── writeiq()
├── ephemeris.c
│ ├── satpos(), satvel()
│ └── eph2sbf()
├── signal.c
│ ├── calcrange() → errors.c (ionodelay, tropdelay)
│ ├── calccaphase()
│ └── gennavmsg() → checksum(), bchencode(), interleave()
├── channels.c
│ ├── initchan(), allocchan(), updatechan(), flushchan()
│ └── gensignal()
├── code.c → gencode() series
└── utils.c → Math, time, and coordinate transformations
```

## Compilation and Debugging Techniques

### Compilation options

The functionality can be controlled via macros at the top of siren.h:

- `-DFLOAT_CARR_PHASE`: Enables floating-point carrier phase (higher precision, slightly slower).
- `-DENABDS`: Enables BeiDou support (enabled by default).
- `-DTRACE`: Enables trace logging.
- `-DDYNAMIC_MAX_DURATION=7200`: Modifies the maximum duration for dynamic mode (default: 3600 seconds).

### Debugging

- Use `-T 3` to enable trace logging (output to siren.trace); the higher the level, the more detailed the output.
- Use `-v` to display channel details (such as satellite PRN, azimuth, pseudorange, etc.).
- The trace logging function `trace(level, fmt, ...)` is implemented in io.c and can be redirected via `traceopen()`.

### Common compilation errors

- tomh.h not found: Ensure that both toml.c and toml.h are present in the source directory.
- Linking error: Check whether the math library -lm has been linked (it’s already included in the Makefile).
- Line-break issue under Windows: Set core.autocrlf=false when checking out with Git.

### Frequently Asked Questions

1. Compilation failure: Verify the installation of GCC and Make. For Windows, use MinGW-w64.
2. Satellites not found: Ensure the RINEX file contains ephemeris data for the current time, and adjust the elevation mask.
3. Unrecognized IQ file: Check if the sampling rate, bit width, and signal type match. Confirm that the data format is interleaved I/Q.
4. Discontinuous dynamic trajectory: Ensure the trajectory file has a sampling rate of 10Hz and contains a sufficient number of points.
5. Out of memory: Reduce the simulation duration or the sampling rate.

## Appendix

### Abbreviation

| Abbreviations | Glossary                                |
| ------------- | --------------------------------------- |
| BCH           | Bose–Chaudhuri–Hocquenghem code         |
| BPSK          | Binary Phase Shift Keying               |
| CDMA          | Code Division Multiple Access           |
| GEO           | Geostationary Earth Orbit               |
| GNSS          | Global Navigation Satellite System      |
| GPS           | Global Positioning System               |
| ICD           | Interface Control Document              |
| IGSO          | Inclined Geosynchronous Satellite Orbit |
| MEO           | Medium Earth Orbit                      |
| NH            | Neuman-Hoffman                          |
| PRN           | Pseudo-Random Noise                     |
| RINEX         | Receiver Independent Exchange Format    |
| SDR           | Software Defined Radio                  |
