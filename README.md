# 项目简介

## 背景知识

### 什么是 SDR

软件无线电（Software Defined Radio，SDR）是一种无线电通信技术，它使用软件来实现无线电信号的调制解调和信号处理等功能。与传统的无线电通信系统相比，SDR系统具有更高的灵活性、可编程性和适应性，可以用于多种不同的应用场景。

SDR 系统的主要特点是使用数字信号处理技术来完成无线电信号的调制解调和信号处理等功能。SDR 系统由硬件和软件两部分组成。硬件部分包括调制解调器、天线、射频前端等组件，用于将无线电信号转换为数字信号或将数字信号转换为无线电信号。软件部分包括处理器、编程算法等组件，用于对数字信号进行处理、调制解调和信号处理等操作。通过软件控制，SDR 系统可以灵活地改变调制方式、频率、带宽等参数，以适应不同的应用需求。

SDR 系统的数字信号通常使用 IQ 文件存储，这是常用的无线电信号文件格式之一，它记录了接收到的无线电信号的 I（实数）和 Q（虚数）分量。IQ 文件通常使用标准的二进制格式，可以被大多数 SDR 软件和工具所支持。软件无线电平台将接收到的无线电信号以很高的采样率进行采样，然后将每个采样点的 I/Q 信号以数字编码，就生成了 IQ 数据信号文件。将该文件发送至硬件平台，硬件平台以设定的频率发射该信号，即得到了由软件实现的无线电。

### GNSS 信号的组成

本质上，GNSS（Global Navigation Satellite System）系统是一种由无线电实现的导航定位服务。以 GPS（Global Positioning System）系统为例，其卫星发射的信号主要由三个组成部分构成：

1. 载波（Carrier Phase）：载波是一种高频信号，是 GPS 信号的基础，可以用相位连续的正弦波表示。载波用于调制伪随机噪声码和数据码。GPS 系统  L1 频段信号的载波频率为 1575.42 MHz，L2 频段为 1227.60 MHz。
2. 伪随机噪声码（Pseudo-random Noise Code）：伪随机噪声码简称伪码，是一种长度为 1023 位的二进制序列。这是一种随机的、非周期性的序列，既可用于区分卫星，也可用于测距。
3. 数据码：数据码是 GPS 信号的第三个层次，是一列载有导航电文（Navigation Message）的二进制码。导航电文包含了卫星的位置、时间、卫星健康状况、星历等信息。

因为数据码的频率远小于伪码，伪码的频率又远小于载波，因此伪码和数据码得以同时调制到载波上，构成最终的 GNSS 信号。GNSS 接收机接收到信号后，通过解调获得的伪码和导航电文，可以得到卫星至接收机的距离以及卫星的位置，并据此实现定位功能。

### GNSS 接收机对信号的处理

概括下来，GNSS 接收机对收到的信号的处理过程，包括以下几个步骤：

1. 信号接收：GNSS 接收机通过天线接收 GNSS 信号，将信号转换为电信号传输到接收机的前置放大器中。
2. 信号放大：接收机的前置放大器将接收到的 GNSS 信号进行放大，增强信号强度，以便后续处理。
3. 信号滤波：GNSS 信号在传输过程中会受到多路径效应、多普勒效应、噪声干扰等影响，因此需要对信号进行滤波处理，去除多余的噪声和干扰，保留所需的信号成分。
4. 信号混频：GNSS 接收机将载波和伪码进行混频，得到中频信号，使信号频率降低到容易处理的范围内。
5. 信号解调：接收机将混频后的信号进行解调，得到伪码和导航电文等信息。
6. 信号比对：接收机将接收到的伪码和导航电文与已知的卫星信息比对，确定卫星的编号和时间信息等。
7. 信号处理：接收机使用伪码和导航电文计算出卫星和接收机之间的距离和位置等信息，完成定位和导航功能。

需要注意的是，GNSS 接收机的性能和精度受多种因素影响，如信号强度、天线位置、多径效应、卫星几何分布等。为了提高 GNSS 接收机的性能和精度，需要采用合适的天线和前置放大器、优化信号滤波和解调算法等措施。

## 程序原理

Siren 是一款基于软件定义无线电技术的 GNSS 信号仿真软件，能够模拟 GNSS 卫星发射的信号，并生成相应的 GNSS 信号文件，用于测试和验证 GNSS 接收机的性能和功能。通过简单的配置（主要是卫星星历和用户位置），该软件即可生成包含 GNSS 仿真信号的 IQ 文件。目前支持仿真的信号包括：

- GPS L1CA；
- BeiDou B1I；
- BeiDou B3I。

GNSS 信号仿真是一种生成虚拟 GNSS 卫星的信号的方法。其原理可以概括为以下几个步骤：

1. 生成卫星轨道和时钟数据：仿真系统需要生成卫星的轨道数据和时钟数据，以模拟 GNSS 卫星发射的信号。这些数据可以从真实的 GNSS 卫星中获取，或者使用模型进行生成。
2. 生成信号：仿真系统根据生成的轨道和时钟数据，生成 GNSS 信号。GNSS 信号是通过将多个频带的载波信号调制成一个复合信号来实现的。仿真系统需要模拟这个过程，以生成与实际 GNSS 信号相似的信号。
3. 传输信号：仿真系统需要将生成的 GNSS 信号通过射频前端和天线传输到接收机，以模拟真实的 GNSS 信号传输过程。在传输过程中，信号可能会受到多路径效应、信号衰减和干扰等影响。
4. 接收信号：GNSS 接收器接收到来自仿真卫星的信号，并使用接收到的信号来计算自身的位置和时间信息。
5. 评估性能：使用 GNSS 接收器输出的位置和时间信息来评估接收器的性能。例如，可以比较仿真系统计算出的位置和时间信息与真实值之间的误差，以评估接收器的定位精度和时钟同步精度。

