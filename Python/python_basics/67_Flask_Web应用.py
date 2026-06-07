# ============================================
# Flask Web 应用项目 - 学生成绩管理系统
# 日期：2026-06-07（第27天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【项目说明】

项目名称：学生成绩管理系统（Web版）
功能：
  1. 首页：展示所有学生列表
  2. 成绩查询：按姓名搜索学生成绩
  3. 添加成绩：POST 表单提交
  4. JSON API：提供 RESTful 接口
  5. 统计页面：展示班级统计数据

运行方式：python 67_Flask_Web应用.py
访问地址：http://127.0.0.1:5001
"""

from flask import Flask, request, jsonify, redirect, url_for

app = Flask(__name__)

# ============================================
# 数据存储（内存中，重启后清空）
# ============================================

# 模拟数据库：学生成绩数据
students_db = [
    {'id': 1, 'name': '张三', 'python': 92, 'math': 85, 'english': 78},
    {'id': 2, 'name': '李四', 'python': 78, 'math': 90, 'english': 88},
    {'id': 3, 'name': '王五', 'python': 85, 'math': 72, 'english': 95},
    {'id': 4, 'name': '赵六', 'python': 65, 'math': 88, 'english': 70},
    {'id': 5, 'name': '钱七', 'python': 95, 'math': 91, 'english': 82},
]

next_id = 6  # 下一个学生的 ID


# ============================================
# 工具函数
# ============================================

def calc_avg(student):
    """计算学生平均分"""
    scores = [student['python'], student['math'], student['english']]
    return round(sum(scores) / len(scores), 1)


def get_grade(avg):
    """根据平均分返回等级"""
    if avg >= 90: return '优秀'
    if avg >= 75: return '良好'
    if avg >= 60: return '及格'
    return '不及格'


def render_page(title, body):
    """生成完整 HTML 页面（避免重复写 head/nav）"""
    nav = """
    <nav style="background:#2196F3;padding:10px 20px;margin-bottom:20px">
        <a href="/" style="color:white;margin-right:20px;text-decoration:none">首页</a>
        <a href="/students" style="color:white;margin-right:20px;text-decoration:none">学生列表</a>
        <a href="/add" style="color:white;margin-right:20px;text-decoration:none">添加学生</a>
        <a href="/stats" style="color:white;margin-right:20px;text-decoration:none">统计数据</a>
        <a href="/api/students" style="color:white;text-decoration:none">JSON API</a>
    </nav>
    """
    return f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{title}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 0; }}
        .container {{ max-width: 900px; margin: 0 auto; padding: 0 20px 40px; }}
        table {{ border-collapse: collapse; width: 100%; }}
        th, td {{ border: 1px solid #ddd; padding: 10px 14px; text-align: left; }}
        th {{ background: #f5f5f5; }}
        tr:hover {{ background: #f9f9f9; }}
        .btn {{ background:#2196F3;color:white;padding:8px 16px;
                border:none;cursor:pointer;border-radius:4px;text-decoration:none; }}
        .btn:hover {{ background:#1976D2; }}
        input[type=text], input[type=number] {{
            padding:8px;border:1px solid #ddd;border-radius:4px;width:200px; }}
        .tag {{ padding:3px 8px;border-radius:3px;font-size:12px; }}
        .优秀 {{ background:#e8f5e9;color:#2e7d32; }}
        .良好 {{ background:#e3f2fd;color:#1565c0; }}
        .及格 {{ background:#fff3e0;color:#e65100; }}
        .不及格 {{ background:#ffebee;color:#c62828; }}
    </style>
</head>
<body>
{nav}
<div class="container">
<h2>{title}</h2>
{body}
</div>
</body>
</html>"""


# ============================================
# 页面路由
# ============================================

