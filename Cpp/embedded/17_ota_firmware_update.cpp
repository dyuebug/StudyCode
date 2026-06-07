// ============================================
// 17_ota_firmware_update.cpp
// 日期：2026-06-04（第61天）
// 主题：OTA 固件升级机制
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -o 17_ota 17_ota_firmware_update.cpp
// ============================================

/*
【核心概念】

是什么？
- OTA（Over-The-Air）：通过无线/网络远程升级设备固件
- 不需要物理连接（USB/JTAG），设备在现场就能升级
- 类比：手机系统自动更新，不需要拿去维修店

为什么需要？
- 修复现场设备的 Bug，不用召回
- 推送新功能到已部署的设备
- 批量升级数千台设备

核心挑战：
- 安全性：防止伪造固件被刷入（需要签名验证）
- 可靠性：升级中断（断电/断网）不能变砖
- 原子性：要么完全成功，要么完全回滚
*/

#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <functional>
#include <iomanip>
#include <numeric>
#include <algorithm>
using namespace std;

// ============================================
// 第一部分：双 Bank 策略 ⭐⭐⭐⭐⭐
// ============================================

/*
【双 Bank（Dual Bank）OTA 原理】

核心思想：永远保留一个可运行的固件版本

Bank A（当前运行）    Bank B（下载区）
┌───────────────┐    ┌───────────────┐
│  固件 v1.0    │    │  空白 / v0.9  │
│  正在运行 ✅   │    │  接收新固件   │
└───────────────┘    └───────────────┘
         ↓  下载完成并校验通过
┌───────────────┐    ┌───────────────┐
│  固件 v1.0    │    │  固件 v2.0 ✅  │
│  （将被替换） │    │  校验通过     │
└───────────────┘    └───────────────┘
         ↓  Bootloader 安装
┌───────────────┐    ┌───────────────┐
│  固件 v2.0    │    │  固件 v1.0    │
│  新版本运行 ✅ │    │  备份保留     │
└───────────────┘    └───────────────┘

好处：
- 下载阶段断电 → 重启仍运行 v1.0，重新下载即可
- 安装阶段异常 → Bootloader 检测到 v2.0 损坏，回滚到 v1.0
*/

// ============================================
// 第二部分：CRC32 校验 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：CRC32】

是什么？
- 循环冗余校验（32位），用于检测数据传输/存储中的错误
- 对同一段数据计算结果唯一，数据改变则 CRC 改变
- 嵌入式中最常用的数据完整性校验算法

使用场景：
- 固件下载完成后，校验是否有位翻转（传输错误）
- 存储固件 CRC 到参数区，每次启动时校验

计算方式：
- 软件 CRC：查表法（256项查找表，速度快）
- 硬件 CRC：STM32 有专用 CRC 外设（更快，1个周期1字节）

多项式：标准 CRC32 使用 0xEDB88320（反向多项式）
*/

class CRC32 {
    uint32_t table_[256];

    void buildTable() {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (int j = 0; j < 8; j++) {
                crc = (crc & 1) ? (0xEDB88320 ^ (crc >> 1)) : (crc >> 1);
            }
            table_[i] = crc;
        }
    }

public:
    CRC32() { buildTable(); }

    // 计算数据块的 CRC32
    // 参数：data — 数据指针，len — 数据长度（字节）
    // 返回：32位 CRC 值
    uint32_t calculate(const uint8_t* data, size_t len) {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; i++) {
            crc = table_[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFF;
    }

    // 流式计算（分片更新，适合大文件）
    // 用法：先 init()，每片数据调用 update()，最后 final()
    uint32_t crc_state_ = 0xFFFFFFFF;

    void init()  { crc_state_ = 0xFFFFFFFF; }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; i++) {
            crc_state_ = table_[(crc_state_ ^ data[i]) & 0xFF] ^ (crc_state_ >> 8);
        }
    }

    uint32_t final() { return crc_state_ ^ 0xFFFFFFFF; }
};

// ============================================
// 第三部分：固件包结构 ⭐⭐⭐⭐
// ============================================

