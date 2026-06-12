# ============================================
# HTML/CSS 基础 - 超详细注释版
# 日期：2026-06-09（第30天）
# 难度：⭐ 初级（前端入门）
# ============================================

"""
【核心概念】简要说明

是什么？
- HTML：网页的"骨架"，决定页面有什么内容（标题、段落、图片、按钮）
- CSS：网页的"皮肤"，决定内容长什么样（颜色、大小、位置、间距）

为什么需要？
- 之前学 Flask 时，我们返回的 HTML 都很简陋
- 学会 HTML/CSS 后，能做出真正美观、专业的网页

核心特点？
- HTML 用"标签"描述内容，如 <h1>标题</h1>
- CSS 用"规则"描述样式，如 h1 { color: red; }
- 两者分工明确：HTML 管内容，CSS 管外观

【重要说明】
本文件是 Python 文件，用 print 展示 HTML/CSS 代码作为教学。
真正写网页时，HTML 要保存成 .html 文件，CSS 保存成 .css 文件。
文件末尾会生成一个真实的 demo.html，可以用浏览器打开看效果！
"""

# ============================================
# 第一部分：HTML 核心概念 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：HTML 核心概念")
print("=" * 60)

# --------------------------------------------
# 1.1 HTML 是什么？标签是什么？
# --------------------------------------------

print("\n1.1 HTML 和标签的概念")
print("-" * 60)

print("""
【HTML】HyperText Markup Language（超文本标记语言）

是什么？
- 一种"标记语言"，用标签来描述网页内容
- 不是编程语言（没有逻辑判断、循环），只是"描述结构"

【标签 Tag】HTML 的基本单位

基本格式：
    <标签名>内容</标签名>
    ↑开始标签        ↑结束标签（多一个斜杠 /）

例子：
    <h1>这是大标题</h1>
    <p>这是一个段落</p>

【双标签 vs 单标签】

双标签（有开始有结束）：
    <p>内容</p>

单标签（自我闭合，无内容）：
    <br>      ← 换行
    <img>     ← 图片
    <hr>      ← 水平分割线
""")

# --------------------------------------------
# 1.2 一个完整 HTML 页面的骨架
# --------------------------------------------

print("\n1.2 HTML 页面的标准骨架 ⭐⭐⭐⭐⭐")
print("-" * 60)

print('''
【完整骨架（背下来！每个网页都长这样）】

<!DOCTYPE html>               <!-- 声明：这是 HTML5 文档 -->
<html lang="zh-CN">           <!-- 根标签，lang 指定语言为中文 -->
<head>                        <!-- 头部：放"看不见"的配置信息 -->
    <meta charset="UTF-8">    <!-- 字符编码，防止中文乱码！⭐ -->
    <title>网页标题</title>    <!-- 浏览器标签页上显示的标题 -->
</head>
<body>                        <!-- 主体：放"看得见"的页面内容 -->
    <h1>欢迎来到我的网页</h1>
    <p>这里是正文内容</p>
</body>
</html>

【逐部分理解】

<!DOCTYPE html>  → 告诉浏览器用 HTML5 标准解析
<html>           → 整个页面的根，所有内容都在里面
  <head>         → 配置区（编码、标题、引入CSS）—— 用户看不到
  <body>         → 内容区（标题、段落、图片）—— 用户能看到

记忆口诀：
  head 是"幕后"（配置），body 是"台前"（内容）
''')

# --------------------------------------------
# 1.3 常用 HTML 标签速览
# --------------------------------------------

print("\n1.3 最常用的 HTML 标签")
print("-" * 60)

print('''
【文本类标签】

<h1> ~ <h6>   标题，h1最大，h6最小
<p>           段落
<br>          换行（单标签）
<strong>      加粗（表示重要）
<em>          斜体（表示强调）
<span>        行内小容器（不换行）

【列表类标签】

<ul>          无序列表（圆点）
    <li>列表项</li>
</ul>

<ol>          有序列表（数字）
    <li>列表项</li>
</ol>

【链接和图片】

<a href="https://baidu.com">点我跳转</a>   链接
                ↑ href 是要跳转的网址
<img src="cat.jpg" alt="一只猫">           图片
        ↑ src 图片地址   ↑ alt 图片加载失败时显示的文字

【容器类标签（最重要！）】

<div>         块级容器（独占一行，用于布局分区）
<span>        行内容器（不换行，用于局部样式）

记忆：div 是"大盒子"（分区），span 是"小标签"（局部）
''')

