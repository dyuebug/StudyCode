// ============================================
// 16_bootloader_basics.cpp
// 日期：2026-06-04（第61天）
// 主题：Bootloader 原理与跳转机制
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -o 16_bootloader 16_bootloader_basics.cpp
// ============================================

/*
【核心概念】

是什么？
- Bootloader 是存放在 Flash 最低地址的一段小程序
- 它在主应用程序之前运行，负责"决定启动哪个程序"
- 类比：电脑的 BIOS/UEFI，先于操作系统启动

为什么需要？
- 支持固件升级（OTA）：升级期间 Bootloader 保持完整，不会砖机
- 支持多镜像：主程序 / 备份程序 / 工厂程序
- 支持完整性校验：CRC 校验通过才跳转，防止运行损坏固件
- 支持恢复模式：长按按键进入升级模式

核心特点：
- 体积极小（通常 < 32KB）
- 永远不会被 OTA 覆盖（位于 Flash 保护区）
- 执行完成后跳转到 App，不再返回
*/

#include <iostream>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <iomanip>
using namespace std;

// ============================================
// 第一部分：Flash 内存映射 ⭐⭐⭐⭐⭐
// ============================================

/*
【STM32 典型 Flash 分区布局】

Flash 起始地址：0x08000000（STM32 典型值）

┌─────────────────────────────────┐  0x08000000
│  Bootloader 区（32KB）           │  ← 上电后 CPU 从这里开始执行
│  - 向量表                        │
│  - 升级状态检查                   │
│  - 跳转逻辑                      │
├─────────────────────────────────┤  0x08008000
│  App 区（主程序，192KB）          │  ← Bootloader 跳转到这里
│  - App 向量表                    │
│  - 主程序代码                    │
│  - 只读数据                      │
├─────────────────────────────────┤  0x08038000
│  备份 App 区（192KB）            │  ← OTA 下载到这里，校验后交换
├─────────────────────────────────┤  0x08068000
│  参数存储区（32KB）              │  ← 升级标志、版本号、CRC 值
└─────────────────────────────────┘  0x08070000（448KB Flash 结束）

关键地址：
- App 向量表偏移 = 0x08008000
- SCB->VTOR 必须设置为 App 向量表地址
*/

// 模拟 Flash 地址常量
namespace FlashMap {
    constexpr uint32_t FLASH_BASE        = 0x08000000;
    constexpr uint32_t BOOTLOADER_BASE   = 0x08000000;  // Bootloader 起始
    constexpr uint32_t BOOTLOADER_SIZE   = 0x8000;      // 32KB
    constexpr uint32_t APP_BASE          = 0x08008000;  // App 起始
    constexpr uint32_t APP_SIZE          = 0x30000;     // 192KB
    constexpr uint32_t APP_BACKUP_BASE   = 0x08038000;  // 备份区
    constexpr uint32_t PARAM_BASE        = 0x08068000;  // 参数区
    constexpr uint32_t PARAM_SIZE        = 0x8000;      // 32KB
}

// ============================================
// 第二部分：向量表结构 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：向量表（Vector Table）】

是什么？
- ARM Cortex-M 的向量表是一个函数指针数组
- 存放在 Flash 最低地址
- 第0项：MSP 初始值（主堆栈指针初始值）
- 第1项：Reset_Handler（复位后第一条指令地址）
- 第2项以后：各种中断处理函数地址

为什么 Bootloader 跳转时要重映射向量表？
- App 有自己的向量表，里面是 App 的中断处理函数
- 如果不重映射，中断发生时会跳到 Bootloader 的中断函数，逻辑错误
- SCB->VTOR = App 向量表地址，告诉 CPU "中断向量表在这里"

STM32 向量表最小结构：
*/

// 模拟向量表（前4项）
struct VectorTable {
    uint32_t initial_sp;       // 初始堆栈指针
    uint32_t reset_handler;    // 复位处理函数地址
    uint32_t nmi_handler;      // NMI 中断地址
    uint32_t hardfault_handler;// HardFault 地址
    // ... 后续还有几十个中断向量
};

// ============================================
// 第三部分：跳转到 App ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：跳转到 App 的核心步骤】

