

## 内部FLASH空间使用详情

----

|开始地址 |结束地址  |占用空间   |代码内宏标记  |用途           |备注   |
|---     |---       |---        |---        |---            |---    |
|0x0     | 0xA000   |TBD        | TBD       |bootloader     |TBD    |
|0xA000  | 0xC7FFF  |760K Bytes    |           |application    |       |
|0xC8000  |0xF7FFF   |192k bytes|QUICK_READ_INFO_ADDR|外部字库快速索引表|       |
|0xF8000 |0xFAFFF   |12K Bytes        |xx         |支付宝|可能用到16K|
|0xFB000 |0xFB7FF   |2K Bytes        |FLASH_ADDR_IOS_CONFIG_LIST|ancs 过滤列表|当前用到680字节|
|0xFB800 |0xFBF7F|1920Bytes|FLASH_ADDR_SURFACE_CONTEXT||当前用到392字节|
|0xFBF80 |0xFBFFF|128Bytes|MIBEACON_FRAME_COUNTER_MAGIC_ADDR|mibeacon frame counter|当前用到8个字节|
|0xFC000 |0xFC7FF|2K Bytes||Device profile ||
|0xFC800 ||||Device setting statue||
|0xFC800 + sizeof(ry_device_setting_t)||||MIBLE||
|0xFD000||||debug_info|
|0xFD200||||FLASH_ADDR_MIJIA_SCENE_TABLE|
| 0xFE000||||FLASH_ADDR_NFC_PARA_RF1|
| 0xFE100||||FLASH_ADDR_NFC_PARA_RF2|
| 0xFE200||||FLASH_ADDR_NFC_PARA_RF3|
| 0xFE300||||FLASH_ADDR_NFC_PARA_RF4|
| 0xFE400||||FLASH_ADDR_NFC_PARA_RF5|
| 0xFE500||||FLASH_ADDR_NFC_PARA_RF6|
| 0xFE600||||FLASH_ADDR_NFC_PARA_TVDD1|
| 0xFE700||||FLASH_ADDR_NFC_PARA_TVDD2|
| 0xFE800||||FLASH_ADDR_NFC_PARA_TVDD3|
| 0xFE900||||FLASH_ADDR_NFC_PARA_NXP_CORE|
| 0xFEA00||||FLASH_ADDR_NFC_PARA_BLOCK|
|0xFF000||||FLASH_ADDR_ACCESS_CARDS||
|0xFF800||||FLASH_ADDR_TRANSIT_CARDS||
|0xFFC00||||FLASH_ADDR_APP_CONTEXT||
|0xFFD00||||DEBUG_FAULT_INFO||
|0xFFE00|0xFFFFF|512Bytes|FLASH_ADDR_ALARM_TABLE|闹钟相关||



----

## 常用的其他地址

|名称|地址|
|---|---|
|设备版本号|0x9C00|