@app.route('/')
def index():
    total = len(students_db)
    avg_all = round(sum(calc_avg(s) for s in students_db) / total, 1) if total else 0
    top = max(students_db, key=calc_avg) if students_db else None
    body = f"""
    <p>欢迎使用学生成绩管理系统</p>
    <table>
        <tr><th>统计项</th><th>数值</th></tr>
        <tr><td>学生总数</td><td>{total} 人</td></tr>
        <tr><td>班级平均分</td><td>{avg_all} 分</td></tr>
        <tr><td>最高分学生</td><td>{top['name'] if top else '-'}（{calc_avg(top):.1f}分）</td></tr>
    </table>
    <br>
    <a href="/students" class="btn">查看学生列表</a>
    """
    return render_page('首页 - 成绩管理系统', body)


@app.route('/students')
def students():
    # 支持按姓名搜索：/students?q=张
    q = request.args.get('q', '').strip()
    data = [s for s in students_db if q in s['name']] if q else students_db

    rows = ''
    for s in data:
        avg = calc_avg(s)
        grade = get_grade(avg)
        rows += f"""<tr>
            <td>{s['id']}</td>
            <td>{s['name']}</td>
            <td>{s['python']}</td>
            <td>{s['math']}</td>
            <td>{s['english']}</td>
            <td>{avg}</td>
            <td><span class="tag {grade}">{grade}</span></td>
            <td><a href="/student/{s['id']}">详情</a></td>
        </tr>"""

    body = f"""
    <form method="GET" style="margin-bottom:16px">
        <input type="text" name="q" value="{q}" placeholder="搜索姓名...">
        <button type="submit" class="btn">搜索</button>
        {'<a href="/students" style="margin-left:8px">清除</a>' if q else ''}
    </form>
    {'<p>共找到 ' + str(len(data)) + ' 条结果</p>' if q else ''}
    <table>
        <tr><th>ID</th><th>姓名</th><th>Python</th><th>数学</th><th>英语</th><th>平均分</th><th>等级</th><th>操作</th></tr>
        {rows}
    </table>
    <br><a href="/add" class="btn">+ 添加学生</a>
    """
    return render_page('学生列表', body)


@app.route('/student/<int:sid>')
def student_detail(sid):
    # 用列表推导找到对应学生
    student = next((s for s in students_db if s['id'] == sid), None)
    if not student:
        return render_page('未找到', '<p>该学生不存在</p><a href="/students">返回列表</a>'), 404

    avg = calc_avg(student)
    grade = get_grade(avg)
    body = f"""
    <table>
        <tr><th>字段</th><th>值</th></tr>
        <tr><td>ID</td><td>{student['id']}</td></tr>
        <tr><td>姓名</td><td>{student['name']}</td></tr>
        <tr><td>Python 成绩</td><td>{student['python']}</td></tr>
        <tr><td>数学成绩</td><td>{student['math']}</td></tr>
        <tr><td>英语成绩</td><td>{student['english']}</td></tr>
        <tr><td>平均分</td><td>{avg}</td></tr>
        <tr><td>等级</td><td><span class="tag {grade}">{grade}</span></td></tr>
    </table>
    <br><a href="/students" class="btn">返回列表</a>
    """
    return render_page(f'学生详情 - {student["name"]}', body)


@app.route('/add', methods=['GET', 'POST'])
def add_student():
    global next_id
    error = ''

    if request.method == 'POST':
        name = request.form.get('name', '').strip()
        try:
            python = int(request.form.get('python', 0))
            math   = int(request.form.get('math', 0))
            english = int(request.form.get('english', 0))
        except ValueError:
            error = '成绩必须是整数'
            python = math = english = 0

        # 输入验证（只在边界处验证用户输入）
        if not error and not name:
            error = '姓名不能为空'
        if not error and not all(0 <= s <= 100 for s in [python, math, english]):
            error = '成绩必须在 0~100 之间'

        if not error:
            students_db.append({
                'id': next_id,
                'name': name,
                'python': python,
                'math': math,
                'english': english
            })
            next_id += 1
            return redirect(url_for('students'))

    body = f"""
    {'<p style="color:red">' + error + '</p>' if error else ''}
    <form method="POST">
        <table>
            <tr><td>姓名：</td><td><input type="text" name="name"></td></tr>
            <tr><td>Python 成绩：</td><td><input type="number" name="python" min="0" max="100"></td></tr>
            <tr><td>数学成绩：</td><td><input type="number" name="math" min="0" max="100"></td></tr>
            <tr><td>英语成绩：</td><td><input type="number" name="english" min="0" max="100"></td></tr>
        </table>
        <br>
        <button type="submit" class="btn">添加</button>
        <a href="/students" style="margin-left:10px">取消</a>
    </form>
    """
    return render_page('添加学生', body)


