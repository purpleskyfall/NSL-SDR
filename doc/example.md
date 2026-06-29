# 示例说明

## 仿真信号文件生成

### bds_b1i_231107_55s_8mhz_8bit.iq

北斗 B1I 信号仿真文件，55s时长，8MHz采样率，8bit位宽。仿真坐标为：35.9993334°E, 120.1199311°N, 30.0m。

仿真卫星号列表：6,8,9,13,16,20,23,25,27,29,30,32,38,39,41。

生成该文件的方法，使用以下内容创建一个 `config.toml` 文件：

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

然后运行命令：

```sh
siren example/brdc3110.23p -c config.toml -l 35.9993334,120.1199311,30.0 -t 2023/11/7,0:0:0 -d 55 -o bds_b1i_231107_55s_8m_8bit.iq
```

实践中，为了加快软件接收机解算速度，可以将采样率设置得更小一点，例如 5MHz。

### bds_b3i_231107_55s_50mhz_8bit.iq

北斗 B3I 信号仿真文件，55s时长，50MHz采样率，8bit位宽。仿真坐标为：35.9993334°E, 120.1199311°N, 30.0m。

仿真卫星号列表：6,8,9,13,16,20,23,25,27,29,30,32,38,39,41。

生成该文件的方法,使用以下内容创建一个 `config.toml` 文件：

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

然后运行命令：

```sh
siren example/brdc3110.23p -c config.toml -l 35.9993334,120.1199311,30.0 -t 2023/11/7,0:0:0 -d 55 -o bds_b3i_231107_55s_50mhz_8bit.iq
```

为了加快软件接收机解算速度，也可以将采样率设置得更小一点，例如 24MHz。

## 软件接收机测试方法

1. 下载 [CU-SDR-Collection](https://github.com/gnsscusdr/CU-SDR-Collection)。
2. 进入对应的接收机文件夹（BDS/B1I 或 BDS/B3I）。
3. 修改 `initSettings.m` 文件中的选项（如下表）。
4. 运行 `init` 启动接收机，当提示是否继续时键入 `1`。

`initSettings.m` 文件设置参数：

| 参数                      | 选项           |
| ------------------------- | -------------- |
| settings.fileName         | 实际的文件路径 |
| settings.msToProcess      | 54000          |
| settings.numberOfChannels | 15             |
| settings.IF               | 0              |
| settings.samplingFreq     | 实际的采样率   |

## 射频测试方法

妥善连接 HackRF 至计算机，并配置必要的驱动程序后，使用命令启动 HackRF 来发射信号：

```sh
hackrf_transfer -t <iqfile> -f <center_freq> -s <fs> -x 40'
```

这里，`<iqfile>` 为仿真信号文件路径，`center_freq` 为射频信号频率，对于北斗 B1I 和 B3I，分别为 1561.098e6 和 1268.52e6，`fs` 为信号采样频率。

如果使用手机测试，测试时最好进入飞行模式。