跳转步骤（必须按顺序！）：
1. 关闭所有中断（防止跳转途中被中断打断）
2. 关闭 SysTick（防止 SysTick 中断干扰）
3. 重映射向量表（SCB->VTOR = App 向量表地址）
4. 读取 App 的初始堆栈指针（向量表第0项）
5. 设置主堆栈指针 MSP
6. 读取 App 的 Reset_Handler 地址（向量表第1项）
7. 用函数指针调用 Reset_Handler → 正式跳入 App

注意：
- 跳转后不再返回 Bootloader
- App 的 Reset_Handler 会初始化 BSS、复制 data 段，然后调用 main()
*/

// 模拟跳转函数（真实代码需在 ARM 上运行）
class Bootloader {
public:
    // 升级标志（存储在 Flash 参数区）
    enum class BootFlag : uint32_t {
        NORMAL      = 0xAAAAAAAA,  // 正常启动 App
        OTA_PENDING = 0xBBBBBBBB, // 有新固件等待安装
        FACTORY     = 0xCCCCCCCC, // 进入工厂模式
        INVALID     = 0xFFFFFFFF  // Flash 未初始化（全F）
    };

    // 模拟：检查是否需要进入升级模式
    bool shouldEnterUpdateMode() {
        // 真实实现：读 GPIO（按键）或读 Flash 参数区的标志
        return false; // 模拟：正常启动
    }

    // 模拟：验证 App 完整性
    bool verifyApp(uint32_t app_base) {
        cout << "[Bootloader] 验证 App 完整性..." << endl;
        cout << "  App 起始地址：0x" << hex << app_base << dec << endl;

        // 真实实现：
        // 1. 读取 App 大小（存在参数区）
        // 2. 计算 CRC32
        // 3. 与存储的期望 CRC 比对

        // 模拟验证通过
        cout << "  CRC32 校验：PASS ✅" << endl;
        return true;
    }

    // 模拟：跳转到 App
    void jumpToApp(uint32_t app_base) {
        cout << "[Bootloader] 准备跳转到 App..." << endl;
        cout << "  1. 关闭所有中断（__disable_irq）" << endl;
        cout << "  2. 关闭 SysTick（SysTick->CTRL = 0）" << endl;
        cout << "  3. 重映射向量表：SCB->VTOR = 0x" << hex << app_base << dec << endl;

        // 模拟读取向量表
        // 真实代码：
        // uint32_t app_sp  = *(uint32_t*)(app_base + 0);  // 第0项：初始 SP
        // uint32_t app_pc  = *(uint32_t*)(app_base + 4);  // 第1项：Reset_Handler

        uint32_t app_sp  = 0x20020000;  // 模拟：SRAM 顶部
        uint32_t app_pc  = app_base + 0x100; // 模拟：Reset_Handler 地址

        cout << "  4. 设置 MSP：0x" << hex << app_sp << dec << endl;
        cout << "  5. 跳转 PC → 0x" << hex << app_pc << dec << endl;

        // 真实代码（ARM 汇编宏）：
        // __set_MSP(app_sp);
        // typedef void (*AppEntry)(void);
        // AppEntry entry = (AppEntry)app_pc;
        // entry();  // 不会返回

        cout << "[App] === App 开始运行 ===" << endl;
    }

    // Bootloader 主流程
    void run() {
        cout << string(60, '=') << endl;
        cout << "=== Bootloader 启动 ===" << endl;
        cout << string(60, '=') << endl;

        cout << "\n[1] 检查硬件按键..." << endl;
        if (shouldEnterUpdateMode()) {
            cout << "  → 进入升级模式（等待新固件）" << endl;
            // enterUpdateMode();
            return;
        }
        cout << "  → 正常启动" << endl;

        cout << "\n[2] 检查升级标志..." << endl;
        BootFlag flag = BootFlag::NORMAL; // 模拟读取
        if (flag == BootFlag::OTA_PENDING) {
            cout << "  → 发现待安装固件，执行 OTA 安装..." << endl;
            // installOtaFirmware();
        } else {
            cout << "  → 标志：NORMAL，直接启动 App" << endl;
        }

        cout << "\n[3] 验证 App..." << endl;
        if (!verifyApp(FlashMap::APP_BASE)) {
            cout << "  ❌ App 校验失败！尝试恢复备份..." << endl;
            // tryRestoreBackup();
            return;
        }

        cout << "\n[4] 跳转到 App..." << endl;
        jumpToApp(FlashMap::APP_BASE);
    }
};

// ============================================
// 第四部分：升级状态机 ⭐⭐⭐⭐
// ============================================