/*
【固件包（Firmware Package）结构】

真实的固件包不只是裸二进制，还包含：
- 魔数（Magic）：标识这是一个合法固件包
- 版本信息：方便防降级检查
- 目标设备类型：防止把 A 型号固件刷到 B 型号
- 固件大小：知道要接收多少字节
- CRC32：校验固件数据完整性
- （可选）数字签名：防止伪造固件

固件包布局：
┌──────────────────────────────────────┐
│  Header（固定大小，如 32 字节）        │
│  - magic: 0x4657504B（"FWPK"）       │
│  - version: 主版本.次版本.修订        │
│  - device_type: 0x0001（设备型号）    │
│  - firmware_size: 固件字节数          │
│  - firmware_crc: 固件数据的 CRC32     │
│  - header_crc: Header 本身的 CRC32   │
├──────────────────────────────────────┤
│  Firmware Data（变长）                │
│  - 实际的 .bin 二进制数据            │
└──────────────────────────────────────┘
*/

// 固件包头（32字节，packed 防止填充）
#pragma pack(push, 1)
struct FirmwareHeader {
    uint32_t magic;           // 0x4657504B = "FWPK"
    uint8_t  version_major;   // 主版本号
    uint8_t  version_minor;   // 次版本号
    uint16_t version_patch;   // 修订号
    uint16_t device_type;     // 设备型号（防止刷错设备）
    uint16_t flags;           // 标志位（是否加密等）
    uint32_t firmware_size;   // 固件数据字节数
    uint32_t firmware_crc;    // 固件数据 CRC32
    uint32_t header_crc;      // Header 前28字节的 CRC32（最后4字节除外）
    uint32_t reserved;        // 预留
};
#pragma pack(pop)

static_assert(sizeof(FirmwareHeader) == 32, "FirmwareHeader 必须是 32 字节");

constexpr uint32_t FIRMWARE_MAGIC   = 0x4657504B;  // "FWPK"
constexpr uint16_t THIS_DEVICE_TYPE = 0x0001;

// ============================================
// 第四部分：OTA 下载管理器 ⭐⭐⭐⭐⭐
// ============================================

/*
【OTA 下载流程】

分片传输：固件通常几十~几百 KB，不能一次性传输
每片（Chunk）包含：
- 片序号（offset）
- 片数据
- 片 CRC（可选，双重校验）

接收方：
- 按片序号写入备份区（Flash）
- 全部接收后计算整体 CRC
- 向发送方确认
*/

// 模拟 Flash 存储
class MockFlash {
    vector<uint8_t> storage_;
    size_t base_addr_;
public:
    explicit MockFlash(size_t size, size_t base = 0)
        : storage_(size, 0xFF), base_addr_(base) {}

    bool write(uint32_t addr, const uint8_t* data, size_t len) {
        size_t offset = addr - base_addr_;
        if (offset + len > storage_.size()) return false;
        // Flash 写：只能把 1 写成 0（擦除才能把 0 变回 1）
        for (size_t i = 0; i < len; i++) {
            storage_[offset + i] &= data[i];
        }
        return true;
    }

    bool read(uint32_t addr, uint8_t* buf, size_t len) const {
        size_t offset = addr - base_addr_;
        if (offset + len > storage_.size()) return false;
        memcpy(buf, storage_.data() + offset, len);
        return true;
    }

    void erase() { fill(storage_.begin(), storage_.end(), 0xFF); }

    const uint8_t* data() const { return storage_.data(); }
    size_t size() const { return storage_.size(); }
};

// OTA 下载状态
enum class OtaStatus {
    IDLE,
    DOWNLOADING,
    VERIFYING,
    READY,     // 校验通过，等待 Bootloader 安装
    ERROR
};

class OtaDownloader {
    MockFlash& backup_flash_;
    CRC32      crc32_;
    OtaStatus  status_  = OtaStatus::IDLE;
    uint32_t   total_size_  = 0;
    uint32_t   received_    = 0;
    uint32_t   expected_crc_ = 0;
    uint32_t   backup_base_;

