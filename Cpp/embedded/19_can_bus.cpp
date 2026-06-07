// ============================================
// 19_can_bus.cpp
// 日期：2026-06-06（第62天）
// 主题：CAN 总线协议
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -o 19_can 19_can_bus.cpp
// ============================================

/*
【核心概念】

是什么？
- CAN（Controller Area Network）：差分串行总线，专为汽车/工业设计
- 两根线：CANH（显性=高电压）和CANL（显性=低电压）
- 差分信号：CANH - CANL > 0.9V = 显性位(0)，< 0.5V = 隐性位(1)

为什么需要？
- 强大的错误检测（5种机制）
- 多主总线仲裁，无需主从
- 工业级可靠性，最远1km（低速）

核心特点：
- 标准帧：11位ID，最多8字节数据
- 扩展帧：29位ID，最多8字节数据
- 仲裁：ID越小优先级越高（显性位0胜出隐性位1）
*/

#include <iostream>
#include <cstdint>
#include <vector>
#include <array>
#include <queue>
using namespace std;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

// ============================================
// 第一部分：CAN 帧结构
// ============================================

/*
【标准CAN帧结构】
  | SOF(1) | ID(11) | RTR(1) | IDE(1) | r0(1) | DLC(4) | DATA(0~64bit) | CRC(15) | ACK(2) | EOF(7) |

字段说明：
  SOF  = Start Of Frame，显性位，标志帧开始
  ID   = 11位标识符，决定优先级和消息类型
  RTR  = Remote Transmission Request，0=数据帧，1=远程帧
  IDE  = Identifier Extension，0=标准帧，1=扩展帧
  DLC  = Data Length Code，数据字节数（0~8）
  CRC  = 15位循环冗余校验
  ACK  = 接收方确认（显性=已收到）
  EOF  = End Of Frame，7个隐性位
*/

struct CanFrame {
    u32  id;          // 帧ID（标准帧11位，扩展帧29位）
    bool is_extended; // true=扩展帧（29位ID），false=标准帧（11位ID）
    bool is_remote;   // true=远程帧（无数据），false=数据帧
    u8   dlc;         // 数据长度（0~8字节）
    u8   data[8];     // 数据（最多8字节）

    CanFrame() : id(0), is_extended(false), is_remote(false), dlc(0) {
        memset(data, 0, sizeof(data));
    }

    CanFrame(u32 id, initializer_list<u8> bytes, bool ext = false)
        : id(id), is_extended(ext), is_remote(false) {
        dlc = min((size_t)8, bytes.size());
        size_t i = 0;
        for (u8 b : bytes) { if (i < 8) data[i++] = b; }
    }

    void print() const {
        cout << "  [CAN帧] ID=0x" << hex << id << dec
             << (is_extended ? "(扩展29位)" : "(标准11位)")
             << " DLC=" << (int)dlc << " DATA:";
        for (int i = 0; i < dlc; ++i)
            cout << " " << hex << (int)data[i] << dec;
        cout << endl;
    }
};

// ============================================
// 第二部分：总线仲裁模拟 ⭐⭐⭐⭐⭐
// ============================================

/*
【概念】CAN总线仲裁

规则：
1. 多个节点同时发送时，逐位比较ID
2. 发送显性位(0)的节点继续，发送隐性位(1)的节点退出
3. ID越小（二进制越小），优先级越高
4. 退出的节点变为接收者，等待总线空闲后重试

示例：
  节点A ID=0x100 = 0b00100000000
  节点B ID=0x080 = 0b00010000000  ← 在第4位先发出显性0，节点A退出
  节点B赢得仲裁
*/

struct CanNode {
    string name;
    CanFrame frame;
    bool lost_arbitration = false;

    CanNode(const string& n, CanFrame f) : name(n), frame(f) {}
};

void simulateArbitration(vector<CanNode>& nodes) {
    cout << string(60, '-') << endl;
    cout << "CAN总线仲裁模拟" << endl;
    cout << string(60, '-') << endl;

    for (auto& n : nodes) {
        cout << "  节点" << n.name << " 发送 ID=0x"
             << hex << n.frame.id << dec << endl;
    }

    // 找出ID最小的节点（仲裁胜者）
    CanNode* winner = &nodes[0];
    for (auto& n : nodes) {
        if (n.frame.id < winner->frame.id)
            winner = &n;
    }

    for (auto& n : nodes) {
        if (&n != winner) {
            n.lost_arbitration = true;
            cout << "  节点" << n.name << " 在第"
                 << (32 - __builtin_clz(n.frame.id ^ winner->frame.id))
                 << "位失去仲裁，退出发送" << endl;
        }
    }
    cout << "  ✅ 节点" << winner->name << " 赢得仲裁（ID最小）" << endl;
}

// ============================================
// 第三部分：过滤器
// ============================================

/*
【概念】CAN过滤器

硬件过滤器减少CPU处理无关帧的负担：

1. 屏蔽模式（Mask Mode）：
   接收条件：(received_id & mask) == (filter_id & mask)
   mask中为1的位必须与filter_id匹配，为0的位任意

2. 列表模式（List Mode）：
   只接收与列表中完全相同ID的帧
*/

