#!/bin/bash

# 设置UTF-8编码
export LANG=zh_CN.UTF-8

echo "========================================"
echo "React学习系统 - 第1天"
echo "========================================"
echo ""

show_menu() {
    echo "请选择要运行的学习材料："
    echo ""
    echo "[1] 01. 变量声明 (30分钟)"
    echo "[2] 02. 数据类型 (30分钟)"
    echo "[3] 03. 模板字符串 (20分钟)"
    echo "[4] 04. 综合练习 (40分钟)"
    echo "[5] 运行所有文件"
    echo "[6] 打开浏览器学习页面"
    echo "[0] 退出"
    echo ""
    read -p "请输入选项 (0-6): " choice
    echo ""
}

run_file1() {
    echo "========================================"
    echo "正在运行：01. 变量声明"
    echo "========================================"
    echo ""
    node docs/react_basics/01_js_variables.jsx
    echo ""
    echo "========================================"
    echo "运行完成！"
    echo "========================================"
    echo ""
    read -p "按Enter键继续..."
}

run_file2() {
    echo "========================================"
    echo "正在运行：02. 数据类型"
    echo "========================================"
    echo ""
    node docs/react_basics/02_js_datatypes.jsx
    echo ""
    echo "========================================"
    echo "运行完成！"
    echo "========================================"
    echo ""
    read -p "按Enter键继续..."
}

run_file3() {
    echo "========================================"
    echo "正在运行：03. 模板字符串"
    echo "========================================"
    echo ""
    node docs/react_basics/03_js_template_strings.jsx
    echo ""
    echo "========================================"
    echo "运行完成！"
    echo "========================================"
    echo ""
    read -p "按Enter键继续..."
}

run_file4() {
    echo "========================================"
    echo "正在运行：04. 综合练习"
    echo "========================================"
    echo ""
    node docs/react_basics/04_practice_hello.jsx
    echo ""
    echo "========================================"
    echo "运行完成！"
    echo "========================================"
    echo ""
    read -p "按Enter键继续..."
}

run_all() {
    echo "========================================"
    echo "正在运行所有学习文件..."
    echo "========================================"
    echo ""

    echo "[1/4] 变量声明"
    node docs/react_basics/01_js_variables.jsx
    echo ""

    echo "[2/4] 数据类型"
    node docs/react_basics/02_js_datatypes.jsx
    echo ""

    echo "[3/4] 模板字符串"
    node docs/react_basics/03_js_template_strings.jsx
    echo ""

    echo "[4/4] 综合练习"
    node docs/react_basics/04_practice_hello.jsx
    echo ""

    echo "========================================"
    echo "所有文件运行完成！"
    echo "========================================"
    echo ""
    read -p "按Enter键继续..."
}

open_browser() {
    echo "正在打开浏览器学习页面..."

    # 检测操作系统并打开浏览器
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        open docs/index.html
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        xdg-open docs/index.html
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        # Windows Git Bash
        start docs/index.html
    else
        echo "无法自动打开浏览器，请手动打开 docs/index.html"
    fi

    echo ""
    echo "提示：如果页面无法正常显示，请安装Live Server插件"
    echo "或者在VSCode中右键点击index.html选择'Open with Live Server'"
    echo ""
    read -p "按Enter键继续..."
}

# 主循环
while true; do
    show_menu

    case $choice in
        1)
            run_file1
            ;;
        2)
            run_file2
            ;;
        3)
            run_file3
            ;;
        4)
            run_file4
            ;;
        5)
            run_all
            ;;
        6)
            open_browser
            ;;
        0)
            echo ""
            echo "感谢使用React学习系统！"
            echo "学习结束后记得说'学习结束'生成今日日志。"
            echo ""
            exit 0
            ;;
        *)
            echo "无效选项，请重新选择"
            echo ""
            ;;
    esac
done
