# 阶段 1：Python 语法与数据结构详解

> **整理日期**：2026-10-30
> **数据范围**：`python_basics/01`～`23` 的实际示例文件、练习项目与测试。
> **状态说明**：本笔记是知识整理，不代表对应学习序号已有完成日志。示例代码均可在本仓库直接运行。
> **与代码冲突时**：以实际文件和运行结果为准，并在文末“待修正内容”记录。

---

## 1. 变量、类型与运算

### 1.1 变量是名字，不是盒子

Python 变量保存的是对象引用。赋值 `a = b` 让两个名字指向同一对象；对不可变对象“修改”会创建新对象，对可变对象原地修改会影响所有引用。

```python
a = [1, 2]
b = a
b.append(3)
print(a)        # [1, 2, 3]

c = a.copy()    # 浅拷贝
c.append(4)
print(a)        # [1, 2, 3]
```

- `is` 比较身份（`id`），`==` 比较值。
- 小整数、部分字符串可能被驻留，`is` 的结果不能作为通用判断依据。
- 判断 `None` 用 `is None`，不要用 `== None`。

### 1.2 常用类型速查

| 类型 | 可变 | 有序 | 可重复 | 典型用途 |
|---|---|---|---|---|
| `str` | 否 | 是 | 是 | 文本 |
| `list` | 是 | 是 | 是 | 有序集合、栈 |
| `tuple` | 否 | 是 | 是 | 固定结构、字典键 |
| `dict` | 是 | 是（3.7+ 插入序） | 键唯一 | 映射、计数 |
| `set` | 是 | 否 | 否 | 去重、集合运算 |

### 1.3 数字与精度

```python
print(0.1 + 0.2)            # 0.30000000000000004
print(round(0.1 + 0.2, 2))  # 0.3
from decimal import Decimal
print(Decimal("0.1") + Decimal("0.2"))  # 0.3
```

浮点数不能精确表示十进制小数。涉及金额时使用 `Decimal` 或以“分”为单位的整数。

---

## 2. 字符串

```python
text = "  Hello, Python  "
text.strip()                 # 'Hello, Python'
text.lower().replace("python", "world")
",".join(["a", "b", "c"])    # 'a,b,c'
"a,b,c".split(",")           # ['a', 'b', 'c']
f"{3.14159:.2f}"             # '3.14'
```

- 字符串不可变，所有“修改”方法都返回新字符串。
- `split()` 不带参数按任意空白切分，并丢弃空串；`split(",")` 保留空字段。
- 拼接大量字符串优先 `join`，避免循环内 `+=` 产生多次复制。
- 判断前缀/后缀用 `startswith`/`endswith`，支持元组：`name.endswith((".py", ".pyw"))`。

**常见陷阱**：

```python
# 反例：路径拼接不要用字符串加号
path = "data" + "/" + "a.txt"

# 正例：使用 pathlib
from pathlib import Path
path = Path("data") / "a.txt"
```

---

## 3. 列表与切片

```python
numbers = [3, 1, 4, 1, 5]
numbers.append(9)        # 尾部添加
numbers.extend([2, 6])   # 合并另一个可迭代对象
numbers.insert(0, 0)     # 指定位置插入，O(n)
numbers.pop()            # 弹出尾部，O(1)
numbers.remove(1)        # 删除第一个匹配值
sorted(numbers)          # 返回新列表
numbers.sort(reverse=True)  # 原地排序
```

### 3.1 切片

```python
data = [0, 1, 2, 3, 4, 5]
data[1:4]      # [1, 2, 3]
data[:3]       # [0, 1, 2]
data[::2]      # [0, 2, 4]
data[::-1]     # 反转
data[1:4] = [] # 原地删除一段
```

切片返回新列表；切片赋值会原地修改。越界切片不报错，索引越界会报 `IndexError`。

### 3.2 列表推导式

```python
squares = [x * x for x in range(10) if x % 2 == 0]
matrix = [[row * col for col in range(3)] for row in range(2)]
```

推导式适合“转换 + 过滤”；嵌套超过两层时应改用普通循环，优先保证可读性。

---

## 4. 字典

```python
scores = {"alice": 90, "bob": 85}
scores["carol"] = 88
scores.get("dave", 0)          # 不存在时返回默认值
scores.setdefault("eve", [])   # 不存在时写入默认值并返回
for name, score in scores.items():
    print(name, score)
```

- `d[key]` 在键不存在时抛 `KeyError`；`d.get(key)` 返回 `None`。
- 删除用 `pop(key, default)`，避免 `del` 触发 `KeyError`。
- 计数优先 `collections.Counter`，分组优先 `collections.defaultdict(list)`。

```python
from collections import Counter, defaultdict

Counter("banana")                       # Counter({'a': 3, 'n': 2, 'b': 1})
groups = defaultdict(list)
for word in ["apple", "avocado", "banana"]:
    groups[word[0]].append(word)
```

**常见陷阱**：遍历字典时修改大小会抛 `RuntimeError: dictionary changed size during iteration`。需要删除时先复制键：`for key in list(d): ...`。

---

## 5. 元组、集合与解包

```python
point = (3, 5)
x, y = point                 # 解包
first, *rest = [1, 2, 3, 4]  # first=1, rest=[2, 3, 4]

unique = {1, 2, 2, 3}        # {1, 2, 3}
a, b = {1, 2, 3}, {2, 3, 4}
a | b      # 并集
a & b      # 交集
a - b      # 差集
a ^ b      # 对称差
```