Siren 程序主要工作在上述步骤的 1-2，传输信号由 SDR 硬件如 [HackTF](https://greatscottgadgets.com/hackrf/one/)，[BladeRF](https://www.nuand.com/bladerf-1/) 和 [USRP](http://www.ettus.com/)  等完成。Siren 软件的工作包括以下几个步骤：

1. 生成卫星轨道：首先需要生成模拟的 GNSS 卫星轨道，软件使用 RINEX（Receiver INdependent EXchange）格式的广播星历计算卫星轨道，卫星轨道通常包含卫星的位置、速度、钟差等信息，轨道数据用于计算卫星信号的传播路径和时间延迟等参数。
2. 生成伪码：伪码是 GNSS 信号的重要组成部分，用于区分卫星和测距。
3. 生成导航电文：导航电文是 GNSS 信号的另一个重要组成部分，包含了卫星的位置、时间、健康状况等信息。
4. 生成载波：GNSS 信号的载波是一种高频的、相位连续的正弦波信号。
5. 信号调制：信号调制是将伪码和导航电文调制到载波上的过程。
6. 信号输出：将调制后的信号输出到文件中，用于测试和验证 GNSS 接收机的性能和功能。输出信号的格式是二进制的 IQ 文件。

## 程序编译

推荐使用 GCC 编译程序，Windows 系统使用 GCC 需要下载和配置 [MinGW-w64](https://www.mingw-w64.org/)，配置完成后即可进行编译。Windows 系统进入 siren/ 文件夹后运行命令：

```
$ mingw32-make
```

Linux 系统进入 siren/ 文件夹后运行命令：

```
$ make
```

即可启动编译，编译完成后可以看到可执行的程序文件。

## 程序使用

程序目前仅提供命令行界面（Command Line Interface, CLI）。在终端中运行 `siren -h` 即可看到如下帮助信息：

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

这里 `<nav_file>` 为 RINEX 格式的导航文件路径，Siren 需要该文件生成卫星信号和导航电文。另一个必须的参数是接收机的位置。Siren 既可以仿真静态场景，也可以仿真动态场景。在静态场景下，只需要输入一个固定的坐标，此时使用 `-l` 参数输入固定点的纬度、经度和大地高，程序会进入静态模式。在动态场景下，需要输入一段轨迹，此时程序进入动态模式。Siren 使用 10Hz 的轨迹数据，轨迹数据的路径在 `-p` 参数后输入，此时程序会进入动态模式。动态模式的轨迹可使用逗号分隔的 CSV（Comma-Separated Values）文件（依次为小数表示的时间、纬度、经度和大地高），也可使用 GPGGA 文件。程序会根据文件扩展名识别（若文件扩展名为 “.csv” 则识别为 CSV 文件，若扩展名为 “.gga” 则识别为 GPGGA 文件）。

例如，使用下面的命令以静态模式仿真指定位置的信号：

```
$ siren example/brdc2580.23n -l 35.681298,139.766247,10.0
```

使用下面的命令以动态模式对指定的轨迹仿真：

```
$ siren example/brdc2580.23n -p example/circle.csv
```

默认情况下，程序从导航文件的开始时间作为仿真信号的起点。当然，你也可以使用 `-s` 参数来指定仿真开始的时间。仿真信号的持续时间通过 `-d` 参数指定。对于动态模式，持续时间最大为 3600 秒；对于静态模式则为 86400 秒。

例如，使用下面的命令指定仿真从 2023 年 9 月 15 日 1 点钟开始，持续 180 秒：

```
$ siren example/brdc2580.23n -p example/circle.csv -s 2023/9/15,00:00:00 -d 180
```

默认情况下，仿真的信号将输出到名为 “gnsssim.iq” 的文件中。当然，你也可以使用 `-o` 参数为其指定输出的文件名。如果你希望将输出发送至标准输出，可使用 “-” 作为文件名。输出数据的采样频率默认为 2.6 MHz，每个数据的宽度为 16 字节（bit），你可以分别使用 `-f` 和 `-b` 参数设置其采样频率和数据宽度。

例如，下面的命令将采样频率设置为 4 MHz，数据宽度为 8 字节：

```
$ siren example/brdc2580.23n -p example/circle.csv -f 4000000 -b 8
```

除了使用命令行参数对仿真过程进行设置，你还可以通过 `-k` 参数来输入一个 TOML 格式的配置文件：

```
$ siren example/brdc2580.23n -k config.toml -o gnsssim_spec.iq
```

此时将使用该配置文件中的设置。你可以参照 “config.toml” 来编写自己的配置文件。当然，你仍可以使用 `-f`、`-b`、`-v` 等控制参数，命令行设置永远是优先的，它将覆盖配置文件中的对应选项。

## 关于

如果您在使用程序过程中，有任何意见、建议或 Bug 等需要反馈，请联系：jiangyingming@live.com。
