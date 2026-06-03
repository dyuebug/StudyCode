# ============================================
# 第三方库综合练习 - 实战项目
# 日期：2026-06-01（第15天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【综合练习目标】

通过实际项目综合练习第三方库：
1. pip 包管理
2. 虚拟环境使用
3. requests 库（HTTP 请求）
4. pandas 库（数据处理）

这个练习重点是体验：
- 如何安装和使用第三方库
- 如何使用 requests 获取数据
- 如何使用 pandas 处理数据
- 如何综合运用多个库

注意：
- 本文件中的代码是示例代码
- 实际使用需要先安装对应的库
- 网络请求需要网络连接
"""

print("=" * 60)
print("第三方库综合练习")
print("=" * 60)

print("""
【重要提示】

本练习文件包含第三方库的使用示例。

在运行代码前，需要先安装对应的库：
$ pip install requests
$ pip install pandas

如果没有安装，代码会报错：
ModuleNotFoundError: No module named 'requests'

建议：
1. 先阅读代码，理解功能
2. 在命令行安装需要的库
3. 再运行代码进行实践
""")

# ============================================
# 项目1：requests 基础使用（模拟示例）
# ============================================

print("\n" + "=" * 60)
print("项目1：requests 基础使用（模拟示例）")
print("=" * 60)

print("""
【requests 库使用示例】

安装：
$ pip install requests

基本用法：
import requests

# GET 请求
response = requests.get('https://api.github.com')
print(response.status_code)  # 200
print(response.json())  # JSON 数据

# POST 请求
data = {'key': 'value'}
response = requests.post('https://example.com/api', json=data)

# 异常处理
try:
    response = requests.get('https://example.com', timeout=5)
    response.raise_for_status()
    data = response.json()
except requests.exceptions.RequestException as e:
    print(f"请求失败：{e}")
""")

# 模拟 requests 使用（不实际发送请求）
print("\n模拟示例：requests 获取数据")
print("-" * 60)

# 模拟响应数据
mock_response = {
    "status": "success",
    "data": [
        {"id": 1, "name": "张三", "score": 85},
        {"id": 2, "name": "李四", "score": 92},
        {"id": 3, "name": "王五", "score": 78}
    ]
}

print("模拟 API 响应：")
print(mock_response)

# ============================================
# 项目2：pandas 基础使用（实际示例）
# ============================================

print("\n" + "=" * 60)
print("项目2：pandas 基础使用（实际示例）")
print("=" * 60)

print("""
【pandas 库使用示例】

安装：
$ pip install pandas

基本用法：
import pandas as pd

# 创建 DataFrame
data = {'name': ['张三', '李四'], 'age': [25, 30]}
df = pd.DataFrame(data)

# 读取 CSV
df = pd.read_csv('data.csv')

# 数据筛选
df[df['age'] > 25]

# 数据统计
df.describe()
""")

# 实际使用 pandas（如果已安装）
print("\n实际示例：pandas 数据处理")
print("-" * 60)

try:
    import pandas as pd

    print("✅ pandas 已安装，运行实际示例")

    # 创建学生数据
    students_data = {
        'name': ['张三', '李四', '王五', '赵六', '钱七'],
        'age': [20, 21, 19, 22, 20],
        'score': [85, 92, 78, 95, 88],
        'city': ['北京', '上海', '广州', '深圳', '杭州']
    }

    df = pd.DataFrame(students_data)

    print("\n原始数据：")
    print(df)

    print("\n数据信息：")
    print(f"行数：{len(df)}")
    print(f"列数：{len(df.columns)}")
    print(f"列名：{list(df.columns)}")

    print("\n筛选成绩大于 85 的学生：")
    high_score = df[df['score'] > 85]
    print(high_score)

    print("\n统计信息：")
    print(f"平均成绩：{df['score'].mean():.2f}")
    print(f"最高成绩：{df['score'].max()}")
    print(f"最低成绩：{df['score'].min()}")

    print("\n按成绩排序：")
    sorted_df = df.sort_values('score', ascending=False)
    print(sorted_df)

except ImportError:
    print("❌ pandas 未安装")
    print("请先安装：pip install pandas")
    print("\n模拟数据处理结果：")
    print("平均成绩：87.6")
    print("最高成绩：95")
    print("最低成绩：78")

# ============================================
# 项目3：requests + pandas 综合应用
# ============================================

print("\n" + "=" * 60)
print("项目3：requests + pandas 综合应用")
print("=" * 60)

print("""
【综合应用场景】

