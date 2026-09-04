#include <string.h>
#include "stm32f10x.h"
#include "delay.h"
#include "uart.h"
#include <stdio.h>
#include "xcmd.h"
#include "xcmd_obj.h"
#include "test.h"

int cmd_get_char(uint8_t *ch)
static xcmder_t xcmder; /* xcmd 实例: 多路 shell 时为每路 IO 各建一个实例 */
{
    return uartRdChar(ch);
}

int cmd_put_char(uint8_t ch)
{
    return uartWrChar(ch);
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init(72);
    uartInit(115200);
    
    xcmd_init(&xcmder, cmd_get_char, cmd_put_char);
    test_cmd_init(&xcmder);
    test_keys_init(&xcmder);
    
    while(1)
    {
        xcmd_task(&xcmder);
    }
}
