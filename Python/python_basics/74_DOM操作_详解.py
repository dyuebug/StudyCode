# ============================================
# DOM 操作 - 超详细注释版
# 日期：2026-06-10（第31天）
# 难度：⭐⭐ 中级（JS 操控网页的核心技能）
# ============================================

"""
【核心概念】简要说明

是什么？
- DOM（Document Object Model，文档对象模型）
- 浏览器把整个 HTML 页面解析成一棵"树"，每个标签都是树上的一个"节点"
- JS 通过操作这棵树，来读取/修改/增删网页内容

为什么需要？
- 上节课学了 JS 语法，但还不能真正"动"网页
- DOM 就是 JS 和网页之间的桥梁
- 有了 DOM，才能实现：点击按钮改文字、表单校验、动态加载内容

核心特点？
- 先"选中"元素（找到要操作的标签）
- 再"操作"元素（改内容、改样式、绑定事件）
- 事件驱动：用户操作（点击/输入）触发 JS 函数执行

【重要说明】
本文件用 print 展示 DOM 代码作为教学。
真正运行需要 HTML + JS 配合，文件末尾会生成可交互的 demo_dom.html。
用浏览器打开它，点击按钮就能看到 DOM 操作的真实效果！
"""

# ============================================
# 第一部分：DOM 核心概念 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：DOM 核心概念")
print("=" * 60)

# --------------------------------------------
# 1.1 DOM 是什么？网页的树形结构
# --------------------------------------------

print("\n1.1 DOM：把网页看成一棵树")
print("-" * 60)

print("""
【一段 HTML】

    <html>
        <body>
            <h1>标题</h1>
            <p>段落</p>
        </body>
    </html>

【浏览器把它理解成一棵"树"】

            html
             │
            body
            ╱  ╲
          h1    p
        "标题" "段落"

【关键理解】
- 每个标签 = 树上的一个"节点"(node)
- 标签之间有父子、兄弟关系（像家谱）
- JS 可以顺着这棵树，找到任何一个节点去操作它

【document 对象】
浏览器提供一个全局对象 document，代表"整个页面"。
所有 DOM 操作都从 document 开始，比如：
    document.getElementById("xxx")
    ↑ 从整个文档里找 id 为 xxx 的元素
""")

# --------------------------------------------
# 1.2 DOM 操作的三步套路
# --------------------------------------------

print("\n1.2 DOM 操作的固定套路（三步）⭐⭐⭐⭐⭐")
print("-" * 60)

print("""
【任何 DOM 操作，都是这三步】

第1步：选中元素（找到要操作的标签）
    const title = document.getElementById("title");

第2步：操作元素（改内容/改样式）
    title.textContent = "新标题";

第3步：（可选）绑定事件（响应用户操作）
    button.addEventListener("click", 函数);

【生活类比】
第1步选中 = 在人群中找到那个人
第2步操作 = 让他做事（换衣服、说话）
第3步事件 = 约定"你拍他肩膀，他就回头"

记住这三步，DOM 操作就不会乱。⭐
""")

# ============================================
# 第二部分：选中元素 + 修改元素 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第二部分：选中元素与修改内容")
print("=" * 60)

# --------------------------------------------
# 2.1 选中元素的方法（函数卡片）
# --------------------------------------------

print("\n2.1 如何选中元素？")
print("-" * 60)

print('''
【函数卡片：getElementById()】

函数名：document.getElementById("id值")
作用：根据 id 找到唯一的一个元素
参数：id 字符串（不带 # 号）
返回：找到的元素对象（找不到返回 null）

示例：
    HTML:  <h1 id="title">标题</h1>
    JS:    const t = document.getElementById("title");
           //                                  ↑不要写 #title

────────────────────────────────────

【函数卡片：querySelector()】更灵活 ⭐

函数名：document.querySelector("CSS选择器")
作用：用 CSS 选择器语法找元素，返回第一个匹配的
参数：CSS 选择器字符串（和写 CSS 一样！）
返回：第一个匹配的元素

示例：
    document.querySelector("#title")     // 按 id（要带 #）
    document.querySelector(".btn")       // 按 class（要带 .）
    document.querySelector("p")          // 按标签名

【两者区别】
getElementById  →  只能按 id，写法简单，速度快
querySelector   →  万能（id/class/标签都行），用 CSS 语法⭐
建议：记住 querySelector，它最灵活
''')

# --------------------------------------------
# 2.2 修改元素内容和样式
# --------------------------------------------

print("\n2.2 选中后能改什么？")
print("-" * 60)

