# ESP32-C6 Thread Border Router

基于 ESP-IDF v5.5.2 的单芯片 Thread 边界路由器。

## 硬件

- **芯片**: ESP32-C6 (WiFi 6 + BT5 + IEEE 802.15.4)
- **Flash**: 2MB (实际 4MB 但只用了 2MB)
- **串口**: COM23 (CH343, VID=1A86 PID=55D3)
- **注意**: 单 RF 天线，WiFi 和 Thread 软件共存分时复用

## 编译 & 烧录 (ESP-IDF PowerShell)

打开开始菜单 → **ESP-IDF 5.5 PowerShell**，然后：

```powershell
cd thread-br
idf.py set-target esp32c6
idf.py build
idf.py -p COM23 flash monitor        # 烧录 + 看日志
```

就这三条命令。首次 `build` 约 3 分钟，后续改代码重编只需十几秒。

> **注意**: 不要用 Git Bash / MSYS2 跑 idf.py，会报工具链找不到。用 ESP-IDF 自带的 PowerShell 快捷方式。

## SDK 配置

首次编译前运行一次，设 WiFi 密码：

```powershell
idf.py menuconfig
# Component config → Thread Border Router Configuration
#   填上你的 WiFi SSID 和 密码
```

或者直接改目录下的 `sdkconfig` 文件。

## 分区表

已配置为 2MB Flash 分区（板子实际 4MB 但只用了 2MB）：

```
nvs:      0x9000~0xEFFF   (24KB)
phy_init: 0xF000~0xFFFF   (4KB)
factory:  0x10000~0x18FFFF (1.5MB)
ot_store: 0x190000~       (剩余)
```

## Thread CLI 命令

烧录后按回车进入 `esp32c6>` 命令行：

| 命令 | 作用 |
|------|------|
| `ot state` | 查看 Thread 角色 (leader/child/router) |
| `ot ipaddr` | 查看 Thread IPv6 地址 |
| `ot channel` | 查看/设置信道 (11-26) |
| `ot panid` | 查看 PAN ID |
| `ot networkname` | 查看网络名 |
| `ot dataset active` | 查看完整数据集 |
| `ot neighbor table` | 查看邻居表 |
| `ot child table` | 查看子设备列表 |
| `help` | 所有命令 |

## 工作原理

```
  Internet / LAN
       |
     WiFi AP
       |  (WiFi STA)
   ESP32-C6  <--- Thread Border Router
       |  (802.15.4)
   Thread Mesh Network
   ├── End Device 1
   ├── End Device 2
   └── Router / Leader
```

## 项目结构

```
thread-br/
├── CMakeLists.txt       # ESP-IDF 顶层
├── sdkconfig            # Kconfig 配置
├── partitions.csv       # 分区表
├── main/
│   ├── CMakeLists.txt   # 组件依赖
│   └── main.c           # 主程序
└── build/               # 编译产物
```

## 踩坑记录

1. **Git Bash 不能跑 ESP-IDF** — idf.py 有 MSYS2 检查，toolchain PATH 传不过去
2. **PlatformIO espidf 不带 OpenThread 头文件** — SCons 构建不走组件 include
3. **mbedtls v4 不兼容 OpenThread** — v5.5.2 的 mbedtls 去掉了旧 API，需禁用 commissioner
4. **官方例程默认 SPINEL** — ESP32-C6 单芯片必须切 NATIVE 射频
