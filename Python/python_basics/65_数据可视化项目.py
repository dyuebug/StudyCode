# ============================================
# 数据可视化项目 - 学习成绩分析仪表盘
# 日期：2026-06-07（第26天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【项目说明】

项目名称：学习成绩分析仪表盘
功能：
  1. 从 CSV 文件读取学生成绩数据
  2. 生成多维度分析图表（趋势、分布、对比、排名）
  3. 保存为高清 PNG 图片
  4. 输出文字分析报告

数据来源：程序自动生成模拟数据（无需外部文件）
"""

import matplotlib.pyplot as plt
import matplotlib
import csv
import json
import os
import random
from pathlib import Path
from datetime import datetime, timedelta

# 中文字体设置
matplotlib.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei']
matplotlib.rcParams['axes.unicode_minus'] = False

# 输出目录
OUTPUT_DIR = Path('E:/Code/ProjectsCode/WorkSpace/ClaudeCode/StudyCode/Python/python_basics')

# ============================================
# 第一部分：数据生成与读写
# ============================================

def generate_student_data():
    """生成模拟学生成绩数据"""
    random.seed(42)  # 固定随机种子，保证每次结果一致
    students = [f'学生{chr(65+i)}' for i in range(8)]  # 学生A~H
    subjects = ['Python', '数学', '英语', '物理', '化学']
    months = ['3月', '4月', '5月', '6月']

    data = []
    for student in students:
        base = random.randint(55, 85)  # 每个学生的基础分
        for month in months:
            scores = {}
            for subject in subjects:
                # 在基础分附近浮动，模拟真实成绩
                score = max(40, min(100, base + random.randint(-15, 20)))
                scores[subject] = score
            data.append({
                'student': student,
                'month': month,
                **scores
            })
    return data, students, subjects, months


def save_to_csv(data, filepath):
    """将数据保存为 CSV 文件"""
    if not data:
        return
    fieldnames = list(data[0].keys())
    with open(filepath, 'w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(data)
    print(f"数据已保存：{filepath}")


def load_from_csv(filepath):
    """从 CSV 文件读取数据"""
    data = []
    with open(filepath, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            # CSV 读取的都是字符串，需要转换数字字段
            for key in row:
                if key not in ('student', 'month'):
                    row[key] = int(row[key])
            data.append(row)
    return data


# ============================================
# 第二部分：数据分析函数
# ============================================

def calc_avg_by_student(data, subjects):
    """计算每个学生的平均分"""
    student_scores = {}
    for row in data:
        s = row['student']
        if s not in student_scores:
            student_scores[s] = []
        for subj in subjects:
            student_scores[s].append(row[subj])
    return {s: round(sum(scores) / len(scores), 1)
            for s, scores in student_scores.items()}


def calc_avg_by_subject(data, subjects):
    """计算每个科目的平均分"""
    return {
        subj: round(sum(row[subj] for row in data) / len(data), 1)
        for subj in subjects
    }


def calc_trend_by_month(data, subjects, months):
    """计算每月全班各科平均分（趋势数据）"""
    trend = {}
    for month in months:
        month_rows = [r for r in data if r['month'] == month]
        trend[month] = {
            subj: round(sum(r[subj] for r in month_rows) / len(month_rows), 1)
            for subj in subjects
        }
    return trend


def calc_score_distribution(data, subjects):
    """统计成绩段分布（各分段人次）"""
    bins = {'优秀(90+)': 0, '良好(75-89)': 0, '中等(60-74)': 0, '不及格(<60)': 0}
    for row in data:
        for subj in subjects:
            score = row[subj]
            if score >= 90:
                bins['优秀(90+)'] += 1
            elif score >= 75:
                bins['良好(75-89)'] += 1
            elif score >= 60:
                bins['中等(60-74)'] += 1
            else:
                bins['不及格(<60)'] += 1
    return bins


# ============================================
# 第三部分：图表生成
# ============================================

def plot_dashboard(data, students, subjects, months, output_path):
    """生成完整的成绩分析仪表盘（2x3 布局）"""

    # 预计算所有数据
    avg_by_student = calc_avg_by_student(data, subjects)
    avg_by_subject = calc_avg_by_subject(data, subjects)
    trend = calc_trend_by_month(data, subjects, months)
    dist = calc_score_distribution(data, subjects)

    # 创建 2x3 布局
    fig, axes = plt.subplots(2, 3, figsize=(18, 11))
    fig.suptitle('学生成绩分析仪表盘', fontsize=20, fontweight='bold', y=0.98)

    colors_main = ['#2196F3', '#4CAF50', '#FF9800', '#E91E63', '#9C27B0']

    # ── 图1（左上）：各学生平均分排名（水平柱状图）──
    ax = axes[0, 0]
    sorted_students = sorted(avg_by_student.items(), key=lambda x: x[1])
    names = [s[0] for s in sorted_students]
    avgs = [s[1] for s in sorted_students]
    bar_colors = ['#4CAF50' if a >= 75 else '#FF9800' if a >= 60 else '#F44336'
                  for a in avgs]
    bars = ax.barh(names, avgs, color=bar_colors, alpha=0.85)
    ax.set_title('学生平均分排名', fontsize=13)
    ax.set_xlabel('平均分')
    ax.axvline(x=75, color='green', linestyle='--', alpha=0.6, label='良好线(75)')
    ax.axvline(x=60, color='red', linestyle='--', alpha=0.6, label='及格线(60)')
    ax.legend(fontsize=8)
    for bar, val in zip(bars, avgs):
        ax.text(val + 0.5, bar.get_y() + bar.get_height()/2,
                f'{val}', va='center', fontsize=9)
    ax.set_xlim(0, 105)

    # ── 图2（中上）：各科目平均分（柱状图）──
    ax = axes[0, 1]
    bars = ax.bar(subjects, [avg_by_subject[s] for s in subjects],
                  color=colors_main, alpha=0.85)
    ax.set_title('各科目平均分', fontsize=13)
    ax.set_ylabel('平均分')
    ax.set_ylim(0, 105)
    ax.axhline(y=75, color='green', linestyle='--', alpha=0.5)
    ax.grid(axis='y', alpha=0.3)
    for bar in bars:
        h = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2, h + 1,
                f'{h}', ha='center', fontsize=9)

    # ── 图3（右上）：成绩段分布（饼图）──
    ax = axes[0, 2]
    dist_colors = ['#4CAF50', '#2196F3', '#FF9800', '#F44336']
    wedges, texts, autotexts = ax.pie(
        list(dist.values()),
        labels=list(dist.keys()),
        autopct='%1.1f%%',
        colors=dist_colors,
        startangle=90
    )
    for at in autotexts:
        at.set_fontsize(9)
    ax.set_title('成绩段分布（全科汇总）', fontsize=13)

    # ── 图4（左下）：月度趋势折线图（5科）──
    ax = axes[1, 0]
    for i, subj in enumerate(subjects):
        monthly_avg = [trend[m][subj] for m in months]
        ax.plot(months, monthly_avg, 'o-',
                color=colors_main[i], linewidth=2,
                markersize=6, label=subj)
    ax.set_title('各科月度平均分趋势', fontsize=13)
    ax.set_ylabel('平均分')
    ax.legend(fontsize=8, loc='lower right')
    ax.grid(True, alpha=0.3)
    ax.set_ylim(50, 100)

    # ── 图5（中下）：最新月份各学生各科雷达式柱状图（top3学生）──
    ax = axes[1, 1]
    last_month = months[-1]
    top3 = sorted(avg_by_student.items(), key=lambda x: x[1], reverse=True)[:3]
    top3_names = [s[0] for s in top3]
    x = range(len(subjects))
    width = 0.25
    for i, name in enumerate(top3_names):
        student_row = next(r for r in data
                           if r['student'] == name and r['month'] == last_month)
        scores = [student_row[subj] for subj in subjects]
        offset = (i - 1) * width
        bars = ax.bar([xi + offset for xi in x], scores,
                      width, label=name,
                      color=colors_main[i], alpha=0.8)
    ax.set_title(f'{last_month} Top3学生各科成绩', fontsize=13)
    ax.set_xticks(list(x))
    ax.set_xticklabels(subjects)
    ax.set_ylabel('分数')
    ax.set_ylim(0, 110)
    ax.legend(fontsize=9)
    ax.grid(axis='y', alpha=0.3)

    # ── 图6（右下）：全班总分趋势（面积图）──
    ax = axes[1, 2]
    monthly_total_avg = []
    for month in months:
        month_rows = [r for r in data if r['month'] == month]
        all_scores = [r[subj] for r in month_rows for subj in subjects]
        monthly_total_avg.append(round(sum(all_scores) / len(all_scores), 1))

    ax.fill_between(months, monthly_total_avg, alpha=0.3, color='steelblue')
    ax.plot(months, monthly_total_avg, 'o-',
            color='steelblue', linewidth=2.5, markersize=8)
    for i, (m, v) in enumerate(zip(months, monthly_total_avg)):
        ax.annotate(f'{v}', xy=(i, v), xytext=(0, 12),
                    textcoords='offset points', ha='center', fontsize=10,
                    fontweight='bold')
    ax.set_title('全班月度综合平均分趋势', fontsize=13)
    ax.set_ylabel('综合平均分')
    ax.set_ylim(50, 100)
    ax.grid(True, alpha=0.3)

    plt.tight_layout(rect=[0, 0, 1, 0.96])
    plt.savefig(output_path, dpi=130, bbox_inches='tight')
    plt.close()
    print(f"仪表盘已保存：{output_path}")


# ============================================
# 第四部分：文字报告生成
# ============================================

def generate_report(data, students, subjects, months):
    """生成文字分析报告"""
    avg_by_student = calc_avg_by_student(data, subjects)
    avg_by_subject = calc_avg_by_subject(data, subjects)
    dist = calc_score_distribution(data, subjects)
    total = sum(dist.values())

    top_student = max(avg_by_student, key=avg_by_student.get)
    low_student = min(avg_by_student, key=avg_by_student.get)
    best_subject = max(avg_by_subject, key=avg_by_subject.get)
    weak_subject = min(avg_by_subject, key=avg_by_subject.get)

    report = f"""
{'=' * 50}
学习成绩分析报告
生成时间：{datetime.now().strftime('%Y-%m-%d %H:%M')}
{'=' * 50}

