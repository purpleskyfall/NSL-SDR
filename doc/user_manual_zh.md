# 用户手册

## 引言

Siren 是一款基于软件无线电（SDR）技术的 GNSS 信号仿真软件。它能够模拟全球定位系统（GPS）和北斗卫星导航系统（BDS）卫星发射的信号，并生成标准的 IQ 信号文件。这些文件可用于测试 GNSS 接收机性能，或通过 SDR 硬件（如 USRP、HackRF）发射。

Siren 的主要功能包括：

• 多系统支持：GPS L1 C/A、北斗 B1I、北斗 B3I。
• 灵活配置：通过 RINEX 广播星历文件和 TOML 配置文件自定义参数。
• 误差模拟：采用 Klobuchar 电离层模型、Saastamoinen 对流层模型。
• 动态轨迹：支持 CSV 或 NMEA GGA 格式的运动轨迹文件。
• 标准输出：二进制 IQ 文件（1/8/16-bit）。
• 模块化设计：代码结构清晰，易于扩展。

Siren 支持的信号包括：

| 信号 | 载波频率 | 伪码速率 | 伪码长度 | 调制方式 |
| --- | ------- | ------ | ------ | -------- |
| GPS L1 C/A | 1575.42 MHz | 1.023 Mcps | 1023 | BPSK |
| 北斗 B1I | 1561.098 MHz | 2.046 Mcps | 2046 | BPSK |
| 北斗 B3I | 1268.52 MHz | 10.23 Mcps | 10230 | BPSK |

## 快速入门

### 编译

Siren 核心算法采用 C 语言编写（C90 标准），编译的环境要求：

- 编译器：GCC（Linux / MinGW-w64）。
- 构建工具：Make。

用户可以采用如下命令编译：

```sh
cd siren/
make          # Linux/Mac
mingw32-make  # Windows (MinGW)
```

编译成功后在当前目录生成可执行文件 siren。

### 使用方法

Siren 是一个命令行程序，通过以下命令使用它：

```sh
siren <navfile> [options]
```

必须的参数包括：

- `<navfile>`：RINEX 导航文件（版本 2.x 或 3.x）。

常用的可选参数有：

- `-c <cfgfile>`：TOML 格式配置文件路径。
- `-l <lat,lon,hgt>`：静态接收机位置。
- `-p <path>`：动态轨迹文件（CSV 或 GGA）。
- `-t <date,time>`：仿真开始时间，默认为导航文件开始时间。
- `-d <duration>`：仿真时长（秒），默认 300。
- `-s <frequency>`：采样率（Hz），默认 2.6e6。
- `-b <bits>`：IQ 位宽（1/8/16），默认 16。
- `-o <path>`：输出的 IQ 文件路径，默认 gnsssim.iq。

静态接收机位置或动态轨迹文件必须设置一项，这两个参数项将仿真状态设置为静态或动态模式。一个完整的命令示例如下：

```sh
siren example/brdc3110.23p -c config.toml -l 35.9,120.1,30.0 -d 55 -o bds_b1i_231107_55s_8m_8bit.iq
```

这里采用 `config.toml` 对仿真信号、采样率、误差模型和信号位宽等进行设置，一个详细的配置文件结构如下（示例）：

