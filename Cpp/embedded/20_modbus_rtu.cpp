// ============================================
// 20_modbus_rtu.cpp
// 日期：2026-06-06（第62天）
// 主题：Modbus RTU 协议
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -o 20_modbus 20_modbus_rtu.cpp
// ============================================

/*
【核心概念】

是什么？
- Modbus RTU：工业领域最广泛的串行通信协议（1979年诞生，至今仍主流）
- 主从架构：1个主站发起请求，最多247个从站响应
- RTU模式：二进制编码（紧凑），另有ASCII模式（可读性好但效率低）

帧结构：
  | 从站地址(1B) | 功能码(1B) | 数据域(nB) | CRC16(2B，低字节在前) |

为什么需要？
- PLC、变频器、仪表等工业设备的标准通信接口
- 简单、开放、免版权费

核心特点：
- 基于UART（RS-485物理层，差分信号，最远1200m）
- 严格的主从时序：主站发请求 → 从站响应，不能主动上报
*/

#include <iostream>
#include <cstdint>
#include <vector>
#include <array>
#include <cstring>
using namespace std;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

// ============================================
// 第一部分：CRC16 计算 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片】calcCRC16

作用：计算Modbus RTU的CRC16校验值
多项式：0xA001（0x8005的位反转，因为低位先传）
语法：u16 calcCRC16(const u8* data, size_t len)
参数：
  data — 要校验的数据（不含CRC本身）
  len  — 数据长度
返回：16位CRC值（低字节先放入帧中）
*/

u16 calcCRC16(const u8* data, size_t len) {
    u16 crc = 0xFFFF;  // 初始值
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];           // XOR当前字节
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;  // 多项式
            else
                crc >>= 1;
        }
    }
    return crc;
}

// 验证帧CRC（含CRC的完整帧，最后2字节是CRC）
bool verifyCRC(const vector<u8>& frame) {
    if (frame.size() < 4) return false;
    u16 calc = calcCRC16(frame.data(), frame.size() - 2);
    u16 recv = frame[frame.size()-2] | (frame[frame.size()-1] << 8); // 低字节在前
    return calc == recv;
}

// ============================================
// 第二部分：功能码定义与帧构造
// ============================================

enum class ModbusFunc : u8 {
    ReadCoils          = 0x01,  // 读线圈（单比特，可读写）
    ReadDiscreteInputs = 0x02,  // 读离散输入（单比特，只读）
    ReadHoldingRegs    = 0x03,  // 读保持寄存器（16位，可读写）★最常用
    ReadInputRegs      = 0x04,  // 读输入寄存器（16位，只读）
    WriteSingleCoil    = 0x05,  // 写单个线圈
    WriteSingleReg     = 0x06,  // 写单个保持寄存器 ★常用
    WriteMultipleRegs  = 0x10,  // 写多个保持寄存器 ★常用
};

// 构造请求帧（主站发送）
vector<u8> buildRequest(u8 slave_addr, ModbusFunc func, u16 start, u16 count_or_value) {
    vector<u8> frame;
    frame.push_back(slave_addr);
    frame.push_back(static_cast<u8>(func));
    frame.push_back(start >> 8);          // 地址高字节
    frame.push_back(start & 0xFF);        // 地址低字节
    frame.push_back(count_or_value >> 8); // 数量/值高字节
    frame.push_back(count_or_value & 0xFF);
    u16 crc = calcCRC16(frame.data(), frame.size());
    frame.push_back(crc & 0xFF);          // CRC低字节在前
    frame.push_back(crc >> 8);
    return frame;
}

// 打印帧内容
void printFrame(const string& label, const vector<u8>& frame) {
    cout << "  " << label << ": [";
    for (size_t i = 0; i < frame.size(); ++i) {
        if (i) cout << " ";
        cout << hex << (int)frame[i] << dec;
    }
    cout << "] CRC=" << (verifyCRC(frame) ? "✅OK" : "❌错误") << endl;
}

// ============================================
// 第三部分：Modbus 从站实现 ⭐⭐⭐⭐⭐
// ============================================

