- [English Version](./Download_Guide_EN.md)

## 下载说明

为了方便用户在搭建好环境前想先进行 demo 的尝试，我们提供了一个[固件](../test_demo/)，直接烧录到 0x0 的地址中就可以运行。

### Linux 用户

1.使用 USB Type-C 数据线将开发板接入电脑，`USB-Serial-Jtag` 在 Linux 系统下无需安装驱动

2.安装下载工具 `esptool`，请打开 `终端` ，并输入以下指令（`pip` 也可指定为 `pip3`）：

```
pin install esptool
```

3.请使用以下指令下载固件（`python` 也可指定为 `python3`）：

```
python -m esptool --chip esp32c3 write_flash 0x0 download_path/test_bin.bin
```

- `0x0` 是一个固定值，表示即将写入的 flash 地址
-  `download_path/test_bin.bin` 是一个变量，为测试 固件的地址

4.下载完成后，工具将提示 `Hash of data verified`，之后之后 **重新上电，即可进入新程序！**

### Windows 用户

我们推荐使用 `Windows 10` 及以上版本，在该系统下 `USB-Serial-Jtag` 的驱动将联网自动下载。如果使用 `Windows 7` 系统，请手动下载 [USB-Serial-JTAG 驱动](https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip) 并安装。

1.使用 USB Type-C 数据线将开发板接入电脑

2.初次使用，请确保电脑已联网，驱动正常自动安装后，我们能在设备管理器看到以下设备。这里将多出两个新的设备 `COMX`(此电脑为 `COM2` ) 和 `USB JTAG/serial debug unit`，前者用于下载固件和输出程序日志，后者用于 JTAG 调试。

3.下载 [Windows download tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.2_0.zip)，并解压到任意文件夹，然后请双击打开下载工具可执行文件 `flash_download_tool_x.x.x.exe`

4.请选择 `chipType`: `ESP32C3`, `workMode`: `develop`, `loadMode`: `usb`，之后点击 `OK` 进入下载工具界面：

5.请按照下图指示配置下载工具:

- 首先选择 `xxxx.bin` 路径，将地址设置为 `0x0`
- 选择下载端口，此电脑为 `COM2` 
- 点击 `START` 开始固件下载

6. 下载完成后，工具将提示 `FINISH`，之后 **重新上电，即可进入新程序！**
7. 如果使用的不是开发板是模组，烧录程序时需要引出以下的引脚： 3V3, GND, TX0, RX0, GPIO9(烧录时拉低)