/*
 * @Author: your name
 * @Date: 2021-09-14 23:58:24
 * @LastEditTime: 2021-10-11 20:14:26
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /xcmd/example/linux/linux_main.c
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "xcmd_platform.h"
#include "xcmd.h"
#include "xcmd_obj.h"
#include "test.h"
#include "ex_keys.h"
#include "ex_cmds.h"
#include "ex_list.h"
#include "fs_cmds.h"
#include "socket_cmds.h"
#include "ff.h"
#include "fatfs_disk_mmc.h"
#include "fatfs_disk_ram.h"

FIL g_fp;

int getch(void)
{
    struct termios tm, tm_old;
    int fd = 0, ch;

    if (tcgetattr(fd, &tm) < 0)
    { //保存现在的终端设置
        return -1;
    }
    tm_old = tm;
    cfmakeraw(&tm); //更改终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
    if (tcsetattr(fd, TCSANOW, &tm) < 0)
    { //设置上更改之后的设置
        return -1;
    }

    ch = getchar();
    if (tcsetattr(fd, TCSANOW, &tm_old) < 0)
    { //更改设置为最初的样子
        return -1;
    }

    return ch;
}

int cmd_get_char(uint8_t *ch)
{
    *ch = getch();
    return 1;
}

int cmd_put_char(uint8_t ch)
{
    putchar(ch);
    return 1;
}

static int key_ctr_c(void *pv)
{
    exit(0);
}

static xcmd_key_t keys[] =
    {
        {KEY_CTR_C, key_ctr_c, "ctr+c", NULL},
};

void user_keys_init(xcmder_t* xcmder)
{
    xcmd_key_register(xcmder, keys, sizeof(keys) / sizeof(xcmd_key_t));
}

void fatfs_test(char* path)
{
    FRESULT res;
    res = f_open(&g_fp, path, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
    if(res == FR_OK)
    {
        char buf[128];
        strcpy(buf, "hello world");
        int len = strlen(buf);
        UINT bw;
        UINT br;
        res = f_write(&g_fp, buf, len, &bw);
        memset(buf, 0, len);
        f_lseek(&g_fp,0);
        res = f_read(&g_fp, buf, len, &br);
        printf("test file:%s\r\n", buf);
        f_close(&g_fp);
    }
}

/* 实例1: 绑定终端 stdio */
static xcmder_t xcmder_stdio;

/* 实例2: 演示第二路 shell —— 绑定到内存回环 IO,
 * 命令互不干扰: 实例2 只注册了默认命令, help 列表与实例1 不同 */
static xcmder_t xcmder_second;

static char second_port_tx[256];
static uint16_t second_port_tx_len = 0;
static const char* second_port_rx; /* 待注入实例2的输入 */
static uint16_t second_port_rx_len = 0;
static uint16_t second_port_rx_pos = 0;

static int second_get_char(uint8_t *ch)
{
    if (second_port_rx_pos < second_port_rx_len)
    {
        *ch = (uint8_t)second_port_rx[second_port_rx_pos++];
        return 1;
    }
    return 0;
}

static int second_put_char(uint8_t ch)
{
    if (second_port_tx_len < sizeof(second_port_tx) - 1)
    {
        second_port_tx[second_port_tx_len++] = (char)ch;
        second_port_tx[second_port_tx_len] = '\0';
    }
    return 1;
}

/* 向实例2 注入一条命令并泵任务直到输入耗尽, 返回实例2的输出 */
static const char* second_exec(const char* line)
{
    second_port_tx_len = 0;
    second_port_tx[0] = '\0';
    second_port_rx = line;
    second_port_rx_len = (uint16_t)strlen(line);
    second_port_rx_pos = 0;
    for (int i = 0; i < 512; i++)
    {
        xcmd_task(&xcmder_second);
        if (second_port_rx_pos >= second_port_rx_len)
        {
            break;
        }
    }
    return second_port_tx;
}

int main(void)
{
    xcmd_init(&xcmder_stdio, cmd_get_char, cmd_put_char);
    ram_disk_init();
    mmc_disk_init();
    test_cmd_init(&xcmder_stdio);
    test_keys_init(&xcmder_stdio);
    user_keys_init(&xcmder_stdio);
    ex_keys_init(&xcmder_stdio);
    ex_cmds_init(&xcmder_stdio);
    socket_cmds_init(&xcmder_stdio);
    fs_cmds_init(&xcmder_stdio);
    ex_list_init(&xcmder_stdio);
    xcmd_display_redraw(&xcmder_stdio); /* 所有初始化输出结束后重绘命令行, 提示符回到行首 */

    /* 实例2: 独立初始化, 只注册默认命令, 提示符也独立 */
    xcmd_init(&xcmder_second, second_get_char, second_put_char);
    xcmd_set_prompt(&xcmder_second, "2nd>");

    /* 多实例隔离验证: 实例2 执行 help, 两个实例互不影响 */
    printf("\r\n[instance 2] exec 'help':\r\n%s\r\n", second_exec("help\r"));
    printf("[instance 2] exec 'example -i 42' (not registered on instance 2):\r\n%s\r\n",
           second_exec("example -i 42\r"));
    printf("[instance 1] 'example' still works here; instance 2 prompt: %s\r\n\r\n",
           xcmd_get_prompt(&xcmder_second));

    while (1)
    {
        xcmd_task(&xcmder_stdio);
        xcmd_task(&xcmder_second);
    }
}
