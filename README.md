# U-Boot for Xiaomi Redmi K20 Pro (Raphael)

面向 **红米 K20 Pro（代号 Raphael，SoC SM8150）** 的 U-Boot 构建与刷机说明。

本仓库在主线 U-Boot 基础上，增加了手机启动菜单、物理按键、EFI 从 **cache** 分区启动等适配，并针对 Raphael 的 ABL 要求使用特定的 Android boot image 打包格式。

## 产物说明

每次推送到 GitHub 后，Actions 会自动编译并发布 Release，包含两个 zip：

| 文件 | 镜像变体 | 屏幕表现 | 适用场景 |
|------|----------|----------|----------|
| `boot_nolog.img.zip` | **nolog**（默认推荐） | 保留 ABL 开机 logo，U-Boot 日志仅输出到串口 | 日常刷机、正常开机观感 |
| `boot_log.img.zip` | **log** | 开机时在屏幕上显示 U-Boot 日志 | 调试、排查启动问题 |

解压后内容：

| 文件 | 说明 |
|------|------|
| `u-boot.img` | 刷入 **boot** 分区的 Android boot image |
| `FLASH.txt` | 简要刷机说明 |

本地编译时，脚本直接生成：

| 本地文件 | 对应变体 |
|----------|----------|
| `boot_nolog.img` | nolog |
| `boot_log.img` | log |

### 两款变体的区别

| 项目 | nolog | log |
|------|-------|-----|
| 环境配置 | `board/qualcomm/raphael-phone.env` | `board/qualcomm/raphael-phone-log.env` |
| 屏幕输出 | `stdout=serial`（不进 vidconsole） | `stdout=serial,vidconsole` |
| U-Boot logo | 关闭（不覆盖 ABL 画面） | 开启 |
| 开机 preboot | 仅 `scsi scan` | `scsi scan; usb start` |
| 进入菜单后 | 自动恢复屏幕输出 | 始终有屏幕输出 |

### boot_nolog.img 如何查看日志

`boot_nolog` 变体为保留 ABL 开机 logo，将 U-Boot 的 `stdout` / `stderr` 设为仅输出到 **串口（serial）**，正常开机时屏幕上不会出现日志。日志仍会写入 **Console Record** 缓冲区（约 24KB），并可通过以下方式查看。

> **快速查看刚才的启动日志（USB 已连接时）**
>
> 1. 开机时长按音量下进入 U-Boot 菜单，选择 **Enable fastboot mode**
> 2. 电脑执行 `fastboot oem console`，导出本次开机以来的完整 U-Boot 日志
>
> **确认已进入 U-Boot fastboot**（不是小米 ABL fastboot）：
> ```bash
> fastboot getvar version-bootloader   # 应显示 U-Boot 版本号
> ```
>
> **关于输出内容**：`fastboot oem console` 导出的是 Console Record 缓冲区的**真实启动日志**，包括 `scsi scan`、设备探测、EFI 警告等。若期间进过启动菜单，日志里还会有菜单重绘的乱码（ANSI 转义序列），属于正常现象。
>
> **旧版镜像 bug**：`oem console` 输出结束后若出现大量 `was not queued to ep1in-bulk` 并死机，是 dwc3 USB 驱动问题，需重启手机。新版已修复，导出结束后会正常返回 `OKAY`，可继续执行其他 fastboot 命令。

#### 方式一：启动菜单（无需电脑，推荐）

**开机时长按音量下** 进入启动菜单。进入菜单时会自动恢复屏幕输出（`vidconsole`），可直接在屏幕上操作和查看信息。

菜单中与日志/调试相关的条目：

| 菜单项 | 作用 |
|--------|------|
| Boot | 尝试启动，失败时 `pause` 暂停 |
| Dump clocks | 打印时钟信息并暂停 |
| Dump environment | 打印环境变量并暂停 |
| Board info | 打印板级信息并暂停 |
| Dump bootargs | 打印设备树 bootargs 并暂停 |
| Drop to shell | 进入 U-Boot 命令行（屏幕可交互） |