1. 使用 requests 从 API 获取数据
2. 将 JSON 数据转换为 pandas DataFrame
3. 使用 pandas 进行数据分析
4. 生成分析报告

这是实际项目中常见的数据处理流程。
""")

# 模拟综合应用
print("\n模拟示例：数据获取和分析流程")
print("-" * 60)

# 模拟 API 数据
mock_api_data = [
    {"date": "2026-06-01", "sales": 1200, "visitors": 150},
    {"date": "2026-06-02", "sales": 1500, "visitors": 180},
    {"date": "2026-06-03", "sales": 1100, "visitors": 140},
    {"date": "2026-06-04", "sales": 1800, "visitors": 200},
    {"date": "2026-06-05", "sales": 1600, "visitors": 190}
]

print("步骤1：模拟从 API 获取数据")
print(f"获取到 {len(mock_api_data)} 条记录")

try:
    import pandas as pd

    print("\n步骤2：转换为 pandas DataFrame")
    df = pd.DataFrame(mock_api_data)
    print(df)

    print("\n步骤3：数据分析")
    print(f"总销售额：{df['sales'].sum()}")
    print(f"平均销售额：{df['sales'].mean():.2f}")
    print(f"总访客数：{df['visitors'].sum()}")
    print(f"平均访客数：{df['visitors'].mean():.2f}")
    print(f"转化率：{(df['sales'].sum() / df['visitors'].sum()):.2f}")

    print("\n步骤4：找出最佳销售日")
    best_day = df.loc[df['sales'].idxmax()]
    print(f"最佳销售日：{best_day['date']}")
    print(f"销售额：{best_day['sales']}")
    print(f"访客数：{best_day['visitors']}")

except ImportError:
    print("\n❌ pandas 未安装，无法进行数据分析")
    print("请安装：pip install pandas")

# ============================================
# 项目4：虚拟环境实践指南
# ============================================

print("\n" + "=" * 60)
print("项目4：虚拟环境实践指南")
print("=" * 60)

print("""
【虚拟环境完整流程】

步骤1：创建项目目录
$ mkdir myproject
$ cd myproject

步骤2：创建虚拟环境
$ python -m venv venv

步骤3：激活虚拟环境
Windows: venv\\Scripts\\activate
Mac/Linux: source venv/bin/activate

步骤4：安装依赖
(venv) $ pip install requests
(venv) $ pip install pandas

步骤5：导出依赖列表
(venv) $ pip freeze > requirements.txt

步骤6：其他人使用
$ python -m venv venv
$ venv\\Scripts\\activate
(venv) $ pip install -r requirements.txt

步骤7：退出虚拟环境
(venv) $ deactivate

【最佳实践】

1. 每个项目一个虚拟环境
2. 虚拟环境不要提交到 git
3. 使用 requirements.txt 记录依赖
4. 激活虚拟环境后再安装包
""")

# ============================================
# 项目5：错误处理实践
# ============================================

print("\n" + "=" * 60)
print("项目5：错误处理实践")
print("=" * 60)

class DataFetcher:
    """数据获取器：演示错误处理"""

    @staticmethod
    def fetch_data(url):
        """获取数据（带完整错误处理）"""
        print(f"\n尝试获取数据：{url}")

        try:
            # 检查是否安装 requests
            import requests

            # 发送请求
            response = requests.get(url, timeout=5)

            # 检查状态码
            response.raise_for_status()

            # 解析 JSON
            data = response.json()

            print("✅ 数据获取成功")
            return data

        except ImportError:
            print("❌ requests 库未安装")
            print("请安装：pip install requests")
            return None

        except requests.exceptions.Timeout:
            print("❌ 请求超时")
            return None

        except requests.exceptions.ConnectionError:
            print("❌ 网络连接失败")
            return None

        except requests.exceptions.HTTPError as e:
            print(f"❌ HTTP 错误：{e}")
            return None

        except ValueError:
            print("❌ JSON 解析失败")
            return None

        except Exception as e:
            print(f"❌ 未知错误：{e}")
            return None

# 测试错误处理
print("\n测试错误处理：")
fetcher = DataFetcher()

# 测试1：无效 URL（会失败）
print("\n测试1：无效 URL")
result = fetcher.fetch_data("https://invalid-url-12345.com")
print(f"结果：{result}")

# ============================================
# 项目6：pandas 数据清洗实践
# ============================================

print("\n" + "=" * 60)
print("项目6：pandas 数据清洗实践")
print("=" * 60)

print("""
【数据清洗常见任务】