    const char* statusName(OtaStatus s) {
        switch (s) {
            case OtaStatus::IDLE:        return "IDLE";
            case OtaStatus::DOWNLOADING: return "DOWNLOADING";
            case OtaStatus::VERIFYING:   return "VERIFYING";
            case OtaStatus::READY:       return "READY";
            case OtaStatus::ERROR:       return "ERROR";
            default:                     return "?";
        }
    }

public:
    OtaDownloader(MockFlash& flash, uint32_t base)
        : backup_flash_(flash), backup_base_(base) {}

    // 开始 OTA：解析 Header，擦除备份区
    bool begin(const FirmwareHeader& hdr) {
        cout << "[OTA] 开始下载，解析固件包头..." << endl;

        // 验证魔数
        if (hdr.magic != FIRMWARE_MAGIC) {
            cout << "  ❌ 魔数错误：0x" << hex << hdr.magic << dec << endl;
            status_ = OtaStatus::ERROR;
            return false;
        }

        // 验证设备类型
        if (hdr.device_type != THIS_DEVICE_TYPE) {
            cout << "  ❌ 设备型号不匹配：" << hdr.device_type << endl;
            status_ = OtaStatus::ERROR;
            return false;
        }

        cout << "  魔数：OK" << endl;
        cout << "  版本：v" << (int)hdr.version_major << "."
                             << (int)hdr.version_minor << "."
                             << hdr.version_patch << endl;
        cout << "  固件大小：" << hdr.firmware_size << " 字节" << endl;
        cout << "  期望 CRC：0x" << hex << setw(8) << setfill('0') << hdr.firmware_crc << dec << endl;

        total_size_   = hdr.firmware_size;
        expected_crc_ = hdr.firmware_crc;
        received_     = 0;

        cout << "  擦除备份区..." << endl;
        backup_flash_.erase();

        crc32_.init();  // 初始化流式 CRC
        status_ = OtaStatus::DOWNLOADING;
        cout << "  ✅ 准备接收固件数据" << endl;
        return true;
    }

    // 接收一片数据
    // offset：在固件中的字节偏移，data：片数据，len：片长度
    bool writeChunk(uint32_t offset, const uint8_t* data, size_t len) {
        if (status_ != OtaStatus::DOWNLOADING) return false;
        if (offset + len > total_size_) {
            cout << "  ❌ 数据超出固件大小" << endl;
            status_ = OtaStatus::ERROR;
            return false;
        }

        // 写入 Flash 备份区
        uint32_t flash_addr = backup_base_ + offset;
        if (!backup_flash_.write(flash_addr, data, len)) {
            cout << "  ❌ Flash 写入失败" << endl;
            status_ = OtaStatus::ERROR;
            return false;
        }

        // 更新流式 CRC
        crc32_.update(data, len);
        received_ += len;

        // 进度显示
        int progress = (int)(received_ * 100 / total_size_);
        cout << "  [" << setw(3) << progress << "%] 已接收 "
             << received_ << "/" << total_size_ << " 字节" << endl;

        return true;
    }

    // 校验完整固件
    bool verify() {
        if (status_ != OtaStatus::DOWNLOADING) return false;
        if (received_ != total_size_) {
            cout << "  ❌ 数据不完整：期望 " << total_size_
                 << " 字节，实际 " << received_ << " 字节" << endl;
            status_ = OtaStatus::ERROR;
            return false;
        }

        status_ = OtaStatus::VERIFYING;
        uint32_t actual_crc = crc32_.final();

        cout << "[OTA] 校验..." << endl;
        cout << "  期望 CRC：0x" << hex << setw(8) << setfill('0') << expected_crc_ << dec << endl;
        cout << "  实际 CRC：0x" << hex << setw(8) << setfill('0') << actual_crc    << dec << endl;

        if (actual_crc != expected_crc_) {
            cout << "  ❌ CRC 不匹配，固件损坏！" << endl;
            status_ = OtaStatus::ERROR;
            return false;
        }

        cout << "  ✅ CRC 校验通过" << endl;
        status_ = OtaStatus::READY;

        // 真实场景：此处写升级标志到参数区
        // 通知 Bootloader：下次重启时安装备份区的固件
        cout << "[OTA] 写升级标志到参数区，下次重启将安装新固件" << endl;
        return true;
    }

