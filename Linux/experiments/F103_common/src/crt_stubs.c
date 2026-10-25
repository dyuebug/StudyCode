/**
 * 最小运行时补丁：承接 crt0 / crti 的义务
 *
 * 背景：链接时使用了 -nostartfiles。原因是 vendor/startup/startup_stm32f103xb.s
 * 已经完成复位入口、.data 拷贝、.bss 清零，再链接默认的 crt0.o 属于重复；
 * 而且 crt0.o 会为了 exit() 拉入 newlib 的 stdio，进而引用
 * _write/_read/_lseek/_close，使每次构建都出现 libnosys 的"未实现"告警。
 *
 * 代价：crti.o 提供的 _init / _fini 不再存在，而 newlib 的
 * __libc_init_array() 会调用 _init()，所以这里补空实现。
 * C++ 全局构造不受影响：__libc_init_array() 仍然遍历 .init_array。
 *
 * 注意：本文件只声明执行顺序占位，不做任何硬件操作。
 */

void _init(void) { }
void _fini(void) { }