1. 处理缺失值
2. 删除重复数据
3. 数据类型转换
4. 数据筛选和过滤
5. 数据排序
""")

try:
    import pandas as pd

    print("\n✅ pandas 已安装，运行数据清洗示例")

    # 创建包含问题的数据
    dirty_data = {
        'name': ['张三', '李四', '王五', '李四', '赵六', None],
        'age': [25, 30, '28', 30, 22, 26],
        'score': [85, 92, 78, 92, None, 88],
        'city': ['北京', '上海', '广州', '上海', '深圳', '杭州']
    }

    df = pd.DataFrame(dirty_data)

    print("\n原始数据（包含问题）：")
    print(df)
    print(f"\n数据形状：{df.shape}")

    # 1. 处理缺失值
    print("\n步骤1：检查缺失值")
    print(df.isnull().sum())

    print("\n删除包含缺失值的行：")
    df_clean = df.dropna()
    print(df_clean)

    # 2. 删除重复数据
    print("\n步骤2：删除重复数据")
    print(f"重复行数：{df.duplicated().sum()}")
    df_clean = df.drop_duplicates()
    print(f"删除后行数：{len(df_clean)}")

    # 3. 数据类型转换
    print("\n步骤3：数据类型转换")
    print("原始数据类型：")
    print(df.dtypes)

    # 4. 数据筛选
    print("\n步骤4：数据筛选")
    print("筛选年龄大于 25 的记录：")
    # 注意：这里会有类型问题，因为 age 列包含字符串
    print("（需要先处理数据类型）")

except ImportError:
    print("\n❌ pandas 未安装")
    print("请安装：pip install pandas")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：第三方库的价值")
print("=" * 60)

print("""
通过今天的综合练习，你已经了解了：

1. pip 包管理工具
   - 安装、卸载、查看包
   - 导出和安装依赖列表
   - 使用国内镜像源加速

2. 虚拟环境
   - 为什么需要虚拟环境
   - 如何创建和使用虚拟环境
   - 虚拟环境的最佳实践

3. requests 库
   - 发送 HTTP 请求
   - 处理响应数据
   - 异常处理和错误处理

4. pandas 库
   - 创建和操作 DataFrame
   - 数据筛选和统计
   - 数据清洗和处理

5. 综合应用
   - requests + pandas 数据处理流程
   - 完整的错误处理
   - 实际项目中的应用场景

现在你对第三方库的理解已经从"知道有这些库"进步到：
你开始理解"如何安装和使用第三方库"。
""")

# ============================================
# 附加练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("附加练习题（自己动手）")
print("=" * 60)

print("""
练习1：安装和测试 requests
  要求：
  - 在命令行安装 requests 库
  - 编写代码访问 https://api.github.com
  - 打印响应状态码和部分内容
  - 添加完整的异常处理

练习2：创建项目虚拟环境
  要求：
  - 创建一个新项目目录
  - 创建虚拟环境
  - 激活虚拟环境
  - 安装 requests 和 pandas
  - 导出 requirements.txt

练习3：pandas 数据分析
  要求：
  - 创建一个包含 10 个学生信息的 DataFrame
  - 包含：姓名、年龄、成绩、城市
  - 筛选成绩前 3 名的学生
  - 计算各城市的平均成绩
  - 找出年龄最大和最小的学生

练习4：数据获取和处理
  要求：
  - 模拟从 API 获取商品数据
  - 数据包含：商品名、价格、销量
  - 使用 pandas 进行数据分析
  - 找出最畅销的商品
  - 计算总销售额

练习5：错误处理实践
  要求：
  - 编写一个函数 safe_request(url)
  - 处理所有可能的异常
  - 返回数据或错误信息
  - 添加日志记录（使用 logging）
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了第三方库的学习！

你已经掌握了：
✅ pip 包管理工具的使用
✅ 虚拟环境的创建和管理
✅ requests 库的基本使用
✅ pandas 库的基本操作
✅ 第三方库的综合应用

下一步：
- 完成附加练习题
- 实际安装和使用这些库
- 在项目中应用第三方库
- 探索更多有用的第三方库

记住：
- 第三方库是 Python 生态的核心优势
- pip 是必须掌握的工具
- 虚拟环境隔离项目依赖
- 学会查阅官方文档
""")




