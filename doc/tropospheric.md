# 对流层延迟仿真

对流层延迟仿真采用 Saastamoinen 模型计算。

## Saastamoinen 模型

为了应用对流层延迟的模型改正，首先需要引入标准大气模型。应用该模型可计算标准大气的气压、气温和水汽压：
$$
\begin{equation}
p = 1013.25 \times (1 - 2.2557 \times 10^{-5}h)^{5.2568}
\end{equation}
$$

$$
\begin{equation}
T = 15.0 - 6.5 \times 10^{-3}h + 273.15
\end{equation}
$$

$$
\begin{equation}
e = 6.108 \times exp(\frac{17.15T - 4684.0}{T - 38.45} \times \frac{h_{rel}}{100})
\end{equation}
$$

这里 $p$ 为全部的大气压力（单位为 hPa），$T$ 为大气的绝对温度（单位为 K），$h$ 是海拔高度，$e$ 为水汽压力部分（单位为 hPa），$h_{rel}$ 为相对湿度。Saastamoinen 模型将对流层延迟表示为标准大气模型中大气压力 $p$、温度 $T$ 和水汽压 $e$ 的函数：
$$
\begin{equation}
T_r^s = \frac{0.002277}{cosz}(p + (\frac{1255}{T} + 0.05)e - tan^2 z)
\end{equation}
$$
这里 $z$ 被称为天顶距（单位为弧度），$z = \pi / 2 - El_r^s$。在不知道测站海拔高度时，可使用大地高代替海拔高。相对湿度 可假定为 70%。