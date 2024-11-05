# NiMh Battery Charger for 10 AA and 6 AAA batteries

Idea taken from BigClive:
[https://www.reddit.com/r/BigCliveDotCom/comments/tcny85/simple_nimh_battery_charger_with_pcb_files/]

For detecting battery and the timers used ch32v003fun:
[https://github.com/cnlohr/ch32v003fun?tab=readme-ov-file]

Example of the dev board:
[https://oshwlab.com/wagiminator/ch32v003f4p6-development-board]

To use the WCH-LinkE on Linux, you need to grant access permissions beforehand by executing the following commands:

```shell
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8010", MODE="666"' | sudo tee /etc/udev/rules.d/99-WCH-LinkE.rules
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8012", MODE="666"' | sudo tee -a /etc/udev/rules.d/99-WCH-LinkE.rules
sudo udevadm control --reload-rules

```


To upload firmware and perform debugging, you need to ensure that the development board is disconnected from any power sources. Then, you should make the following connections to the WCH-LinkE:
```
WCH-LinkE      DevBoard
+-------+      +------+
|  SWDIO|      |DIO   |
|    3V3| ---> |3V3   |
|    GND| ---> |GND   |
+-------+      +------+

```

## Quick reference
![ch32v003f4u6](https://raw.githubusercontent.com/Tengo10/pinout-overview/main/pinouts/CH32v003/ch32v003f4u6.svg)