/*
【升级状态机设计】

Bootloader 升级流程的状态转换：

  上电
   ↓
[IDLE] ──────────────────────────────→ [JUMP_TO_APP]
   │  无升级标志                              ↑
   │  有升级标志                              │ 校验通过
   ↓                                          │
[ERASE_BACKUP] → [RECEIVE_FW] → [VERIFY_FW] ─┘
                                    │
                                    │ 校验失败
                                    ↓
                                [ERROR] → 保留旧版本 → [JUMP_TO_APP]
*/

enum class BootState {
    IDLE,           // 空闲，等待决策
    CHECK_FLAG,     // 检查升级标志
    ERASE_BACKUP,   // 擦除备份区
    RECEIVE_FW,     // 接收新固件
    VERIFY_FW,      // 校验固件
    INSTALL_FW,     // 安装固件（备份区 → 主 App 区）
    JUMP_TO_APP,    // 跳转 App
    ERROR           // 错误，回滚
};

class BootStateMachine {
    BootState state_ = BootState::IDLE;
    int retry_count_ = 0;
    static constexpr int MAX_RETRY = 3;

    const char* stateName(BootState s) {
        switch (s) {
            case BootState::IDLE:         return "IDLE";
            case BootState::CHECK_FLAG:   return "CHECK_FLAG";
            case BootState::ERASE_BACKUP: return "ERASE_BACKUP";
            case BootState::RECEIVE_FW:   return "RECEIVE_FW";
            case BootState::VERIFY_FW:    return "VERIFY_FW";
            case BootState::INSTALL_FW:   return "INSTALL_FW";
            case BootState::JUMP_TO_APP:  return "JUMP_TO_APP";
            case BootState::ERROR:        return "ERROR";
            default:                      return "UNKNOWN";
        }
    }

    void transition(BootState next) {
        cout << "  状态转换：" << stateName(state_)
             << " → " << stateName(next) << endl;
        state_ = next;
    }

public:
    void run() {
        cout << "\n" << string(60, '=') << endl;
        cout << "升级状态机演示" << endl;
        cout << string(60, '=') << endl;

        // 模拟：有新固件等待安装
        bool has_new_firmware = true;
        bool verify_ok = true;

        while (true) {
            switch (state_) {
                case BootState::IDLE:
                    transition(BootState::CHECK_FLAG);
                    break;

                case BootState::CHECK_FLAG:
                    if (has_new_firmware) {
                        cout << "  发现新固件标志" << endl;
                        transition(BootState::ERASE_BACKUP);
                    } else {
                        transition(BootState::JUMP_TO_APP);
                    }
                    break;

                case BootState::ERASE_BACKUP:
                    cout << "  擦除备份区（耗时约500ms）" << endl;
                    transition(BootState::RECEIVE_FW);
                    break;

                case BootState::RECEIVE_FW:
                    cout << "  接收固件数据（分片传输）" << endl;
                    transition(BootState::VERIFY_FW);
                    break;

                case BootState::VERIFY_FW:
                    cout << "  校验 CRC32..." << endl;
                    if (verify_ok) {
                        transition(BootState::INSTALL_FW);
                    } else {
                        retry_count_++;
                        if (retry_count_ >= MAX_RETRY) {
                            transition(BootState::ERROR);
                        } else {
                            cout << "  校验失败，重试 " << retry_count_ << "/" << MAX_RETRY << endl;
                            transition(BootState::RECEIVE_FW);
                        }
                    }
                    break;

                case BootState::INSTALL_FW:
                    cout << "  安装固件到 App 区..." << endl;
                    cout << "  清除升级标志" << endl;
                    transition(BootState::JUMP_TO_APP);
                    break;

                case BootState::JUMP_TO_APP:
                    cout << "  → 跳转到 App，启动！" << endl;
                    return;

                case BootState::ERROR:
                    cout << "  ❌ 升级失败，回滚到上一版本" << endl;
                    return;
            }
        }
    }
};

// ============================================
// 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：跳转前没有关闭中断
问题代码：
  jumpToApp(APP_BASE);  // 直接跳转

结果：SysTick 中断在跳转途中触发，跳到 Bootloader 的 SysTick 处理函数
     → 地址错误，HardFault，程序崩溃

✅ 正确：
  __disable_irq();         // 先关中断
  SysTick->CTRL = 0;       // 关 SysTick
  SCB->VTOR = APP_BASE;    // 重映射向量表
  __set_MSP(app_sp);       // 设置堆栈
  entry();                 // 再跳转

