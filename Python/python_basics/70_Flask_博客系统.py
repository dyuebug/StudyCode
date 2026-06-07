# ============================================
# Flask 个人博客系统 - 综合项目
# 日期：2026-06-08（第29天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【项目说明】

项目名称：个人博客系统
综合运用：Flask 路由 + Jinja2 模板 + SQLite 数据库

功能：
  1. 首页：文章列表（标题、摘要、发布时间）
  2. 文章详情页
  3. 发布文章（表单提交）
  4. 删除文章
  5. 分类筛选
  6. JSON API 接口

运行：python 70_Flask_博客系统.py
访问：http://127.0.0.1:5004
"""

import sqlite3
import os
from flask import Flask, request, jsonify, redirect, url_for
from datetime import datetime

app = Flask(__name__)

# 数据库文件路径（与本文件同目录）
DB_PATH = os.path.join(os.path.dirname(__file__), 'blog.db')

# ============================================
# 数据库初始化
# ============================================

def init_db():
    """创建数据库表，并插入示例数据（仅首次）"""
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS articles (
                id        INTEGER PRIMARY KEY AUTOINCREMENT,
                title     TEXT    NOT NULL,
                content   TEXT    NOT NULL,
                category  TEXT    NOT NULL DEFAULT '技术',
                created_at TEXT   NOT NULL
            )
        """)
        # 仅在表为空时插入示例数据
        count = conn.execute("SELECT COUNT(*) FROM articles").fetchone()[0]
        if count == 0:
            sample = [
                ('Python Flask 入门总结',
                 '经过三天的学习，我掌握了 Flask 的路由、视图函数、请求处理和 JSON API。\n\n'
                 'Flask 最吸引人的地方是简洁：一个装饰器就能定义一个 Web 接口。\n\n'
                 '下一步计划学习 Jinja2 模板和数据库集成。',
                 '技术', '2026-06-06 10:00:00'),
                ('Jinja2 模板学习心得',
                 'Jinja2 模板让 HTML 和 Python 数据的结合变得优雅。\n\n'
                 '核心语法只有三种：{{ 变量 }}、{% 逻辑 %} 和 {# 注释 #}。\n\n'
                 '模板继承是最强大的功能，用 extends + block 避免大量重复 HTML。',
                 '技术', '2026-06-07 14:00:00'),
                ('SQLite 数据持久化实践',
                 '今天把内存数据换成了 SQLite 数据库，重启后数据不再丢失。\n\n'
                 '关键点：用参数化查询防 SQL 注入，用 with 语句确保连接正确关闭。\n\n'
                 'PRG 模式（POST→Redirect→GET）防止刷新重复提交，是 Web 开发的最佳实践。',
                 '技术', '2026-06-08 09:00:00'),
                ('学习 Python 的第29天',
                 '今天完成了个人博客系统，是 Flask 阶段的收官之作。\n\n'
                 '从第27天的路由基础，到今天的完整博客，用了3天时间。\n\n'
                 '感受：把知识点串联成项目，理解会深很多。',
                 '日记', '2026-06-08 16:00:00'),
            ]
            conn.executemany(
                "INSERT INTO articles (title, content, category, created_at) VALUES (?,?,?,?)",
                sample
            )


# ============================================
# 工具函数
# ============================================

def get_articles(category=None):
    """查询文章列表，可按分类筛选"""
    with sqlite3.connect(DB_PATH) as conn:
        conn.row_factory = sqlite3.Row  # 让结果可以用列名访问
        if category:
            rows = conn.execute(
                "SELECT * FROM articles WHERE category=? ORDER BY created_at DESC",
                (category,)
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT * FROM articles ORDER BY created_at DESC"
            ).fetchall()
    return [dict(r) for r in rows]


def get_article(article_id):
    """查询单篇文章"""
    with sqlite3.connect(DB_PATH) as conn:
        conn.row_factory = sqlite3.Row
        row = conn.execute(
            "SELECT * FROM articles WHERE id=?", (article_id,)
        ).fetchone()
    return dict(row) if row else None


def get_categories():
    """获取所有分类（去重）"""
    with sqlite3.connect(DB_PATH) as conn:
        rows = conn.execute(
            "SELECT DISTINCT category FROM articles ORDER BY category"
        ).fetchall()
    return [r[0] for r in rows]


def make_excerpt(content, length=80):
    """截取文章摘要"""
    text = content.replace('\n', ' ')
    return text[:length] + '…' if len(text) > length else text


# ============================================
# HTML 模板
# ============================================

