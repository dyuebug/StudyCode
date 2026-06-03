# ============================================
# 综合练习题实战 - 巩固所学知识
# 日期：2026-06-03（第17天）
# 难度：⭐⭐⭐⭐ 综合应用
# ============================================

"""
【练习目标】

通过4个综合项目，巩固前16天学到的所有知识：
1. 学生管理系统（综合练习）
2. 日志分析工具
3. 数据处理工具
4. 简单爬虫（可选）

练习要求：
- 独立完成，不看答案
- 综合运用所学知识
- 注重代码质量
- 完整的错误处理

练习方法：
- 先理解需求
- 设计数据结构和类
- 分步实现功能
- 测试和调试
"""

# ============================================
# 项目1：学生管理系统（综合练习）⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("项目1：学生管理系统（综合练习）")
print("=" * 60)

print("""
【项目需求】

开发一个学生管理系统，实现以下功能：

1. 添加学生
   - 输入：学号、姓名、年龄、成绩
   - 验证：学号唯一、年龄合理、成绩0-100

2. 查看学生
   - 显示所有学生信息
   - 按成绩排序显示
   - 查找指定学号的学生

3. 修改成绩
   - 输入学号和新成绩
   - 验证成绩范围
   - 更新学生成绩

4. 删除学生
   - 输入学号
   - 确认删除
   - 从系统中移除

5. 统计分析
   - 计算平均分
   - 找出最高分和最低分
   - 统计各分数段人数

6. 数据持久化
   - 保存到JSON文件
   - 从JSON文件加载
   - 自动保存

【技术要点】

- 面向对象设计（Student类、StudentManager类）
- 文件操作（JSON读写）
- 异常处理（输入验证、文件操作）
- 数据验证（学号唯一性、成绩范围）
- 列表和字典操作

【实现提示】

1. Student类
   - 属性：学号、姓名、年龄、成绩
   - 方法：to_dict()、from_dict()、__str__()

2. StudentManager类
   - 属性：学生列表、数据文件路径
   - 方法：add_student()、remove_student()、update_score()
   - 方法：list_students()、find_student()、statistics()
   - 方法：load_data()、save_data()

3. 主程序
   - 创建管理器实例
   - 提供菜单选项
   - 调用相应功能
""")

# 示例代码框架
print("\n示例代码框架：")
print("-" * 60)

import json
import os

class Student:
    """学生类"""

    def __init__(self, student_id, name, age, score):
        self.student_id = student_id
        self.name = name
        self.age = age
        self.score = score

    def to_dict(self):
        """转换为字典"""
        return {
            'student_id': self.student_id,
            'name': self.name,
            'age': self.age,
            'score': self.score
        }

    @staticmethod
    def from_dict(data):
        """从字典创建学生对象"""
        return Student(
            data['student_id'],
            data['name'],
            data['age'],
            data['score']
        )

    def __str__(self):
        return f"学号：{self.student_id}，姓名：{self.name}，年龄：{self.age}，成绩：{self.score}"


