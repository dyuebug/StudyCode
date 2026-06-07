# ============================================
# matplotlib 基础 - 超详细注释版
# 日期：2026-06-07（第26天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】

是什么？
- matplotlib 是 Python 最流行的数据可视化库
- 可以绘制折线图、柱状图、饼图、散点图等

为什么需要？
- 数据用图表展示比数字更直观
- 科学计算、数据分析的标配工具

核心特点：
- pyplot 模块提供类似 MATLAB 的绘图接口
- 高度可定制：颜色、字体、样式、布局
- 支持保存为 PNG、PDF、SVG 等格式
"""

import matplotlib.pyplot as plt
import matplotlib
import numpy as np

# 解决中文显示问题（Windows）
matplotlib.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'Arial Unicode MS']
matplotlib.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

print("""
【matplotlib 两个核心对象】

1. Figure（画布）
   - 整个绘图窗口
   - 可以包含一个或多个子图（Axes）
   - 用 plt.figure() 创建

2. Axes（坐标轴/子图）
   - 实际绘图区域，包含 x轴、y轴、标题等
   - 一个 Figure 可以有多个 Axes
   - 用 plt.subplot() 或 plt.subplots() 创建

关系：Figure 是画布，Axes 是画布上的绘图区域
""")

print("""
【函数卡片：plt.figure()】

作用：创建画布
语法：fig = plt.figure(figsize=(宽, 高), dpi=分辨率)

参数：
- figsize: 元组，单位英寸，默认 (6.4, 4.8)
- dpi: 每英寸像素点数，默认 100

示例：
fig = plt.figure(figsize=(10, 6))  # 宽10英寸，高6英寸
""")

print("""
【函数卡片：plt.subplots()】

作用：一次性创建 Figure 和 Axes（推荐用法）
语法：fig, ax = plt.subplots(行数, 列数, figsize=(...))

参数：
- nrows: 子图行数，默认 1
- ncols: 子图列数，默认 1
- figsize: 画布大小

返回：(Figure对象, Axes对象 或 Axes数组)

示例：
fig, ax = plt.subplots()             # 单个子图
fig, axes = plt.subplots(1, 2)       # 1行2列，2个子图
fig, axes = plt.subplots(2, 2)       # 2行2列，4个子图
""")

# ============================================
# 第二部分：折线图 plot() ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第二部分：折线图 plot()")
print("=" * 60)

print("""
【函数卡片：plt.plot()】

作用：绘制折线图（连接数据点的线）
语法：plt.plot(x, y, 格式字符串, **kwargs)

常用参数：
- x: x轴数据（列表或数组）
- y: y轴数据（列表或数组）
- color / c: 线条颜色，如 'red'、'#FF5733'、'b'
- linewidth / lw: 线宽，默认 1.5
- linestyle / ls: 线型，'-'实线、'--'虚线、':'点线
- marker: 数据点标记，'o'圆点、's'方形、'^'三角
- markersize / ms: 标记大小
- label: 图例标签（配合 legend() 使用）

