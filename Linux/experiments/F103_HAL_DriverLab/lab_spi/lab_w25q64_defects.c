/*
 * lab_w25q64_defects.c —— W25Q64 驱动的四条语义/缺陷校验（教材 9.5）
 *
 * 目的：把"看代码觉得有问题"变成"跑出来是这样"。四段：
 *   1. W25Q64_Erase 的对齐判据用了 && 而不是 ||：哪些非法输入能溜过去；
 *   2. 页写回卷：教材 9.5.1 说"地址到页尾会从页首开始"，不分段会覆盖已写数据；
 *   3. Flash 只能 1→0：不擦除就写，写 0xFF 也清不掉旧 0；
 *   4. W25Q64_WaitReady 的超时分支不可达：`while (timeout--)` + `if (!timeout)` 的组合。
 *
 * 注意：本程序复刻的是驱动里的**逻辑**（逐行对应 driver_spiflash_w25q64.c 的相应片段），
 *       不是调用原函数——原函数依赖 HAL 与真实 SPI，主机上跑不了。
 *       所以这里的结论是"逻辑必然如此"，不是"真芯片上观测到如此"。
 *
 * 构建与运行：
 *   gcc -std=c11 -Wall -Wextra -o lab_w25q64_defects lab_w25q64_defects.c
 *   ./lab_w25q64_defects
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SECTOR_SIZE 4096u
#define PAGE_SIZE   256u

/* ============ 1. 对齐判据 ============ */

/*
 * driver_spiflash_w25q64.c:360
 *     if ((offset & (4096-1)) && (len & (4096-1)))
 *         return -1;
 * 本意：offset 与 len 都必须 4 KB 对齐，任一不对齐就拒绝。
 * 用 && 的后果：只要有一个对齐，检查就通过。
 */
static int check_and(uint32_t offset, uint32_t len)
{
    return ((offset & (SECTOR_SIZE - 1)) && (len & (SECTOR_SIZE - 1))) ? -1 : 0;
}

static int check_or(uint32_t offset, uint32_t len)
{
    return ((offset & (SECTOR_SIZE - 1)) || (len & (SECTOR_SIZE - 1))) ? -1 : 0;
}

/* 通过检查之后实际会擦几个扇区：driver 的循环是 for (sector = 0; sector < len/4096; sector++) */
static uint32_t sectors_erased(uint32_t len)
{
    return len / SECTOR_SIZE;
}

/* 调用方真正想覆盖的扇区数：从 offset 起 len 字节跨了几个 4 KB 扇区 */
static uint32_t sectors_needed(uint32_t offset, uint32_t len)
{
    return ((offset & (SECTOR_SIZE - 1)) + len + SECTOR_SIZE - 1) / SECTOR_SIZE;
}

/* ============ 2/3. 一个最小的 NOR Flash 模型 ============ */

static uint8_t g_flash[2 * SECTOR_SIZE];

static void flash_reset(void)
{
    memset(g_flash, 0xFF, sizeof g_flash);   /* 擦除后全 1 */
}

/*
 * 页写：页边界由**起始地址**决定，芯片内部的 8 位列计数只在起始页内自增，
 * 越过第 255 列回到该页第 0 列（教材 9.5.1 原话）。
 * 注意 page_base 必须由 offset 算，不能由 offset+i 算——否则"回卷"会被模型算成
 * "换到下一页接着写"，损坏就观察不到了（首版正是这么写的，实测才发现）。
 */
static void program_page_wrap(uint32_t offset, const uint8_t *src, uint32_t len)
{
    uint32_t i;
    uint32_t page_base = offset & ~(PAGE_SIZE - 1);

    for (i = 0; i < len; i++) {
        uint32_t col = ((offset & (PAGE_SIZE - 1)) + i) & (PAGE_SIZE - 1);
        uint32_t idx = page_base + col;

        if (idx < sizeof g_flash)
            g_flash[idx] &= src[i];          /* NOR 只能把 1 写成 0 */
    }
}

/* 驱动的分段写法（driver_spiflash_w25q64.c:290-295、334-338） */
static void program_chunked(uint32_t offset, const uint8_t *src, uint32_t len)
{
    uint32_t cur_len = PAGE_SIZE - (offset & (PAGE_SIZE - 1));
    uint32_t pos     = offset;
    uint32_t remain  = len;

    if (cur_len > len)
        cur_len = len;

    while (pos < offset + len) {
        program_page_wrap(pos, src, cur_len);   /* 每次都没跨页 */
        pos    += cur_len;
        src    += cur_len;
        remain -= cur_len;
        cur_len = (remain < PAGE_SIZE) ? remain : PAGE_SIZE;
    }
}

static void dump(const char *tag, uint32_t from, uint32_t n)
{
    uint32_t i;

    printf("  %-10s", tag);
    for (i = 0; i < n; i++)
        printf(" %02X", g_flash[from + i]);
    printf("\n");
}

/* ============ 4. WaitReady 的超时返回值 ============ */

/*
 * driver_spiflash_w25q64.c:157-177
 *     int timeout = W25Q64_TIMEOUT;      // 500
 *     while (timeout--) { ...; if (ready) return 0; mdelay(1); }
 *     if (!timeout) return -1;
 *     return 0;
 * 模型：从机永远 BUSY，看最后返回什么、循环了几次。
 */