- 单元素元组必须写 `(1,)`，`(1)` 只是整数。
- 集合元素必须可哈希；列表和字典不能放入集合。
- 集合无序，不能依赖迭代顺序。

---

## 6. 控制流程

```python
for index, value in enumerate(items, start=1):
    if value is None:
        continue
    if value == "stop":
        break
else:
    print("循环正常结束，没有被 break")
```

- `for ... else` 的 `else` 在未触发 `break` 时执行，适合“查找失败”分支。
- `while` 必须有明确的退出条件；等待外部状态时加超时，避免死循环。
- 链式比较 `0 <= x < 100` 比 `x >= 0 and x < 100` 更清晰。
- 三元表达式 `a if condition else b` 只用于简单分支。

---

## 7. 函数

### 7.1 参数规则

```python
def create_user(name, age=18, *, active=True, **extra):
    return {"name": name, "age": age, "active": active, **extra}
```

- 位置参数在前，默认参数在后。
- `*args` 收集多余位置参数，`**kwargs` 收集多余关键字参数。
- `*` 之后的参数只能按关键字传递，适合布尔开关，避免调用方传错顺序。
- 默认参数在函数定义时求值一次；不要用可变对象作默认值。

```python
# 反例
def append_item(item, target=[]):
    target.append(item)
    return target

# 正例
def append_item(item, target=None):
    target = [] if target is None else target
    target.append(item)
    return target
```

### 7.2 作用域

- 函数内赋值会创建局部变量；读取外层变量不需要声明。
- 修改外层变量需要 `nonlocal`（嵌套函数）或 `global`（模块级），应尽量少用。
- 闭包捕获的是变量绑定，不是定义时的值。

### 7.3 函数卡片示例：`enumerate`

| 项目 | 内容 |
|---|---|
| 作用 | 同时取得索引与元素 |
| 语法 | `enumerate(iterable, start=0)` |
| 参数 | `iterable` 任意可迭代对象；`start` 起始编号 |
| 返回 | 产生 `(index, value)` 的迭代器 |
| 最小示例 | `list(enumerate(["a", "b"], start=1))` → `[(1, 'a'), (2, 'b')]` |

---

## 8. 模块与文件操作

```python
from pathlib import Path

path = Path("data") / "notes.txt"
path.parent.mkdir(parents=True, exist_ok=True)
path.write_text("第一行\n第二行\n", encoding="utf-8")
content = path.read_text(encoding="utf-8")
```

- 始终显式指定 `encoding="utf-8"`，避免 Windows 默认编码差异。
- 大文件用逐行迭代，不要一次 `read()`：

  ```python
  with path.open(encoding="utf-8") as handle:
      for line in handle:
          process(line.rstrip("\n"))
  ```

- `with` 保证文件关闭，即使中途抛异常。
- 二进制文件用 `"rb"`/`"wb"`，不要做文本编码转换。
- 遍历目录用 `Path.glob("*.py")` 或 `rglob("*.py")`，不要手工拼接字符串。

**常见陷阱**：

```python
# 反例：相对路径依赖当前工作目录
open("data/notes.txt")

# 正例：相对当前文件定位
BASE = Path(__file__).resolve().parent
(BASE / "data" / "notes.txt").open(encoding="utf-8")
```

---

## 9. 对应文件与验证

| 主题 | 文件 |
|---|---|
| 变量与类型 | `python_basics/01_变量和数据类型_详解.py` |
| 字符串 | `python_basics/04_字符串操作_详解.py` |
| 列表 | `python_basics/07_列表基础_详解.py`、`08_列表进阶_详解.py` |
| 字典 | `python_basics/10_字典基础_详解.py` |
| 元组与集合 | `python_basics/11_元组基础_详解.py`、`13_集合基础_详解.py` |
| 控制流程 | `python_basics/14_条件判断_详解.py`、`16_for循环_详解.py`、`17_while循环_详解.py` |
| 函数 | `python_basics/19_函数基础_详解.py`、`20_函数综合练习.py` |
| 模块与文件 | `python_basics/21_模块基础_详解.py`、`22_文件操作_详解.py` |

运行单个示例：

```bash
python_basics/.venv/Scripts/python.exe "python_basics/01_变量和数据类型_详解.py"
```

---

## 10. 练习（不附答案）

1. 用列表推导式取出 1～100 中能被 3 整除但不能被 5 整除的数。
2. 统计一段文本中每个单词出现的次数，按次数降序输出前 5 个。
3. 写一个函数 `safe_divide(a, b, default=None)`，除数为 0 时返回默认值。
4. 用 `pathlib` 找出某目录下所有 `.py` 文件，并按文件大小降序输出。
5. 解释为什么 `def f(x, target=[])` 是危险的，并改写。

---

## 11. 常见错误清单

| 现象 | 原因 | 修正 |
|---|---|---|
| `TypeError: unhashable type: 'list'` | 用列表做字典键或集合元素 | 改为元组 |
| `KeyError` | 直接用 `d[key]` 读取不存在的键 | 用 `get` 或先判断 |
| `IndexError` | 索引越界 | 用切片或先检查长度 |
| 修改列表时漏元素 | 正向遍历同时删除 | 倒序遍历或遍历副本 |
| 文件乱码 | 未指定编码 | 显式 `encoding="utf-8"` |
| 默认参数跨调用共享 | 可变默认值只创建一次 | 默认值改为 `None` |

---

## 12. 待修正内容

- 本笔记依据当前示例文件整理，未逐行核对每个练习的预期输出。
- 若后续示例文件重命名或删除，应同步更新第 9 节的文件表。