示例：
plt.plot(x, y, color='blue', linewidth=2, marker='o', label='数据')
""")

# 示例1：基础折线图
print("示例1：月销售额折线图")
print("-" * 60)

# 准备数据
months = ['1月', '2月', '3月', '4月', '5月', '6月']
sales = [12000, 15000, 13500, 18000, 22000, 19500]

# 创建画布和坐标轴
fig, ax = plt.subplots(figsize=(10, 5))

# 绘制折线图
# color: 线条颜色
# linewidth: 线条宽度
# marker: 数据点标记形状（'o' = 圆点）
# markersize: 标记大小
# label: 图例标签
ax.plot(months, sales,
        color='steelblue',
        linewidth=2,
        marker='o',
        markersize=8,
        label='月销售额')

# 设置标题和标签
ax.set_title('2026年上半年月销售额', fontsize=16, pad=15)
ax.set_xlabel('月份', fontsize=12)
ax.set_ylabel('销售额（元）', fontsize=12)

# 显示图例
ax.legend()

# 添加网格线（alpha控制透明度）
ax.grid(True, alpha=0.3)

# 在每个数据点上方显示数值
for i, (month, sale) in enumerate(zip(months, sales)):
    ax.annotate(f'{sale:,}',           # 格式化数字，加千位分隔符
                xy=(i, sale),          # 数据点坐标
                xytext=(0, 10),        # 文字偏移量（向上10像素）
                textcoords='offset points',
                ha='center',           # 水平居中
                fontsize=9)

plt.tight_layout()  # 自动调整布局，防止标签被截断
plt.savefig('E:/Code/ProjectsCode/WorkSpace/ClaudeCode/StudyCode/Python/python_basics/output_折线图.png',
            dpi=120, bbox_inches='tight')
plt.close()  # 关闭图形，释放内存
print("折线图已保存：output_折线图.png")

# ============================================
# 第三部分：柱状图 bar() ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第三部分：柱状图 bar()")
print("=" * 60)

print("""
【函数卡片：ax.bar()】

作用：绘制柱状图（比较不同类别的数值大小）
语法：ax.bar(x, height, width=0.8, color=..., **kwargs)

常用参数：
- x: x轴位置（字符串列表 或 数字数组）
- height: 柱子高度（数值列表）
- width: 柱子宽度，默认 0.8
- color: 柱子颜色
- edgecolor: 柱子边框颜色
- alpha: 透明度（0~1）
- label: 图例标签

返回：BarContainer 对象（可用于添加数值标签）
""")

# 示例2：产品销量柱状图
print("示例2：各产品季度销量柱状图（分组柱状图）")
print("-" * 60)

products = ['产品A', '产品B', '产品C', '产品D']
q1_sales = [45, 62, 38, 71]
q2_sales = [58, 55, 65, 48]

x = np.arange(len(products))  # [0, 1, 2, 3]
width = 0.35                   # 每根柱子的宽度

fig, ax = plt.subplots(figsize=(10, 6))

# 绘制两组柱状图
# x - width/2: 第一组柱子向左偏移
# x + width/2: 第二组柱子向右偏移
bars1 = ax.bar(x - width/2, q1_sales, width,
               color='steelblue', label='Q1', alpha=0.85)
bars2 = ax.bar(x + width/2, q2_sales, width,
               color='coral', label='Q2', alpha=0.85)

ax.set_title('各产品季度销量对比', fontsize=16)
ax.set_xlabel('产品', fontsize=12)
ax.set_ylabel('销量（件）', fontsize=12)
ax.set_xticks(x)           # 设置x轴刻度位置
ax.set_xticklabels(products)  # 设置x轴刻度标签
ax.legend()
ax.grid(axis='y', alpha=0.3)  # 只显示y轴网格线

# 在柱子顶部添加数值标签
def add_value_labels(bars):
    for bar in bars:
        height = bar.get_height()
        ax.annotate(f'{height}',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 5),
                    textcoords='offset points',
                    ha='center', fontsize=9)

add_value_labels(bars1)
add_value_labels(bars2)

plt.tight_layout()
plt.savefig('E:/Code/ProjectsCode/WorkSpace/ClaudeCode/StudyCode/Python/python_basics/output_柱状图.png',
            dpi=120, bbox_inches='tight')
plt.close()
print("柱状图已保存：output_柱状图.png")

# ============================================
# 第四部分：饼图 pie() ⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第四部分：饼图 pie()")
print("=" * 60)

print("""
【函数卡片：ax.pie()】

作用：绘制饼图（展示各部分占总体的比例）
语法：ax.pie(x, labels=..., autopct=..., explode=..., **kwargs)