# --------------------------------------------
# 1.4 表单标签（回顾 Flask 用过的）
# --------------------------------------------

print("\n1.4 表单标签（和 Flask 后端配合）")
print("-" * 60)

print('''
【表单：收集用户输入，提交给后端】

<form action="/submit" method="POST">
       ↑提交到哪个网址      ↑用什么方法（还记得GET/POST吗？）

    <input type="text" name="username">     文本输入框
           ↑类型          ↑后端用这个名字取值

    <input type="password" name="pwd">      密码框
    <input type="email" name="email">       邮箱框
    <textarea name="content"></textarea>     多行文本

    <button type="submit">提交</button>      提交按钮
</form>

【关键联系】
还记得第27天 Flask 里的 request.form["username"] 吗？
这里 <input name="username"> 的 name 值，
就是后端 request.form["username"] 要用的键名！
前后端就是通过 name 对应起来的。⭐
''')

# ============================================
# 第二部分：CSS 核心概念 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第二部分：CSS 核心概念")
print("=" * 60)

# --------------------------------------------
# 2.1 CSS 是什么？语法结构
# --------------------------------------------

print("\n2.1 CSS 的语法结构")
print("-" * 60)

print('''
【CSS】Cascading Style Sheets（层叠样式表）

作用：给 HTML 元素"化妆"——设置颜色、大小、间距、位置

【CSS 规则的语法】

    选择器 {
        属性: 值;
        属性: 值;
    }

例子：
    h1 {                  ← 选择器：选中所有 <h1> 标签
        color: red;       ← 属性:值，文字颜色设为红色
        font-size: 32px;  ← 字号 32 像素
    }                     ← 每条规则用 {} 包裹，每行结尾加分号 ;

【三种引入 CSS 的方式】

方式1：行内样式（写在标签里，不推荐）
    <p style="color: blue;">蓝色文字</p>

方式2：内部样式（写在 <head> 的 <style> 里）
    <head>
        <style>
            p { color: blue; }
        </style>
    </head>

方式3：外部样式（独立 .css 文件，最推荐）⭐
    <head>
        <link rel="stylesheet" href="style.css">
    </head>
''')

# --------------------------------------------
# 2.2 CSS 选择器（如何"选中"元素）
# --------------------------------------------

print("\n2.2 CSS 选择器：三种最常用 ⭐⭐⭐⭐⭐")
print("-" * 60)

print('''
选择器决定"这条样式应用到哪些元素上"。

【选择器1：标签选择器】
直接写标签名，选中所有该标签
    p { color: red; }        ← 所有 <p> 变红
    h1 { font-size: 32px; }  ← 所有 <h1> 字号32

【选择器2：类选择器（class）】最常用！⭐
用 . 开头，选中指定 class 的元素
    HTML:  <p class="warning">注意</p>
    CSS:   .warning { color: orange; }
           ↑ . 表示 class
    特点：一个 class 可以用在多个元素上（批量控制）

【选择器3：ID 选择器（id）】
用 # 开头，选中指定 id 的元素
    HTML:  <div id="header">顶部</div>
    CSS:   #header { background: gray; }
           ↑ # 表示 id
    特点：id 在页面中应该唯一（一个 id 只用一次）

【对比记忆】
.类名  →  可重复，批量控制（推荐多用）
#id    →  唯一，单独控制
标签名 →  控制所有同类标签
''')

# --------------------------------------------
# 2.3 盒模型（CSS 最核心的概念）
# --------------------------------------------

print("\n2.3 盒模型 Box Model ⭐⭐⭐⭐⭐")
print("-" * 60)

