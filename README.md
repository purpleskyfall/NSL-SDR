# Project Overview

## Background knowledge

### What is SDR?

Software Defined Radio (SDR) is a radio communication technology that uses software to perform functions such as modulation, demodulation, and signal processing of wireless signals. Compared to traditional radio communication systems, SDR systems offer greater flexibility, programmability, and adaptability, making them suitable for a wide variety of application scenarios.

The main feature of an SDR system is its use of digital signal processing technology to perform functions such as modulation, demodulation, and signal processing of radio signals. An SDR system consists of two main components: hardware and software. The hardware component includes elements such as modems, antennas, and RF frontends, which are used to convert radio signals into digital signals or vice versa. The software component comprises elements like processors and programming algorithms, which are used to process, modulate, demodulate, and perform other signal-processing operations on digital signals. By means of software control, an SDR system can flexibly adjust parameters such as modulation scheme, frequency, and bandwidth to meet the requirements of various applications.

Digital signals in SDR systems are typically stored using IQ files, which are one of the commonly used file formats for radio signals. These files record the I (real) and Q (imaginary) components of the received radio signal. IQ files usually adopt a standard binary format and are supported by most SDR software and tools. The software-defined radio platform samples the received radio signal at a very high sampling rate, then encodes the I/Q signals of each sample point digitally, thereby generating an IQ data file. This file is then sent to the hardware platform, which transmits the signal at a pre-set frequency, thus realizing radio functionality through software.

### Composition of GNSS Signals

Essentially, a GNSS (Global Navigation Satellite System) is a navigation and positioning service implemented via radio signals. Taking the GPS (Global Positioning System) as an example, the signals transmitted by its satellites consist of three main components:

1. Carrier Phase: The carrier is a high-frequency signal that forms the foundation of GPS signals and can be represented as a continuous-phase sinusoidal wave. It is used to modulate both the Pseudo-Random Noise (PRN) code and the navigation data code. For the GPS system, the carrier frequency of the L1 band is 1575.42 MHz, and that of the L2 band is 1227.60 MHz.
2. Pseudo-Random Noise (PRN) Code: The PRN code, commonly referred to as the pseudo-code, is a binary sequence with a length of 1023 bits. It is a random, non-periodic sequence that serves the dual purpose of distinguishing individual satellites and enabling ranging measurements.
3. Data Code: The data code constitutes the third layer of the GPS signal. It is a binary sequence that carries the navigation message, which contains essential information such as satellite ephemerides, precise timing, satellite health status, and orbital parameters.

Because the frequency of the data code is much lower than that of the pseudocode, and the frequency of the pseudocode is much lower than that of the carrier, the pseudocode and the data code can be simultaneously modulated onto the carrier, thus forming the final GNSS signal. After receiving the signal, a GNSS receiver can obtain the pseudocode and navigation message through demodulation, enabling it to determine the distance between the satellite and the receiver as well as the satellite’s position, and thereby achieve positioning functionality.

### GNSS Receiver Signal Processing

In summary, the GNSS receiver's processing of the received signals involves the following steps:

1. Signal Reception: The GNSS receiver captures GNSS signals via an antenna and converts them into electrical signals, which are then transmitted to the receiver's front-end amplifier.
2. Signal Amplification: The front-end amplifier boosts the received GNSS signals to enhance their strength, facilitating subsequent processing stages.
3. Signal Filtering: During transmission, GNSS signals are subject to multipath effects, Doppler shifts, and noise interference. Therefore, filtering is required to eliminate extraneous noise and interference while preserving the desired signal components.
4. Signal Mixing: The GNSS receiver mixes the carrier wave and the pseudo-code to generate an Intermediate Frequency (IF) signal, thereby down-converting the signal to a frequency range that is easier to process.
5. Signal Demodulation: The receiver demodulates the mixed signal to extract the pseudo-code and the navigation message.
6. Signal Correlation: The receiver correlates the extracted pseudo-code and navigation message with known satellite information to determine the satellite's identification number (PRN) and timing parameters.
7. Signal Processing: Using the pseudo-code and navigation message, the receiver calculates the distance (pseudorange) to the satellites and determines the receiver's position, thereby accomplishing the positioning and navigation functions.

