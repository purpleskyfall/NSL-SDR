# File Description

## File content

### bds_b1i_231107_55s_8mhz_8bit.iq

Beidou B1I signal simulation file, 55 seconds in duration, 8 MHz sampling rate, 8-bit word length. The simulation coordinates are: 35.9993334° E, 120.1199311° N, 30.0 m.

List of simulated satellite numbers: 6, 8, 9, 13, 16, 20, 23, 25, 27, 29, 30, 32, 38, 39, 41.

To generate this file, create a `config.toml` file using the following content:

```toml
# Configure demo for Siren software, in TOML language

version = '0.1.0'

[signal]
satsys = 'C'               # GNSS system [G:GPS, R:GLO, E:GAL, C:BDS]
exclsats = []              # exclude satellites ['G01', 'C02', ...]
band = 'B1I'               # GNSS frequency band [L1CA, B1I, B3I]
sample = 8.0e+6            # sampling frequency (Hz)
elvmask = 0                # mask of elevation angle (degree)
snrmask = 0                # mask of signal SNR (dBHz)
pathloss = true            # enable path loss

[errors]
ionospheric = 'brdc'       # ionospheric delay model [off, brdc]
tropospheric = 'saas'      # tropospheric delay model [off, saas]

[output]
bits = 8                   # I/Q data format [1/8/16]
verbose = false            # verbose mode enable
```

Then run the command:

```sh
siren example/brdc3110.23p -c config.toml -l 35.9993334,120.1199311,30.0 -t 2023/11/7,0:0:0 -d 55 -o bds_b1i_231107_55s_8m_8bit.iq
```

In practice, to speed up the computation of software receivers, the sampling rate can be set slightly lower—for example, to 5 MHz.

### bds_b3i_231107_55s_50mhz_8bit.iq

Beidou B3I signal simulation file, 55 seconds in duration, 50 MHz sampling rate, 8-bit word length. The simulation coordinates are: 35.9993334° E, 120.1199311° N, 30.0 m.

List of simulated satellite numbers: 6, 8, 9, 13, 16, 20, 23, 25, 27, 29, 30, 32, 38, 39, 41.

To generate this file, create a `config.toml` file using the following content:

```toml
# Configure demo for Siren software, in TOML language

version = '0.1.0'

[signal]
satsys = 'C'               # GNSS system [G:GPS, R:GLO, E:GAL, C:BDS]
exclsats = []              # exclude satellites ['G01', 'C02', ...]
band = 'B3I'               # GNSS frequency band [L1CA, B1I, B3I]
sample = 50.0e+6           # sampling frequency (Hz)
elvmask = 0                # mask of elevation angle (degree)
snrmask = 0                # mask of signal SNR (dBHz)
pathloss = true            # enable path loss

[errors]
ionospheric = 'brdc'       # ionospheric delay model [off, brdc]
tropospheric = 'saas'      # tropospheric delay model [off, saas]

[output]
bits = 8                   # I/Q data format [1/8/16]
verbose = false            # verbose mode enable
```

Then run the command:

```sh
siren example/brdc3110.23p -c config.toml -l 35.9993334,120.1199311,30.0 -t 2023/11/7,0:0:0 -d 55 -o bds_b3i_231107_55s_50mhz_8bit.iq
```

To speed up the computation of the software receiver, you can also set the sampling rate lower, for example, to 24 MHz.

## Software Receiver Testing Method

1. Download [CU-SDR-Collection](https://github.com/gnsscusdr/CU-SDR-Collection).
2. Navigate to the corresponding receiver folder (BDS/B1I or BDS/B3I).
3. Modify the options in the `initSettings.m` file (as shown in the table below).
4. Run `init` to launch the receiver, and type `1` when prompted to continue.

The `initSettings.m` file sets the parameters:

| Options                   | Settings             |
| ------------------------- | -------------------- |
| settings.fileName         | Actual file path     |
| settings.msToProcess      | 54000                |
| settings.numberOfChannels | 15                   |
| settings.IF               | 0                    |
| settings.samplingFreq     | Actual sampling rate |

## RF Testing Methodology

After properly connecting the HackRF to the computer and installing the  necessary drivers, use the following command to launch the HackRF for  signal transmission:

```sh
hackrf_transfer -t <iqfile> -f <center_freq> -s <fs> -x 40'
```

where `<iqfile>` specifies the file path of the simulated signal, `center_freq` denotes the radio frequency (RF). For the BeiDou B1I and B3I signals,  the center frequencies are 1561.098 MHz and 1268.52 MHz, respectively.  Additionally, `fs` represents the sampling frequency of the signal.

When testing with a smartphone, it is highly recommended to enable Airplane Mode.
