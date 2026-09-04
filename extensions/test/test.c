/*
 * @Author: your name
 * @Date: 2021-09-22 22:33:17
 * @LastEditTime: 2021-10-11 13:41:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /xcmd/extensions/test/test.c
 */
#include "test.h"

#include <stdlib.h>
#include <string.h>

#include "xcmd.h"

#define EXIT_MESSAGE(xcmder) xcmd_print(xcmder, "press \"q\" or \"Q\" to exit!\r\n")

static uint8_t param_check(xcmder_t* xcmder, int need, int argc, char* argv[]) {
    uint8_t i, ret = 0;
    if (need < (argc)) {
        ret = 1;
    } else {
        xcmd_print(xcmder, "err need %d but input %d:\r\n", need, argc - 1);
        xcmd_print(xcmder, "input= ");
        for (i = 0; i < argc; i++) {
            if (argv[i] != NULL) {
                xcmd_print(xcmder, "%s ", argv[i]);
            }
        }
        xcmd_print(xcmder, "\r\n");
        ret = 0;
    }
    return ret;
}

static int cmd_example(xcmder_t* xcmder, int argc, char* argv[]) {
    uint8_t i;
    if (param_check(xcmder, 1, argc, argv)) {
        if (strcmp(argv[1], "-s") == 0) {
            for (i = 2; i < argc; i++) {
                xcmd_print(xcmder, "%s\r\n", argv[i]);
            }
        }
        if (strcmp(argv[1], "-i") == 0) {
            for (i = 2; i < argc; i++) {
                xcmd_print(xcmder, "%d\r\n", atoi(argv[i]));
            }
        }
        if (strcmp(argv[1], "-f") == 0) {
            for (i = 2; i < argc; i++) {
                xcmd_print(xcmder, "%f\r\n", atof(argv[i]));
            }
        }
    }
    return 0;
}

static int cmd_history(xcmder_t* xcmder, int argc, char* argv[]) {
    char *line = xcmd_history_slider_head(xcmder);
    (void)argc;
    (void)argv;

    while (line) {
            xcmd_print(xcmder, "%s\r\n", line);
        line = xcmd_history_next(xcmder);
    }
    return 0;
}

static int cmd_delete_cmd(xcmder_t* xcmder, int argc, char* argv[]) {
    int res = 0;
    if (argc == 2) {
        res = xcmd_unregister_cmd(xcmder, argv[1]);
        if (res) {
            goto error;
        }
    }
    return 0;
error:
    xcmd_print(xcmder, "Too many parameters are entered or there is no command\r\n");
    return -1;
}

static int cmd_delete_key(xcmder_t* xcmder, int argc, char* argv[]) {
    int res = 0;
    if (argc == 2) {
        res = xcmd_unregister_key(xcmder, argv[1]);
        if (res) {
            goto error;
        }
    }
    return 0;
error:
    xcmd_print(xcmder, "Too many parameters are entered or there is no command\r\n");
    return -1;
}

static int cmd_ctr_q(void* pv) {
    xcmder_t* xcmder = (xcmder_t*)pv;
    xcmd_print(xcmder, "this is ctr+q\n");
    return 0;
}

static int cmd_print_color(xcmder_t* xcmder, int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    xcmd_print(xcmder, TX_DEF "txt_color = DEF    \r\n" TX_DEF);
    xcmd_print(xcmder, TX_RED "txt_color = RED    \r\n" TX_DEF);
    xcmd_print(xcmder, TX_BLACK "txt_color = BLACK  \r\n" TX_DEF);
    xcmd_print(xcmder, TX_GREEN "txt_color = GREEN  \r\n" TX_DEF);
    xcmd_print(xcmder, TX_YELLOW "txt_color = YELLOW \r\n" TX_DEF);
    xcmd_print(xcmder, TX_BLUE "txt_color = BLUE   \r\n" TX_DEF);
    xcmd_print(xcmder, TX_WHITE "txt_color = WHITE  \r\n" TX_DEF);
    xcmd_print(xcmder, TX_WHITE "txt_color = WHITE  \r\n" TX_DEF);

    xcmd_print(xcmder, BK_DEF "background_color = BK_DEF" BK_DEF "\r\n");
    xcmd_print(xcmder, BK_BLACK "background_color = BK_BLACK" BK_DEF "\r\n");
    xcmd_print(xcmder, BK_RED "background_color = BK_RED" BK_DEF "\r\n");
    xcmd_print(xcmder, BK_GREEN "background_color = BK_GREEN" BK_DEF "\r\n");
    xcmd_print(xcmder, BK_YELLOW "background_color = BK_YELLOW" BK_DEF "\r\n");
    xcmd_print(xcmder, BK_BLUE "background_color = BK_BLUE" BK_DEF "\r\n");
    xcmd_print(xcmder, BK_WHITE "background_color = BK_WHITE" BK_DEF "\r\n");
    return 0;
}

XCMD_EXPORT_CMD(history, cmd_history, "show history list")
XCMD_EXPORT_CMD(example, cmd_example, "example [-f|-i|-s] [val]")
XCMD_EXPORT_CMD(color, cmd_print_color, "printf color text")

static xcmd_t cmds[] = {
#ifndef ENABLE_XCMD_EXPORT
    {"history", cmd_history, "show history list", NULL},
    {"example", cmd_example, "example [-f|-i|-s] [val]", NULL},
    {"delcmd", cmd_delete_cmd, "delete cmd [val]", NULL},
    {"delkey", cmd_delete_key, "delete key [val]", NULL},
    {"color", cmd_print_color, "printf color text", NULL},
#endif
};

static xcmd_key_t keys[] = {
#ifndef ENABLE_XCMD_EXPORT
    {KEY_CTR_Q, cmd_ctr_q, "ctr+q", NULL},
#endif
};

void test_cmd_init(xcmder_t* xcmder) {
    xcmd_cmd_register(xcmder, cmds, sizeof(cmds) / sizeof(xcmd_t));
}

void test_keys_init(xcmder_t* xcmder) {
    xcmd_key_register(xcmder, keys, sizeof(keys) / sizeof(xcmd_key_t));
}