print('''
【改文字内容】
    const t = document.querySelector("#title");
    t.textContent = "新的标题";    // 改纯文字（最常用）⭐
    t.innerHTML = "<b>加粗</b>";   // 可插入 HTML 标签

【改样式】
    t.style.color = "red";         // 文字变红
    t.style.fontSize = "30px";     // 字号
    // 注意：CSS 的 font-size → JS 写成 fontSize（驼峰命名）

【改属性】
    const img = document.querySelector("img");
    img.src = "new.jpg";           // 换图片

【改 class（推荐用于切换样式）】
    t.classList.add("active");     // 加一个 class
    t.classList.remove("active");  // 删一个 class
    t.classList.toggle("active");  // 有就删，没有就加（开关效果）⭐

【读取表单输入值】⭐
    const input = document.querySelector("#username");
    const value = input.value;     // 拿到用户输入的内容
    // 这就是前端获取用户输入的方式！
''')

# ============================================
# 第三部分：事件监听（DOM 的灵魂）⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：事件监听 —— 让网页响应用户")
print("=" * 60)

print("\n3.1 什么是事件？")
print("-" * 60)

print("""
【事件 Event】
用户在网页上的"动作"，都叫事件：
    点击按钮      → click 事件
    鼠标移入      → mouseover 事件
    键盘输入      → input 事件
    提交表单      → submit 事件

【事件驱动思想】⭐
传统程序：从上到下顺序执行
网页程序：平时"等待"，用户触发事件 → 才执行对应函数
         （就像门铃，平时安静，有人按 → 响铃）
""")

print("\n3.2 绑定事件：addEventListener()")
print("-" * 60)

print('''
【函数卡片：addEventListener()】⭐⭐⭐⭐⭐

函数名：元素.addEventListener("事件名", 处理函数)
作用：给元素绑定一个事件监听器
参数1：事件名字符串，如 "click"（注意：不带 on 前缀）
参数2：事件发生时要执行的函数
返回：无

【完整示例】
    HTML:  <button id="btn">点我</button>

    JS:
    const btn = document.getElementById("btn");  // 1.选中按钮

    btn.addEventListener("click", function() {    // 2.绑定点击事件
        alert("你点击了按钮！");                    // 3.点击后执行
    });

【用箭头函数写（更简洁）⭐】
    btn.addEventListener("click", () => {
        alert("你点击了按钮！");
    });

【完整案例：点赞计数器】
    let count = 0;                                // 计数变量
    const btn = document.querySelector("#like");
    const display = document.querySelector("#num");

    btn.addEventListener("click", () => {
        count++;                                   // 每次点击 +1
        display.textContent = count;               // 更新显示
    });
    // 这就实现了"点一次，数字加一"的交互！
''')

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：DOM 操作常见错误和陷阱")
print("=" * 60)

print("""
6个高频踩坑点：

❌ 错误1：JS 在元素加载前就执行
   现象：getElementById 返回 null，操作报错
   原因：<script> 放在 <head>，此时 body 元素还没生成
   ✅ 正确：把 <script> 放在 </body> 前面

❌ 错误2：getElementById 参数加了 #
   错误：document.getElementById("#title")
   原因：getElementById 不需要 #，那是 querySelector 的语法
   ✅ 正确：getElementById("title") 或 querySelector("#title")

❌ 错误3：CSS 属性名照搬
   错误：el.style.font-size = "20px";
   原因：JS 中带连字符的属性要写成驼峰
   ✅ 正确：el.style.fontSize = "20px";

❌ 错误4：事件名写错带了 on
   错误：addEventListener("onclick", fn)
   原因：addEventListener 的事件名不带 on
   ✅ 正确：addEventListener("click", fn)

❌ 错误5：把函数"调用结果"当参数传
   错误：btn.addEventListener("click", fn());  // 多了()
   原因：fn() 会立即执行，传的是返回值不是函数本身
   ✅ 正确：btn.addEventListener("click", fn);  // 不加()

❌ 错误6：textContent 和 innerHTML 混用
   说明：textContent 当纯文字（安全），innerHTML 会解析标签
   风险：用 innerHTML 插入用户输入有 XSS 攻击风险
   ✅ 建议：显示纯文字优先用 textContent
""")

# ============================================
# 第五部分：DOM 速查表 ⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第五部分：DOM 操作速查表")
print("=" * 60)

