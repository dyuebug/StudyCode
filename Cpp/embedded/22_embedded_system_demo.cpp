// ============================================
// 22_embedded_system_demo.cpp
// 日期：2026-06-07（第63天）
// 主题：嵌入式系统综合实战 - IoT节点模拟
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -o 22_demo 22_embedded_system_demo.cpp
// ============================================

/*
【核心概念】

是什么？
- 一个完整的IoT节点模拟程序，综合运用阶段11所有知识
- 模拟真实嵌入式设备的软件架构

集成的模块：
- UART命令解析（来自11_uart_driver）
- Modbus从站响应（来自20_modbus_rtu）
- FreeRTOS多任务框架（来自21_freertos_advanced）
- 看门狗保护（来自18_debug_techniques）
- OTA状态机（来自17_ota_firmware_update）
- 低功耗事件驱动（来自08_low_power）

架构图：
  ┌─────────────────────────────────────┐
  │           IoT节点主循环              │
  │  ┌──────────┐  ┌──────────────────┐ │
  │  │ 传感器任务 │  │  通信任务(Modbus) │ │
  │  └────┬─────┘  └────────┬─────────┘ │
  │       │   事件组同步      │           │
  │  ┌────▼─────────────────▼─────────┐ │
  │  │        系统管理任务              │ │
  │  │  看门狗 + OTA状态机 + 低功耗    │ │
  │  └────────────────────────────────┘ │
  └─────────────────────────────────────┘
*/

#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>
#include <functional>
#include <array>
#include <vector>
#include <map>
using namespace std;

// ============================================
// 第一部分：基础类型与常量定义
// ============================================

// 固定宽度整数（嵌入式标准做法）
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

// 系统时间（模拟HAL_GetTick）
static u32 g_tick = 0;
u32 HAL_GetTick() { return g_tick; }
void HAL_Delay(u32 ms) { g_tick += ms; }

// ============================================
// 第二部分：FreeRTOS 任务框架（简化模拟）
// ============================================

/*
【概念】事件组
- 32位标志位，每一位代表一个事件
- 多个任务可以等待不同位的组合
- 比多个信号量更节省内存
*/

struct EventGroup {
    u32 bits = 0;

    // 置位事件（模拟 xEventGroupSetBits）
    void setBits(u32 mask) { bits |= mask; }

    // 清除事件（模拟 xEventGroupClearBits）
    void clearBits(u32 mask) { bits &= ~mask; }

    // 等待事件（模拟 xEventGroupWaitBits）
    // waitAll=true：所有位都置位才返回
    // waitAll=false：任一位置位即返回
    bool waitBits(u32 mask, bool waitAll) const {
        if (waitAll) return (bits & mask) == mask;
        return (bits & mask) != 0;
    }
};

// 系统事件标志位定义
constexpr u32 EVT_SENSOR_READY   = (1 << 0);  // 传感器数据就绪
constexpr u32 EVT_CMD_RECEIVED   = (1 << 1);  // 收到UART命令
constexpr u32 EVT_OTA_REQUEST    = (1 << 2);  // 收到OTA升级请求
constexpr u32 EVT_WATCHDOG_WARN  = (1 << 3);  // 看门狗即将超时

EventGroup g_events;  // 全局事件组

// ============================================
// 第三部分：传感器模块
// ============================================

struct SensorData {
    float temperature = 0.0f;  // 温度（℃）
    float humidity    = 0.0f;  // 湿度（%RH）
    u16   adc_raw     = 0;     // ADC原始值
    u32   timestamp   = 0;     // 采集时间戳
};

class SensorTask {
public:
    // 模拟采集传感器数据（真实场景：读ADC、I2C传感器）
    SensorData sample() {
        SensorData data;
        data.temperature = 25.0f + (HAL_GetTick() % 100) * 0.1f;  // 25~35℃
        data.humidity    = 60.0f + (HAL_GetTick() % 40)  * 0.5f;  // 60~80%
        data.adc_raw     = 2048 + (HAL_GetTick() % 512);           // 12位ADC
        data.timestamp   = HAL_GetTick();
        return data;
    }

    // 任务入口（每500ms采集一次）
    void run(SensorData& out) {
        static u32 last_tick = 0;
        if (HAL_GetTick() - last_tick >= 500) {
            out = sample();
            last_tick = HAL_GetTick();
            g_events.setBits(EVT_SENSOR_READY);  // 通知其他任务数据就绪
            cout << "[传感器] 采集完成 T=" << out.temperature
                 << "℃ H=" << out.humidity << "%" << endl;
        }
    }
};

// ============================================
// 第四部分：Modbus从站模块
// ============================================

