# Timer Application Note

## 特性
- 支持4组独立的计数器
- 每组计数器可以独立生成中断
- 支持timer link mode

## 功能描述
Timer支持的功能详细描述如下：
1. Timer模块包含4个独立的计数器单元，每个计数器是32bit；
2. Timer的计数模式只有递减方式，定时器使能时自动加载TIMER_ LDVAL 寄存器指定起始值，倒计数至 0 时 将生成一个触发脉冲并设
置超时标志，然后再次装载起始值重新从LDVAL开始计数。
3. 定时器中断可通过置位TIE 来使能。当相关定时器发生超时，TIF 超时标志置位为 1 ，写入 1 清零。在
使用链接功能时，一般只使能定时器 n 中断，与 n 链接 的 定时器 中断 都是 关闭 状态 。

## 链接模式
若32bit的timer计时宽度不足则可以使用链接模式，链接模式描述如下：
1. 链接模式打开后，当前的timer计数时不再自动减一，而是只有当其链接的timer触发脉冲后才会减1；
2. timer只能向下一级链接，例如：timer3链接模式打开后，链接的是timer2；若timer2链接模式打开，则链接的是timer1；因此timer0无法打开链接模式
举例：
1）timer1配置LDVAL=1000，同时关闭链接模式；
2）timer2配置LDVAL=2000，同时使能链接模式；
此时，使能timer1和timer2后，timer1开始从1000规律性的自动减1，timer2等待timer1事件；当timer1从1000减到0后触发一个脉冲信号，此时timer2开始从2000减1，
之后继续等待下一次脉冲；timer1触发脉冲后重新从LDVAL开始计数；

## 当前值
每个timer都有一个当前tick数的寄存器：TIMERx_CVAL；TIMERx_CVAL的计数方式是递增模式，也就是当timer计数减一时，TIMERx_CVAL的值加1，TIMERx_CVAL默认从0开始计数；

## 全局开关
有一个全局开关寄存器TIMER_MCR，bit[1]为1时表示所有timer都可用，bit[1]为0时表示所有timer都不可用；

## 寄存器地址
TIMER基地址 = 0x40011000
TIMER_CH0基地址 0x40011100
TIMER_CH1基地址 0x40011110
TIMER_CH2基地址 0x40011120
TIMER_CH3基地址 0x40011130

## 中断号
中断号18，4个timer共用一根中断信号线；