常用参数：
- x: 各扇区的数值（自动计算百分比）
- labels: 每个扇区的标签
- autopct: 显示百分比格式，如 '%1.1f%%'
- explode: 各扇区的偏移量（突出显示），如 (0.1, 0, 0, 0)
- colors: 各扇区颜色列表
- startangle: 起始角度，默认 0（从右边开始）
- shadow: 是否显示阴影

返回：(patches, texts, autotexts) 元组
""")

# 示例3：市场份额饼图
print("示例3：市场份额饼图")
print("-" * 60)

brands = ['品牌A', '品牌B', '品牌C', '品牌D', '其他']
shares = [35, 25, 20, 12, 8]
colors = ['#2196F3', '#4CAF50', '#FF9800', '#E91E63', '#9E9E9E']
explode = (0.05, 0, 0, 0, 0)  # 第一个扇区略微突出

fig, ax = plt.subplots(figsize=(8, 8))

wedges, texts, autotexts = ax.pie(
    shares,
    labels=brands,
    autopct='%1.1f%%',    # 显示一位小数的百分比
    explode=explode,
    colors=colors,
    startangle=90,         # 从顶部开始
    shadow=True
)

# 美化百分比文字
for autotext in autotexts:
    autotext.set_fontsize(11)
    autotext.set_fontweight('bold')

ax.set_title('2026年市场份额分布', fontsize=16, pad=20)

plt.tight_layout()
plt.savefig('E:/Code/ProjectsCode/WorkSpace/ClaudeCode/StudyCode/Python/python_basics/output_饼图.png',
            dpi=120, bbox_inches='tight')
plt.close()
print("饼图已保存：output_饼图.png")

# ============================================
# 第五部分：多子图 subplots ⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第五部分：多子图布局")
print("=" * 60)

print("""
【多子图技巧】

fig, axes = plt.subplots(2, 2, figsize=(12, 10))

axes 是 2x2 的数组：
- axes[0, 0]：左上
- axes[0, 1]：右上
- axes[1, 0]：左下
- axes[1, 1]：右下

plt.subplots_adjust(hspace=0.4, wspace=0.3)  # 调整子图间距
""")

# 示例4：综合展示（2x2 子图）
print("示例4：综合图表（2x2布局）")

months_data = ['1月', '2月', '3月', '4月', '5月', '6月']
revenue = [8.2, 9.5, 8.8, 11.2, 13.5, 12.1]    # 万元
cost = [5.1, 5.8, 5.5, 6.8, 7.9, 7.2]
profit = [r - c for r, c in zip(revenue, cost)]
categories = ['电子', '服装', '食品', '家居']
cat_sales = [42, 28, 35, 19]

fig, axes = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle('2026年上半年业务综合报表', fontsize=18, fontweight='bold')

# 左上：收入折线图
axes[0, 0].plot(months_data, revenue, 'b-o', linewidth=2, label='收入')
axes[0, 0].plot(months_data, cost, 'r--s', linewidth=2, label='成本')
axes[0, 0].set_title('收入 vs 成本（万元）')
axes[0, 0].legend()
axes[0, 0].grid(True, alpha=0.3)

# 右上：利润柱状图
colors_bar = ['green' if p > 0 else 'red' for p in profit]
axes[0, 1].bar(months_data, profit, color=colors_bar, alpha=0.8)
axes[0, 1].set_title('月度利润（万元）')
axes[0, 1].axhline(y=0, color='black', linewidth=0.8)  # 添加零线
axes[0, 1].grid(axis='y', alpha=0.3)

# 左下：类别销量柱状图
axes[1, 0].barh(categories, cat_sales,
                color=['#2196F3', '#4CAF50', '#FF9800', '#9C27B0'],
                alpha=0.85)
axes[1, 0].set_title('各类别销量（件）')
axes[1, 0].grid(axis='x', alpha=0.3)

# 右下：饼图
axes[1, 1].pie(cat_sales, labels=categories, autopct='%1.0f%%',
               startangle=90,
               colors=['#2196F3', '#4CAF50', '#FF9800', '#9C27B0'])
axes[1, 1].set_title('类别销量占比')

plt.tight_layout()
plt.savefig('E:/Code/ProjectsCode/WorkSpace/ClaudeCode/StudyCode/Python/python_basics/output_综合图表.png',
            dpi=120, bbox_inches='tight')
plt.close()
print("综合图表已保存：output_综合图表.png")

# ============================================
# 第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
❌ 错误1：中文显示乱码（方块）
原因：matplotlib 默认字体不支持中文
✅ 正确：在文件开头设置字体
matplotlib.rcParams['font.sans-serif'] = ['SimHei']
matplotlib.rcParams['axes.unicode_minus'] = False

────────────────────────────────────

❌ 错误2：忘记 plt.close()，内存泄漏
原因：每次 plt.show() 或 savefig() 后图形仍在内存
✅ 正确：保存后立即关闭
plt.savefig('output.png')
plt.close()  # 或 plt.close('all')

────────────────────────────────────

❌ 错误3：x轴数据和y轴数据长度不一致
x = [1, 2, 3]
y = [10, 20]  # 长度不同 → ValueError
✅ 正确：确保 x 和 y 长度相同

────────────────────────────────────

❌ 错误4：图例不显示
原因：绘图时没有指定 label 参数
✅ 正确：
ax.plot(x, y, label='数据名称')  # 必须有 label
ax.legend()                       # 才能显示图例

────────────────────────────────────

❌ 错误5：标签/标题被截断
原因：图形边距不足
✅ 正确：使用 tight_layout() 自动调整
plt.tight_layout()
plt.savefig('output.png', bbox_inches='tight')

────────────────────────────────────

❌ 错误6：多次绘图叠加在同一张图上
原因：没有用 plt.close() 关闭前一张图
✅ 正确：每次绘图前确认是新的 figure
fig, ax = plt.subplots()   # 每次创建新图
# ... 绘图 ...
plt.close()
""")

