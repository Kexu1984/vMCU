# Flash Application Note
本篇文档用于说明系统环境中的Flash设计，可供后续编码参考

## 特性
- PAGE大小可调，可配置为2048B或者4096B
- 主存储区支持下面操作：
  读
  页编程 （编程 最小 单元 64 bit）
  整片擦除
  页擦除
  页擦除验证
  整片擦除验证
- 支持读写保护
  写保护避免存储数据的意外编程或擦除
  读保护避免非法获取数据
- 支持RWW
  读 P Flash 时候可以写 D Flash
  读 D Flash 时候可以写 P Flash
- 支持ECC
  2 bit 错误检测
  1 bit 错误 纠正
- 支持AEC-Q100 grade 1
- 支持unlock和lock状态
- 是否信息区配置


## 系统组成
系统的flash内存组织为：1个flash controller + 2片主存储区 flash memory + 1片Info区 flash memory；
其中flash controller是一组全局控制寄存器，用于控制后面的flash memory；2片flash memory作为主存储器区用于存储用户数据（分别称之为：PFlash和DFlash）；

### flash controller
一组全局寄存器，可用于管理系统中的所有flash memory的行为（包括pflash，dflash以及Info区flash），详细的寄存器信息可以参考：flash_registers.yaml

### flash memory
flash memory是Non-volatile memory，掉电数据不丢失；每个flash memory的大小可配置，最小擦除单元都是4KB；
Pflash：256K内存空间，最小擦除单元（页）为4K，最小编程单元是128bit；
Dflash：128K内存空间，最小擦除单元（页）为2K，最小编程单元是128bit；
Info区：12K内存空间，最小擦除单元（页）为4K，最小编程单元是64bit

Flash memory的初始值是全0xF，当program时，可以从bit 1改成bit 0；但是无法从bit 0改为bit 1；
如果需要从bit 0改为bit 1只能通过擦除命令实现，擦除命令包括：单片擦除和整片擦除；
如果某些页配置成不可擦除（出厂阶段配置），那么就可以将这个Flash当作OTP来使用；

### address space
flash设备存在多个地址空间:
flash controller寄存器的地址空间：0x40002000 ~ 0x40003000
Pflash memory的地址空间：0x08000000 ~ 0x08040000
Dflash memory的地址空间：0x08100000 ~ 0x08120000
Info flash memory的地址空间：0x08200000 ~ 0x08202000

## 功能描述

### 读保护
为了避免非法访问flash memory中的数据, flash模块实现了读保护的控制功能：
1. 主存储区flash的读保护开关是由几个全局标记实现的，而这些全局标记会保存在标明为Info区的flash设备中；
2. Info区flash memory无读保护

### 写保护
当前设计了两种方式去配置Flash的写保护：寄存器控制方式（实时）和Info区控制方式（非实时）

#### 寄存器配置
寄存器控制规则如下：
1. 由2个controller中的32bit寄存器控制每个Flash的写保护开关；因此每个bit控制的范围也就是：flash_size / 64，当对应bit为0时，写保护关闭，当对应bit为1时，写保护打开；
pflash是256KB，则每个bit控制256KB / 64 = 4KB的空间；之后如果写保护寄存器如果是Reg0 = 0x3 & Reg1 = 0x0，则表明：0~8K的flash区域打开了写保护，剩余空间写保护关闭；
dflash是128KB，则每个bit控制256KB / 64 = 2KB的空间；之后如果写保护寄存器如果是Reg0 = 0x3 & Reg1 = 0x0，则表明：0~4K的flash区域打开了写保护，剩余空间写保护关闭；
通过寄存器配置的方式可以使得开关立即生效；
#### flash配置规则
写保护也可以像读保护一样，将开关配置在Info区Flash中；配置规则如下：
1. 从Info区Memory的第二个Page开始保存Pflash和Dflash的写保护信息；
2. Info区Memory的第二个Page的绝对地址是0x0820_1000，offset是0x1000；
3. 0x0820_1000 ~ 0x0820_1010保存的是Pflash的写保护控制信息；0x0820_1010 ~ 0x0820_1020保存的是Dflash的写保护控制信息
4. 由步骤3可知，pflash和dflash均有16个字节，也就是128bit的控制信息
5. 128bit的控制信息中只有64个bit是有效的，分别是：bit[31:0]和bit[127:96]
6. 当用户需要配置打开Flash的第N（N < 64）个页写保护时，需要做出如下配置：
if N < 32:
   bit[N] = 1; 
   bit[N + 32] = 0;
else:
   bit[N + 64] = 1;
   bit[N + 96] = 0;
   
### Info区写保护
Info区memory也有写保护，但是方式不同，只需要通过CCR寄存器打开关闭即可；

#### 写保护触发异常
如果设置对应区域为写保护(写保护位为1)， 擦除和编程操作该Flash 区域会失败且置位EFLASH_CSR [PWVIO]或EFLASH_CSR [DWVIO]；但不影响正常读取Flash 数据。