/*
【概念】Modbus寄存器映射
- 保持寄存器（40001起）：可读可写，存储设备参数
- 输入寄存器（30001起）：只读，存储传感器数据
- 线圈（00001起）：单比特读写，控制输出
*/

class ModbusSlave {
    // 保持寄存器（HR）：设备配置
    array<u16, 16> holding_regs = {
        1000,  // HR0: 采样周期（ms）
        100,   // HR1: 上报阈值
        0,     // HR2: 控制字
        0,     // HR3: OTA触发（写1启动）
    };

    // 输入寄存器（IR）：传感器数据（只读）
    array<u16, 8> input_regs = {};

public:
    // 更新输入寄存器（由传感器任务调用）
    void updateSensorRegs(const SensorData& d) {
        input_regs[0] = static_cast<u16>(d.temperature * 10);  // 温度×10（保留1位小数）
        input_regs[1] = static_cast<u16>(d.humidity * 10);     // 湿度×10
        input_regs[2] = d.adc_raw;
        input_regs[3] = static_cast<u16>(d.timestamp & 0xFFFF);
    }

    // 功能码03：读保持寄存器
    // 参数：start=起始地址，count=寄存器数量，out=输出缓冲
    bool readHoldingRegs(u16 start, u16 count, vector<u16>& out) {
        if (start + count > holding_regs.size()) return false;
        out.assign(holding_regs.begin() + start,
                   holding_regs.begin() + start + count);
        return true;
    }

    // 功能码04：读输入寄存器
    bool readInputRegs(u16 start, u16 count, vector<u16>& out) {
        if (start + count > input_regs.size()) return false;
        out.assign(input_regs.begin() + start,
                   input_regs.begin() + start + count);
        return true;
    }

    // 功能码06：写单个保持寄存器
    bool writeSingleReg(u16 addr, u16 value) {
        if (addr >= holding_regs.size()) return false;
        holding_regs[addr] = value;
        // 特殊处理：写HR3=1触发OTA
        if (addr == 3 && value == 1) {
            g_events.setBits(EVT_OTA_REQUEST);
            cout << "[Modbus] 收到OTA触发命令" << endl;
        }
        return true;
    }

    // 处理Modbus请求（模拟接收到完整帧后的处理）
    void processRequest(u8 func, u16 addr, u16 data) {
        vector<u16> result;
        cout << "[Modbus从站] 功能码=" << (int)func
             << " 地址=" << addr << " 数据=" << data << endl;

        switch (func) {
            case 0x03:
                if (readHoldingRegs(addr, data, result)) {
                    cout << "  → 读保持寄存器成功，返回" << result.size() << "个值：";
                    for (auto v : result) cout << v << " ";
                    cout << endl;
                }
                break;
            case 0x04:
                if (readInputRegs(addr, data, result)) {
                    cout << "  → 读输入寄存器成功，返回" << result.size() << "个值：";
                    for (auto v : result) cout << v << " ";
                    cout << endl;
                }
                break;
            case 0x06:
                if (writeSingleReg(addr, data))
                    cout << "  → 写寄存器成功" << endl;
                break;
            default:
                cout << "  → 不支持的功能码" << endl;
        }
    }
};

// ============================================
// 第五部分：UART命令解析模块
// ============================================

/*
【概念】命令解析器
- 接收文本命令（如 "read temp"、"ota start"）
- 解析后调用对应处理函数
- 使用map存储命令→处理函数的映射
*/

class CommandParser {
    map<string, function<void(const string&)>> handlers;

public:
    // 注册命令处理函数
    void registerCmd(const string& cmd, function<void(const string&)> handler) {
        handlers[cmd] = handler;
    }

    // 解析并执行命令
    void parse(const string& line) {
        size_t space = line.find(' ');
        string cmd  = line.substr(0, space);
        string args = (space != string::npos) ? line.substr(space + 1) : "";

        auto it = handlers.find(cmd);
        if (it != handlers.end()) {
            it->second(args);
        } else {
            cout << "[UART] 未知命令: " << cmd << endl;
        }
    }
};

// ============================================
// 第六部分：OTA状态机
// ============================================

/*
【概念】OTA状态机
- 状态转移：IDLE → DOWNLOADING → VERIFYING → FLASHING → REBOOT
- 每个状态有明确的进入/退出条件
- 任何异常都回到IDLE（不能变砖）
*/

enum class OtaState { IDLE, DOWNLOADING, VERIFYING, FLASHING, REBOOT };

class OtaManager {
    OtaState state = OtaState::IDLE;
    u32 progress   = 0;   // 下载进度（0~100）
    bool crc_ok    = false;

