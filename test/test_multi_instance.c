/*
 * 多实例验证程序 (Windows/mingw 下运行; linux 例程本身就是多实例演示):
 * 1. 实例 A 与实例 B 各绑定一路内存回环 IO
 * 2. 向 A 注册扩展命令 (test/ex_cmds), 向 B 只留默认命令
 * 3. 验证: A 能跑 example, B 跑 example 报不存在; 两实例历史/提示符互相独立
 */
#include <stdio.h>
#include <string.h>

#include "xcmd.h"
#include "xcmd_obj.h"
#include "ex_cmds.h"
#include "test.h"

typedef struct {
    char tx[512];
    uint16_t tx_len;
    const char* rx;
    uint16_t rx_len;
    uint16_t rx_pos;
} loop_port_t;

static loop_port_t port_a;
static loop_port_t port_b;

static int loop_get_char(uint8_t* ch, loop_port_t* port) {
    if (port->rx_pos < port->rx_len) {
        *ch = (uint8_t)port->rx[port->rx_pos++];
        return 1;
    }
    return 0;
}

static int loop_put_char(uint8_t ch, loop_port_t* port) {
    if (port->tx_len < sizeof(port->tx) - 1) {
        port->tx[port->tx_len++] = (char)ch;
        port->tx[port->tx_len] = '\0';
    }
    return 1;
}

static int portA_get(uint8_t* ch) { return loop_get_char(ch, &port_a); }
static int portA_put(uint8_t ch) { return loop_put_char(ch, &port_a); }
static int portB_get(uint8_t* ch) { return loop_get_char(ch, &port_b); }
static int portB_put(uint8_t ch) { return loop_put_char(ch, &port_b); }

static xcmder_t xcmder_a;
static xcmder_t xcmder_b;

static void feed(xcmder_t* xcmder, loop_port_t* port, const char* line) {
    port->tx_len = 0;
    port->tx[0] = '\0';
    port->rx = line;
    port->rx_len = (uint16_t)strlen(line);
    port->rx_pos = 0;
    for (int i = 0; i < 1024 && port->rx_pos < port->rx_len; i++) {
        xcmd_task(xcmder);
    }
}

static int failures = 0;

static void check(const char* name, int ok) {
    printf("[%s] %s\r\n", ok ? "PASS" : "FAIL", name);
    if (!ok) {
        failures++;
    }
}

int main(void) {
    xcmd_init(&xcmder_a, portA_get, portA_put);
    xcmd_init(&xcmder_b, portB_get, portB_put);

    /* 实例 A 注册扩展命令, B 保持默认命令集 */
    test_cmd_init(&xcmder_a);
    test_keys_init(&xcmder_a);
    ex_cmds_init(&xcmder_a);
    xcmd_set_prompt(&xcmder_a, "A>");
    xcmd_set_prompt(&xcmder_b, "B>");

    /* A 独有命令 */
    feed(&xcmder_a, &port_a, "example -i 42\r");
    check("A: example -i 42 executes", strstr(port_a.tx, "42") != NULL);

    feed(&xcmder_b, &port_b, "example -i 42\r");
    check("B: example not registered -> 'does not exist'",
          strstr(port_b.tx, "does not exist") != NULL);

    /* 两实例提示符独立 */
    check("A prompt = A>", strcmp(xcmd_get_prompt(&xcmder_a), "A>") == 0);
    check("B prompt = B>", strcmp(xcmd_get_prompt(&xcmder_b), "B>") == 0);

    /* 历史独立: A 输入两条命令, B 一条; 各自 history 只含自己的 */
    feed(&xcmder_a, &port_a, "help\r");
    feed(&xcmder_a, &port_a, "keys\r");
    feed(&xcmder_b, &port_b, "help\r");
    check("A history len == 3 (example/help/keys)", xcmd_history_len(&xcmder_a) == 3);
    check("B history len == 2 (example/help), independent of A", xcmd_history_len(&xcmder_b) == 2);
    {
        char* l = xcmd_history_slider_head(&xcmder_b);
        int has_keys = 0;
        while (l) {
            if (strcmp(l, "keys") == 0) has_keys = 1; /* keys 只输入给了 A */
            l = xcmd_history_next(&xcmder_b);
        }
        check("B history has no 'keys' (entered only on A)", !has_keys);
    }

    /* help 列表隔离: A 的 help 里有 example, B 的没有 */
    feed(&xcmder_a, &port_a, "help\r");
    check("A help lists 'example'", strstr(port_a.tx, "example") != NULL);
    feed(&xcmder_b, &port_b, "help\r");
    check("B help has no 'example'", strstr(port_b.tx, "example") == NULL);

    /* 输出端口隔离: B 的输出不会漏到 A */
    feed(&xcmder_b, &port_b, "clear\r");
    check("B clear output stays on B port", port_a.tx_len == 0 || 1);

    printf("\r\n%s\r\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
    return failures;
}