struct CanFilter {
    u32  filter_id;
    u32  filter_mask;  // 0=don't care，1=必须匹配
    bool list_mode;    // false=屏蔽模式，true=列表模式
    vector<u32> id_list; // 列表模式的ID列表

    bool accept(u32 id) const {
        if (list_mode) {
            for (u32 lid : id_list)
                if (lid == id) return true;
            return false;
        }
        // 屏蔽模式：(id & mask) == (filter_id & mask)
        return (id & filter_mask) == (filter_id & filter_mask);
    }
};

void demoFilter() {
    cout << string(60, '-') << endl;
    cout << "CAN过滤器演示" << endl;
    cout << string(60, '-') << endl;

    // 屏蔽模式：接收ID = 0x1xx（高3位为001的所有帧）
    CanFilter f1;
    f1.filter_id   = 0x100;
    f1.filter_mask = 0x700;  // 只检查高3位
    f1.list_mode   = false;

    vector<u32> test_ids = {0x100, 0x150, 0x1FF, 0x200, 0x080};
    cout << "  屏蔽模式（接收0x1xx范围）：" << endl;
    for (u32 id : test_ids) {
        cout << "    ID=0x" << hex << id << dec
             << (f1.accept(id) ? " → 接收" : " → 过滤") << endl;
    }

    // 列表模式：只接收特定ID
    CanFilter f2;
    f2.list_mode = true;
    f2.id_list   = {0x100, 0x200, 0x300};

    cout << "  列表模式（只接收0x100/0x200/0x300）：" << endl;
    for (u32 id : test_ids) {
        cout << "    ID=0x" << hex << id << dec
             << (f2.accept(id) ? " → 接收" : " → 过滤") << endl;
    }
}

// ============================================
// 第四部分：错误检测
// ============================================

void showErrorDetection() {
    cout << R"(
============================================================
CAN 5种错误检测机制
============================================================

1. 位错误（Bit Error）
   发送方监听总线，若发送1却读到0（非仲裁阶段），报位错误
   → 原因：总线短路、强干扰

2. 位填充错误（Stuff Error）
   CAN规定：5个连续相同位后必须插入1个相反位（位填充）
   若填充规则被违反，报填充错误
   → 原因：帧损坏、同步丢失

3. 帧格式错误（Form Error）
   EOF、ACK界定符等固定格式域出现显性位
   → 原因：时序错误

4. ACK错误（ACK Error）
   发送方在ACK时隙未检测到显性位（无接收方确认）
   → 原因：总线上只有一个节点，或接收方故障

5. CRC错误（CRC Error）
   接收方计算的CRC与帧中CRC不符
   → 原因：数据传输错误

错误计数：
  发送错误计数（TEC）和接收错误计数（REC）
  TEC/REC > 127 → 错误被动（Error Passive）
  TEC > 255      → 总线关闭（Bus Off），退出总线
============================================================
)";
}

// ============================================
// 常见错误
// ============================================

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱
============================================================

❌ 错误1：CAN终端电阻缺失
  总线两端必须各接120Ω终端电阻，否则信号反射导致通信失败
  ✅ 标准CAN总线：两端各120Ω，并联后=60Ω

❌ 错误2：混用标准帧和扩展帧ID
  同一总线上0x100（标准帧）和0x100（扩展帧）是不同的帧
  仲裁时IDE位参与比较，标准帧IDE=0优先于扩展帧IDE=1

❌ 错误3：DLC与实际数据长度不符
  dlc = 5; data只填了3字节  // ❌ 接收方读到垃圾数据
  // ✅ dlc必须等于实际填充的data字节数

❌ 错误4：波特率不一致
  同一总线所有节点必须使用完全相同的波特率
  常用：125kbps / 250kbps / 500kbps / 1Mbps

❌ 错误5：在总线关闭（Bus Off）后不处理恢复
  Bus Off后需要手动触发恢复流程（等待128×11个隐性位）
============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "19_can_bus.cpp - CAN总线协议" << endl;
    cout << string(60, '=') << endl;

    // 仲裁演示
    vector<CanNode> nodes = {
        {"A", CanFrame(0x200, {0x01, 0x02})},
        {"B", CanFrame(0x100, {0x03, 0x04})},
        {"C", CanFrame(0x180, {0x05, 0x06})},
    };
    simulateArbitration(nodes);

    // 过滤器演示
    demoFilter();

    // 错误检测说明
    showErrorDetection();

    // 帧结构演示
    cout << string(60, '-') << endl;
    cout << "CAN帧示例" << endl;
    cout << string(60, '-') << endl;
    CanFrame f1(0x100, {0x01, 0x02, 0x03, 0x04});
    CanFrame f2(0x1FFFF, {0xAA, 0xBB}, true); // 扩展帧
    f1.print();
    f2.print();

    showMistakes();

    cout << R"(
练习题
============================================================
1. 实现CAN帧的CRC15计算（多项式0x4599）
2. 模拟4个节点同时发送，输出每一位的仲裁过程（按位显示）
3. 实现一个CAN消息分发器：按ID范围注册回调函数，收到帧后自动路由
============================================================
)";
    return 0;
}
