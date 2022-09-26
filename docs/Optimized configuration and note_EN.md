- [中文版本](./Optimized%20configuration%20and%20note_CN.md)
## Optimized configuration and Note

### Optimized configuration

These configuration items are set already in sdkconfig.defaults，you can also use `idf.py menuconfig` to config them by yourself.

```
-  Items in Flash  
  - Serial flasher config->Flash SPI mode->QIO
  - Serial flasher config->Flash SPI speed->80MHz
  - Serial flasher config->Flash size->4MB
  - Component config->ESP System Settings->Flash leakage current workaround in light sleep->check

- Items in Wi-Fi
   - Example Configuration->WiFi listen interval->10
   - Example Configuration->power save mode->maximum modem
   - Example Configuration->Maximum CPU frequence->160MHz
   - Example Configuration->Minimum CPU frequence->40MHz 
   - Component config->Wi-Fi->Power Management for satation at disconnection->check
   - Component config->PHY->Max wifi TX power ->10
   - Component config->PHY->Power down MAC and baseband of Wi-Fi and Bluetooth when PHY is disabled->check

-  Items in Free RTOS
   - Component config->FreeRTOS->Tick rate->1000
   - Component config->FreeRTOS->Run FreeRTOS only on first core->check
   - Component config->FreeRTOS->Tickless idle support->check
   - Component config->FreeRTOS->Minimum number of ticks to enter sleep mode for->3

- Items in LWIP
  - Component config->LWIP->TCP->TCP timer interval->1000
  - Component config->LWIP->DHCP: Perform ARP check on any offered address->uncheck。

- Adjust strategy for beacon lost
  - Component config->Wi-Fi->Wifi sleep optimize when beacon lost  
    - There are two configuration: Beacon loss timeout and Maximum number of consecutive lost beacons allowed. You can add Beacon loss timeout and decrease the value of Maximum number of consecutive lost beacons allowed when the beacon lost is frequently, 
  - Component config->Wi-Fi->WiFi SLP IRAM speed optimization
    - There are two items: Minimum active time and Maximum keep alive time。You can reduce the value of Minimum active time when the wireless environment is good.
```

### Notes

- An external 32.768 crystal is needed. If you want to have a temporary try, you should use `idf.py menuconfig`, and modify following configurations:

  ```
  - Component config -> ESP32C3-Specific -> RTC clock source -> Internal 150kHz RC oscillator
  - Component config -> ESP32C3-Specific -> Number of cycles for RTC_SLOW_CLK calibration  3000 to 1024
  ```

- When use NovaHome APP, you need an account for Claiming, you just need choose the **Use Self Claiming** in *menuconfig->ESP RainMaker Config->Claiming Type*.
  