#### 方式二：USB 串口 + 命令查看历史日志

1. 进入启动菜单，选择 **Enable serial console gadget**（或 **Drop to shell**）
2. USB 连接电脑，打开串口：

```bash
# Linux 示例
sudo picocom -b 115200 /dev/ttyACM0
```

3. **查看刚才的启动日志** — 在串口终端输入：

```text
run fastboot
```

手机进入 fastboot 后，在电脑上执行：

```bash
fastboot oem console
```

会逐行输出本次启动以来记录的完整日志（含 `scsi scan`、`bootefi` 报错等）。若缓冲区为空会提示 `Empty console`。日志末尾出现菜单乱码或（旧版）dwc3 报错时，见上文说明。

4. **查看当前状态**（非完整历史，但无需进 fastboot）— 在串口终端直接输入：

```text
bdinfo
printenv
clk dump
```

也可跳过串口，直接从菜单选 **Enable fastboot mode**，再执行 `fastboot oem console`。

通过 fastboot 还可远程执行命令：

```bash
fastboot oem run printenv
fastboot oem run bdinfo
```

#### 方式三：硬件串口（进阶）

设备树中 `stdout-path = "serial0:115200n8"`（`uart2`）。若有飞线/测试点条件，可接 USB 转串口在 **115200 8N1** 下直接查看开机日志。

#### 方式四：临时改用 log 变体

若需要**全程在屏幕上**显示开机日志，可刷入 `boot_log.img.zip` 中的镜像；调试完成后再刷回 `boot_nolog.img`。

#### nolog 启动时日志去向

```text
开机（屏幕保持 ABL logo）
  → U-Boot 日志 → serial（屏幕不可见）
  → 同时写入 Console Record 缓冲区
  → bootefi bootmgr
       ├─ 成功 → 进入系统
       └─ 失败 → 进入启动菜单（屏幕恢复输出）
```

常见日志（通过上述方式查看时可能见到）：

| 日志 | 是否正常 | 说明 |
|------|----------|------|
| `scanning bus for devices...` | 正常 | `preboot` 扫描 UFS |
| `No USB controllers found` | 正常 | 开机阶段未启动 USB 时常见 |
| `Cannot persist EFI variables without system partition` | 警告 | 无 system 分区，EFI 变量无法持久化，一般可忽略 |
| `Missing RNG device for EFI_RNG_PROTOCOL` | 警告 | 无硬件 RNG，一般可忽略 |
| `*** U-Boot Boot Menu ***` 及乱码行 | 正常 | 导出前进过启动菜单，含屏幕重绘的 ANSI 控制符 |
| `was not queued to ep1in-bulk` 大量循环 | **异常（旧版 bug）** | dwc3 + `oem console` 触发死循环，需刷新版或重启 |

## 刷机方式

### 前提

- 已解锁 Bootloader
- 已安装 `fastboot` / `adb`
- 已准备 **cache 启动镜像** `xiaomi-k20pro-boot.img`（EFI 启动用，刷入 cache 分区）