It is important to note that the performance and accuracy of GNSS receivers are influenced by a variety of factors, such as signal strength, antenna placement, multipath effects, and satellite geometry. To enhance the performance and accuracy of GNSS receivers, it is necessary to adopt appropriate antennas and preamplifiers, optimize signal filtering and demodulation algorithms, and implement other relevant measures.

## Program Principle

Siren is a GNSS signal simulation software based on software-defined radio technology. It can simulate signals transmitted by GNSS satellites and generate corresponding GNSS signal files for testing and validating the performance and functionality of GNSS receivers. By means of simple configuration—primarily involving satellite ephemeris data and user location—the software can generate IQ files containing simulated GNSS signals. Currently, the following signals are supported for simulation:

- GPS L1CA.
- BeiDou B1I.
- BeiDou B3I.

GNSS signal simulation is a method for generating signals from virtual GNSS satellites. The underlying principle can be summarized in the following steps:

1. Generation of Satellite Orbit and Clock Data: The simulation system must generate satellite ephemeris and clock data to simulate the signals transmitted by GNSS satellites. These data can be obtained from actual GNSS satellites or generated using mathematical models.
2. Signal Generation: Based on the generated orbit and clock data, the simulation system synthesizes the GNSS signals. A GNSS signal is formed by modulating carrier signals across multiple frequency bands into a composite signal. The simulation system must replicate this process to generate signals that closely resemble actual GNSS signals.
3. Signal Transmission: The simulation system transmits the synthesized GNSS signals to the receiver via a radio frequency (RF) front-end and an antenna, thereby emulating the real-world GNSS signal transmission process. During transmission, the signals may be affected by multipath effects, signal attenuation, and interference.
4. Signal Reception: The GNSS receiver captures the signals from the simulated satellites and uses them to compute its own position and timing information.
5. Performance Evaluation: The positioning and timing outputs of the GNSS receiver are used to evaluate its performance. For instance, the errors between the position and time information computed by the simulation system and the true reference values can be compared to assess the receiver's positioning accuracy and clock synchronization accuracy.

