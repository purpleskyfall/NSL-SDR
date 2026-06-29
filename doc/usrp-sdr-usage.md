# USRP 软件无线电平台配置与使用

本文档描述使用 USRP B210 软件无线电设备进行 GPS 或北斗信号的仿真。

## 依赖软件安装

USRP 支持在 Windows 或 GNU/Linux 操作系统上运行，你需要首先安装好 Python3、GNU Radio 以及相关的依赖项。以 Ubuntu 系统为例，运行以下命令安装依赖项：

```sh
$ sudo apt-get install autoconf automake build-essential ccache cmake cpufrequtils doxygen ethtool fort77 g++ gir1.2-gtk-3.0 gobject-introspection gpsd gpsd-clients inetutils-tools libasound2-dev libboost-all-dev libcomedi-dev libcppunit-dev libfftw3-bin libfftw3-dev libfftw3-doc libfontconfig1-dev libgmp-dev libgps-dev libgsl-dev liblog4cpp5-dev libncurses5 libncurses5-dev libpulse-dev libqt5opengl5-dev libqwt-qt5-dev libsdl1.2-dev libtool libudev-dev libusb-1.0-0 libusb-1.0-0-dev libusb-dev libxi-dev libxrender-dev libzmq3-dev libzmq5 ncurses-bin python3-cheetah python3-click python3-click-plugins python3-click-threading python3-dev python3-docutils python3-gi python3-gi-cairo python3-gps python3-lxml python3-mako python3-numpy python3-opengl python3-pyqt5 python3-requests python3-scipy python3-setuptools python3-six python3-sphinx python3-yaml python3-zmq python3-ruamel.yaml swig wget
```

```sh
$ sudo apt install git cmake g++ libboost-all-dev libgmp-dev swig python3-numpy python3-mako python3-sphinx python3-lxml doxygen libfftw3-dev libsdl1.2-dev libgsl-dev libqwt-qt5-dev libqt5opengl5-dev python3-pyqt5 liblog4cpp5-dev libzmq3-dev python3-yaml python3-click python3-click-plugins python3-zmq python3-scipy python3-gi python3-gi-cairo gir1.2-gtk-3.0 libcodec2-dev libgsm1-dev pybind11-dev python3-matplotlib libsndfile1-dev python3-pip libsoapysdr-dev soapysdr-tools libiio-dev libad9361-dev libspdlog-dev
```

然后使用下面的命令安装 UHD 驱动：

```sh
$ sudo add-apt-repository ppa:ettusresearch/uhd
$ sudo apt-get update
$ sudo apt-get install libuhd-dev libuhd4.1.0 uhd-host
```

之后下载和配置 UHD：

```sh
$ sudo uhd_images_downloader
```

然后打开文件 /etc/profile，在其中最后添加：

```sh
export UHD_IMAGES_DIR=/usr/share/uhd/images
```

最后，查看 UHD 版本：

```sh
$ sudo uhd_find_devices
```

若终端能够打印出 UHD 版本，则说明安装成功。

接下来，安装 GNU Radio：

```sh
$ sudo add-apt-repository ppa:gnuradio/gnuradio-releases-3.X
$ sudo apt-get update
$ sudo apt install gnuradio
```

操作完成后用如下命令查看其版本，检查是否安装成功：

```sh
$ gnuradio-config-info –version
```

## 设备连接

使用网线连接计算机和 USRP 设备，然后将计算机网络属性修改为：

- IP 地址：192.168.10.1

- 子网掩码：255.255.255.0

- 默认网关：192.168.10.2

之后，在终端输入以下命令查看所连接的 USRP 设备：

```sh
$ uhd_usrp_probe
$ uhd_find_devices
```

若能够看到 USRP 设备，则说明设置成功。

## 信号发射

将要发射的卫星信号复制到工作文件夹中，运行信号发射程序 `trans_via_uhd.py`：

```sh
$ sudo trans_via_uhd.py bds_b1i.iq -g 25 -f 1561.098e3 -s 5.0e6 -b 16 -a addr=192.168.10.2 -c internal
```

其中 `bds_b1i.iq` 为文件位置， `-g` 为信号增益，`-f` 为信号载波频率，`-s` 为信号采样频率，`-b` 为信号位宽，`-a` 为 USRP 设备地址，`-c` 为参考时钟。这些内容的设置要与所发射的卫星信号相匹配。若命令正确，信号将会循环播放，如果要终止信号发射任务，请按下回车键即可。

## 注意事项

在 USRP2 设备上，采样率的设置需要基于 100 MHz 主时钟生成偶数倍的分频系数。当选择 2.5 MHz 采样率时，对应的分频系数为 40（100 MHz ÷ 40 = 2.5 MHz）。

推荐的采样率设置：

- GPS L1 C/A：2.5 MHz；
- BeiDou B1I：5.0 Mhz；
- BeiDou B3I：25.0 MHz。