@app.route('/stats')
def stats():
    subjects = ['python', 'math', 'english']
    subject_names = {'python': 'Python', 'math': '数学', 'english': '英语'}
    rows = ''
    for subj in subjects:
        scores = [s[subj] for s in students_db]
        rows += f"""<tr>
            <td>{subject_names[subj]}</td>
            <td>{round(sum(scores)/len(scores), 1)}</td>
            <td>{max(scores)}</td>
            <td>{min(scores)}</td>
        </tr>"""

    body = f"""
    <table>
        <tr><th>科目</th><th>平均分</th><th>最高分</th><th>最低分</th></tr>
        {rows}
    </table>
    """
    return render_page('统计数据', body)


# ============================================
# JSON API 路由（RESTful 风格）
# ============================================

@app.route('/api/students')
def api_students():
    """GET /api/students — 获取所有学生（含平均分）"""
    data = [{**s, 'avg': calc_avg(s), 'grade': get_grade(calc_avg(s))}
            for s in students_db]
    return jsonify({'total': len(data), 'students': data})


@app.route('/api/students/<int:sid>')
def api_student(sid):
    """GET /api/students/<id> — 获取单个学生"""
    student = next((s for s in students_db if s['id'] == sid), None)
    if not student:
        return jsonify({'error': '学生不存在'}), 404
    return jsonify({**student, 'avg': calc_avg(student), 'grade': get_grade(calc_avg(student))})


@app.route('/api/students', methods=['POST'])
def api_add_student():
    """POST /api/students — 添加学生（JSON 格式）"""
    global next_id
    data = request.json
    if not data:
        return jsonify({'error': '请求体必须是 JSON'}), 400

    name = data.get('name', '').strip()
    if not name:
        return jsonify({'error': '姓名不能为空'}), 400

    try:
        python  = int(data.get('python', 0))
        math    = int(data.get('math', 0))
        english = int(data.get('english', 0))
    except (ValueError, TypeError):
        return jsonify({'error': '成绩必须是整数'}), 400

    if not all(0 <= s <= 100 for s in [python, math, english]):
        return jsonify({'error': '成绩必须在 0~100 之间'}), 400

    new_student = {'id': next_id, 'name': name,
                   'python': python, 'math': math, 'english': english}
    students_db.append(new_student)
    next_id += 1
    return jsonify(new_student), 201  # 201 Created


@app.route('/api/stats')
def api_stats():
    """GET /api/stats — 获取统计数据"""
    subjects = ['python', 'math', 'english']
    stats = {}
    for subj in subjects:
        scores = [s[subj] for s in students_db]
        stats[subj] = {
            'avg': round(sum(scores) / len(scores), 1),
            'max': max(scores),
            'min': min(scores)
        }
    return jsonify({'total_students': len(students_db), 'subjects': stats})


# ============================================
# 启动
# ============================================

if __name__ == '__main__':
    print("=" * 50)
    print("学生成绩管理系统（Web版）")
    print("=" * 50)
    print("访问地址：http://127.0.0.1:5001")
    print()
    print("页面路由：")
    print("  /            首页")
    print("  /students    学生列表（支持 ?q= 搜索）")
    print("  /student/<id> 学生详情")
    print("  /add         添加学生")
    print("  /stats       统计数据")
    print()
    print("JSON API：")
    print("  GET  /api/students       所有学生")
    print("  GET  /api/students/<id>  单个学生")
    print("  POST /api/students       添加学生")
    print("  GET  /api/stats          统计数据")
    print()
    print("按 Ctrl+C 停止")
    print("=" * 50)
    app.run(debug=True, port=5001)
