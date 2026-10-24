#include <linux/module.h>
#include <linux/kernel.h>

/*
 * Day2 最小内核模块
 *
 * 与其他代码的区别：
 *   - 没有 main()，入口通过 module_init() 注册
 *   - 输出走内核日志，用 dmesg 查看
 *   - 编译产物是 .ko，由 insmod 加载
 */

static int __init hello_init(void)
{
	pr_info("hello_module: init\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello_module: exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("study");
MODULE_DESCRIPTION("Day2 minimal kernel module");