static int wait_ready_model(int always_busy, int *iters)
{
    int timeout = 500;
    int n = 0;

    while (timeout--) {
        n++;
        if (!always_busy)
            goto ready;                  /* 成功路径：立即返回 */
    }
    *iters = n;
    if (!timeout)                        /* 退出循环时 timeout 已经变成 -1 */
        return -1;
    return 0;

ready:
    *iters = n;
    return 0;
}

int main(void)
{
    printf("== 1. 擦除对齐判据：&& vs || ==\n");
    {
        struct { uint32_t off, len; } cases[] = {
            {    0, 4096 },   /* 都对：应当放行 */
            { 4096, 4096 },   /* 都对 */
            {    0,  100 },   /* len 不对：应当拒绝，实际会擦 0 个扇区却返回 100 */
            {  100, 4096 },   /* offset 不对：应当拒绝，实际擦错扇区 */
            {  100,  100 },   /* 都不对：应当拒绝 */
        };
        unsigned i;

        printf("  %-12s %-8s %-8s %-8s %-12s %s\n",
               "offset,len", "&& 判定", "|| 判定", "擦的扇区", "该擦的扇区", "结论");
        for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            uint32_t o = cases[i].off, l = cases[i].len;
            int a = check_and(o, l);
            int b = check_or(o, l);
            uint32_t got     = (a == 0) ? sectors_erased(l) : 0;
            uint32_t want    = sectors_needed(o, l);
            const char *verdict;

            if (a != 0)
                verdict = "拒绝（正确）";
            else if (got != want)
                verdict = "放行但擦错 ⇒ 静默损坏";
            else
                verdict = "放行（正确）";

            printf("  %-12u %-8u %-8s %-8s %-12u %-10u %s\n",
                   o, l, a ? "拒绝" : "放行", b ? "拒绝" : "放行",
                   got, want, verdict);
        }
        printf("  => 只有 (offset,len) 都不对齐时 && 才拒绝；(0,100) 这种会静默擦 0 个扇区并返回成功\n");
    }

    printf("\n== 2. 页写回卷：不分段 vs 分段（页 = 256 B）==\n");
    {
        /*
         * 源数据前 256 字节用 0x10..0x1F 循环，后 44 字节全部 0xC0。
         * 这个"两段不同"的构造是必需的：如果整段都用周期 16 的花纹，
         * 因为 256 是 16 的倍数，回卷覆盖出来的字节和新写入的完全一样，
         * 损坏就观察不到——测试向量本身会让判据失效。
         */
        uint8_t src[300];
        uint32_t i, bad = 0;

        for (i = 0; i < 256; i++)
            src[i] = (uint8_t)(0x10 + (i & 0x0F));
        for (i = 256; i < sizeof src; i++)
            src[i] = 0xC0;

        flash_reset();
        program_page_wrap(0, src, 300);
        printf("  不分段写 300 B @0：第 257 字节起回到页首，覆盖前 44 字节\n");
        dump("byte 0..15  ", 0, 16);
        dump("byte 32..47 ", 32, 16);
        dump("byte 240..255", 240, 16);
        for (i = 0; i < 256; i++)
            if (g_flash[i] != src[i])
                bad++;
        printf("  与源数据不一致的字节数：%u / 256（应为 44，即被尾部覆盖的那段）\n", bad);
        printf("  被覆盖处是 0x00 而不是 0xC0：尾部又写了一次，0x10 & 0xC0 = 0x00（同一物理规律）\n");

        flash_reset();
        program_chunked(0, src, 300);
        dump("分段 byte0..15  ", 0, 16);
        dump("分段 byte240..255", 240, 16);
        dump("分段 byte256..271", 256, 16);
        bad = 0;
        for (i = 0; i < sizeof src; i++)
            if (g_flash[i] != src[i])
                bad++;
        printf("  与源数据不一致的字节数：%u / 300\n", bad);
        printf("  => 不分段会静默覆盖；驱动按 256 字节切段后 memcmp 全等\n");
    }

    printf("\n== 3. 不擦除就写：NOR 只能 1→0 ==\n");
    {
        uint8_t zero = 0x00, ones = 0xFF;

        flash_reset();
        program_page_wrap(0, &zero, 1);       /* 先写 0x00 */
        program_page_wrap(0, &ones, 1);       /* 不擦除，再写 0xFF */
        printf("  写 0x00 后写 0xFF，结果为 0x%02X（不会回到 0xFF）\n", g_flash[0]);
        printf("  => 这就是\"写之前必须先擦扇区\"的物理原因，不是驱动偷懒\n");
    }

    printf("\n== 4. W25Q64_WaitReady 的超时分支 ==\n");
    {
        int iters = 0;
        int r1 = wait_ready_model(1, &iters);   /* 一直 BUSY */
        printf("  从机一直 BUSY：循环 %d 次，返回 %d（声明里说超时应返回 -1）\n", iters, r1);

        iters = 0;
        int r2 = wait_ready_model(0, &iters);   /* 立刻就绪 */
        printf("  从机立刻就绪：循环 %d 次，返回 %d\n", iters, r2);
        printf("  => while (timeout--) 退出时 timeout = -1，if (!timeout) 恒假，-1 不可达\n");
    }

    return 0;
}