print('''
【核心思想】网页上每个元素，都是一个"盒子"

一个盒子从内到外有四层：

    ┌─────────────────────────────┐
    │      margin（外边距）         │  ← 盒子和别人的距离
    │  ┌───────────────────────┐  │
    │  │   border（边框）       │  │  ← 盒子的边线
    │  │  ┌─────────────────┐  │  │
    │  │  │ padding（内边距）│  │  │  ← 内容和边框的距离
    │  │  │  ┌───────────┐  │  │  │
    │  │  │  │  content  │  │  │  │  ← 真正的内容（文字/图片）
    │  │  │  └───────────┘  │  │  │
    │  │  └─────────────────┘  │  │
    │  └───────────────────────┘  │
    └─────────────────────────────┘

【四层对应的 CSS 属性】

    .box {
        width: 200px;            /* 内容宽度 */
        padding: 20px;           /* 内边距：内容到边框 */
        border: 2px solid black; /* 边框：粗细 样式 颜色 */
        margin: 10px;            /* 外边距：盒子到其他元素 */
    }

【生活类比：寄快递】
content = 商品本身
padding = 商品周围的填充泡沫
border  = 快递盒子的硬纸板
margin  = 这个包裹和其他包裹之间的间隔

【记忆口诀】
内边距 padding 在"里面"（内容和边框之间）
外边距 margin 在"外面"（盒子和盒子之间）
''')

# ============================================
# 第三部分：Flexbox 弹性布局 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：Flexbox 弹性布局")
print("=" * 60)

print("\n3.1 为什么需要 Flexbox？")
print("-" * 60)

print('''
【痛点】
默认情况下，<div> 是块级元素，每个独占一行，竖着排。
但很多时候我们想让元素"横着排"（如导航栏、卡片列表）。
以前要用各种 hack 技巧，现在用 Flexbox 一招搞定。

【Flexbox 核心思想】
给"父容器"设置 display: flex，
它的"子元素"就会自动横向排列，并且可以灵活控制对齐方式。
''')

print("\n3.2 Flexbox 核心属性")
print("-" * 60)

print('''
【父容器上设置】

.container {
    display: flex;                  /* 开启弹性布局（关键！）*/

    justify-content: center;        /* 主轴(水平)对齐方式 */
    /* 可选值：
       flex-start    左对齐（默认）
       center        居中
       flex-end      右对齐
       space-between 两端对齐，中间均分
       space-around  每个元素两侧均匀留白 */

    align-items: center;            /* 交叉轴(垂直)对齐方式 */
    /* 可选值：
       flex-start    顶部对齐
       center        垂直居中
       flex-end      底部对齐 */

    gap: 10px;                      /* 子元素之间的间距 */
}

【经典应用：完美居中】
想让一个东西在屏幕正中央？三行搞定：
    .container {
        display: flex;
        justify-content: center;  /* 水平居中 */
        align-items: center;      /* 垂直居中 */
    }

这是面试高频题，记住这个"居中三件套"！⭐
''')

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：HTML/CSS 常见错误和陷阱")
print("=" * 60)

print("""
❌ 错误1：忘记写 <meta charset="UTF-8">
   现象：网页中文显示成乱码"æ±‰å­—"
   原因：浏览器不知道用什么编码解析中文
   ✅ 正确：<head> 里第一行写 <meta charset="UTF-8">

❌ 错误2：标签没有闭合
   错误：<p>段落1 <p>段落2
   原因：忘了 </p>，浏览器解析混乱
   ✅ 正确：<p>段落1</p> <p>段落2</p>

❌ 错误3：CSS 属性后忘记分号
   错误：color: red  font-size: 16px
   原因：缺少 ; 分隔，第二条样式失效
   ✅ 正确：color: red; font-size: 16px;

❌ 错误4：class 和 id 选择器符号搞反
   错误：CSS 里写 #warning 却想选 class="warning"
   原因：. 选 class，# 选 id，不能混用
   ✅ 正确：class 用 .warning，id 用 #header

❌ 错误5：混淆 padding 和 margin
   现象：想让内容离边框远一点，却用了 margin
   原因：margin 是盒子外间距，padding 才是内间距
   ✅ 正确：内容到边框用 padding，盒子到盒子用 margin

❌ 错误6：忘记给父容器加 display: flex
   现象：设置了 justify-content 却没效果
   原因：Flex 属性必须先在父容器开启 display: flex
   ✅ 正确：父容器先写 display: flex，再写对齐属性
""")