class ModbusSlave {
    u8   addr;                  // 从站地址（1~247）
    array<u16, 64> holding;     // 保持寄存器 HR0~HR63
    array<u16, 32> input_regs;  // 输入寄存器 IR0~IR31

public:
    explicit ModbusSlave(u8 a) : addr(a) {
        holding.fill(0);
        input_regs.fill(0);
        // 预置一些数据
        holding[0] = 1000;  // 采样周期
        holding[1] = 100;   // 阈值
        input_regs[0] = 250; // 温度×10 = 25.0℃
        input_regs[1] = 650; // 湿度×10 = 65.0%
    }

    // 处理请求帧，返回响应帧
    // 返回空vector表示帧不是给自己的（广播除外）
    vector<u8> process(const vector<u8>& req) {
        if (req.size() < 8) return {};
        if (!verifyCRC(req)) {
            cout << "  [从站" << (int)addr << "] CRC校验失败，忽略" << endl;
            return {};
        }
        if (req[0] != addr && req[0] != 0x00) return {}; // 不是给自己的

        u8   func  = req[1];
        u16  start = (req[2] << 8) | req[3];
        u16  count = (req[4] << 8) | req[5];

        switch (func) {
            case 0x03: return respondReadHolding(start, count);
            case 0x04: return respondReadInput(start, count);
            case 0x06: return respondWriteSingle(start, count); // count即值
            default:   return buildException(func, 0x01); // 非法功能码
        }
    }

private:
    // 功能码03响应：读保持寄存器
    vector<u8> respondReadHolding(u16 start, u16 count) {
        if (start + count > holding.size())
            return buildException(0x03, 0x02); // 非法数据地址

        vector<u8> resp;
        resp.push_back(addr);
        resp.push_back(0x03);
        resp.push_back(count * 2); // 字节数
        for (u16 i = 0; i < count; ++i) {
            resp.push_back(holding[start + i] >> 8);
            resp.push_back(holding[start + i] & 0xFF);
        }
        appendCRC(resp);
        return resp;
    }

    // 功能码04响应：读输入寄存器
    vector<u8> respondReadInput(u16 start, u16 count) {
        if (start + count > input_regs.size())
            return buildException(0x04, 0x02);

        vector<u8> resp;
        resp.push_back(addr);
        resp.push_back(0x04);
        resp.push_back(count * 2);
        for (u16 i = 0; i < count; ++i) {
            resp.push_back(input_regs[start + i] >> 8);
            resp.push_back(input_regs[start + i] & 0xFF);
        }
        appendCRC(resp);
        return resp;
    }

    // 功能码06响应：写单个寄存器
    vector<u8> respondWriteSingle(u16 addr_reg, u16 value) {
        if (addr_reg >= holding.size())
            return buildException(0x06, 0x02);

        holding[addr_reg] = value;

        // 响应是请求的镜像（echo）
        vector<u8> resp = {addr, 0x06,
            (u8)(addr_reg >> 8), (u8)(addr_reg & 0xFF),
            (u8)(value >> 8),    (u8)(value & 0xFF)};
        appendCRC(resp);
        return resp;
    }

    // 构造异常响应（功能码 | 0x80 = 错误响应）
    vector<u8> buildException(u8 func, u8 code) {
        vector<u8> resp = {addr, (u8)(func | 0x80), code};
        appendCRC(resp);
        return resp;
    }

    void appendCRC(vector<u8>& frame) {
        u16 crc = calcCRC16(frame.data(), frame.size());
        frame.push_back(crc & 0xFF);
        frame.push_back(crc >> 8);
    }
};

// ============================================
// 第四部分：主从通信演示
// ============================================