class StudentManager:
    """学生管理器"""

    def __init__(self, data_file="students.json"):
        self.data_file = data_file
        self.students = []
        self.load_data()

    def load_data(self):
        """加载数据"""
        try:
            if os.path.exists(self.data_file):
                with open(self.data_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    self.students = [Student.from_dict(item) for item in data]
                print(f"成功加载 {len(self.students)} 个学生")
            else:
                print("数据文件不存在，创建新文件")
        except Exception as e:
            print(f"加载数据失败：{e}")

    def save_data(self):
        """保存数据"""
        try:
            data = [student.to_dict() for student in self.students]
            with open(self.data_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False, indent=2)
            print("数据保存成功")
        except Exception as e:
            print(f"保存数据失败：{e}")

    def add_student(self, student_id, name, age, score):
        """添加学生"""
        # TODO: 实现添加学生功能
        # 1. 验证学号是否已存在
        # 2. 验证年龄和成绩范围
        # 3. 创建学生对象
        # 4. 添加到列表
        # 5. 保存数据
        pass

    def remove_student(self, student_id):
        """删除学生"""
        # TODO: 实现删除学生功能
        pass

    def update_score(self, student_id, new_score):
        """修改成绩"""
        # TODO: 实现修改成绩功能
        pass

    def list_students(self, sort_by_score=False):
        """列出所有学生"""
        # TODO: 实现列出学生功能
        pass

    def find_student(self, student_id):
        """查找学生"""
        # TODO: 实现查找学生功能
        pass

    def statistics(self):
        """统计分析"""
        # TODO: 实现统计分析功能
        # 1. 计算平均分
        # 2. 找出最高分和最低分
        # 3. 统计各分数段人数
        pass


# 测试代码
print("\n测试代码：")
print("-" * 60)

# 创建管理器
manager = StudentManager("test_students.json")

# 添加学生（示例）
print("\n添加学生示例：")
student1 = Student("001", "张三", 20, 85)
student2 = Student("002", "李四", 21, 92)
student3 = Student("003", "王五", 19, 78)

manager.students.append(student1)
manager.students.append(student2)
manager.students.append(student3)

# 显示学生
print("\n学生列表：")
for student in manager.students:
    print(f"  {student}")

# 保存数据
manager.save_data()

print("""
【练习任务】

请完成以下功能：

1. 实现 add_student() 方法
   - 验证学号唯一性
   - 验证年龄（10-100）和成绩（0-100）
   - 添加学生并保存

2. 实现 remove_student() 方法
   - 查找学生
   - 删除学生
   - 保存数据

3. 实现 update_score() 方法
   - 查找学生
   - 验证成绩范围
   - 更新成绩并保存

4. 实现 list_students() 方法
   - 显示所有学生
   - 支持按成绩排序

5. 实现 find_student() 方法
   - 根据学号查找
   - 返回学生对象或None

6. 实现 statistics() 方法
   - 计算平均分
   - 找出最高分和最低分
   - 统计各分数段人数（优秀90+、良好80-89、及格60-79、不及格<60）

7. 添加主程序菜单
   - 提供用户交互界面
   - 循环显示菜单
   - 调用相应功能
""")

# ============================================
# 项目2：日志分析工具 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("项目2：日志分析工具")
print("=" * 60)

print("""
【项目需求】

开发一个日志分析工具，实现以下功能：

1. 读取日志文件
   - 支持多种日志格式
   - 逐行读取和解析

2. 统计日志级别
   - 统计DEBUG、INFO、WARNING、ERROR、CRITICAL数量
   - 计算各级别占比

3. 查找关键词
   - 搜索包含指定关键词的日志
   - 支持多个关键词
   - 显示匹配的日志行

4. 时间范围筛选
   - 筛选指定时间范围的日志
   - 支持日期格式解析

5. 生成分析报告
   - 总结日志统计信息
   - 列出重要错误
   - 保存报告到文件

【技术要点】

- 文件操作（逐行读取）
- 字符串处理（split、find、正则表达式）
- 字典统计（计数）
- 日期时间处理（datetime模块）
- 异常处理（文件不存在、格式错误）

【实现提示】

1. 日志格式示例
   2026-06-03 10:30:45 - INFO - 用户登录成功
   2026-06-03 10:31:20 - ERROR - 数据库连接失败

2. LogAnalyzer类
   - 方法：read_log()、count_levels()、search_keyword()
   - 方法：filter_by_time()、generate_report()

3. 数据结构
   - 使用字典统计各级别数量
   - 使用列表存储匹配的日志行
""")

print("""
【练习任务】

请实现以下功能：

1. 创建 LogAnalyzer 类
   - __init__(log_file)：初始化
   - read_log()：读取日志文件

2. 实现 count_levels() 方法
   - 统计各级别日志数量
   - 返回字典：{'INFO': 100, 'ERROR': 5, ...}

3. 实现 search_keyword() 方法
   - 搜索包含关键词的日志
   - 返回匹配的日志行列表

4. 实现 generate_report() 方法
   - 生成分析报告
   - 包含：总行数、各级别统计、错误日志列表
   - 保存到文件

5. 创建测试日志文件
   - 生成包含不同级别的日志
   - 测试分析功能

提示：
- 使用字符串的 split() 方法解析日志
- 使用字典的 get() 方法安全地计数
- 使用 with 语句确保文件正确关闭
""")

# ============================================
# 项目3：数据处理工具 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("项目3：数据处理工具")
print("=" * 60)

print("""
【项目需求】

开发一个数据处理工具，实现以下功能：

1. 读取CSV数据
   - 解析CSV格式
   - 处理表头
   - 转换为数据结构

2. 数据清洗
   - 删除重复数据
   - 处理缺失值
   - 数据类型转换

3. 数据分析
   - 计算统计指标（平均值、最大值、最小值）
   - 数据分组统计
   - 数据筛选

4. 数据可视化（文本）
   - 生成简单的文本图表
   - 显示数据分布

5. 生成报告
   - 汇总分析结果
   - 保存到文件

【技术要点】

- 文件操作（CSV读写）
- 列表和字典操作
- 数据处理算法
- 字符串格式化
- 异常处理

【实现提示】

1. CSV格式示例
   姓名,年龄,城市,工资
   张三,25,北京,8000
   李四,30,上海,12000

2. DataProcessor类
   - 方法：read_csv()、clean_data()、analyze()
   - 方法：filter_data()、generate_report()

3. 数据结构
   - 使用列表存储行数据
   - 使用字典表示每一行
""")

print("""
【练习任务】

请实现以下功能：

1. 创建 DataProcessor 类
   - __init__(csv_file)：初始化
   - read_csv()：读取CSV文件

2. 实现 clean_data() 方法
   - 删除重复行
   - 处理缺失值（删除或填充）
   - 数据类型转换

3. 实现 analyze() 方法
   - 计算数值列的统计指标
   - 返回分析结果字典

4. 实现 filter_data() 方法
   - 根据条件筛选数据
   - 支持多个条件

5. 实现 generate_report() 方法
   - 生成分析报告
   - 保存到文件

6. 创建测试CSV文件
   - 包含多列数据
   - 包含一些重复和缺失值
   - 测试处理功能

提示：
- 使用 split(',') 解析CSV行
- 使用列表推导式处理数据
- 使用 try-except 处理类型转换错误
""")

# ============================================
# 项目4：简单爬虫（可选）⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("项目4：简单爬虫（可选）")
print("=" * 60)

print("""
【项目需求】

开发一个简单的网页爬虫，实现以下功能：

1. 获取网页内容
   - 使用requests库
   - 处理HTTP错误
   - 设置超时

2. 解析网页数据
   - 提取标题
   - 提取链接
   - 提取文本内容

3. 数据存储
   - 保存到文件
   - 结构化存储（JSON）

4. 错误处理
   - 网络错误
   - 超时处理
   - 编码问题

【技术要点】

- requests库（HTTP请求）
- 字符串处理（查找、提取）
- 正则表达式（可选）
- 文件操作
- 异常处理

【实现提示】

1. WebCrawler类
   - 方法：fetch_page()、parse_content()、save_data()

2. 安全注意事项
   - 遵守robots.txt
   - 设置合理的请求间隔
   - 不要过度请求

3. 简单解析方法
   - 使用字符串的 find() 方法
   - 提取 <title> 标签内容
   - 提取 <a href="..."> 链接
""")

print("""
【练习任务】

请实现以下功能：

1. 创建 WebCrawler 类
   - __init__(url)：初始化
   - fetch_page()：获取网页内容

2. 实现 parse_title() 方法
   - 提取网页标题
   - 返回标题字符串

3. 实现 parse_links() 方法
   - 提取所有链接
   - 返回链接列表

4. 实现 save_data() 方法
   - 保存提取的数据
   - 使用JSON格式

5. 完整的错误处理
   - 处理网络错误
   - 处理超时
   - 处理编码问题

6. 测试爬虫
   - 爬取一个简单的网页
   - 提取并保存数据

提示：
- 使用 requests.get(url, timeout=5)
- 使用 response.raise_for_status() 检查状态码
- 使用 response.text 获取网页内容
- 使用字符串的 find() 和切片提取内容

注意：
- 这是一个简化的爬虫示例
- 实际项目建议使用 BeautifulSoup 或 lxml 库
- 遵守网站的使用条款和 robots.txt
""")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结")
print("=" * 60)

print("""
【练习总结】

通过这4个综合项目，你将练习到：

1. 学生管理系统
   ✅ 面向对象设计
   ✅ 文件操作（JSON）
   ✅ 数据验证
   ✅ 异常处理
   ✅ 列表和字典操作

2. 日志分析工具
   ✅ 文件读取
   ✅ 字符串处理
   ✅ 字典统计
   ✅ 数据分析

3. 数据处理工具
   ✅ CSV文件处理
   ✅ 数据清洗
   ✅ 数据分析
   ✅ 报告生成

4. 简单爬虫（可选）
   ✅ HTTP请求
   ✅ 网页解析
   ✅ 数据存储
   ✅ 错误处理

【学习建议】

1. 独立完成
   - 不要直接看答案
   - 遇到问题先思考
   - 查阅文档和笔记

2. 注重质量
   - 代码结构清晰
   - 命名规范
   - 注释适当
   - 完整的错误处理

3. 测试验证
   - 测试正常情况
   - 测试边界情况
   - 测试异常情况

4. 总结反思
   - 遇到了什么困难？
   - 如何解决的？
   - 有什么收获？

【下一步】

完成练习后：
- 整理知识体系
- 总结学习经验
- 规划下一步学习

加油！通过这些练习，你将真正掌握Python编程！💪
""")
