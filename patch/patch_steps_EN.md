- [中文版本](./patch_steps_CN.md)
## Steps for applying the patch

1. Get the latest IDF.

   `git clone https://github.com/espressif/esp-idf.git`

2. Clone the submodule.

   `git submodule update --init--recursive`

3. Checkout to the  commit ID: cc5e2107bb12c901

   ``git checkout cc5e2107bb12c901 ``

4. Update the submodule once more.

   `git submodule update --init--recursive`

5. Clean the status.

   ```
   git status
   rm-r components/
   git submodule update --init--recursive
   ```

6. Reset the head.

   ```
   git reset--hard HEAD
   git status
   ```

7. Apply the patch.

   `git apply XXXX/esp-lowpower/patch/idf-low-power.diff`

   XXXX is the path for patch document

8. Enter the esp-lowpower/patch/component_lib , use folder  **"mqtt"** , **"lib"** , **"lwip"**  to replace the folders **"esp-idf/components/mqtt"** , **"esp-idf/components/esp_wifi/lib"** ,**"esp-idf/components/lwip/lwip"**  on  esp-idf/components.

9. unzip the esp-qcloud.zip, and use the folder after unzipping to replace the components/esp_qcloud  on esp-lowpower 