void demoMasterSlave() {
    cout << string(60, '-') << endl;
    cout << "Modbus RTU 主从通信演示" << endl;
    cout << string(60, '-') << endl;

    ModbusSlave slave(0x01); // 从站地址1

    // 场景1：主站读保持寄存器HR0~HR1
    {
        cout << "\n场景1：主站读HR0~HR1（功能码03）" << endl;
        auto req = buildRequest(0x01, ModbusFunc::ReadHoldingRegs, 0, 2);
        printFrame("请求帧", req);
        auto resp = slave.process(req);
        printFrame("响应帧", resp);
        if (resp.size() >= 7) {
            u16 hr0 = (resp[3] << 8) | resp[4];
            u16 hr1 = (resp[5] << 8) | resp[6];
            cout << "  → HR0=" << hr0 << " HR1=" << hr1 << endl;
        }
    }

    // 场景2：主站读输入寄存器IR0~IR1（传感器数据）
    {
        cout << "\n场景2：主站读IR0~IR1（功能码04）" << endl;
        auto req = buildRequest(0x01, ModbusFunc::ReadInputRegs, 0, 2);
        printFrame("请求帧", req);
        auto resp = slave.process(req);
        printFrame("响应帧", resp);
        if (resp.size() >= 7) {
            float temp = ((resp[3] << 8) | resp[4]) / 10.0f;
            float humi = ((resp[5] << 8) | resp[6]) / 10.0f;
            cout << "  → 温度=" << temp << "℃ 湿度=" << humi << "%" << endl;
        }
    }

    // 场景3：主站写HR0=2000
    {
        cout << "\n场景3：主站写HR0=2000（功能码06）" << endl;
        auto req = buildRequest(0x01, ModbusFunc::WriteSingleReg, 0, 2000);
        printFrame("请求帧", req);
        auto resp = slave.process(req);
        printFrame("响应帧（镜像）", resp);
    }

    // 场景4：非法地址
    {
        cout << "\n场景4：非法寄存器地址（功能码03，地址0x0080，超范围）" << endl;
        auto req = buildRequest(0x01, ModbusFunc::ReadHoldingRegs, 0x0080, 2);
        printFrame("请求帧", req);
        auto resp = slave.process(req);
        printFrame("异常响应", resp);
        if (resp.size() >= 5)
            cout << "  → 异常码=0x" << hex << (int)resp[2] << dec
                 << "（0x02=非法数据地址）" << endl;
    }
}

// ============================================
// 常见错误
// ============================================

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱
============================================================

❌ 错误1：CRC字节顺序错误
  frame[n]   = crc >> 8;    // ❌ 高字节在前
  // ✅ Modbus RTU: 低字节在前，高字节在后
  frame[n]   = crc & 0xFF;
  frame[n+1] = crc >> 8;

❌ 错误2：寄存器地址与Modbus协议地址混淆
  HR1 在协议中地址=0x0000（从0开始），但文档标注为"40001"（从1开始）
  实际请求时使用 start=0x0000 对应HR1

❌ 错误3：主站未等待从站响应超时就发下一帧
  发完请求帧后必须等待（通常100~500ms），超时才能认为从站无响应

❌ 错误4：RS-485方向控制（半双工）
  发送前必须拉高DE（驱动使能），发送完毕立即拉低
  忘记拉低DE → 总线冲突，收不到响应

❌ 错误5：广播地址0x00发写命令后期待响应
  Modbus规定：广播地址（0x00）的请求，所有从站执行但不响应
============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "20_modbus_rtu.cpp - Modbus RTU协议" << endl;
    cout << string(60, '=') << endl;

    // CRC验证
    cout << string(60, '-') << endl;
    cout << "CRC16 验证" << endl;
    cout << string(60, '-') << endl;
    // 已知测试向量：地址01 功能03 起始0000 数量0002 → CRC = 0xC40B（低字节0x0B，高字节0xC4）
    vector<u8> test = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
    u16 crc = calcCRC16(test.data(), test.size());
    cout << "  测试帧CRC16 = 0x" << hex << crc << dec
         << (crc == 0xC40B ? " ✅正确" : " ❌错误") << endl;

    demoMasterSlave();
    showMistakes();

    cout << R"(
练习题
============================================================
1. 实现功能码16（写多个保持寄存器），并测试写HR0~HR3
2. 为ModbusSlave添加线圈（Coil）支持，实现功能码01/05
3. 实现一个Modbus主站轮询器：每500ms轮询3个从站的温湿度数据
============================================================
)";
    return 0;
}
