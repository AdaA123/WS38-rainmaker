- 如果使用该控制面板，需要将原理图中的C24, C25, C26防抖电容去掉，不然有可能造成使用异常

  Remove the the capacitor C24, C25, C26 before use the schematic of control board, it is found that these capacitors may make error. 

- 该控制面板上预留了一个电流测量口在 J4 的位置，如果要测量通过模组的电流可以将 R1 的 0Ω 电阻去掉然后将电流表接入 1 和 2 两端即可。

  There is a test port for measuring the current of ESP32-C3 module in J4, you can remove the 0Ω resistor R1 and connect the ammeter to point 1 and 2.