    OtaStatus status() const { return status_; }
};

// ============================================
// 第五部分：回滚机制 ⭐⭐⭐⭐
// ============================================

/*
【回滚（Rollback）机制】

为什么需要回滚？
- 新固件有严重 Bug，设备启动后立刻崩溃
- 新固件与硬件不兼容
- 升级中断（断电），备份区数据不完整

回滚策略：
方案1：心跳确认（推荐）
  - App 启动后，30秒内必须调用 confirmUpdate()
  - 超时未确认 → Bootloader 认为新固件不可用 → 回滚

方案2：启动计数
  - Bootloader 记录"本固件已尝试启动次数"
  - 超过 3 次未成功确认 → 回滚

方案3：CRC 二次验证
  - App 启动时再次校验自身 CRC
  - 校验失败 → 立即触发回滚
*/

class RollbackManager {
    int  boot_count_      = 0;
    bool update_confirmed_ = false;
    static constexpr int MAX_BOOT_ATTEMPTS = 3;

public:
    // Bootloader 调用：记录本次启动
    void incrementBootCount() {
        boot_count_++;
        cout << "[Rollback] 启动计数：" << boot_count_
             << "/" << MAX_BOOT_ATTEMPTS << endl;
    }

    // App 调用：确认新固件可用
    void confirmUpdate() {
        update_confirmed_ = true;
        boot_count_ = 0;  // 重置计数
        cout << "[Rollback] 新固件已确认，清除回滚标志" << endl;
    }

    // Bootloader 调用：是否需要回滚
    bool shouldRollback() const {
        return boot_count_ >= MAX_BOOT_ATTEMPTS && !update_confirmed_;
    }

    void demo() {
        cout << "\n" << string(60, '-') << endl;
        cout << "回滚机制演示" << endl;
        cout << string(60, '-') << endl;

        // 模拟：新固件有 Bug，3次启动都没有确认
        for (int i = 1; i <= 3; i++) {
            cout << "\n第 " << i << " 次启动新固件..." << endl;
            incrementBootCount();

            if (i < 3) {
                cout << "  App 崩溃，未能调用 confirmUpdate()" << endl;
            }
        }

        if (shouldRollback()) {
            cout << "\n[Bootloader] 检测到连续启动失败，触发回滚！" << endl;
            cout << "  → 切换到备份固件（旧版本）" << endl;
            cout << "  → 设备恢复正常运行" << endl;
        }
    }
};

// ============================================
// 第六部分：完整 OTA 流程演示
// ============================================

void demoFullOTA() {
    cout << "\n" << string(60, '=') << endl;
    cout << "完整 OTA 升级流程演示" << endl;
    cout << string(60, '=') << endl;

    // 模拟备份区 Flash（1KB 演示用）
    MockFlash backup_flash(1024, 0x08038000);
    OtaDownloader ota(backup_flash, 0x08038000);
    CRC32 crc32;

    // 模拟固件数据（64字节）
    vector<uint8_t> firmware(64);
    for (int i = 0; i < 64; i++) firmware[i] = (uint8_t)(i * 3 + 7);

    uint32_t fw_crc = crc32.calculate(firmware.data(), firmware.size());

    // 构造固件包头
    FirmwareHeader hdr{};
    hdr.magic          = FIRMWARE_MAGIC;
    hdr.version_major  = 2;
    hdr.version_minor  = 0;
    hdr.version_patch  = 0;
    hdr.device_type    = THIS_DEVICE_TYPE;
    hdr.firmware_size  = (uint32_t)firmware.size();
    hdr.firmware_crc   = fw_crc;

    cout << "\n步骤1：接收固件包头" << endl;
    if (!ota.begin(hdr)) {
        cout << "OTA 失败：Header 无效" << endl;
        return;
    }

    cout << "\n步骤2：分片传输固件数据（每片 16 字节）" << endl;
    constexpr size_t CHUNK_SIZE = 16;
    for (size_t offset = 0; offset < firmware.size(); offset += CHUNK_SIZE) {
        size_t len = min(CHUNK_SIZE, firmware.size() - offset);
        if (!ota.writeChunk((uint32_t)offset, firmware.data() + offset, len)) {
            cout << "OTA 失败：写入错误" << endl;
            return;
        }
    }

    cout << "\n步骤3：校验固件" << endl;
    if (!ota.verify()) {
        cout << "OTA 失败：校验错误" << endl;
        return;
    }

    cout << "\n步骤4：重启，Bootloader 执行安装" << endl;
    cout << "  → 擦除 App 区" << endl;
    cout << "  → 从备份区复制到 App 区" << endl;
    cout << "  → 清除升级标志" << endl;
    cout << "  → 跳转到新版本 App" << endl;
    cout << "\n✅ OTA 升级完成！设备运行 v2.0.0" << endl;

    // 演示回滚
    RollbackManager rb;
    rb.demo();
}