# ============================================
# 第七部分：函数速查
# ============================================

print("=" * 60)
print("第七部分：函数速查")
print("=" * 60)

print("""
【常用函数速查】

创建：
  fig, ax = plt.subplots(figsize=(宽, 高))

折线图：
  ax.plot(x, y, color=, linewidth=, marker=, label=)

柱状图：
  ax.bar(x, height, width=0.8, color=, label=)
  ax.barh(y, width)  # 水平柱状图

饼图：
  ax.pie(x, labels=, autopct='%1.1f%%', explode=)

装饰：
  ax.set_title('标题', fontsize=)
  ax.set_xlabel('x轴标签')
  ax.set_ylabel('y轴标签')
  ax.legend()
  ax.grid(True, alpha=0.3)
  ax.set_xlim(最小值, 最大值)
  ax.set_ylim(最小值, 最大值)

保存/关闭：
  plt.tight_layout()
  plt.savefig('文件名.png', dpi=120, bbox_inches='tight')
  plt.close()
""")

# ============================================
# 练习题
# ============================================

print("=" * 60)
print("练习题")
print("=" * 60)

print("""
1. 绘制一个折线图，展示某城市一周的最高气温变化
   数据：['周一','周二','周三','周四','周五','周六','周日']
          [28, 30, 32, 29, 27, 31, 33]
   要求：添加数据点标记、网格线、标题和轴标签

2. 绘制一个柱状图，比较5名学生的Python成绩
   要求：给不同分数段设置不同颜色（<60红色，60-80黄色，>80绿色）

3. 绘制一个饼图，展示你一天时间分配
   （睡眠8h、工作/学习8h、娱乐3h、运动1h、其他4h）

4. 综合题：用 2x1 布局，左图展示折线图，右图展示对应的柱状图，
   数据相同，保存为一张 PNG 图片
""")

print("=" * 60)
print("matplotlib 基础学习完成！")
print("生成了4张图表：output_折线图.png、output_柱状图.png、output_饼图.png、output_综合图表.png")
print("=" * 60)
