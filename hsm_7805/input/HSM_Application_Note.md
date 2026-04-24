# HSM Application Note

## 特性
- 支持AES-128加解密，只有ECB和CBC模式
- 支持AES-128 CMAC的mac生成和MAC验证流程
- 支持随机数生成器
- 支持密钥管理
- 支持lock和unlock的状态
- 支持安全启动
- 支持通过DMA处理数据搬运流程

## 功能描述
HSM支持的功能详细描述如下：
### 密钥管理
HSM支持两种类型的密钥存储单元：寄存器存储密钥 & 内存存储密钥;
HSM支持如下几种类型的密钥：
  - HSM_CHIP_ROOT_KEY：0
  - HSM_BL_VERIFY_KEY：1
  - HSM_FW_UPGRADE_KEY：2
  - HSM_DEBUG_AUTH_KEY：3
  - HSM_FW_INSTALL_KEY：4
  - HSM_KEY_READ_KEY：5
  - HSM_KEY_INSTALL_KEY：6
  - HSM_CHIP_STATE_KEY：7
  - HSM_RAM_KEY：18

用户通过输入Key index去向HSM指定需要的密钥，例如：用户在请求AES加密服务时配置key index为18，则HSM就会用HSM_RAM_KEY去进行加密；
HSM_RAM_KEY: 保存在4个RAM Key寄存器中，寄存器信息可以参考hsm_register_map.yaml：RAM_KEY0~RAM_KEY3，每个寄存器32bit加起来刚好128bit
上述其他8个密钥均是存储在内存中，存储在固定的位置，每个密钥占据16字节的空间，分别从：0x8300010~0x8300090；HSM会根据用户输入的密钥编号来从内存中读取密钥实体；

## 中断
- HSM的中断号是20，当HSM处理完command请求或者出现异常时，如果用户配置了HSM的中断使能，就会触发中断；
HSM模块支持一个中断，用户使用HSM中断的用法如下：
1. 设置寄存器 SYS_CTRL 的 IRQEN 位为 1 
2. 执行 HSM 相关命令
3. 检查中断产生状态

### Challenge-response
HSM支持鉴权功能，鉴权功能也就是HSM的CMAC算法的验证功能，当用户需要用到鉴权功能时，指定的密钥ID只能为 3 ~ 7，并且完整的鉴权流程操作如下：
1. 确认HSM的CST寄存器的Unlock位是否1，为1表示当前HSM为Busy状态，不可继续；为0表示当前HSM为IDLE状态，可以继续后续步骤
2. 设置 寄存器 CMD 的 CIPMODE 位为鉴权模式（0x3），KEYID 位指定密钥ID，TXLEN位配置为16字节
3. 设置 寄存器 CST 的 START位 值为 1 启动鉴权流程
4. 等待 AUTH_STAT寄存器的 ACV位是否置位为 1
5. 从 CHA寄存器读取 16字节的 challenge值
6. 使用对应密钥对 challenge值计算 CMAC
7. 把 CMAC值写入到 RSP寄存器
8. 设置 AUTH_STAT寄存器的 ARV位为 1
9. 通过 AUTH_STAT寄存器检查鉴权结果

### True Random Number Generator (TRNG)
HSM支持获取TRNG功能，当用户需要获取TRNG功能时，需要按照以下操作步骤：
1. 确认HSM的CST寄存器的Unlock位是否1，为1表示当前HSM为Busy状态，不可继续；为0表示当前HSM为IDLE状态，可以继续后续步骤
2. 设置 寄存器 CMD 的 CIPMODE 位为RND模式（0x4），TXLEN位配置为需要获取的TRNG长度（4对齐），单位为字节；
3. 设置 寄存器 CST 的 START位 值为 1 启动TRNG获取流程
4. 等待SYS_CTRL寄存器的TE位为1，为1表明输出buffer不为空
5. 读取CIPOUT寄存器作为4字节结果，len += 4;
6. 判断len是否已经等于步骤2配置的长度，如果长度不足继续步骤4，5，6；

### UUID
HSM支持保存UUID到内部寄存器中，UUID保存在内存中，地址：0x8300000~0x8300010；
UUID一共16字节，分别保存在4个UUID寄存器中，寄存器信息可以参考hsm_register_map.yaml：UID0~UID3，每个寄存器32bit加起来刚好128bit

