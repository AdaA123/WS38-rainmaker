- [English Version](./patch_steps_EN.md)
## 打补丁步骤介绍

1. 进入IDF仓库，拉取 IDF 的最新版本。

   `git clone https://github.com/espressif/esp-idf.git`

2. 进入下载好的 IDF 文件夹进行子模块的克隆保证下载完全。

   `git submodule update --init--recursive`

3. 子模块下载完毕后切换分支到  commit ID: cc5e2107bb12c901

   ``git checkout cc5e2107bb12c901 ``

4. 再次更新子模块

   `git submodule update --init--recursive`

5. 确认当前 IDF 文件夹的状态，会提示不干净，执行以下操作进行更新：

   ```
   git status
   rm-r components/
   git submodule update --init--recursive
   ```

6. 重置头指针，然后再次确认文件夹，保证此时 IDF 文件夹是干净的

   ```
   git reset--hard HEAD
   git status
   ```

7. 进入esp-lowpower/patch 打补丁，将  idf-low-power.diff  应用于 IDF 。

   `git apply XXXX/esp-lowpower/patch/idf-low-power.diff`

   XXXX 为文件夹路径

8. 进入 esp-lowpower/patch/component_lib ，使用里面的  **"mqtt"** 文件夹， **"lib"** 文件夹， **"lwip"** 文件夹分别替换 esp-idf/components 中的 **"esp-idf/components/mqtt"** 文件夹, **"esp-idf/components/esp_wifi/lib"** 文件夹,  **"esp-idf/components/lwip/lwip"** 文件夹。

9. 解压 patch 中的 esp-qcloud，使用解压后的文件夹替换 esp-lowpower 文件夹中的 components/esp_qcloud 



