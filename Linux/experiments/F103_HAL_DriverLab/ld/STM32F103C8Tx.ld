/*
 * STM32F103C8T6 链接脚本（GCC / arm-none-eabi-ld）
 *
 * 为什么需要自撰：课程素材只随 Keil MDK 工程提供 .sct 分散加载文件，
 * CMSIS 模板目录里没有 GCC 的 .ld。本文件按芯片手册的存储布局手写，
 * 符号名严格对齐 CMSIS gcc 启动文件 startup_stm32f103xb.s 的引用。
 *
 * 目标芯片事实：
 *   Flash  64 KB  @ 0x08000000
 *   SRAM   20 KB  @ 0x20000000
 */

ENTRY(Reset_Handler)

/* 初始栈顶 = SRAM 末尾。栈向下增长，可用深度由 ._user_heap_stack 的预留量约束 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* 预留量只在链接期做溢出检查，不代表运行期的堆栈预算 */
_Min_Heap_Size  = 0x200;   /* 512 B */
_Min_Stack_Size = 0x400;   /* 1 KB */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
  /* 中断向量表必须落在 Flash 起始地址：Cortex-M3 复位后从这里取 MSP 和 PC */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* newlib 的 __libc_init_array() 通过这三个区间遍历构造函数，缺符号会链接失败 */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } >FLASH

  /* .data 的初值驻留 Flash，由启动文件在进入 main 前拷贝到 RAM */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /*
   * 溢出哨兵：把堆和栈的预留量真实占进 RAM。
   * 若 .data + .bss 增长到吃掉预留区，链接期即报 region RAM overflowed，
   * 而不是等到运行期踩坏栈才表现成随机死机。
   */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
