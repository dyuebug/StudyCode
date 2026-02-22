// ============================================
// 07_grade_calculator.cpp - 成绩计算器（综合项目）
// 日期：2026-02-18（第2天）
// 难度：⭐⭐ 中级
// 项目类型：综合实战项目
// ============================================

/*
【项目说明】

项目名称：成绩计算器
项目目标：综合运用if-else和switch-case知识

功能要求：
1. 输入学生姓名和各科成绩
2. 计算总分和平均分
3. 判断成绩等级（A/B/C/D/F）
4. 判断是否及格
5. 给出评语和建议
6. 显示完整的成绩报告

学习目标：
- 综合运用条件判断
- 掌握多分支选择
- 学会格式化输出
- 培养项目思维
*/

#include <iostream>
#include <iomanip>   // 用于格式化输出
#include <string>    // 用于字符串类型
#include <cstdlib>
using namespace std;

int main() {
    system("chcp 65001");  // 设置控制台编码为 UTF-8，支持中文输出

    // ============================================
    // 第一部分：程序介绍
    // ============================================

    cout << string(60, '=') << endl;
    cout << "        成绩计算器 v1.0" << endl;
    cout << string(60, '=') << endl;
    cout << "本程序将计算学生成绩并生成成绩报告" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    // ============================================
    // 第二部分：输入学生信息
    // ============================================

    cout << "第一步：输入学生信息" << endl;
    cout << string(60, '-') << endl;

    // 声明变量：存储学生姓名
    string studentName;
    cout << "请输入学生姓名：";
    getline(cin, studentName);  // 使用getline读取整行，支持空格

    // 声明变量：存储各科成绩（double类型，支持小数）
    double mathScore;      // 数学成绩
    double englishScore;   // 英语成绩
    double chineseScore;   // 语文成绩

    // 输入各科成绩
    cout << "请输入数学成绩（0-100）：";
    cin >> mathScore;

    cout << "请输入英语成绩（0-100）：";
    cin >> englishScore;

    cout << "请输入语文成绩（0-100）：";
    cin >> chineseScore;

    cout << endl;

    // ============================================
    // 第三部分：数据验证
    // ============================================

    cout << "第二步：验证输入数据" << endl;
    cout << string(60, '-') << endl;

    // 验证成绩是否在有效范围内（0-100）
    // 使用逻辑运算符组合条件
    bool isValid = true;  // 标记数据是否有效

    // 验证数学成绩
    if (mathScore < 0 || mathScore > 100) {
        cout << "❌ 错误：数学成绩无效（应在0-100之间）" << endl;
        isValid = false;
    }

    // 验证英语成绩
    if (englishScore < 0 || englishScore > 100) {
        cout << "❌ 错误：英语成绩无效（应在0-100之间）" << endl;
        isValid = false;
    }

    // 验证语文成绩
    if (chineseScore < 0 || chineseScore > 100) {
        cout << "❌ 错误：语文成绩无效（应在0-100之间）" << endl;
        isValid = false;
    }

    // 如果数据无效，终止程序
    if (!isValid) {
        cout << "程序终止，请重新运行并输入有效数据。" << endl;
        return 1;  // 返回1表示程序异常退出
    }

    cout << "✅ 数据验证通过！" << endl;
    cout << endl;

    // ============================================
    // 第四部分：计算总分和平均分
    // ============================================

    cout << "第三步：计算总分和平均分" << endl;
    cout << string(60, '-') << endl;

    // 计算总分：三科成绩相加
    double totalScore = mathScore + englishScore + chineseScore;

    // 计算平均分：总分除以科目数
    double averageScore = totalScore / 3.0;  // 使用3.0确保浮点数除法

    // 输出计算结果（保留2位小数）
    cout << fixed << setprecision(2);  // 设置固定小数点格式，保留2位
    cout << "总分：" << totalScore << " 分" << endl;
    cout << "平均分：" << averageScore << " 分" << endl;
    cout << endl;

    // ============================================
    // 第五部分：判断成绩等级
    // ============================================

    cout << "第四步：判断成绩等级" << endl;
    cout << string(60, '-') << endl;

    // 声明变量：存储等级字母
    char grade;

    // 使用if-else if-else判断等级
    // 根据平均分划分等级
    if (averageScore >= 90) {
        grade = 'A';  // 优秀
    } else if (averageScore >= 80) {
        grade = 'B';  // 良好
    } else if (averageScore >= 70) {
        grade = 'C';  // 中等
    } else if (averageScore >= 60) {
        grade = 'D';  // 及格
    } else {
        grade = 'F';  // 不及格
    }

    cout << "成绩等级：" << grade << endl;
    cout << endl;

    // ============================================
    // 第六部分：判断是否及格
    // ============================================

    cout << "第五步：判断是否及格" << endl;
    cout << string(60, '-') << endl;

    // 判断是否及格（平均分>=60）
    bool isPassed = (averageScore >= 60);

    if (isPassed) {
        cout << "✅ 恭喜！成绩及格！" << endl;
    } else {
        cout << "❌ 很遗憾，成绩不及格。" << endl;
    }

    // 判断各科是否及格
    cout << endl;
    cout << "各科及格情况：" << endl;

    // 数学
    if (mathScore >= 60) {
        cout << "  数学：✅ 及格（" << mathScore << "分）" << endl;
    } else {
        cout << "  数学：❌ 不及格（" << mathScore << "分）" << endl;
    }

    // 英语
    if (englishScore >= 60) {
        cout << "  英语：✅ 及格（" << englishScore << "分）" << endl;
    } else {
        cout << "  英语：❌ 不及格（" << englishScore << "分）" << endl;
    }

    // 语文
    if (chineseScore >= 60) {
        cout << "  语文：✅ 及格（" << chineseScore << "分）" << endl;
    } else {
        cout << "  语文：❌ 不及格（" << chineseScore << "分）" << endl;
    }

    cout << endl;

    // ============================================
    // 第七部分：生成评语和建议
    // ============================================

    cout << "第六步：生成评语和建议" << endl;
    cout << string(60, '-') << endl;

    // 使用switch-case根据等级生成评语
    cout << "综合评语：" << endl;

    switch (grade) {
        case 'A':
            cout << "  🌟 优秀！你的成绩非常出色！" << endl;
            cout << "  继续保持，争取更大进步！" << endl;
            break;

        case 'B':
            cout << "  👍 良好！你的成绩很不错！" << endl;
            cout << "  再接再厉，向优秀冲刺！" << endl;
            break;

        case 'C':
            cout << "  📚 中等，还有提升空间。" << endl;
            cout << "  加强薄弱科目，争取进入良好！" << endl;
            break;

        case 'D':
            cout << "  ⚠️ 及格，但成绩不够理想。" << endl;
            cout << "  需要更加努力，提高学习效率！" << endl;
            break;

        case 'F':
            cout << "  ❗ 不及格，需要引起重视！" << endl;
            cout << "  建议：" << endl;
            cout << "  1. 找出薄弱环节，针对性学习" << endl;
            cout << "  2. 寻求老师或同学的帮助" << endl;
            cout << "  3. 制定学习计划，坚持执行" << endl;
            break;

        default:
            cout << "  未知等级" << endl;
            break;
    }

    cout << endl;

    // 针对性建议：找出最弱的科目
    cout << "针对性建议：" << endl;

    // 找出最低分的科目
    double minScore = mathScore;  // 假设数学最低
    string weakSubject = "数学";

    if (englishScore < minScore) {
        minScore = englishScore;
        weakSubject = "英语";
    }

    if (chineseScore < minScore) {
        minScore = chineseScore;
        weakSubject = "语文";
    }

    // 如果最低分不及格，给出建议
    if (minScore < 60) {
        cout << "  ⚠️ " << weakSubject << "是你的薄弱科目（" << minScore << "分）" << endl;
        cout << "  建议重点加强" << weakSubject << "的学习！" << endl;
    } else {
        cout << "  ✅ 各科成绩均衡，继续保持！" << endl;
    }

    cout << endl;

    // ============================================
    // 第八部分：生成完整成绩报告
    // ============================================

    cout << string(60, '=') << endl;
    cout << "        成绩报告" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    // 学生信息
    cout << "学生姓名：" << studentName << endl;
    cout << string(60, '-') << endl;

    // 各科成绩
    cout << "各科成绩：" << endl;
    cout << "  数学：" << setw(6) << mathScore << " 分" << endl;
    cout << "  英语：" << setw(6) << englishScore << " 分" << endl;
    cout << "  语文：" << setw(6) << chineseScore << " 分" << endl;
    cout << string(60, '-') << endl;

    // 统计信息
    cout << "统计信息：" << endl;
    cout << "  总分：" << setw(6) << totalScore << " 分" << endl;
    cout << "  平均分：" << setw(6) << averageScore << " 分" << endl;
    cout << "  等级：" << setw(6) << grade << endl;
    cout << "  是否及格：" << (isPassed ? "是" : "否") << endl;
    cout << string(60, '-') << endl;

    // 等级说明
    cout << "等级说明：" << endl;
    cout << "  A: 90-100分（优秀）" << endl;
    cout << "  B: 80-89分（良好）" << endl;
    cout << "  C: 70-79分（中等）" << endl;
    cout << "  D: 60-69分（及格）" << endl;
    cout << "  F: 0-59分（不及格）" << endl;
    cout << string(60, '=') << endl;

    cout << endl;
    cout << "感谢使用成绩计算器！" << endl;
    cout << endl;

    // ============================================
    // 项目总结
    // ============================================

    cout << string(60, '=') << endl;
    cout << "项目总结" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    cout << R"(
【本项目使用的知识点】

1. 变量声明和初始化
   - string类型（学生姓名）
   - double类型（成绩、总分、平均分）
   - char类型（等级）
   - bool类型（是否及格、数据有效性）

2. 输入输出
   - getline()读取带空格的字符串
   - cin读取数字
   - cout输出信息
   - setprecision()格式化小数

3. 条件判断（if-else）
   - 数据验证（范围判断）
   - 等级判断（多分支选择）
   - 及格判断（简单条件）
   - 找最低分（嵌套if）

4. 多分支选择（switch-case）
   - 根据等级生成评语
   - 代码结构清晰

5. 逻辑运算符
   - || 逻辑或（数据验证）
   - && 逻辑与（可扩展）
   - ! 逻辑非（数据有效性）

6. 格式化输出
   - fixed和setprecision()控制小数位数
   - setw()控制字段宽度
   - 美化输出效果

【项目扩展建议】

1. 增加更多科目
   - 物理、化学、生物等
   - 动态计算科目数

2. 增加权重计算
   - 不同科目不同权重
   - 加权平均分

3. 增加排名功能
   - 输入多个学生成绩
   - 按总分排序

4. 增加历史记录
   - 保存成绩到文件
   - 查询历史成绩

5. 增加图形化界面
   - 使用图形库
   - 更友好的交互

6. 增加统计分析
   - 最高分、最低分
   - 标准差、方差
   - 成绩分布图
)" << endl;

    cout << string(60, '=') << endl;
    cout << "07_grade_calculator.cpp 项目完成！" << endl;
    cout << "编译命令：g++ -o 07_grade_calculator 07_grade_calculator.cpp" << endl;
    cout << "运行命令：./07_grade_calculator 或 07_grade_calculator.exe" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