// ============================================
// 第七部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：直接往 App 区写新固件（不用备份区）
问题：写到一半断电 → App 损坏 → 设备变砖

✅ 正确：先写备份区 → 校验 → 再由 Bootloader 原子性安装

────────────────────────────────────────────

❌ 错误2：只校验片段 CRC，不校验整体 CRC
问题代码：每片传输时只校验当前片，没有最终整体校验

结果：单片数据正确，但整体顺序错乱（乱序重组），整体 CRC 不对却没发现

✅ 正确：
  - 片级 CRC：快速发现单片传输错误，及时重传
  - 整体 CRC：写完全部数据后，对整个固件再算一次

────────────────────────────────────────────

❌ 错误3：没有验证目标设备型号
问题代码：没有 device_type 字段，任何设备都接受任何固件

结果：把 A 型号固件（不同外设配置）刷到 B 型号 → 外设初始化错误 → 崩溃

✅ 正确：固件包 Header 中包含 device_type，接收方强制检查

────────────────────────────────────────────

❌ 错误4：没有回滚机制
问题：新固件有严重 Bug，启动即崩溃，设备无法使用

✅ 正确：
  - 保留备份区的旧固件不立即覆盖
  - App 启动后 30 秒内调用 confirmUpdate()
  - 超时未确认 → Bootloader 自动回滚

────────────────────────────────────────────

❌ 错误5：OTA 期间不检查剩余 Flash 空间
问题代码：直接写入，没有预先检查

结果：固件太大，写到 Flash 末尾溢出，覆盖参数区 → 升级标志损坏

✅ 正确：begin() 时检查 firmware_size <= backup_area_size

────────────────────────────────────────────

❌ 错误6：固件包没有签名，任意数据都能刷入
问题：攻击者构造恶意固件，通过 OTA 接口刷入

✅ 正确（生产环境必须）：
  - 固件包用私钥签名（RSA/ECDSA）
  - 设备存储公钥，接收时验证签名
  - CRC 只能防传输错误，不能防篡改！

────────────────────────────────────────────

❌ 错误7：不检查版本号（允许降级攻击）
问题：攻击者发送旧版本固件（已知有漏洞的版本）

✅ 正确：
  - 记录当前运行版本
  - 新固件版本号必须 >= 当前版本
  - 生产设备禁止降级
)" << endl;
}

// ============================================
// 第八部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【CRC32::calculate(data, len)】
作用：一次性计算整块数据的 CRC32
参数：
  data (const uint8_t*)  — 数据起始指针
  len  (size_t)          — 数据字节数
返回：uint32_t — CRC32 值
示例：
  CRC32 crc;
  uint32_t result = crc.calculate(firmware.data(), firmware.size());

────────────────────────────────────

【CRC32 流式接口（大数据分片计算）】
语法：
  CRC32 crc;
  crc.init();                          // 初始化
  crc.update(chunk1_data, chunk1_len); // 喂入第1片
  crc.update(chunk2_data, chunk2_len); // 喂入第2片
  uint32_t result = crc.final();       // 获取最终结果

────────────────────────────────────