────────────────────────────────────────────

❌ 错误2：Bootloader 跳转后忘记重映射向量表
问题代码：
  entry();  // 没有设置 SCB->VTOR

结果：App 中断发生时，CPU 仍用 Bootloader 的向量表查找处理函数
     → 跳到 Bootloader 的中断函数，逻辑完全错误

✅ 正确：SCB->VTOR = APP_BASE; 必须在跳转前设置

────────────────────────────────────────────

❌ 错误3：App 起始地址没有 4 字节对齐
问题代码：
  #define APP_BASE 0x08007001  // 奇数地址！

结果：ARM Cortex-M 向量表必须按 2^N 字节对齐（通常256字节），
     奇数地址触发 HardFault

✅ 正确：APP_BASE 必须 >= 256 字节对齐，且 Flash 扇区对齐

────────────────────────────────────────────

❌ 错误4：Bootloader 校验通过但 App 区仍是空白（全 0xFF）
问题代码：
  if (crc_ok) jumpToApp();  // 只校验 CRC，没检查内容

结果：全 0xFF 的 Flash 的 Reset_Handler 地址也是 0xFFFFFFFF，
     跳到 0xFFFFFFFF → 立即 HardFault

✅ 正确：额外检查向量表第1项（Reset_Handler）是否在合法范围内：
  uint32_t reset_vec = *(uint32_t*)(APP_BASE + 4);
  if (reset_vec < APP_BASE || reset_vec > APP_BASE + APP_SIZE) {
      // App 无效
  }

────────────────────────────────────────────

❌ 错误5：OTA 下载到 App 区（而不是备份区）
问题代码：
  writeFlash(APP_BASE, new_firmware);  // 直接覆盖 App

结果：下载到一半断电或校验失败 → App 损坏，无法启动，永久变砖！

✅ 正确：
  1. 下载到备份区（APP_BACKUP_BASE）
  2. 校验备份区的 CRC
  3. 校验通过后，从备份区复制到 App 区
  4. 全程 Bootloader 保持完整，可以回滚

────────────────────────────────────────────

❌ 错误6：Bootloader 自身被 OTA 覆盖
问题代码：
  // 没有设置 Flash 写保护
  writeFlash(BOOTLOADER_BASE, new_firmware);  // 覆盖 Bootloader！

结果：Bootloader 损坏，设备永久变砖，只能通过 JTAG/SWD 恢复

✅ 正确：
  - 设置 Flash 写保护（FLASH->WRPR 寄存器）
  - OTA 程序禁止写 Bootloader 地址范围
  - Bootloader 区域加 Option Bytes 保护

────────────────────────────────────────────

❌ 错误7：两个程序共用同一段 RAM，变量互相覆盖
问题代码：
  // Bootloader 和 App 的链接脚本都使用 0x20000000 开始的全部 RAM

结果：Bootloader 的局部变量可能被 App 的 .bss 初始化清零
     （App 启动时会清零 BSS 段）

✅ 正确：
  - Bootloader 的堆栈放在 RAM 底部（低地址）
  - App 的 RAM 从高于 Bootloader 堆栈的地址开始
  - 或者：Bootloader 用 NOINIT 段保留升级状态变量
)" << endl;
}

// ============================================
// 第六部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【SCB->VTOR — 向量表偏移寄存器】
作用：告诉 CPU 向量表在哪里
语法：SCB->VTOR = (uint32_t)app_vector_table_address;
要求：地址必须 >= 256 字节对齐
示例：SCB->VTOR = 0x08008000;  // App 在 32KB 偏移处

────────────────────────────────────

【__disable_irq() / __enable_irq()】
作用：关闭/开启所有可屏蔽中断（PRIMASK 寄存器）
语法：__disable_irq();  // 关中断
      __enable_irq();   // 开中断
注意：NMI 和 HardFault 不受影响

────────────────────────────────────

【__set_MSP(uint32_t topOfMainStack)】
作用：设置主堆栈指针 MSP
语法：__set_MSP(initial_sp_value);
示例：
  uint32_t app_sp = *(uint32_t*)APP_BASE;  // 读向量表第0项
  __set_MSP(app_sp);

────────────────────────────────────

