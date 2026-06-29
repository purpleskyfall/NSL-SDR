# Siren Software Overview

## Background knowledge

### What is SDR?

Software Defined Radio (SDR) is a radio communication technology that uses software to perform functions such as modulation, demodulation, and signal processing of wireless signals. Compared to traditional radio communication systems, SDR systems offer greater flexibility, programmability, and adaptability, making them suitable for a wide variety of application scenarios.

The main feature of an SDR system is its use of digital signal processing technology to perform functions such as modulation, demodulation, and signal processing of radio signals. An SDR system consists of two main components: hardware and software. The hardware component includes elements such as modems, antennas, and RF frontends, which are used to convert radio signals into digital signals (or vice versa). The software component comprises elements such as processors and programming algorithms, which are used to process, modulate, demodulate, and perform other signal-processing operations on digital signals. By means of software control, an SDR system can flexibly adjust parameters such as modulation scheme, frequency, and bandwidth to meet the requirements of various applications.

SDR technology is widely used in various radio communication systems, such as broadcasting and television, satellite communications, mobile communications, and military communications. This technology can enhance the flexibility and programmability of communication systems, reduce system costs and power consumption, and simultaneously improve system security and anti-interference performance.

Common SDR hardware includes:

- [Universal Software Radio Peripheral (USRP)](http://www.ettus.com/): The USRP is an SDR hardware platform developed by Ettus Research. It enables the implementation of various radio communication systems through pluggable RF front-ends and digital signal processing daughterboards. Renowned for its high flexibility, scalability, and performance, the USRP is widely applied in fields such as radio communications, radar, and signal processing.

- [BladeRF](https://www.nuand.com/bladerf-1/): The BladeRF is an SDR hardware platform developed by Nuand. Utilizing an FPGA and an ARM Cortex-A9 processor, it supports multiple radio communication standards and frequency ranges. The BladeRF is characterized by its high performance, low power consumption, and programmability.

- [HackRF](https://greatscottgadgets.com/hackrf/one/): The HackRF is an SDR hardware platform developed by Great Scott Gadgets. Capable of radio reception and transmission from 1 MHz to 6 GHz, it supports various modulation schemes and encoding formats. The HackRF boasts advantages such as high performance, low cost, and open-source accessibility.

### SDR file format

An IQ file is a commonly used radio signal file format in SDR technology. It records the I (real) and Q (imaginary) components of a radio signal and can be used for offline signal processing and analysis. IQ files typically use a standard binary format and are supported by most SDR software and tools. The software platform of software-defined radio samples the radio signal at a very high sampling rate, then encodes the I/Q signals of each sample point digitally, thereby generating an IQ data signal file.

When storing radio signals, IQ files typically use single-byte (8-bit) or double-byte (16-bit) integers to represent the signal amplitude. Obviously, using double-byte integers provides a more “fine-grained” representation of the signal, but it also requires significantly more storage space.

### Composition of GNSS Signals

Take GPS as an example: The GPS (Global Positioning System) signal consists primarily of the following three components:

1. Navigation Message: The navigation message serves as the fundamental information of the GPS signal, containing data such as satellite ephemeris, time, and health  status. It is transmitted at a data rate of 50 bps (bits per second),  with each data stream comprising multiple navigation data frames and  error-correction codes.
2. Carrier Waveform: The carrier waveform is the high-frequency component of the GPS signal, used to transmit the navigation message and positioning information.  The carrier frequencies for GPS signals are located in the L1 band  (1575.42 MHz) and the L2 band (1227.60 MHz), forming a continuous-phase  sinusoidal signal.
3. Pseudo-Random Noise Code (PRN Code): The PRN code is the pseudo-code component of the GPS signal, utilized  to distinguish signals from different satellites and to provide  resistance against interference. It is a pseudo-code sequence composed  of binary bits, with a length of 1023 chips.

During the transmission of GPS signals, pseudorandom noise codes and navigation messages are modulated onto a carrier waveform for transmission. By demodulating the received pseudorandom noise codes and navigation messages, the receiver can calculate the distance between the satellite and the receiver, as well as the satellite’s coordinates, and subsequently determine its own position information.

### GNSS Receiver Signal Processing

The process by which a GNSS (Global Navigation Satellite System) receiver processes the received signals includes the following steps:

1. Signal Reception: The receiver captures the signal via an antenna and then converts it  into an electrical signal, which is transmitted to the receiver's  front-end amplifier.
2. Signal Amplification: The front-end amplifier of the receiver amplifies the received signal to enhance its strength for subsequent processing.
3. Signal Filtering: During transmission, the signal is subject to multipath effects, noise  interference, and other impairments. Therefore, the signal must be  filtered to remove noise and interference while retaining the desired  signal components.
4. Signal Mixing: The receiver mixes the carrier waveform and the pseudo-random noise  code to obtain an intermediate frequency (IF) signal, thereby reducing  the signal frequency to a range that is easier to process.
5. Signal Demodulation: The receiver demodulates the mixed signal to extract information such  as the navigation message and the pseudo-random noise code.
6. Signal Matching: The receiver compares the received pseudo-random noise code and  navigation message with known satellite information to determine the  satellite's identification number (PRN) and timing information.
7. Signal Processing: Based on the pseudo-random noise code and phase of the signal, the  receiver continuously calculates the distance to the satellite and its  own position, thereby accomplishing the positioning and navigation  functions.

It is important to note that the performance of a GNSS receiver is influenced by various factors, such as signal strength, antenna placement, multipath effects, and satellite geometry. To enhance the performance of a GNSS receiver, it is necessary to adopt appropriate antennas and preamplifiers, optimize signal filtering and demodulation algorithms, and implement other relevant measures.

## Siren

Siren is GPS signal simulation software based on software-defined radio technology. It can simulate the signals transmitted by GPS satellites and generate corresponding IQ signal files for testing the performance of GPS receivers. The project is primarily developed using the C language. By means of simple configuration—mainly including satellite ephemeris data and user location—it can generate the corresponding GPS simulation signal IQ files. These simulated signals are then transmitted at a specified frequency, allowing nearby receiving devices to capture the simulated signals.

### Program Principle

GPS signal simulation is a method used to test and evaluate the performance of GPS receivers by simulating the signals emitted by GPS satellites. The underlying principle can be briefly summarized in the following steps:

1. Generate satellite orbit and clock data: The simulation system needs to generate satellite orbit and clock data to simulate the signals transmitted by GPS satellites. This data can be obtained from real GPS satellites or generated using mathematical models.
2. Generate signals: Based on the satellite orbit and clock data, the simulation system generates the GPS signals. GPS signals are implemented by modulating carrier signals across multiple frequency bands into a composite signal. The simulation system must replicate this process to generate signals that closely resemble actual ones.
3. Transmit signals: The simulation system transmits the generated GPS signals to the receiver via an antenna to simulate the real-world GPS signal transmission process. During transmission, the signals may be affected by multipath effects, signal attenuation, and interference.
4. Receive signals: The GPS receiver receives the simulated signals and calculates its own position and time.
5. Evaluate performance: The position and time outputs from the GPS receiver are used to evaluate its performance. For example, the positioning accuracy and clock synchronization accuracy of the receiver can be assessed by comparing the errors between the position and time calculated by the simulation system and the true reference values.

The Siren program primarily operates on steps 1 and 2 mentioned above. The transmission of signals is carried out by relevant SDR hardware such as [HackTF]([HackRF One - Great Scott Gadgets](https://greatscottgadgets.com/hackrf/one/)), [BladeRF]([bladeRF - Nuand](https://www.nuand.com/bladerf-1/)), and [USRP](http://www.ettus.com/). The Siren software’s operation involves the following steps:

1. Generate satellite orbits: First, simulated GPS satellite orbits must be generated. The software  uses RINEX format broadcast ephemeris files to generate the satellite  orbit data. This data typically includes satellite position, velocity,  and satellite clock bias, which are used to calculate signal propagation paths and time delays.
2. Generate navigation messages: The navigation message is a crucial component of the GPS signal,  containing information such as satellite ephemeris, time, and satellite  health status.
3. Generate Pseudo-Random Noise (PRN) codes: The PRN code is another essential component of the GPS signal, utilized to distinguish signals from different satellites and to provide  resistance against interference.
4. Generate carrier waveforms: The GPS signal carrier comprises the L1 band (1575.42 MHz) and the L2  band (1227.60 MHz), forming a continuous-phase sinusoidal signal.  Currently, the program only supports the L1 band.
5. Signal modulation: Signal modulation involves modulating the navigation message and PRN  code onto the carrier wave to obtain the I/Q signal values for each  sample point. The signals from all visible satellites are then  accumulated to generate the final composite signal.
6. Simulated signal output: The modulated signal is exported to a file in a binary IQ format. The  bit width for the sample values can be 1-bit, 8-bit, or 16-bit.

### Main program flow

Ignoring the necessary configuration parameter loading and configuration item checks, the main program flow is as shown in the figure below:

```flow
start=>start: Start
end=>end: End
eph=>operation: Ephemeris Loading and Selection
visible=>operation: Find Visible Satellites
rho=>operation: Calculate Satellite-Receiver Distance
range=>operation: Calculate C/A Code and Data Code Phase
iq=>operation: Generate Signal
cycle=>condition: Update Ephemeris?
endt=>condition: Reach Simulation Duration
start->eph->visible->rho->range->iq->endt
endt(no)->cycle
endt(yes)->end
cycle(yes)->eph
cycle(no)->rho
```

Note: The program checks whether the ephemeris needs to be updated at the 0th and 30th second of each minute.
