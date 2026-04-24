本仓库用于生成设备模型和C语言软件驱动。
生成的设备模型搭配上DevCommV3仓库中的框架，可以用于测试C驱动的正确性以及覆盖率；
框架描述如下：
+------------------------+
|  driver      test      |
|^^^^^^^^^^^^^^^^^^^^^^^^+
| +-------------------+  |
| |   libdrvintf.a    |  |
| |^^^^^^^^^^^^^^^^^^^|  |
| |   communication   |  |
| |   read_register   |  |
| |   write_register  |  |                                         +-----------------------+
| |request_int_handler|  |                                         |      DevComm_V3       |
| +-------------------+  |                                         |^^^^^^^^^^^^^^^^^^^^^^^|
|                        |  ------------------------------------\  | It's a python model   |
| +-------------------+  |           Socket ---> Command         > | framework.            |
| |    test case.c    |  |  ------------------------------------/  |                       |
| +-------------------+  |                                         |                       |
|                        |                                         | +-------------------+ |
| +-------------------+  |                                         | |  device_model.py  | |
| |  device_driver.c  |  |                                         | +-------------------+ |
| +-------------------+  |                                         +-----------------------+
|                        |
| +-------------------+  |
| |   device_hal.c    |  |
| +-------------------+  |
|                        |
+------------------------+

DevComm_V3:
https://github.com/ATC-WangChu/DevCommV3/

后续新增的设备模型需要遵循如下规则步骤：
1. 定义设备名称：dev_name，例如：crc
2. 在顶层目录下创建一个对应设备名称的目录，例如：crc
3. 在dev_name目录下创建两个子目录：input和output
input目录说明如下：
  1）input目录必须由用户传入对应设备的工作说明文档，例如: crc/input/description.docx
  2) input目录必须由用户传入对应设备的寄存器说明文档，例如: crc/input/register.yaml
output目录说明如下：
  1) output目录用于存放自动化生成的设备模型以及驱动；
  2）设备模型用"设备名称_device.py"的形式命名，例如：crc_device.py
  3）设备模型的生成规则参考文档：autogen_rules/device_model_gen.txt
  4）output目录下需要再创建一个子目录: c_driver
  5）c_driver目录用于存放自动化生成的设备模型的驱动，是C语言实现；
  6）c语言驱动的生成规则参考文档：autogen_rules/device_driver_gen.txt
  7）c_driver下还需要生成对应驱动的测试程序，测试程序的生成规则参考：autogen_rules/device_driver_test_gen.txt

4. 测试程序的编译需要从顶层Makefile开启，例如：make crc
5. 生成的binary可用于测试C语言驱动与python模型的联动效果