【函数指针跳转（App 入口）】
语法：
  typedef void (*AppEntry)(void);
  uint32_t reset_addr = *(uint32_t*)(APP_BASE + 4);  // 向量表第1项
  AppEntry entry = (AppEntry)reset_addr;
  entry();  // 跳转，不返回

────────────────────────────────────

【HAL_FLASH_Unlock / Lock】
作用：解锁/锁定 Flash 写操作
语法：
  HAL_FLASH_Unlock();          // 解锁
  HAL_FLASH_Program(...);      // 写入
  HAL_FLASH_Lock();            // 锁定（重要！）
注意：写完必须锁定，防止意外写入

────────────────────────────────────

【HAL_FLASHEx_Erase】
作用：按扇区擦除 Flash
参数：
  FLASH_EraseInitTypeDef erase_cfg;
  erase_cfg.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_cfg.Sector    = 扇区编号;
  erase_cfg.NbSectors = 擦除扇区数;
  erase_cfg.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  uint32_t error;
  HAL_FLASHEx_Erase(&erase_cfg, &error);
)" << endl;
}

// ============================================
// 第七部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 分区计算
   一块 STM32 有 512KB Flash（起始 0x08000000）。
   请设计分区：Bootloader(32KB) + App(224KB) + 备份区(224KB) + 参数区(32KB)
   计算每个区域的起始地址和结束地址。
   提示：32KB = 0x8000，224KB = 0x38000

2. 验证函数设计
   实现 bool isAppValid(uint32_t app_base) 函数：
   - 检查向量表第0项（初始 SP）是否在 SRAM 范围内（0x20000000 ~ 0x20020000）
   - 检查向量表第1项（Reset_Handler）是否在 App Flash 范围内
   - 两项都满足才返回 true
   提示：*(uint32_t*)(app_base + 0) 读取第0项

3. 升级标志设计
   在参数区设计一个"升级控制块"结构体，包含：
   - magic（魔数，用于判断是否有效）
   - boot_flag（启动标志：正常/OTA待安装）
   - firmware_size（新固件大小）
   - firmware_crc（新固件 CRC32）
   - version（新固件版本号）
   思考：magic 用什么值合适？为什么？

4. 状态机扩展
   在 BootStateMachine 中增加一个 WATCHDOG_FEED 状态：
   在 ERASE_BACKUP 和 RECEIVE_FW 期间每 100ms 喂一次看门狗
   提示：擦写 Flash 可能耗时较长，看门狗超时会复位

5. 防降级保护
   在 verifyApp() 中增加版本号检查：
   - 读取当前运行的 App 版本号（存在参数区）
   - 读取新固件的版本号
   - 如果新版本号 < 当前版本号，拒绝安装（防止攻击者回滚到有漏洞的旧版本）
   思考：什么情况下合法用户需要降级？如何设计"允许降级"的授权机制？
)" << endl;
}

int main() {
    cout << string(60, '=') << endl;
    cout << "16_bootloader_basics.cpp — Bootloader 原理与跳转" << endl;
    cout << "日期：2026-06-04（第61天）" << endl;
    cout << string(60, '=') << endl;

    // 演示内存映射
    cout << "\n【Flash 分区地址】" << endl;
    cout << "  Bootloader:  0x" << hex << setw(8) << setfill('0') << FlashMap::BOOTLOADER_BASE
         << " ~ 0x" << setw(8) << setfill('0') << FlashMap::BOOTLOADER_BASE + FlashMap::BOOTLOADER_SIZE - 1
         << "  (32KB)" << dec << endl;
    cout << "  App:         0x" << hex << setw(8) << setfill('0') << FlashMap::APP_BASE
         << " ~ 0x" << setw(8) << setfill('0') << FlashMap::APP_BASE + FlashMap::APP_SIZE - 1
         << "  (192KB)" << dec << endl;
    cout << "  App Backup:  0x" << hex << setw(8) << setfill('0') << FlashMap::APP_BACKUP_BASE
         << "  (192KB)" << dec << endl;
    cout << "  Parameters:  0x" << hex << setw(8) << setfill('0') << FlashMap::PARAM_BASE
         << "  (32KB)" << dec << endl;

    // 演示 Bootloader 流程
    cout << endl;
    Bootloader bl;
    bl.run();

    // 演示状态机
    BootStateMachine sm;
    sm.run();

    // 常见错误
    showCommonErrors();

    // 函数卡片
    showFunctionCards();

    // 练习题
    showExercises();

    return 0;
}