print('''
【选中元素】
document.getElementById("id")        按 id 选中（不带#）
document.querySelector("选择器")      CSS语法选第一个（万能）⭐
document.querySelectorAll("选择器")   选中所有匹配的（返回列表）

【修改内容】
el.textContent = "文字"              改纯文本⭐
el.innerHTML = "<b>html</b>"         改HTML内容
el.value                             读/写表单输入值⭐

【修改样式】
el.style.color = "red"               改单个CSS属性
el.style.fontSize = "20px"           注意驼峰命名
el.classList.add("类名")             加class
el.classList.remove("类名")          删class
el.classList.toggle("类名")          切换class（开关）⭐

【事件监听】
el.addEventListener("click", fn)     绑定点击⭐
el.addEventListener("input", fn)     绑定输入
el.addEventListener("submit", fn)    绑定表单提交

【常用事件名】
click     点击       input    输入
mouseover 鼠标移入   submit   提交表单
keydown   按键       change   值改变
''')

# ============================================
# 第六部分：练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("第六部分：动手练习题")
print("=" * 60)

print("""
打开末尾生成的 demo_dom.html，参考着完成下面的练习：

1. 【选中+修改】
   页面有 <h1 id="title">原标题</h1>，
   用 JS 把它的文字改成"我学会DOM了"。
   提示：getElementById + textContent

2. 【事件】
   有一个按钮，点击后弹出 alert("Hello")。
   提示：addEventListener("click", ...)

3. 【计数器】
   做一个按钮，每点一次，旁边数字 +1。
   提示：用 let count 变量，点击时 count++ 并更新显示

4. 【样式切换】
   点击按钮，让一个段落的文字颜色在红/黑之间切换。
   提示：classList.toggle 或判断当前 color

5. 【读取输入】
   有一个输入框和按钮，点击按钮后，
   把输入框里的内容显示到页面上。
   提示：input.value 获取输入

6. 【综合挑战】
   做一个简易"待办清单"：
   输入框输入内容 → 点击"添加" → 内容出现在下面的列表里。
   提示：createElement 创建新元素 + appendChild 添加
""")

print("\n" + "=" * 60)
print("✅ DOM 操作学习完毕！")
print("正在生成可交互网页 demo_dom.html...")
print("=" * 60)

# ============================================
# 彩蛋：生成一个可交互的网页
# ============================================

html_content = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>DOM 交互演示</title>
    <style>
        body { font-family: "Microsoft YaHei", sans-serif;
               max-width: 500px; margin: 40px auto; padding: 20px; }
        h1 { color: #1677ff; }
        button { padding: 8px 20px; margin: 5px;
                 background: #1677ff; color: white;
                 border: none; border-radius: 6px; cursor: pointer; }
        button:hover { background: #0958d9; }
        .box { padding: 20px; margin: 15px 0;
               border: 1px solid #ddd; border-radius: 8px; }
        .highlight { color: red; font-weight: bold; }
        input { padding: 8px; width: 200px; }
    </style>
</head>
<body>
    <h1 id="title">DOM 操作演示</h1>

    <!-- 案例1：点赞计数器 -->
    <div class="box">
        <p>案例1：点赞计数器</p>
        <button id="likeBtn">👍 点赞</button>
        <span>点赞数：<b id="count">0</b></span>
    </div>

    <!-- 案例2：样式切换 -->
    <div class="box">
        <p>案例2：点击切换颜色</p>
        <p id="colorText">这段文字会变色</p>
        <button id="colorBtn">切换颜色</button>
    </div>

    <!-- 案例3：读取输入 -->
    <div class="box">
        <p>案例3：显示你的输入</p>
        <input id="nameInput" placeholder="输入你的名字">
        <button id="greetBtn">打招呼</button>
        <p id="greeting"></p>
    </div>

    <script>
        // 案例1：点赞计数器
        let count = 0;
        document.getElementById("likeBtn").addEventListener("click", () => {
            count++;
            document.getElementById("count").textContent = count;
        });

        // 案例2：样式切换
        document.getElementById("colorBtn").addEventListener("click", () => {
            document.getElementById("colorText").classList.toggle("highlight");
        });

        // 案例3：读取输入并显示
        document.getElementById("greetBtn").addEventListener("click", () => {
            const name = document.getElementById("nameInput").value;
            document.getElementById("greeting").textContent = `你好，${name}！欢迎学习 DOM 操作 🎉`;
        });
    </script>
</body>
</html>"""

import os
output_path = os.path.join(os.path.dirname(__file__), "demo_dom.html")
with open(output_path, "w", encoding="utf-8") as f:
    f.write(html_content)

print(f"\n✅ 可交互网页已生成：{output_path}")
print("用浏览器打开，试试点赞、切换颜色、输入名字打招呼！")
