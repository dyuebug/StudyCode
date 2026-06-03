#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "TaskDialog.h"

/**
 * @brief MainWindow 类 - 主窗口
 *
 * 应用程序的主界面，显示任务列表和提供各种操作
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // ========================================
    // 任务操作槽函数
    // ========================================

    void onAddTask();           // 添加任务
    void onEditTask();          // 编辑任务
    void onDeleteTask();        // 删除任务
    void onCompleteTask();      // 标记任务完成/未完成
    void onRefresh();           // 刷新任务列表

    // ========================================
    // 筛选槽函数
    // ========================================

    void onCategoryFilterChanged(int index);    // 分类筛选
    void onPriorityFilterChanged(int index);    // 优先级筛选
    void onStatusFilterChanged(int index);      // 状态筛选

    // ========================================
    // 其他槽函数
    // ========================================

    void onTaskSelectionChanged();  // 任务选择改变
    void onAbout();                 // 关于对话框

private:
    // ========================================
    // UI 初始化方法
    // ========================================

    void setupUI();             // 设置 UI
    void setupMenuBar();        // 设置菜单栏
    void setupToolBar();        // 设置工具栏
    void setupStatusBar();      // 设置状态栏
    void setupCentralWidget();  // 设置中心控件

    // ========================================
    // 数据操作方法
    // ========================================

    void loadTasks();           // 加载任务列表
    void updateStatistics();    // 更新统计信息
    void applyFilters();        // 应用筛选条件

    // ========================================
    // UI 控件
    // ========================================

    // 任务列表
    QTableWidget *m_taskTable;

    // 筛选控件
    QComboBox *m_categoryFilter;
    QComboBox *m_priorityFilter;
    QComboBox *m_statusFilter;

    // 按钮
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_completeButton;
    QPushButton *m_refreshButton;

    // 状态栏标签
    QLabel *m_statsLabel;

    // ========================================
    // 数据成员
    // ========================================

    int m_currentCategoryFilter;    // 当前分类筛选（0 = 全部）
    int m_currentPriorityFilter;    // 当前优先级筛选（0 = 全部）
    int m_currentStatusFilter;      // 当前状态筛选（0 = 全部，1 = 未完成，2 = 已完成）
};

#endif // MAINWINDOW_H