### DMA
HSM可以通过 DMA 进行数据的传输，使用 DMA 做传输数据的流程如下
1. 初始化 DMA 通道 1从 HSM搬运数据到 SRAM
2. 初始化 DMA通道 0从 SRAM搬运数据到 HSM
3. 初始化 DMA搬运数据位宽为 32-bit
4. 使能 HSM寄存器 SYS_CTRL中的 DMAEN位
5. 配置 HSM命令相关的参数，触发命令的执行
6. 等待寄存器 SYS_CTRL中的 IDLE位
7. 检查寄存器 STAT错误状态
8. 检查 DMA搬运是否结束
9. 失能 HSM寄存器 SYS_CTRL中的 DMAEN位
10. 读取 DMA搬运到 SRAM的数据

## AES ECB/CBC
HSM支持AES ECB/CBC加解密，当用户需要用到AES加解密功能时，指定的密钥ID只能为 18，并且完整的加解密流程操作如下：
1. 确认HSM的CST寄存器的Unlock位是否1，为1表示当前HSM为Busy状态，不可继续；为0表示当前HSM为IDLE状态，可以继续后续步骤
2. 设置 寄存器 CMD 的 CIPMODE 位为ECB模式（0x0）/CBC模式（0x1），decrypt_en位用于指定加解密方向1为解密，0为加密，KEYID 位指定密钥ID，TXLEN位配置为需要加解密的输入文本长度（16字节对齐）；
3. 设置 寄存器 CST 的 START位 值为 1 启动鉴权流程
HSM处理AES的ECB/CBC模式时每次处理16字节，处理流程如下
4. 等待 SYS_CTRL寄存器的RF位为0，为1表明输入buffer不为空无法继续写入数据；
5. 当RF为0后，一次写入输入数据中的16字节分为4次，依次写入CIPIN寄存器中；
6. 输入数据长度len += 16，判断len是否已经等于步骤2配置的长度，如果长度不足继续步骤4，5，6
7. 当所有输入数据输入完成后，等待SYS_CTRL寄存器的TE位为1，为1表明输出buffer不为空
8. 当TE为1则读取16字节的结果，读取方式为：连续读取4次CIPOUT寄存器，拼接作为16字节结果；


## AES CMAC
HSM支持AES CMAC加密，当用户需要用到AES CMAC加密功能时，指定的密钥ID只能为 18，并且完整的加密流程操作如下：
1. 确认HSM的CST寄存器的Unlock位是否1，为1表示当前HSM为Busy状态，不可继续；为0表示当前HSM为IDLE状态，可以继续后续步骤
2. 设置 寄存器 CMD 的 CIPMODE 位为CMAC模式（0x2），decrypt_en位用于指定加密方向为0，KEYID 位指定密钥ID，TXLEN位配置为需要加解密的输入文本长度（无限制）；
3. 设置 寄存器 CST 的 START位 值为 1 启动CMAC加密流程；
HSM处理AES的CMAC模式时每次处理16字节（除了最后一组非16字节对齐的数据），处理流程如下：
4. 等待 SYS_CTRL寄存器的RF位为0，为1表明输入buffer不为空无法继续写入数据；
5. 当RF为0后，一次写入输入数据中的16字节分为4次，依次写入CIPIN寄存器中；如果剩余数据长度不足16字节，就把最后不满4字节的数据按照地位优先的顺序填入CIPIN寄存器中，例如：
最后的输入数据为：1234567811223344556677 共4+4+3=11字节，则写入流程：CIPIN=0x78563412; CIPIN=0x44332211;CIPIN=0x00776655;
上例给出最后3个字节不满4字节，但是为了能写入寄存器我们对剩余的高位补0，然后写入到CIPIN寄存器中；
6. 判断输入数据的长度是否已经等于步骤2配置的长度，如果长度不足继续步骤4，5，6
7. 当所有输入数据输入完成后，等待SYS_CTRL寄存器的TE位为1，为1表明输出buffer不为空
8. 当TE为1则读取16字节的结果，读取方式为：连续读取4次CIPOUT寄存器，拼接作为16字节结果；

### HSM内部实现
1. HSM内部实现了AES的ECB/CBC/CMAC的引擎
2. HSM内部实现了随机数生成器的功能
3. HSM内部会对用户的输入进行检查，如果不符合预期就会对一些指定的状态寄存器置位
4. HSM内部处理完成后会去判断中断使能位是否被置位，如果被置位就会触发外部中断
5. HSM内部处理数据的输入和输出的切分，因为输入和输出是通过寄存器的方式，因此HSM内部会去维护输入数据；