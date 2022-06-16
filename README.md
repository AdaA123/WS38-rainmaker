- [中文版本](./README_cn.md)
# Single fire wire Demo Development Guide

### Features

- Current before provisioning: 0.8 mA.
- Time  for provision is about  1S  and the current is about 15 mA during this interval.
- Current after provisioning and keep Wi-Fi alive:  0.8 mA.

### Open-Source Content

- [PCB Schematic](./References/Hardware_Overview/)
- [Demo code](./projects/)
- [Test reports](./References/)
- [Demo video](./References/)

### User Guide

- For more details of hardware information, please refer to the [Hardware Overview](./References/Hardware_Overview/).
- For test reports or some demo video, please refer to [References](./References/).
- For test firmware, please refer to [Download Guide](./docs/Download_Guide_EN.md).
- For how to use the APP to control the switch, please refer to [APP User Guide](./docs/APP_User_Guide_EN.md).

### Developer Guide

- **Step1**. If this is your first time using the ESP-IDF, please refer to [ESP-IDF Development Guide](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32c3/index.html).
- **Step2**. Next, setting up development environment. Please refer to [ESP-IDF Installation](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32c3/get-started/index.html), step by step.
- **Step3**. Apply patch and replace some libs follow the [Steps for applying patch](./patch/patch_steps_EN.md).
- **Step4**. Finally you can build and flash the code, you should refer to [Optimized configuration and Note](./docs/Optimized%20configuration%20and%20note_EN.md) before build.