BASE_STYLE = """
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: 'Microsoft YaHei', Arial, sans-serif; background: #f5f7fa; color: #333; }
nav { background: #1a1a2e; padding: 14px 0; }
nav .inner { max-width: 860px; margin: 0 auto; padding: 0 20px;
             display: flex; align-items: center; justify-content: space-between; }
nav a { color: #ccc; text-decoration: none; margin-left: 18px; font-size: 14px; }
nav a:hover, nav a.active { color: #fff; }
nav .brand { color: #fff; font-size: 18px; font-weight: bold; margin-left: 0; }
.container { max-width: 860px; margin: 32px auto; padding: 0 20px; }
.card { background: #fff; border-radius: 8px; padding: 24px; margin-bottom: 20px;
        box-shadow: 0 1px 4px rgba(0,0,0,0.08); }
h1 { font-size: 22px; margin-bottom: 12px; }
h2 { font-size: 18px; margin-bottom: 8px; }
.meta { color: #888; font-size: 13px; margin-bottom: 10px; }
.tag { display: inline-block; padding: 2px 10px; border-radius: 20px; font-size: 12px;
       background: #e8f4fd; color: #1976d2; margin-right: 6px; }
.excerpt { color: #555; font-size: 14px; line-height: 1.7; }
.btn { display: inline-block; padding: 8px 18px; border-radius: 5px; font-size: 14px;
       text-decoration: none; border: none; cursor: pointer; }
.btn-primary { background: #1976d2; color: #fff; }
.btn-primary:hover { background: #1565c0; }
.btn-danger { background: #e53935; color: #fff; font-size: 12px; padding: 5px 12px; }
.btn-danger:hover { background: #c62828; }
.btn-outline { background: #fff; color: #1976d2; border: 1px solid #1976d2; }
input[type=text], input[type=email], textarea, select {
    width: 100%; padding: 9px 12px; border: 1px solid #ddd; border-radius: 5px;
    font-size: 14px; margin-bottom: 14px; font-family: inherit; }
textarea { min-height: 200px; resize: vertical; line-height: 1.6; }
label { display: block; font-size: 13px; color: #555; margin-bottom: 4px; }
.filter-bar { margin-bottom: 20px; }
.filter-bar a { display: inline-block; padding: 5px 14px; border-radius: 20px; font-size: 13px;
                text-decoration: none; color: #555; background: #fff; border: 1px solid #ddd;
                margin-right: 8px; margin-bottom: 8px; }
.filter-bar a.active, .filter-bar a:hover { background: #1976d2; color: #fff; border-color: #1976d2; }
.article-content { line-height: 1.9; font-size: 15px; white-space: pre-wrap; }
.empty { text-align: center; color: #999; padding: 60px 0; }
.error { color: #e53935; font-size: 13px; margin-bottom: 12px; padding: 8px 12px;
         background: #ffebee; border-radius: 4px; }
</style>
"""


def render(title, body, active=''):
    """生成完整页面 HTML"""
    nav_links = [('/', '首页', 'home'), ('/new', '+ 写文章', 'new')]
    nav_html = ''.join(
        f'<a href="{href}" class="{"active" if active == key else ""}">{label}</a>'
        for href, label, key in nav_links
    )
    return f"""<!DOCTYPE html>
<html lang="zh">
<head><meta charset="utf-8"><title>{title} - 我的博客</title>{BASE_STYLE}</head>
<body>
<nav><div class="inner">
  <a href="/" class="brand">我的博客</a>
  <div>{nav_html}<a href="/api/articles" style="color:#888;font-size:12px">API</a></div>
</div></nav>
<div class="container">{body}</div>
</body></html>"""


# ============================================
# 路由
# ============================================

@app.route('/')
def index():
    category = request.args.get('cat', '').strip()
    articles = get_articles(category or None)
    categories = get_categories()

    # 分类筛选栏
    filter_html = '<div class="filter-bar">'
    filter_html += f'<a href="/" class="{"active" if not category else ""}">全部</a>'
    for cat in categories:
        active_cls = 'active' if cat == category else ''
        filter_html += f'<a href="/?cat={cat}" class="{active_cls}">{cat}</a>'
    filter_html += '</div>'

    if not articles:
        cards = '<div class="empty"><p>暂无文章</p><br><a href="/new" class="btn btn-primary">写第一篇文章</a></div>'
    else:
        cards = ''
        for a in articles:
            excerpt = make_excerpt(a['content'])
            cards += f"""
            <div class="card">
              <div class="meta">
                <span class="tag">{a['category']}</span>
                {a['created_at'][:16]}
              </div>
              <h2><a href="/article/{a['id']}" style="color:#1a1a2e;text-decoration:none">{a['title']}</a></h2>
              <p class="excerpt" style="margin:8px 0 14px">{excerpt}</p>
              <a href="/article/{a['id']}" class="btn btn-outline" style="font-size:13px">阅读全文</a>
            </div>"""

    body = f"""
    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px">
      <h1 style="font-size:20px">{'「' + category + '」文章' if category else '全部文章'}（{len(articles)} 篇）</h1>
      <a href="/new" class="btn btn-primary">+ 写文章</a>
    </div>
    {filter_html}
    {cards}
    """
    return render('首页', body, 'home')