# ============================================
# 第五部分：标签和属性速查 ⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第五部分：HTML/CSS 速查表")
print("=" * 60)

print('''
【HTML 常用标签速查】

结构：  <html> <head> <body> <div> <span>
标题：  <h1>~<h6>
文本：  <p> <br> <strong> <em>
列表：  <ul><li> <ol><li>
链接：  <a href="网址">文字</a>
图片：  <img src="路径" alt="说明">
表单：  <form> <input> <textarea> <button>

────────────────────────────────────

【CSS 常用属性速查】

文字：  color（颜色） font-size（字号） font-weight（粗细）
        text-align（对齐：left/center/right）
背景：  background-color（背景色） background-image（背景图）
盒模型：width height padding margin border
布局：  display（block/inline/flex）
        justify-content align-items（flex对齐）

────────────────────────────────────

【颜色的三种写法】

英文名：  color: red;
十六进制：color: #FF0000;   （# + 6位，红绿蓝各2位）
RGB：     color: rgb(255, 0, 0);
''')

# ============================================
# 第六部分：练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("第六部分：动手练习题")
print("=" * 60)

print("""
1. 【基础结构】
   手写一个完整的 HTML 页面骨架，要求：
   - 标题栏显示"我的第一个网页"
   - 页面里有一个 h1 标题和两个段落
   提示：别忘了 DOCTYPE 和 charset

2. 【列表+链接】
   做一个"我喜欢的网站"列表，要求：
   - 用无序列表 <ul>
   - 每一项是一个可点击的链接 <a>
   提示：<li> 里嵌套 <a>

3. 【CSS 样式】
   给上面的 h1 标题加样式：
   - 文字颜色为蓝色
   - 居中对齐
   - 字号 36px
   提示：用标签选择器 h1 { }

4. 【盒模型】
   做一个"卡片"div，要求：
   - 宽 300px
   - 内边距 20px
   - 1px 灰色边框
   - 外边距 10px
   提示：width/padding/border/margin

5. 【Flexbox】
   把三个 div 横向排列并水平居中。
   提示：父容器 display: flex + justify-content

6. 【综合挑战】
   结合 Flask（第27天）：
   写一个登录表单 HTML，提交到 /login，方法 POST。
   想一想：后端要用 request.form 取哪些字段？
   提示：input 的 name 属性 = 后端取值的键名
""")

print("\n" + "=" * 60)
print("✅ HTML/CSS 基础学习完毕！")
print("正在生成真实网页 demo.html，请用浏览器打开查看效果...")
print("=" * 60)

# ============================================
# 彩蛋：生成一个真实的 HTML 网页文件
# ============================================

# 把 HTML 内容写成多行字符串
html_content = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>我的第一个网页</title>
    <style>
        /* 整个页面用 Flexbox 居中 */
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 90vh;
            background-color: #f0f2f5;
            font-family: "Microsoft YaHei", sans-serif;
        }
        /* 卡片样式（盒模型实战）*/
        .card {
            width: 320px;
            padding: 30px;
            border: 1px solid #ddd;
            border-radius: 12px;
            background-color: white;
            box-shadow: 0 4px 12px rgba(0,0,0,0.1);
            text-align: center;
        }
        h1 { color: #1677ff; font-size: 28px; }
        p { color: #666; line-height: 1.6; }
        .tag {
            display: inline-block;
            padding: 4px 12px;
            background-color: #1677ff;
            color: white;
            border-radius: 16px;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>学习第30天</h1>
        <p>我已经掌握了 HTML 骨架、CSS 选择器、盒模型和 Flexbox 布局！</p>
        <span class="tag">前端入门成功</span>
    </div>
</body>
</html>"""

# 写入文件（保存到 python_basics 目录）
import os
output_path = os.path.join(os.path.dirname(__file__), "demo.html")
with open(output_path, "w", encoding="utf-8") as f:
    f.write(html_content)

print(f"\n✅ 网页已生成：{output_path}")
print("用浏览器打开它，你会看到一张漂亮的居中卡片！")