    const char* stateName() const {
        switch (state) {
            case OtaState::IDLE:        return "IDLE";
            case OtaState::DOWNLOADING: return "DOWNLOADING";
            case OtaState::VERIFYING:   return "VERIFYING";
            case OtaState::FLASHING:    return "FLASHING";
            case OtaState::REBOOT:      return "REBOOT";
        }
        return "?";
    }

public:
    void start() {
        if (state != OtaState::IDLE) return;
        state    = OtaState::DOWNLOADING;
        progress = 0;
        cout << "[OTA] 开始升级 → " << stateName() << endl;
    }

    // 模拟每次tick推进状态机
    void tick() {
        switch (state) {
            case OtaState::IDLE: break;

            case OtaState::DOWNLOADING:
                progress += 20;
                cout << "[OTA] 下载进度 " << progress << "%" << endl;
                if (progress >= 100) {
                    state = OtaState::VERIFYING;
                    cout << "[OTA] → " << stateName() << endl;
                }
                break;

            case OtaState::VERIFYING:
                crc_ok = true;  // 模拟CRC校验通过
                cout << "[OTA] CRC校验" << (crc_ok ? "通过" : "失败") << endl;
                state = crc_ok ? OtaState::FLASHING : OtaState::IDLE;
                cout << "[OTA] → " << stateName() << endl;
                break;

            case OtaState::FLASHING:
                cout << "[OTA] 写入Flash..." << endl;
                state = OtaState::REBOOT;
                cout << "[OTA] → " << stateName() << endl;
                break;

            case OtaState::REBOOT:
                cout << "[OTA] 升级完成，准备重启！" << endl;
                state = OtaState::IDLE;  // 模拟重启后回到IDLE
                break;
        }
    }

    bool isIdle() const { return state == OtaState::IDLE; }
};

// ============================================
// 第七部分：看门狗模块
// ============================================

class Watchdog {
    u32 timeout_ms;   // 超时时间
    u32 last_feed;    // 上次喂狗时间
    bool enabled;

public:
    explicit Watchdog(u32 timeout = 5000)
        : timeout_ms(timeout), last_feed(0), enabled(false) {}

    void enable() {
        enabled   = true;
        last_feed = HAL_GetTick();
        cout << "[WDG] 看门狗已启动，超时=" << timeout_ms << "ms" << endl;
    }

    // 喂狗（必须在超时前调用）
    void feed() {
        last_feed = HAL_GetTick();
    }

    // 检查是否超时（真实场景：硬件自动复位，这里仅打印警告）
    void check() {
        if (!enabled) return;
        u32 elapsed = HAL_GetTick() - last_feed;
        if (elapsed > timeout_ms * 8 / 10) {  // 超过80%超时时间，发出警告
            g_events.setBits(EVT_WATCHDOG_WARN);
            cout << "[WDG] ⚠ 警告：距上次喂狗已" << elapsed << "ms，请及时喂狗！" << endl;
        }
        if (elapsed > timeout_ms) {
            cout << "[WDG] ❌ 看门狗超时！系统复位！" << endl;
            last_feed = HAL_GetTick();  // 模拟复位
        }
    }
};

// ============================================
// 第八部分：系统管理任务（主调度器）
// ============================================

class SystemManager {
    SensorTask   sensor;
    ModbusSlave  modbus;
    CommandParser parser;
    OtaManager   ota;
    Watchdog     wdg{3000};  // 3秒看门狗
    SensorData   latest;

    void setupCommands() {
        // 注册UART命令
        parser.registerCmd("read", [&](const string& args) {
            if (args == "temp")
                cout << "[CMD] 温度=" << latest.temperature << "℃" << endl;
            else if (args == "humi")
                cout << "[CMD] 湿度=" << latest.humidity << "%" << endl;
            else if (args == "all") {
                cout << "[CMD] T=" << latest.temperature
                     << " H=" << latest.humidity
                     << " ADC=" << latest.adc_raw << endl;
            }
        });

        parser.registerCmd("modbus", [&](const string& args) {
            // 格式：modbus <func> <addr> <data>
            // 示例：modbus 3 0 4  → 读HR0~HR3
            int func = 0, addr = 0, data = 0;
            if (sscanf(args.c_str(), "%d %d %d", &func, &addr, &data) == 3)
                modbus.processRequest((u8)func, (u16)addr, (u16)data);
        });

        parser.registerCmd("ota", [&](const string& args) {
            if (args == "start") ota.start();
        });

        parser.registerCmd("help", [&](const string&) {
            cout << "[CMD] 支持命令：read <temp|humi|all>, modbus <func> <addr> <data>, ota start" << endl;
        });
    }

public:
    SystemManager() { setupCommands(); }