参考官方发布：[GengWei1997/linux-xiaomi-raphael-uboot v1.0.0](https://github.com/GengWei1997/linux-xiaomi-raphael-uboot/releases/tag/v1.0.0)

### 刷入步骤

1. 从 [Releases](https://github.com/GavinLiuOnline/xiaomi_raphael_uboot/releases) 下载 `boot_nolog.img.zip` 或 `boot_log.img.zip`
2. 解压得到 `u-boot.img`
3. 手机进入 fastboot 模式后执行：

```bash
fastboot flash boot u-boot.img
fastboot flash cache xiaomi-k20pro-boot.img
fastboot erase dtbo
fastboot reboot
```

> **boot** 分区刷 U-Boot；**cache** 分区刷 EFI 启动镜像；**dtbo** 需擦除，否则可能干扰启动。

### 开机与菜单

- 默认自动执行 `bootefi bootmgr`，从 cache 分区启动系统
- 启动失败时会暂停并进入启动菜单
- **开机时长按音量下** 可直接进入启动菜单

菜单功能包括：正常启动、USB 串口调试、USB 大容量存储、fastboot、shell、重启、查看时钟/环境变量/板信息等。

## 本地编译

### 依赖

Debian / Ubuntu 示例：

```bash
sudo apt install -y \
  bc bison build-essential device-tree-compiler flex \
  gcc-aarch64-linux-gnu libssl-dev mkbootimg \
  python3 python3-pyelftools python3-setuptools swig
```

若系统自带的 `mkbootimg` 不可用，可从源码构建：

```bash
git clone --depth=1 https://github.com/osm0sis/mkbootimg
make -C mkbootimg CFLAGS=-Wstringop-overflow=0 mkbootimg
export PATH="$PWD/mkbootimg:$PATH"
```

### 一键构建

```bash
# 无屏幕日志（推荐）
./build-raphael-bootimg.sh nolog
# 输出: boot_nolog.img

# 有屏幕日志（调试用）
./build-raphael-bootimg.sh log
# 输出: boot_log.img
```

可选指定交叉编译前缀：

```bash
CROSS_COMPILE=aarch64-linux-gnu- ./build-raphael-bootimg.sh nolog
```

### 构建配置说明

脚本等价于：

```text
make qcom_defconfig qcom-phone.config <raphael 变体 config>
CONFIG_DEFAULT_DEVICE_TREE = qcom/sm8150-xiaomi-raphael
```

打包格式（Raphael ABL 要求）：

```text
gzip(u-boot-nodtb.bin) + u-boot.dtb
mkbootimg --pagesize 4096 --base 0x80000000
```

### 手动编译（不使用脚本）

```bash
export CROSS_COMPILE=aarch64-linux-gnu-

# nolog 示例
make qcom_defconfig qcom-phone.config raphael-phone.config
./scripts/config --set-str CONFIG_DEFAULT_DEVICE_TREE "qcom/sm8150-xiaomi-raphael"
./scripts/config --disable TOOLS_MKEFICAPSULE
./scripts/config --disable EFI_RUNTIME_UPDATE_CAPSULE
./scripts/config --disable EFI_CAPSULE_ON_DISK
./scripts/config --disable EFI_CAPSULE_FIRMWARE_RAW
make olddefconfig
make -j"$(nproc)"

gzip -kf u-boot-nodtb.bin
cat u-boot-nodtb.bin.gz u-boot.dtb > u-boot-nodtb.bin.gz-dtb
mkbootimg --kernel u-boot-nodtb.bin.gz-dtb \
  --pagesize 4096 --base 0x80000000 --output boot_nolog.img
```

log 变体将 `raphael-phone.config` 换成 `raphael-phone-log.config` 即可。

## 相关文件

```text
build-raphael-bootimg.sh              # 一键构建脚本
board/qualcomm/raphael-phone.env      # nolog 环境变量
board/qualcomm/raphael-phone.config   # nolog Kconfig 片段
board/qualcomm/raphael-phone-log.env  # log 环境变量
board/qualcomm/raphael-phone-log.config
board/qualcomm/qcom-phone.config      # 手机通用配置（按键、菜单、fastboot 等）
dts/upstream/src/arm64/qcom/sm8150-xiaomi-raphael.dts
.github/workflows/release.yml         # CI 自动编译与发布
```

## GitHub Actions 自动发布

- 触发条件：每次 `push`
- Release 标签：`build-<序号>`
- 附件：`boot_nolog.img.zip`、`boot_log.img.zip`

仓库需开启 **Settings → Actions → General → Workflow permissions → Read and write permissions**，否则无法创建 Release。

## 注意事项

- 刷机有风险，请自行备份重要数据
- 仅验证于 **Redmi K20 Pro (Raphael)**，其他机型请勿直接使用
- 日常使用推荐 **nolog** 变体；需看完整开机日志时可用菜单 + `fastboot oem console`，或临时刷 **log** 变体
