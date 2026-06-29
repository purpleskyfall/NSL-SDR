# 电离层延迟仿真

电离层延迟仿真主要采用模型实现，例如经典的经验模型有 Kolbuchar 模型，也可以通过实测的 VTEC 数据计算。在卫星仰角较高时，电离层延迟约为数米，但在卫星仰角较低时可达十几米。

## Kolbuchar 模型

该模式是 GPS 广播星历采用的电离层改正模型。由该模型的参数计算 L1 频段电离层延迟的公式为：
$$
\begin{equation}
I_z = \left\{
\begin{array}{lr}
5 \times 10^{-9} + A cos(2\pi \frac{t-50400}{T}), & |t-50400| < \frac T4 \\
5 \times 10^{-9}, & |t-50400| \ge \frac T4
\end{array}
\right.
\end{equation}
$$
该式中，$A$ 是余弦函数的振幅，$T$ 是值必大于 20 小时的余弦函数周期。由此可见，该模型将午夜至凌晨的电离层延迟看作是一个常数；对于白天的电离层延迟看作是以当地时间 14 点为峰值的半个余弦函数周期（谢钢，2017）。

$A$ 用下式计算：
$$
\begin{equation}
A = \left\{
\begin{array}{lr}
\sum_{n=0}^{3} \alpha_n \varphi_m^n, & A > 0 \\
0, & A < 0
\end{array}
\right.
\end{equation}
$$
$T$ 用下式计算：
$$
\begin{equation}
T = \left\{
\begin{array}{lr}
\sum_{n=0}^3 \beta_n \varphi_m^n, & T \ge 72000 \\
72000, & T < 72000
\end{array}
\right.
\end{equation}
$$
上式（1）-（3）中，$\alpha_0$、$\alpha_1$、$\alpha_2$、$\alpha_3$、$\beta_0$、$\beta_1$、$\beta_2$、$\beta_3$ 都是由 GPS 广播星历播发的参数，该模型称之为八参数的 Kolbuchar 模型。$\varphi_m$ 为电离层穿刺点的地磁纬度，可由下式计算：
$$
\begin{equation}
\psi = \frac{0.0137}{El + 0.11} - 0.022
\end{equation}
$$

$$
\begin{equation}
\varphi_i = \varphi + \psi cosAz
\end{equation}
$$

$$
\begin{equation}
\lambda_i = \lambda + \psi \frac{sinAz}{cos \varphi_i}
\end{equation}
$$

$$
\begin{equation}
\varphi_m = \varphi_i + 0.064 cos(\lambda_i - 1.617)
\end{equation}
$$

式（4）-（6）中，$Az$、$El$ 分别为卫星的方位角和高度角，$\psi$ 为用户与穿刺点在地心的夹角，$\lambda_i$、$\varphi_i$ 为穿刺点的经纬度。

实际上，式（1）计算的是穿刺点处的天顶电离层延迟，还需要将 $I_z$ 转换到信号的传播方向上：
$$
\begin{equation}
I = F I_z
\end{equation}
$$
式中 $F$ 为映射函数，其计算方法为：
$$
\begin{equation}
F = 1 + 16 \times (0.53 - \frac{El}{\pi})^3
\end{equation}
$$
以上计算的电离层延迟 $I$ 的单位为秒，若要转换为距离，还要乘上光速 $c$。