```TOML
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

命令行选项优先级高于配置文件。

## 程序架构与核心概念

### 代码结构

Siren 的核心代码放在 src 文件夹，tools 文件夹内包含一些辅助脚本，这些脚本采用 Python 编写。

```sh
siren/
├── src/                 # C 核心算法
│   ├── channels.c       # 通道管理（分配、更新、信号生成）
│   ├── code.c           # 伪码生成（GPS L1CA、BDS B1I/B3I、NH20）
│   ├── ephemeris.c      # 星历处理与卫星位置计算
│   ├── errors.c         # 电离层/对流层误差模型
│   ├── io.c             # 文件输入输出（RINEX、轨迹、IQ 写入）
│   ├── options.c        # TOML 配置文件解析
│   ├── signal.c         # 导航电文生成与校验、伪距计算、码相位计算
│   ├── siren.c          # 主程序入口
│   ├── siren.h          # 主头文件（常量、类型、函数声明）
│   ├── toml.c/h         # TOML 解析库（第三方）
│   └── utils.c          # 通用工具（向量运算、坐标转换、时间转换）
├── tools/               # Python 辅助脚本
│   ├── combine_l1b1i.py # 使用 FDM 合并 GPS L1C/A 和北斗 B1I 信号
│   ├── genpath.py       # 生成动态轨迹文件
│   ├── plotpath.py      # 在地图上绘制轨迹
│   ├── plotskymap.py    # 绘制仿真卫星天空图
│   ├── samebin.py       # 检查两个二进制文件是否一致（测试使用）
│   └── trans_via_uhd.py # 通过 UHD 设备发送信号
└── makefile
```

### 核心数据结构

#### gtime\_t – GNSS 时间

| 字段 | 类型   | 说明                       |
| ---- | ------ | -------------------------- |
| week | int    | GPS 周数（自 1980/01/06 起） |
| sec  | double | 周内秒（秒）               |

#### datetime\_t – UTC 时间

| 字段  | 类型   | 说明 |
| ----- | ------ | ---- |
| year  | int    | 年   |
| month | int    | 月   |
| day   | int    | 日   |
| hour  | int    | 时   |
| min   | int    | 分   |
| sec   | double | 秒   |

#### eph\_t – 广播星历

| 字段 | 类型 | 说明 |
| ---- | ---- | ---- |
| sat | int | 卫星编号 |
| iode | int | 星历数据期号（IODE/AODE） |
| iodc | int | 时钟数据期号（IODC/AODC） |
| sva | int | 精度指数（URA） |
| svh | int | 健康状态（0 正常） |
| week | int | GPS 周（BDS 为 BD周） |
| code | int | GPS:L2 码类型 / BDS:信号源 |
| flag | int | GPS:L2P 标志 / BDS:导航类型 |
| toc | gtime\_t | 时钟参考时间 |
| toe | gtime\_t | 星历参考时间 |
| ttr | gtime\_t | 传输时间 |
| A | double | 轨道半长轴（米） |
| e | double | 偏心率 |
| i0 | double | 轨道倾角（弧度） |
| idot | double | 倾角变化率（弧度/秒） |
| deln | double | 平均角速度修正量（弧度/秒） |
| M0 | double | 平近点角（弧度） |
| omg | double | 近地点幅角（弧度） |
| OMG0 | double | 升交点赤经（弧度） |
| OMGd | double | 升交点赤经变化率（弧度/秒） |
| cuc/cus | double | 纬度幅角余弦/正弦修正项（弧度） |
| cic/cis | double | 倾角余弦/正弦修正项（弧度） |
| crc/crs | double | 轨道半径余弦/正弦修正项（米） |
| toes | double | 星历参考时间（周内秒） |
| fit | double | 拟合间隔（小时） |
| f0/f1/f2 | double | 时钟偏差、漂移、漂移率 |
| tgd\[6] | double | 群延迟参数（不同系统含义不同） |
| Adot/ndot | double | CNAV 中的半长轴变化率和平均运动变化率 |

#### nav\_t – 导航数据集合

| 字段 | 类型 | 说明 |
| ---- | ---- | ---- |
| n | int | 星历数量 |
| nmax | int | 已分配容量 |
| eph | eph\_t\* | 星历数组 |
| utc\_gps\[8] | double | GPS-UTC 参数 |
| utc\_gal\[8] | double | Galileo-UTC 参数 |
| utc\_bds\[8] | double | 北斗-UTC 参数 |
| ion\_gps\[8] | double | GPS 电离层参数 |
| ion\_gal\[4] | double | Galileo 电离层参数 |
| ion\_bds\[8] | double | 北斗电离层参数 |

#### range\_t – 卫地距

| 字段 | 类型 | 说明 |
| ---- | ---- | ---- |
| gt | gtime\_t | 接收时间 |
| range | double | 伪距（米，含误差） |
| rate | double | 伪距变化率（米/秒） |
| d | double | 几何距离（米） |
| azel\[2] | double | 方位角/仰角（弧度） |
| iono\_delay | double | 电离层延迟（米） |
| trop\_delay | double | 对流层延迟（米） |

#### channel\_t – 信号通道

| 字段 | 类型 | 说明 |
| ---- | ---- | ---- |
| sys | int | 卫星系统 |
| prn | int | PRN 号 |
| ctype | int | 码类型（CTYPE\_L1CA/CTYPE\_B1I/CTYPE\_B3I） |
| code |short\* | 测距码序列 |
| clen | int | 测距码长度 |
| nh |short\* | NH 码序列 |
| nhlen | int | NH 码长度 |
| f\_carr |double | 载波多普勒频率（Hz） |
| f\_code |double | 码频率（chip/s） |
| f\_code0 |double | 基带码频率（chip/s） |
| carr\_phase | double/unsigned int | 载波相位（浮点或定点） |
| carr\_phasestep | int | 载波相位步进（定点模式） |
| code\_phase |double | 码相位（chip） |
| g0 | gtime\_t | 整帧起始的 GPS 时间 |
| sbf\[5]\[10] | unsigned long | 5 个子帧各 10 个字（原始内容） |
| dwrd\[60] | unsigned long | 60 个数据字（含前一帧子帧） |
| iword/ibit/icode | int | 当前字/比特/码索引 |
| codechip | int | 当前码片电平（±1） |
| databit | int | 当前数据比特电平（±1） |
| nhbit | int | 当前 NH 码比特电平（±1） |
| azel\[2] |double | 卫星方位/仰角（弧度） |
| rho0 | range\_t | 上一历元的伪距信息 |

#### options\_t – 配置选项

| 字段 | 类型 | 说明 |
| ---- | ---- | ---- |
| satsys | int | 仿真卫星系统（SYS\_GPS/SYS\_BDS） |
| exclsats\[] | int | 排除卫星标志（按卫星编号） |
| band | int | 仿真频段（CTYPE\_L1CA/CTYPE\_B1I/CTYPE\_B3I） |
| sample | double | 采样率（Hz） |
| elvmask | double | 仰角掩膜（度） |
| pathloss | int | 路径损耗模拟开关 |
| ionospheric | int | 电离层模型（IONOOPT\_BRDC/IONOOPT\_OFF） |
| tropospheric | int | 对流层模型（TROPOPT\_SAAS/TROPOPT\_OFF） |
| bits | int | IQ 位宽（SC01/SC08/SC16） |
| verbose | int | 详细输出开关 |

### 信号生成流程

1. 加载星历：readnav() → 解析 RINEX 文件，填充 nav\_t。
2. 选择星历：根据仿真开始时间选择每颗卫星的当前有效星历。
3. 初始化通道：initchan() → 清除通道，allocchan() → 计算可见卫星，分配通道，生成测距码、NH 码、子帧、初始伪距和载波相位。
4. 循环生成信号（每 0.1 秒一次）：
   - updatechan() → 更新伪距、码相位、载波频率、增益。
   - gensignal() → 为每个采样点累加所有可见卫星的 I/Q 值。
   - writeiq() → 写入文件。
   - 每 30 秒调用 flushchan() 更新导航电文，并检查是否需要切换星历。
5. 结束：释放资源，关闭文件。

## API 参考手册

### 基础函数（utils.c）

| 函数原型 | 说明 |
| ------- | ---- |
| void sub3(const double \*x1, const double \*x2, double \*y)  | 三维向量减法 |
| double norm3(const double \*x)  | 三维向量模长 |
| double dot3(const double \*x1, const double \*x2)  | 三维向量点积 |
| double str2num(const char \*str, int i, int n)  | 字符串转数字 |
| datetime\_t str2date(const char \*str, int i, int n)  | 字符串转 UTC 日期 |
| int satno(int sys, int prn)  | 系统+PRN → 卫星编号 |
| int satid2no(const char \*id)  | 卫星 ID（如"G01"）→ 卫星编号 |
| int satsys(int sat, int \*prn)  | 卫星编号 → 系统和 PRN |
| char syschr(int sys)  | 系统代号字符（G/E/C） |
| gtime\_t date2gpst(const datetime\_t dt)  | UTC 日期 → GPS 时间 |
| datetime\_t gpst2date(const gtime\_t gt)  | GPS 时间 → UTC 日期 |
| gtime\_t addgtime(gtime\_t g0, double dt)  | GPS 时间加法 |
| double subgtime(gtime\_t g1, gtime\_t g0)  | GPS 时间差（秒） |
| gtime\_t bdt2gpst(gtime\_t t0)  | BDT → GPST（+14秒） |
| gtime\_t gpst2bdt(gtime\_t t0)  | GPST → BDT（-14秒） |
| gtime\_t bdt2time(int week, double sec)  | BDT 周+秒 → gtime\_t |
| int bdsorbit(int prn)  | 北斗卫星轨道类型（GEO/IGSO/MEO） |
| void xyz2llh(const double \*xyz, double \*llh)  | ECEF → 经纬高 |
| void llh2xyz(const double \*llh, double \*xyz)  | 经纬高 → ECEF |
| void ltcmat(const double \*llh, double t\[3]\[3])  | 计算 NEU 旋转矩阵 |
| void ecef2neu(...)  | ECEF → NEU |
| void neu2azel(const double \*neu, double \*azel)  | NEU → 方位角/仰角 |

### 伪码生成（code.c）

| 函数原型 | 说明 |
| ------- | ---- |
| short \*gencode(int prn, int ctype, int \*len, double \*crate) | 通用伪码生成入口，根据 ctype 调用具体实现 |
| static short \*gencode\_L1CA(int prn, int \*len, double \*crate) | GPS L1 C/A 码（G1/G2 移位寄存器，1023 码片） |
| static short \*gencode\_B1IB2I(int prn, int \*len, double \*crate) | 北斗 B1I/B2I 码（11 级移位寄存器，2046 码片） |
| static short \*gencode\_B3I(int prn, int \*len, double \*crate) | 北斗 B3I 码（13 级移位寄存器，10230 码片，含复位机制） |
| static short \*gencode\_NH20(int \*len, double \*crate) | 20 位 Neuman-Hoffman 码（固定序列） |
| static int equal13(const short \*reg, const short \*ref) | 判断两个长度为 13 的数组是否相等（B3I 专用） |

### 星历与卫星位置（ephemeris.c）

| 函数原型 | 说明 |
| ------- | ---- |
| void satpos(gtime\_t time, const eph\_t \*eph, double \*pos, double \*dts) | 计算卫星 ECEF 位置和钟差 |
| void satvel(gtime\_t time, const eph\_t \*eph, double \*vel, double \*ddts) | 数值微分法计算卫星速度和钟漂 |
| int satvisible(gtime\_t gt, eph\_t \*eph, const double \*rcv, double elvmask, double \*azel) | 判断卫星可见性 |
| void eph2sbf(int sys, const eph\_t \*eph, const double \*utc, const double \*iono, unsigned long sbf\[5]\[N\_DWRD\_SBF]) | 将星历参数编码为子 |帧字（GPS/BDS）
| static void eph2sbf\_gps(...) | GPS 子帧编码（含电离层/UTC 页） |
| static void eph2sbf\_bds(...) | 北斗 D1 导航电文子帧编码 |

### 误差模型（errors.c）

| 函数原型 | 说明 |
| ------- | ---- |
| double ionodelay(gtime\_t gt, const double \*iono, double \*rcv, double \*azel) | Klobuchar 电离层延迟（米） |
| double tropdelay(const double \*pos, const double \*azel, double humi) | Saastamoinen 对流层延迟（米） |

### 输入输出（io.c）

| 函数原型 | 说明 |
| ------- | ---- |
| int readnav(const char \*filename, nav\_t \*nav)  | 读取 RINEX 导航文件 |
| int readtraj\_csv(const char \*filename, const int length, double \*rcvpos)  | 读取 CSV 轨迹文件 |
| int readtraj\_gga(const char \*filename, const int length, double \*rcvpos)  | 读取 NMEA GGA 轨迹文件 |
| int writeiq(short \*buff, const int buff\_size, FILE \*fp, const int format)  | 写入 IQ 数据（支持1/8/16-bit） |
| void traceopen(const char \*file)  | 打开跟踪日志文件 |
| void traceclose(void)  | 关闭跟踪日志文件 |
| void tracelevel(int level)  | 设置跟踪级别 |
| void trace(int level, const char \*format, ...)  | 输出跟踪信息 |

### 配置解析（options.c）

| 函数原型 | 说明 |
| ------- | ---- |
| int readopt(const char \*filename, options\_t \*opt) | 解析 TOML 配置文件 |

### 通道操作（channels.c）

| 函数原型 | 说明 |
| ------- | ---- |
| void initchan(void) | 初始化所有通道 |
| void printchan(FILE \*fp, const int head) | 打印通道状态 |
| int allocchan(gtime\_t gt, nav\_t \*nav, int \*iephs, double \*rcv, options\_t opt) | 分配可见卫星通道 |
| int updatechan(gtime\_t gt, nav\_t \*nav, int \*iephs, double \*rcv, options\_t opt) | 更新通道参数（码相位、增益） |
| void flushchan(gtime\_t gt, nav\_t \*nav, int \*iephs, const int neweph) | 刷新导航电文 |
| void gensignal(short \*buff, const int buff\_size, const double interval) | 生成 IQ 信号样本 |

### 信号生成（signal.c）

| 函数原型 | 说明 |
| ------- | ---- |
| void calcrange(gtime\_t gt, const eph\_t \*eph, const double \*iono, double rcv[], options\_t opt, range\_t \*rho)  | 计算伪距（含地球自转改正、误差）|
| void calccaphase(range\_t rho1, double dt, channel\_t \*chan)  | 计算码相位、载波频率、数据位计数器 |
| int gennavmsg(gtime\_t gt, channel\_t \*chan, int init)  | 生成导航电文（GPS/BDS） |
| static int gennavmsg\_gps(...)  | GPS 导航电文生成（含奇偶校验） |
| static int gennavmsg\_bds(...)  | 北斗导航电文生成（含 BCH 编码、交织） |
| static unsigned long checksum(...)  | GPS 奇偶校验（汉明码） |
| static unsigned long bchencode(...)  | BCH (15,11)编码 |
| static unsigned long interleave(...)  | 两路 15 位字的交织 |

## 内部算法详解

### 伪码生成

- GPS L1 C/A：采用两个 10 级 Gold 码发生器 G1 和 G2，G2 相位偏移由 PRN 决定，最终码片值为 -G1\[i]\*G2\[j]，长度为 1023。
- 北斗 B1I/B2I：采用两个 11 级线性反馈移位寄存器，G2 抽头位置由PRN查表得到，长度为 2046。
- 北斗 B3I：采用两个 13 级寄存器，G2 初始相位由 PRN 对应的复位位置决定，当 CA 寄存器达到特定状态时自动复位，长度为 10230。
- NH 码：20 位固定序列 0x1A4C1（二进制 00001010011000001，映射为 ±1）。

### 导航电文生成

GPS 子帧结构：

- 每帧 5 个子帧，每个子帧 10 个字（30 比特），总长 30 秒。
- 子帧 1 包含星期数、时钟参数；子帧2-3包含星历参数；子帧 4-5 包含电离层、UTC、历书等。
- 每个字经过汉明码奇偶校验（`checksum()`），其中第 2 和第 10 个字为非信息承载位（需特殊处理）。

北斗 D1 导航电文：

- 每帧 5 个子帧，每个子帧 10 个字，总长 30 秒。
- 子帧 1 包含周计数、钟差、电离层参数等；子帧 2-3 包含星历；子帧 4-5 备用。
- 每个字先进行 BCH(15,11) 编码，然后对第 2-10 字进行两路交织（interleave）。

### 载波相位计算

- 使用 `calccaphase()` 根据伪距变化率计算多普勒频率 `f_carr` 和码频率 `f_code`。
- 载波相位初始值通过参考点（原点）与接收机位置的几何关系设定，使信号到达接收机时相位对齐。
- 支持两种模式：浮点相位（`FLOAT_CARR_PHASE` 宏）和定点相位（32 位无符号整数，高 9 位查表）。

### 信号合成

`gensignal()` 中对于每个采样点：

1. 遍历所有活跃通道，计算 `ip = nhbit * databit * codechip * CosTable[itable] * gain[i]`，`qp` 同理用 `SinTable`。
2. 累加所有通道的 I 和 Q。
3. 右移 7 位缩放（适配 12-bit bladeRF）。
4. 更新码相位、载波相位、数据位计数器、NH 码位。

### 卫星位置计算

siren.h 中定义了 satpos() 实现经典开普勒方程迭代求解，支持 GPS、Galileo、北斗。北斗 GEO 卫星有特殊的坐标旋转处理（绕 X 轴旋转 -5°）。satvel() 采用数值微分（1ms 步长）求速度。

## 辅助工具详解

本章详细介绍 tools/ 目录下的 Python 辅助脚本，帮助用户完成轨迹生成、信号合并、地图绘制、天空图绘制、二进制对比以及通过 USRP 发射信号等任务。

### genpath.py

根据给定的航点坐标（时间、纬度、经度、高度）插值生成一条连续的动态轨迹文件，供 Siren 使用。输出为 CSV 格式，采样率为 10 Hz。使用方法为：

```sh
python genpath.py <csvfile> -out <pathfile> [-len <duration>]
```

其中的参数为：

- `<csvfile>`：CSV 文件，每行包含“时间(秒),纬度(度),经度(度),高度(米)”，至少需要两个航点。
- `-out`：输出轨迹 CSV 文件路径。
- `-len`：轨迹总时长（秒），默认 300 秒。

示例，假设 waypoints.csv 内容如下：

```
0,39.9,116.4,50
600,39.95,116.5,100
```

使用如下命令生成轨迹文件：

```sh
python genpath.py waypoints.csv -out trajectory.csv -len 600
```

该命令将生成 600 秒的轨迹，起点为 (39.9,116.4,50)，终点为 (39.95,116.5,100)，中间采用线性插值填充。

### plotpath.py

将轨迹文件（CSV、NMEA GGA 或 GeoJSON）绘制在地图上，支持多种底图样式（卫星、街道、地形等）。输出为 JPG 图片。使用方法为：

```sh
python plotpath.py <path> [-out <jpgpath>] [-style <style>]
```

其中的参数为：

- `<path>`：支持 `.csv`、`.gga`、`.geojson`格式。
- `-out`：输出图片文件名，默认为轨迹文件名（扩展名改为 .jpg）。
- `-style`：底图样式，可选 `satellite`、`street`、`terrain`、`only_streets`，默认为 `satellite`。

运行该脚本需要安装 `cartopy`、`matplotlib` 和 `pynmea2`（用于 GGA 格式解析）：

```sh
pip install cartopy matplotlib pynmea2
```

使用示例：

```sh
python plotpath.py trajectory.csv -out track.jpg -style street
```

### plotskymap.py

将 Siren 输出的卫星信息文件（文本格式，包含卫星编号、仰角、方位角）绘制成极坐标天空图，直观显示卫星分布。输出为 JPG 图片。使用方法为：

```sh
python plotskymap.py <csvfile> [-out <jpgpath>]
```

其中的参数为：

- `<csvfile>`：Siren 运行时输出的卫星列表。
- `-out`：输出图片文件名，默认为输入文件名加上 `_skymap.jpg`。

使用示例，假设 siren 输出的卫星信息保存为 satlist.txt，格式可参考：

```
Sat Elev Azimu
G01 45.2 120.3
C06 62.1 80.5
```

运行以下指令进行绘制：

```sh
python plotskymap.py satlist.txt -out sky.jpg
```

### samebin.py

逐字节比较两个二进制文件是否完全相同，常用于验证 Siren 生成的 IQ 文件与预期结果的一致性。使用方法为：

```sh
python samebin.py <文件1> <文件2>
```

使用示例：

```sh
python samebin.py output.iq reference.iq
```

程序将输出 `NO` 或 `OK`。

### combine\_l1b1i.py

将 Siren 独立生成的 GPS L1C/A 和北斗 B1I 两个 IQ 文件合并为一个复合信号文件，适用于双频段同时发射（如使用 HackRF）。合并时对两个信号分别上变频至中心频率两侧，避免频谱重叠。使用方法为：

```sh
python combine_l1b1i.py --gps <file> --bds <file> [-o <file>] [-s <SAMPLE>]
```

其中的参数为：

- `--gps <file>`：GPS L1C/A 信号的文件路径。
- `--bds <file>`：北斗 B1I 信号的文件路径。
- `-s <SAMPLE>`：信号文件采样率，默认为 8MHz。
- `-o <file>`：输出文件路径，默认为 combined\_signal.bin。

运行后输出合并后的文件，并提示 HackRF 发射命令。

注意事项：

- 输入文件必须是 8-bit 交错 IQ 格式（int8）。
- 两个信号的采样率必须相同。
- 输出文件为 8-bit 交错 IQ 格式，中心频率为 GPS 和北斗频率的平均值（约 1568.259 MHz）。

## 模块依赖关系

```SH
siren.c (main)
├── options.c → toml.c/toml.h
├── io.c
│   ├── readnav() → 解析 RINEX
│   ├── readtraj_csv/gga()
│   └── writeiq()
├── ephemeris.c
│   ├── satpos(), satvel()
│   └── eph2sbf()
├── signal.c
│   ├── calcrange() → errors.c (ionodelay, tropdelay)
│   ├── calccaphase()
│   └── gennavmsg() → checksum(), bchencode(), interleave()
├── channels.c
│   ├── initchan(), allocchan(), updatechan(), flushchan()
│   └── gensignal()
├── code.c → gencode() 系列
└── utils.c → 数学、时间、坐标转换
```

## 编译与调试技巧

## 编译选项

在 siren.h 顶部可通过宏控制功能：

- `-DFLOAT_CARR_PHASE`：启用浮点载波相位（更高精度，稍慢）。
- `-DENABDS`：启用北斗支持（默认开启）。
- `-DTRACE`：启用跟踪日志。
- `-DDYNAMIC_MAX_DURATION=7200`：修改动态模式最大时长（默认 3600 秒）。

### 调试

- 使用 `-T 3` 开启跟踪日志（输出到 siren.trace），级别越高越详细。
- 使用 `-v` 显示通道详细信息（卫星PRN、方位角、伪距等）。
- 跟踪日志函数 `trace(level, fmt, ...)` 在 io.c 中实现，可通过 `traceopen()` 重定向。

### 常见编译错误

- 未找到 toml.h：确保 toml.c 和 toml.h 在源码目录。
- 链接错误：检查是否链接了数学库 -lm（makefile 中已包含）。
- Windows 下换行符问题：使用 Git 检出时设置 core.autocrlf=false。

## 常见问题

1. 编译失败：检查 GCC 和 Make 安装，Windows 使用 MinGW-w64。
2. 找不到卫星：确认 RINEX 文件包含当前时间的星历，调整仰角掩膜。
3. IQ 文件无法识别：检查采样率、位宽、信号类型是否匹配，确认数据格式为 I/Q 交错。
4. 动态轨迹不连续：确保轨迹文件采样率为 10Hz，且点数足够。
5. 内存不足：减少仿真时长或采样率。

## 附录

### 缩略词

| 缩写 | 全称 |
| ---- | ---- |
| BCH | Bose–Chaudhuri–Hocquenghem code |
| BPSK | Binary Phase Shift Keying |
| CDMA | Code Division Multiple Access |
| GEO | Geostationary Earth Orbit |
| GNSS | Global Navigation Satellite System |
| GPS | Global Positioning System |
| ICD | Interface Control Document |
| IGSO | Inclined Geosynchronous Satellite Orbit |
| MEO | Medium Earth Orbit |
| NH | Neuman-Hoffman |
| PRN | Pseudo-Random Noise |
| RINEX | Receiver Independent Exchange Format |
| SDR | Software Defined Radio |