【OtaDownloader::begin(header)】
作用：解析固件包头，准备接收
参数：FirmwareHeader — 固件包头结构体
返回：bool — 是否成功（魔数/设备型号验证）

【OtaDownloader::writeChunk(offset, data, len)】
作用：写入一片固件数据
参数：
  offset — 在固件中的字节偏移
  data   — 片数据指针
  len    — 片长度
返回：bool — 是否成功

【OtaDownloader::verify()】
作用：接收完成后校验整体 CRC
返回：bool — 校验是否通过

────────────────────────────────────

【#pragma pack(push, 1) / pop】
作用：取消结构体字节对齐，让成员紧密排列
用途：网络/Flash 协议结构体，确保大小精确
示例：
  #pragma pack(push, 1)
  struct Header { uint8_t a; uint32_t b; };  // 大小 = 5，不是 8
  #pragma pack(pop)
)" << endl;
}

// ============================================
// 第九部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. CRC32 验证
   用 CRC32 类计算字符串 "Hello, Embedded!" 的 CRC32 值。
   然后修改其中一个字节，观察 CRC32 值的变化。
   思考：修改1位数据，CRC32 会有多大变化？为什么说 CRC 是"雪崩效应"？

2. 固件包头设计扩展
   在 FirmwareHeader 中增加以下字段（保持总大小 64 字节）：
   - build_timestamp（uint32_t）：编译时间戳
   - min_bootloader_version（uint16_t）：要求的最低 Bootloader 版本
   - max_retry（uint8_t）：最大重试次数
   注意：#pragma pack(1) 和 static_assert 的配合使用

3. 断点续传
   当前 OtaDownloader 不支持断点续传（断电后从头重传）。
   设计一个机制，使得重启后可以从断点继续：
   - 在 Flash 参数区记录"已接收字节数"
   - 重启后，发送方从断点偏移继续发送
   提示：Flash 记录的内容如何防止掉电时写到一半导致的数据不一致？

4. 双 CRC 设计
   实现 verifyWithDualCRC()：
   - 第1次：对备份区 Flash 数据计算 CRC（读 Flash 验证写入无误）
   - 第2次：与固件包头中的期望 CRC 比对
   为什么要两次 CRC？第1次和第2次分别能发现什么问题？

5. 版本比较函数
   实现 bool isNewerVersion(uint8_t ma, uint8_t mi, uint16_t pa,
                             uint8_t cb, uint8_t cmi, uint16_t cpa)：
   检查新固件版本（ma.mi.pa）是否 > 当前版本（cb.cmi.cpa）
   规则：主版本优先，主版本相同比次版本，次版本相同比修订号
)" << endl;
}

int main() {
    cout << string(60, '=') << endl;
    cout << "17_ota_firmware_update.cpp — OTA 固件升级机制" << endl;
    cout << "日期：2026-06-04（第61天）" << endl;
    cout << string(60, '=') << endl;

    // CRC32 演示
    cout << "\n【CRC32 演示】" << endl;
    CRC32 crc;
    const char* msg = "Hello, OTA!";
    uint32_t result = crc.calculate((const uint8_t*)msg, strlen(msg));
    cout << "  \"" << msg << "\" 的 CRC32 = 0x"
         << hex << setw(8) << setfill('0') << result << dec << endl;

    // 修改一字节后 CRC 变化
    string msg2 = "Hello, OTB!";  // 把 A 改成 B
    uint32_t result2 = crc.calculate((const uint8_t*)msg2.c_str(), msg2.size());
    cout << "  \"" << msg2 << "\" 的 CRC32 = 0x"
         << hex << setw(8) << setfill('0') << result2 << dec << endl;
    cout << "  （改变1字节，CRC32 完全不同 — 雪崩效应）" << endl;

    // FirmwareHeader 大小确认
    cout << "\n【FirmwareHeader 大小】：" << sizeof(FirmwareHeader) << " 字节" << endl;

    // 完整 OTA 流程
    demoFullOTA();

    // 常见错误
    showCommonErrors();

    // 函数卡片
    showFunctionCards();

    // 练习题
    showExercises();

    return 0;
}