The Siren program primarily operates on steps 1 and 2 mentioned above. The transmission of signals is accomplished using SDR hardware such as [HackTF](https://greatscottgadgets.com/hackrf/one/), [BladeRF](https://www.nuand.com/bladerf-1/), and [USRP](http://www.ettus.com/). The Siren software’s operation involves the following steps:

1. Satellite Orbit Generation: The first step is to generate simulated GNSS satellite orbits. The software computes these orbits using broadcast ephemeris in RINEX (Receiver INdependent EXchange) format. The orbit data typically includes satellite position, velocity, and clock bias, and is utilized to calculate parameters such as signal propagation paths and time delays.
2. Pseudo-Code Generation: The pseudo-code (PRN code) is a vital component of the GNSS signal, serving the dual purposes of satellite identification and ranging measurements.
3. Navigation Message Generation: The navigation message is another crucial component of the GNSS signal, containing essential information such as satellite position, precise timing, and health status.
4. Carrier Wave Generation: The carrier of a GNSS signal is a high-frequency, continuous-phase sinusoidal wave.
5. Signal Modulation: Signal modulation is the process of modulating the pseudo-code and navigation message onto the carrier wave.
6. Signal Output: The modulated signal is exported to a file to test and validate the performance and functionality of the GNSS receiver. The output signal is formatted as a binary IQ file.

## Program compilation

We recommend using GCC to compile the program. If you're using a Windows system, you'll need to download and configure [MinGW-w64](https://www.mingw-w64.org/) before you can start compiling. 

After configuring MinGW-w64, simply navigate to the siren/ folder in your Windows:

```sh
mingw32-make
```

After entering the siren/ folder in the Linux system, run the command:

```sh
make
```

## Program Usage

The program currently offers only a command-line interface (CLI). Simply run `siren -h` in your terminal to see the following help information:

```
Usage: siren <navfile> [options]

Siren project for GNSS signal (GPS L1 C/A, Beidou B1I & B3I) simulation.

In dynamic mode, a trajectory can be specified in either a CSV file, which
contains the geodetic positions, or a NMEA GGA stream. The sampling rate of
trajectory has to be 10Hz. You can also start the static mode by inputting
a geodetic position. The longest simulation duration for the dynamic mode is
3600s, while for the static mode, it's 86400s. The satellite constellation
input through a GNSS broadcast ephemeris file in RINEX format. You can set
the program by command line options. With -c option, the processing options
are input from a configuration file. In this case, command line options
precede options in the configuration file.

Positional arguments:
  <navfile>        RINEX navigation file of GNSS ephemerides

Options:
  -c <cfgfile>     Input options from configuration file in TOML [off]
  -l <location>    Position in (lat,lon,hgt) [static mode]
  -p <pathfile>    Trajectory in (time,lat,lon,hgt) or GGA [dynamic mode]
  -t <date,time>   Scenario start time in (YYYY/MM/DD,hh:mm:ss)
  -d <duration>    Duration of signal simulation (s) [default: 300]
  -o <output>      I/Q data output (- for stdout) [default: gnsssim.iq]
  -s <frequency>   Sampling frequency (Hz) [default: 2600000]
  -b <bits>        I/Q data format (1/8/16) [default: 16]
  -T <level>       Output trace level (valid in debug version) [off]
  -v               Show details about simulated channels
  -V, --version    Show program's version and exit
  -h, --help       Show this help message and exit

Copyright (C) Jon Jiang, 2023-2026, All rights reserved.
```

Here,  `<nav_file` `>`  specifies the path to the navigation file in RINEX format. Siren requires this file to generate satellite signals and navigation messages. Another required parameter is the receiv er’s location. Siren can simulate either static or dynamic scenarios. In a static scenario, you only need to provide a fixed set of coordinates; in this case, use the  `-l`  parameter to input the latitude, longitude, and ellipsoidal height of the fixed point, and the program will enter static mode. In a dynamic scenario, you need to provide a traject ory file; in this case, the program will enter dynamic mode. Siren uses trajectory data sampled at 10 Hz. The path to the trajectory data file should be specified after the  `-p`  parameter, and the program will then enter dynamic mode. The trajectory data for dynamic mode can be provided either as a comma-separated values (CSV) file—where the time, latitud e, longitude, and ellipsoidal height are listed sequentially in decimal form—or as a GPGGA file. The program identifies the file type based on its extension: if the file has the ex tension  ".csv, " it is recognized as a CSV file; if the extension is  ".gga, " it is recognized as a GPGGA file.

For example, use the following command to simulate the signal at the specified location in static mode:

```
siren example/brdc2580.23n -l 35.681298,139.766247,10.0
```

Use the following command to simulate the specified trajectory in dynamic mode:

```
siren example/brdc2580.23n -p example/circle.csv
```

By default, the program uses the start time from the navigation file as the starting point for the simulation signal. Of course, you can also use the `-s` parameter to specify a custom start time for the simulation. The duration of the simulation signal is specified via the `-d` parameter. In dynamic mode, the maximum duration is 3600 seconds; in static mode, it is 86400 seconds.

For example, use the following command to specify that the simulation starts at 1:00 a.m. on September 15, 2023, and lasts for 180 seconds:

```sh
siren example/brdc2580.23n -p example/circle.csv -s 2023/9/15,00:00:00 -d 180
```

By default, the simulated signal will be output to a file named “gnsssim.iq.” Of course, you can also use the `-o` parameter to specify a different output filename. If you’d like to send the output to standard output, you can use “-” as the filename. The default sampling frequency of the output data is 2.6 MHz, and each data sample is 16 bytes (bits) wide. You can use the `-f` and `-b` parameters, respectively, to set the sampling frequency and data width.

For example, the following command sets the sampling frequency to 4 MHz and the data width to 8 bytes:

```sh
siren example/brdc2580.23n -p example/circle.csv -f 4000000 -b 8
```

In addition to using command-line arguments to configure the simulation process, you can also use the `-k` argument to specify a configuration file in TOML format:

```sh
siren example/brdc2580.23n -k config.toml -o gnsssim_spec.iq
```

At this point, the settings in this configuration file will be used. You can refer to “config.toml” to create your own configuration file. Of course, you can still use control parameters such as `-f`, `-b`, and `-v`; command-line settings always take precedence and will override the corresponding options in the configuration file.

## About

If you have any comments, suggestions, or bugs to report while using the program, please contact: jiangyingming@live.com.