    // 系统主循环
    void run(const vector<string>& cmd_sequence) {
        wdg.enable();
        cout << "\n" << string(60, '=') << endl;
        cout << "IoT节点启动" << endl;
        cout << string(60, '=') << endl;

        size_t cmd_idx = 0;

        for (int cycle = 0; cycle < 15; ++cycle) {
            HAL_Delay(200);  // 模拟200ms主循环周期

            cout << "\n--- 主循环 #" << cycle
                 << " tick=" << HAL_GetTick() << " ---" << endl;

            // 1. 传感器任务
            sensor.run(latest);

            // 2. 更新Modbus输入寄存器
            if (g_events.waitBits(EVT_SENSOR_READY, false)) {
                modbus.updateSensorRegs(latest);
                g_events.clearBits(EVT_SENSOR_READY);
            }

            // 3. 处理UART命令（模拟按序注入命令）
            if (cmd_idx < cmd_sequence.size()) {
                cout << "[UART] 收到命令: " << cmd_sequence[cmd_idx] << endl;
                parser.parse(cmd_sequence[cmd_idx++]);
                g_events.setBits(EVT_CMD_RECEIVED);
                g_events.clearBits(EVT_CMD_RECEIVED);
            }

            // 4. OTA状态机推进
            if (g_events.waitBits(EVT_OTA_REQUEST, false)) {
                g_events.clearBits(EVT_OTA_REQUEST);
                ota.start();
            }
            if (!ota.isIdle()) ota.tick();

            // 5. 看门狗喂狗（系统管理任务负责）
            wdg.feed();
            wdg.check();
        }

        cout << "\n" << string(60, '=') << endl;
        cout << "主循环结束" << endl;
        cout << string(60, '=') << endl;
    }
};

// ============================================
// 第九部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

❌ 错误1：在中断中操作FreeRTOS API（非ISR版本）
  xSemaphoreGive(sem);         // ❌ ISR中禁止
  xSemaphoreGiveFromISR(sem);  // ✅ ISR中必须用FromISR版本

❌ 错误2：忘记喂看门狗（在阻塞操作中）
  while (download_in_progress) {  // ❌ 长时间不喂狗 → 系统复位
    receiveChunk();
  }
  // ✅ 在循环内周期性喂狗
  while (download_in_progress) {
    receiveChunk();
    wdg.feed();  // 每次循环都喂
  }

❌ 错误3：OTA升级中断电后无法恢复
  flashFirmware();  // ❌ 写到一半断电 → 启动损坏的固件
  // ✅ 双Bank策略：写Bank B，写完再切换启动标志

❌ 错误4：Modbus响应帧CRC字节顺序错误
  frame[n]   = crc >> 8;    // ❌ CRC高字节在前
  frame[n]   = crc & 0xFF;  // ✅ Modbus RTU要求低字节在前
  frame[n+1] = crc >> 8;    // ✅ 高字节在后

❌ 错误5：事件组被多个任务同时读写（未加保护）
  // FreeRTOS事件组内部有临界区保护，但自定义实现必须加锁
  // ✅ 使用FreeRTOS原生API，不要自己实现事件组

============================================================
)";
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "22_embedded_system_demo.cpp" << endl;
    cout << "嵌入式系统综合实战 - IoT节点模拟" << endl;
    cout << string(60, '=') << endl;

    // 模拟一系列UART命令
    vector<string> commands = {
        "help",
        "read all",
        "modbus 3 0 4",     // 读HR0~HR3
        "modbus 4 0 4",     // 读IR0~IR3（传感器数据）
        "read temp",
        "modbus 6 3 1",     // 写HR3=1，触发OTA
        "read humi",
        "modbus 6 0 2000",  // 写HR0=2000（修改采样周期）
        "modbus 3 0 1",     // 验证HR0已修改
    };

    SystemManager sys;
    sys.run(commands);

    showCommonMistakes();

    cout << R"(
============================================================
练习题
============================================================

1. 扩展Modbus从站，支持功能码16（写多个寄存器）
   提示：需要解析数据长度字段，循环写入多个寄存器

2. 为OTA模块添加断点续传功能
   提示：记录已下载的分片编号，重连后从断点继续

3. 实现基于优先级的命令队列
   提示：用 priority_queue<> 存储命令，OTA命令优先级最高

4. 为看门狗添加复位原因记录功能
   提示：在复位前将原因写入非易失性存储器（模拟用全局变量）

5. 用事件组实现"传感器就绪 AND 网络就绪"才上报数据的逻辑
   提示：EVT_SENSOR_READY | EVT_NETWORK_READY，用waitAll=true
============================================================
)";

    return 0;
}
