- [English Version](./README.md)
# 单火线开关 demo 开发指南

### 方案特性

- 配网前广播电流 0.8 mA
- 连接中，通信间隔协调过程 大约 1S，平均电流 15 mA。
- 连接后保持 Wi-Fi 长连接的提情况下平均电流 0.8 mA。

### 开源内容

- [硬件参考原理图](./References/Hardware_Overview/)
- [参考示例程序](./projects/)
- [相关测试报告](./References/)
- [demo 展示视频](./References/)

### 使用指引

- 想要了解相关硬件设计，请查看[硬件参考设计](./References/Hardware_Overview/)
- 想要查看一些报告或者演示视频，请查看[参考资料](./References/)
- 您可以在开发前烧写一个预置的固件进行测试，请查看[烧录说明](./docs/Download_Guide_CN.md)
- 想要了解烧写 demo 后的 APP 使用步骤，请查看 [APP 使用指南](./docs/APP_User_Guide_CN.md)

### 开发指南

- **Step1**. 如果您是首次接触 ESP-IDF 开发，建议先浏览 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32c3/index.html)。
- **Step2**. 接下来您可以阅读 [ESP-IDF 环境搭建指引](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32c3/get-started/index.html)，一步一步完成开发环境的搭建。
- **Step3**. 当前需要指定 IDF 版本并打上 patch,同时还需要增加一个 Qcloud 相关的文件。这些patch 文件和打 patch 的过程可以参照 [打补丁步骤介绍](./patch/patch_steps_CN.md)
- **Step4**. 在搭建完 ESP-IDF 环境后可以进行程序的编译和烧录，编译前请先查看[优化配置和注意事项](./docs/Optimized%20configuration%20and%20note_CN.md)。