### Info区内容说明
1. Info区的大小为12K
2. Info区的program单元为64B
3. Info区的第一个Page（4096B）中的第一个program单元用于控制其他主存储区Flash的读保护功能（注意：一个开关控制所有主存储区），也就是Info区内存地址的0x0~0x8
格式如下：
bit[63:48] = 0xFFFF
bit[47:32] = 0xFFFF
bit[31:24] = 0xFF
bit[23:16] = nRDP
bit[15: 8] = 0xFF
bit[7 : 0] = RDP
#### 读保护设置伪代码说明如下
if (RDP = 0xAC && nRDP = 0x53)
    读保护 = 关闭
else
    读保护 = 打开

#### 默认值
第一个Page的第一个program默认值是：0xFFFFFFFFFF53FFAC，也就是说默认读保护是打开的（注意：这里与主存储区的默认全F不一样）
4. Info区的第二个Page（4096B）中的内容用于控制其他主存储区Flash的写保护功能，也就是Info区内存地址的0x1000开始的配置
格式如下：
1）为每个主存储区分为16字节的区域用于存储写保护控制信息，如上所述，写保护信息是1个bit控制一块区域（区域的大小取决于主存储区的总大小）；
因此，这里16个字节相当于控制了：16 * 8 = 256个区域
2）所以，0x1000 ~ 0x1010用于控制主存储器-0的写保护；0x1010 ~ 0x1020用于控制主存储器-1的写保护，后续依次类推
3）地址0x1000 + 0x10 * N的位置用于存储外部复位控制，其中N表示当前系统中存在多少个主存储区flash


### Flash控制命令

页擦除：指令码 0x1，功能： 按页擦除主存储区Flash
选项字节擦除：指令码 0x2，功能： 按页擦除选项字节区域
整片擦除：指令码 0x3，功能： 整个擦除主存储区Flash

页编程：指令码 0x11，功能： 按照32-bit 对齐方式编程主存储区
选项字节编程：指令码 0x12，功能： 按照32-bit 对齐方式编程选项字节区域

页擦除验证：指令码 0x21，功能： 按照32-bit 对齐方式验证主存储区Flash是否被擦除，长度由EFLASH_CCR_命令控制寄存器(EFLASH_CCR)[LEN]指定
整片擦除验证：指令码 0x22，功能： 验证整个主存储区Flash是否被擦除;

### ECC功能
1. 主存储区Flash 默认使能ECC功能，可以通过EFLASH_GCR[xECCEN]来禁用。
2. 软件可以查询EFLASH_CSR[xECCSTA] 的值来判断当前Flash ECC 错误的状态。
 如果xECCSTA =2’b00，表明Flash ECC 没有检测到错误。
 如果xECCSTA =2’b01，表明Flash ECC 检测到2-bit 错误，此时软件通过读取寄存器EFLASH_ADRx[ADRx]获取当前发生2-bit 错误的地址，且会将异常发送到SMU。
 如果xECCSTA =2’b10 或者2b’11，表明Flash ECC 检测到1-bit 错误。
3. 主存储区Flash ECC 的EFLASH_CSR[xECCSTA] 和EFLASH_ADRx[ADRx] (x=flash编号，如果系统中存在多个flash的话)在系统不复位情况下会保留最近一次Flash ECC 检查
到的错误状态以及错误地址，软件通过向EFLASH_CSR[xECCSTA] 写入2’b11 清除状态，通过向错误地址寄存器EFLASH_ADRx[ADRx] 写1清除地址。

### lock & Unlock
主存储区flash存在lock和unlock状态，如果是lock状态则无法操作；
1. 从lock状态下去解锁flash设备使其进入unlock状态的操作：设置UKR寄存器分两次填入密钥：0x00AC7805以及0x01234567后即可解锁；
2. 从unlock状态下去解定flash设备使其进入lock状态的操作：配置GSR寄存器的bit[0] = 1

### Flash操作

#### 页擦除
页擦除操作能把指定地址所在页的数据全部擦除，可作用于除选项字节区外的主存储区flash 区域。
页擦除流程：
1. 在配置EFLASH_GCR 和EFLASH_CCR 之前，必须通过读取EFLAHS_GSR[LOCK]的状态
来检查控制器是否被锁定。如果控制器处于锁定状态，必须顺序写入0xac7805 和0x01234567至EFLASH_UKR解锁序列寄存器然后才能解锁。如果控制器不处于锁
定状态，可以直接进入下一步；
2. 解锁后，先通过读出 EFLASH_CSR[CMDBUSY] 的状态来检查是否有正在进行的命令操作。CMDBUSY 等于 1 表示某些命令操作未完成,必须等到 CMDBUSY 等于 0。
事实上，解锁过程可以在检查 CMDBUSY 之前或之后完成；
3. 在 EFLASH_CAR 中配置页擦除起始地址，该值是所需擦除页的 起始 地址； 通过地址值区分擦
除的是哪一块主存储器Flash，且地址值必须 8 字节 对齐。
4. 通过控制 EFLASH_CCR 来启动命令并触发它。应保证在 EFLASH_CCR 中配置其他位后，<Start> 位必须从 0 变为 1 才能获得有效触发。 EFLASH_CCR 命令控制寄存器 CMD 配置为 0x1 以确定传入命令操作
是页擦除。 其他位应为 0 ，并将用于其他命令操作
5. 在有效触发 之后，命令操作开始。应通过读出 CMDBUSY 来检查命令操作是否完成。当
CMDBUSY 等于 0 时，表示命令操作已完成
6. 清除 EFLASH_CCR
7. 读出状态以检查是否存在某些错误， 例如 EFLASH_C SR PGMERR ]]，特别是对于写保护情况用户无法擦除已经被写保护的页。
其他6 种 命令的流程与页擦除命令类似， 相同 部分后面不再赘述。

