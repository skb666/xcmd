/*
 * @Author: your name
 * @Date: 2021-09-14 23:58:24
 * @LastEditTime: 2021-09-22 22:50:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /xcmd/example/linux/linux_main.c
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "xcmd.h"
#include "xcmd_obj.h"
#include "test.h"

static xcmder_t xcmder; /* xcmd 实例: 多路 shell 时为每路 IO 各建一个实例 */

int cmd_get_char(unsigned char *ch)
{
    *ch = getc(stdin);
    return 1;
}

int cmd_put_char(unsigned char ch)
{
    putchar(ch);
    return 1;
}

void app_main(void)
{
    xcmd_init(&xcmder, cmd_get_char, cmd_put_char);
    test_cmd_init(&xcmder);
    test_keys_init(&xcmder);

    while(1)
    {
        xcmd_task(&xcmder);
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}
