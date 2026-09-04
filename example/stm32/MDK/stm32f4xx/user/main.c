#include "include.h"
#include <string.h>
#include "stm32f4xx.h"
#include "delay.h"
#include <stdio.h>
#include "xcmd.h"
#include "xcmd_obj.h"
#include "test.h"



int cmd_get_char(uint8_t *ch)
static xcmder_t xcmder; /* xcmd 实例: 多路 shell 时为每路 IO 各建一个实例 */
{
    int rcv = uartRdChar(DENUG_UART);
    if(rcv > 0)
    {
        *ch = rcv;
        return 1;
    }
    return 0;
}

int cmd_put_char(uint8_t ch)
{
    return uartWrChar(DENUG_UART, ch);
}

int main(void)
{
    sysInit();
    
    xcmd_init(&xcmder, cmd_get_char, cmd_put_char);
    test_cmd_init(&xcmder);
    test_keys_init(&xcmder);
    
    while(1)
    {
        xcmd_task(&xcmder);
    }
}