#### 整片擦除
整片擦除可以擦除整个Flash 主存储区。 与页擦除命令流程相比，整片擦除流程
有两个不同：一是 EFLASH_CCR 命令控制寄存器 《CMD》位设置值不同；二是EFLASH_CAR 设置为主存储器Flash的任意地址，而不一定是首地址 。
特别地，当主存储器属性从读保护改变为非读保护时，将自动执行整片擦除以保护用户代码不被非法读取。例如，当用户将选项字节中的 RDP 从默认的 0xFF 编程为 0xAC 时，整片擦除会自动执行。

#### 页编程
页编程命令可作用于除选项字节区外的主存储器flash区域。页编程命令流程也类似于页擦除，但
EFLASH_CCR CMD 值不同、且需要配置编程长度和编程数据，具体步骤如下：
1. 配置编程 地址值到 EFLASH_CAR 寄存器
2. 配置 EFLASH_CCR 命令控制寄存器 (LEN) 来配置页编程命令写入的编程长度
3. 配置 EFLASH_CCR CMD 为 0x11
4. 配置 EFLASH_CCR START 为 1 即启动编程命令；
5. 软件 判断 EFLASH_C SR DBUFRDY 等于 1 时候 ，将 要 编程的数据写入 EFLASH_CDR 寄存器直到写完LEN[11:0]指定的长度。
注意：LEN[11:0]位配置为指定值，单位是一个编程长度：64 bit 。例如，如果LEN[11:0]配置为150，则用户应写入不超过 150 个编程单位。
如果用户写入超过150个编程单位，例如 180 个编程单位，则最后 30 个 编程单位实际上不会写入flash。同时，如果用户写入少于 150 个 编程
单位，如 110个编程单位，则写操作将会验证正常，但编程过程还不会结束，直到数据数目等于长度或使用 abort命令强制编程终止。

#### 页擦除验证
页擦除验证命令可作用于除选项字节区外的主存储器flash区域。该操作通常在擦除操作之后执行，以验证擦除操作是否成功执行。
与页擦除命令流程相比，页擦除验证流程有两个不同一是 EFLASH_CCR (CMD)设置值不同 二是需要在EFLASH_CCR(LEN)配置验证的长度 。

#### 整片擦除验证
整片擦除验证命令可用于 Flash 主存储区。该操作通常在整片擦除操作之后执行，以便验证整片擦除操作是否成功完成。
与页擦除命令流程相比，整片擦除验证流程有两个不同：一是EFLASH_CCR (CMD)设置值不同，二是 EFLASH_CAR设置为Flash内存地址范围内的任意地址，而不需要是首地址 。

#### 选项字节擦除
选项字节擦除命令用于选项字节区。与页擦除命令流程相比，选项字节擦除流程有两点区别：一是
EFLASH_CCR (CMD)设置值不同，二是在 EFLASH_CAR 寄存器中指定地址为选项字节地址。

#### 选项字节编程
选项字节编程命令用于选项字节区。与页编程命令流程相比，选项字节编程流程有两点区别：一是
EFLASH_CCR (CMD)设置值不同，二是在 EFLASH_CAR 寄存器中指定地址为选项字节地址。
在进行选项字节编程时，用户应高度重视读取保护字符的更改，如果将读取保护字符从保护更改为不保
护， eflash 控制器将自动执行整片擦除命令，此时将会擦除主存储区的内容。

## 设计要求
1. 通常来说，模拟内存设备只需要用软件中的数组即可实现基本的读写功能，但是对于flash而言存在非易失性这个特点，因此这里我建议内存值的初始状态
用一个真实文件去处理；用户可以选择用全F的初始文件，也可以选用多次program之后的文件作为初始文件。关于这个设计请参考进去；
2. 分别实现两个类：flash memory和flash controller两个类；其中flash memory类用于管理文件的读写操作，flash controller类用于管理寄存器操作；
flashdevice需要包含这两个类，同时把3个flash memory和1个flash controller的地址空间都注册进去；
3. flashdevice的读操作需要区分offset：当offset指示寄存器的范围时就要走到register manager的读写操作流程，当offset指示的是flash memory范围，
那么就要区分具体是哪一个flash memory，然后读取目标文件；
4. flashdevice的写操作只能针对寄存器，写flash memory的操作需要返回异常；
