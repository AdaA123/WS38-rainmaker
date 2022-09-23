- [English Version](./Optimized%20configuration%20and%20note_EN.md)
## 优化配置和注意事项

### 优化配置

以下的优化配置项已经默认配置好，也可以通过`idf.py menuconfig` 自行修改

```
- 调整 Flash  
  - Serial flasher config->Flash SPI mode->QIO
  - Serial flasher config->Flash SPI speed->80MHz
  - Serial flasher config->Flash size->4MB
  - Component config->ESP System Settings->Flash leakage current workaround in light sleep->勾选

- 调整 Wi-Fi
   - Example Configuration->WiFi listen interval->10
   - Example Configuration->power save mode->maximum modem
   - Example Configuration->Maximum CPU frequence->160MHz
   - Example Configuration->Minimum CPU frequence->40MHz 
   - Component config->Wi-Fi->Power Management for satation at disconnection->勾选
   - Component config->PHY->Max wifi TX power ->10
   - Component config->PHY->Power down MAC and baseband of Wi-Fi and Bluetooth when PHY is disabled->勾选

-  调整 Free RTOS
   - Component config->FreeRTOS->Tick rate->1000
   - Component config->FreeRTOS->Run FreeRTOS only on first core->勾选
   - Component config->FreeRTOS->Tickless idle support->勾选
   - Component config->FreeRTOS->Minimum number of ticks to enter sleep mode for->3

- 调整 LWIP
  - Component config->LWIP->TCP->TCP timer interval->1000
  - Component config->LWIP->DHCP: Perform ARP check on any offered address->取消勾选。

- 调整丢包策略
  - Component config->Wi-Fi->Wifi sleep optimize when beacon lost  
    - 该配置项对应两个设置 Beacon loss timeout 和 Maximum number of consecutive lost beacons allowed，当网络情况较为复杂时，可以增加 Beacon loss timeout 的数值和减少 Maximum number of consecutive lost beacons allowed 的数值。
  - Component config->Wi-Fi->WiFi SLP IRAM speed optimization
    - 该配置项对应两个设置 Minimum active time 和 Maximum keep alive time。当网络状况良好时，可以减少 Minimum active time，网络情况不太好时，需要增加这个数值。
```

### 注意事项

- 使用的硬件需要外接 32.768 KHz 的晶振，如果没有晶振，使用默认配置编译程序的术后会出现不断重启的现象。如果暂时没有外接晶振的硬件设备但想先看下效果，需要在程序中使用`idf.py menuconfig`，对以下两个配置项进行修改：

  ```
  - Component config -> ESP32C3-Specific -> RTC clock source -> Internal 150kHz RC oscillator
  - Component config -> ESP32C3-Specific -> Number of cycles for RTC_SLOW_CLK calibration 从 3000 改为 1024
  ```

- 使用 NovaHome APP 的时候，需要账号进行 Claiming，在 *menuconfig->ESP RainMaker Config->Claiming Type* 选择 **Use Self Claiming** 即可。

  