@app.route('/article/<int:aid>')
def article_detail(aid):
    a = get_article(aid)
    if not a:
        return render('未找到', '<div class="card"><p>文章不存在</p><br><a href="/">返回首页</a></div>'), 404

    body = f"""
    <div class="card">
      <div class="meta">
        <span class="tag">{a['category']}</span>
        发布于 {a['created_at'][:16]}
      </div>
      <h1>{a['title']}</h1>
      <hr style="border:none;border-top:1px solid #eee;margin:16px 0">
      <div class="article-content">{a['content']}</div>
      <div style="margin-top:28px;display:flex;gap:10px;align-items:center">
        <a href="/" class="btn btn-outline">← 返回列表</a>
        <form method="POST" action="/delete/{a['id']}"
              onsubmit="return confirm('确认删除《{a['title']}》？')">
          <button type="submit" class="btn btn-danger">删除文章</button>
        </form>
      </div>
    </div>
    """
    return render(a['title'], body)


@app.route('/new', methods=['GET', 'POST'])
def new_article():
    error = ''
    # 保留用户输入，校验失败时回填表单
    form = {'title': '', 'content': '', 'category': '技术'}

    if request.method == 'POST':
        form['title']    = request.form.get('title', '').strip()
        form['content']  = request.form.get('content', '').strip()
        form['category'] = request.form.get('category', '技术').strip()

        if not form['title']:
            error = '标题不能为空'
        elif not form['content']:
            error = '内容不能为空'
        elif len(form['title']) > 100:
            error = '标题不能超过100个字符'

        if not error:
            created_at = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            with sqlite3.connect(DB_PATH) as conn:
                cursor = conn.execute(
                    "INSERT INTO articles (title, content, category, created_at) VALUES (?,?,?,?)",
                    (form['title'], form['content'], form['category'], created_at)
                )
                new_id = cursor.lastrowid
            # PRG 模式：POST 成功后重定向，防止刷新重复提交
            return redirect(url_for('article_detail', aid=new_id))

    categories = ['技术', '日记', '读书', '生活', '其他']
    cat_options = ''.join(
        f'<option value="{c}" {"selected" if c == form["category"] else ""}>{c}</option>'
        for c in categories
    )

    body = f"""
    <div class="card">
      <h1>写新文章</h1>
      <hr style="border:none;border-top:1px solid #eee;margin:16px 0 20px">
      {'<div class="error">' + error + '</div>' if error else ''}
      <form method="POST">
        <label>标题</label>
        <input type="text" name="title" value="{form['title']}" placeholder="输入文章标题..." maxlength="100">
        <label>分类</label>
        <select name="category" style="width:auto;margin-bottom:14px">{cat_options}</select>
        <label>正文</label>
        <textarea name="content" placeholder="写下你的内容...">{form['content']}</textarea>
        <div style="display:flex;gap:10px">
          <button type="submit" class="btn btn-primary">发布文章</button>
          <a href="/" class="btn btn-outline">取消</a>
        </div>
      </form>
    </div>
    """
    return render('写文章', body, 'new')


@app.route('/delete/<int:aid>', methods=['POST'])
def delete_article(aid):
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("DELETE FROM articles WHERE id=?", (aid,))
    return redirect(url_for('index'))


# ============================================
# JSON API
# ============================================

@app.route('/api/articles')
def api_articles():
    articles = get_articles()
    return jsonify({
        'total': len(articles),
        'articles': [
            {**a, 'excerpt': make_excerpt(a['content'])}
            for a in articles
        ]
    })


@app.route('/api/articles/<int:aid>')
def api_article(aid):
    a = get_article(aid)
    if not a:
        return jsonify({'error': '文章不存在'}), 404
    return jsonify(a)


# ============================================
# 启动
# ============================================

if __name__ == '__main__':
    init_db()
    print("=" * 50)
    print("个人博客系统")
    print("=" * 50)
    print("访问地址：http://127.0.0.1:5004")
    print()
    print("页面路由：")
    print("  /               首页（文章列表）")
    print("  /?cat=技术       按分类筛选")
    print("  /article/<id>   文章详情")
    print("  /new            发布新文章")
    print("  /delete/<id>    删除文章（POST）")
    print()
    print("JSON API：")
    print("  /api/articles        所有文章")
    print("  /api/articles/<id>   单篇文章")
    print()
    print("数据库：", DB_PATH)
    print("按 Ctrl+C 停止")
    print("=" * 50)
    app.run(debug=True, port=5004)
