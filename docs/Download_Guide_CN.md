- [中文版本](./Download_Guide_CN.md)

## Download Guide

Some users want to try the demo before setting up the environment, we provide a [firmware](../test_demo/), which can be directly burned to the address of 0x0.

### Linux Users

1.Connect your development board to the computer through USB Type-C cable, there is no need to install the driver of USB-Serial-Jtag under Linux system.

2.Install `esptool`, input the following commands in `Terminal` (`pip` can be specified as `pip3`) :

```
pin install esptool
```

3.Follow the instruction to download the firmware (`pip` can be specified as `pip3`)：：

```
python -m esptool --chip esp32c3 write_flash 0x0 download_path/test_bin.bin
```

- `0x0` is the fixed flash address
-  `download_path/test_bin.bin` need be replaced with your firmware path and name.

4.After updating, the download tool will prompt `Hash of data verified`. Next, **please reboot to run the new firmware!**

### Windows Users

We recommend using `Windows 10` and above system. Under `Windows 10` system, the driver of `USB-Serial-Jtag` will be downloaded automatically. If you use the `Windows 7`, please download and install [USB-Serial-JTAG drive](https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip) manually.

1.Connect your development board to the computer through USB Type-C cable.

2.Please make sure the computer is connected to the Internet first. When the driver is installed, you can find two new devices appear on `Device Manager` list, `COMX` (`COM2` for example) and `USB JTAG/serial debug unit`, the former is used to download firmware or output program logs, the latter is used for JTAG debugging.

3.Download [Windows download tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.2_0.zip)，and unzip it to any folder, then run the executable file `flash_download_tool_x.x.x.exe`

4.Please choose `chipType`: `ESP32C3`, `workMode`: `develop`, `loadMode`: `usb`，then click `OK` to enter the download tool config interface.

5.Follow the instruction below to configure the downloaded tool:

- Choose the path of firmware  `xxxx.bin` , then set the download address to  `0x0`
- Select the COM port, like `COM2` for this PC.
- Click `START` to start the downloading.

6. After downloading, `FINISH` will appear on the tool. Next, **please reboot to run the new firmware!**