【学生排名 TOP3】
"""
    ranked = sorted(avg_by_student.items(), key=lambda x: x[1], reverse=True)
    for i, (name, avg) in enumerate(ranked[:3], 1):
        report += f"  {i}. {name}：{avg}分\n"

    report += f"""
【科目表现】
  最强科目：{best_subject}（均分 {avg_by_subject[best_subject]}）
  薄弱科目：{weak_subject}（均分 {avg_by_subject[weak_subject]}）

【成绩分布】
  优秀(90+)：{dist['优秀(90+)']}/{total} ({dist['优秀(90+)']/total*100:.1f}%)
  良好(75-89)：{dist['良好(75-89)']}/{total} ({dist['良好(75-89)']/total*100:.1f}%)
  中等(60-74)：{dist['中等(60-74)']}/{total} ({dist['中等(60-74)']/total*100:.1f}%)
  不及格(<60)：{dist['不及格(<60)']}/{total} ({dist['不及格(<60)']/total*100:.1f}%)

【建议】
  - 重点关注 {low_student}（均分 {avg_by_student[low_student]}），需要额外辅导
  - 加强 {weak_subject} 科目的整体教学质量
{'=' * 50}
"""
    return report


# ============================================
# 主程序
# ============================================

def main():
    print("=" * 60)
    print("学生成绩分析仪表盘")
    print("=" * 60)

    # 1. 生成数据
    print("\n[1/4] 生成模拟数据...")
    data, students, subjects, months = generate_student_data()
    print(f"  生成 {len(data)} 条成绩记录（{len(students)} 名学生 × {len(months)} 个月）")

    # 2. 保存为 CSV
    print("\n[2/4] 保存数据到 CSV...")
    csv_path = OUTPUT_DIR / 'student_scores.csv'
    save_to_csv(data, csv_path)

    # 3. 从 CSV 重新加载（演示完整流程）
    print("\n[3/4] 从 CSV 读取数据...")
    loaded_data = load_from_csv(csv_path)
    print(f"  成功读取 {len(loaded_data)} 条记录")

    # 4. 生成图表
    print("\n[4/4] 生成可视化仪表盘...")
    dashboard_path = OUTPUT_DIR / 'output_成绩仪表盘.png'
    plot_dashboard(loaded_data, students, subjects, months, dashboard_path)

    # 5. 输出报告
    report = generate_report(loaded_data, students, subjects, months)
    print(report)

    # 6. 保存报告
    report_path = OUTPUT_DIR / 'output_成绩报告.txt'
    report_path.write_text(report, encoding='utf-8')
    print(f"文字报告已保存：{report_path}")

    print("\n所有输出文件：")
    print(f"  - student_scores.csv（原始数据）")
    print(f"  - output_成绩仪表盘.png（6图仪表盘）")
    print(f"  - output_成绩报告.txt（文字分析）")


if __name__ == '__main__':
    main()
