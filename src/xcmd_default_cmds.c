#include "xcmd_default_cmds.h"

#include <stdlib.h>

#include "xcmd.h"
#include "xcmd_default_confg.h"

static int cmd_clear(xcmder_t* xcmder, int argc, char* argv[]) {
    xcmd_print(xcmder, "\033[2J");           /* 清屏 */
    xcmd_print(xcmder, "\033[H");            /* 光标回到屏幕左上角 */
    xcmd_display_clear(xcmder);              /* 清空输入行缓存并重绘提示符 */
    xcmd_display_redraw_set(xcmder);         /* 已自行重绘命令行, 框架收尾不再补 \r\n+提示符 */
    return 0;
}

static int cmd_help(xcmder_t* xcmder, int argc, char* argv[]) {
    xcmd_t* p = NULL;
    XCMD_CMD_FOR_EACH(xcmder, p) {
        xcmd_print(xcmder, "%-20s %s\r\n", p->name, p->help);
    }
    return 0;
}

static int cmd_keys(xcmder_t* xcmder, int argc, char* argv[]) {
    xcmd_key_t* p = NULL;
    XCMD_KEY_FOR_EACH(xcmder, p) {
        xcmd_print(xcmder, "0x%08X\t", (uint32_t)p->key);
        xcmd_print(xcmder, "%s\r\n", p->help);
    }
    return 0;
}

static int cmd_logo(xcmder_t* xcmder, int argc, char* argv[]) {
    char* log =
        "\r\n\
 _  _  ___  __  __  ____  \r\n\
( \\/ )/ __)(  \\/  )(  _ \\ \r\n\
 )  (( (__  )    (  )(_) )\r\n\
(_/\\_)\\___)(_/\\/\\_)(____/\r\n ";
    xcmd_print(xcmder, "%s", log);
    xcmd_print(xcmder, "\r\n%-10s %s %s\r\n", "Build", __DATE__, __TIME__);
    xcmd_print(xcmder, "%-10s %s\r\n", "Version", VERSION);
    return 0;
}

XCMD_EXPORT_CMD(clear, cmd_clear, "clear screen")
XCMD_EXPORT_CMD(help, cmd_help, "show this list")
XCMD_EXPORT_CMD(keys, cmd_keys, "show keys")
XCMD_EXPORT_CMD(logo, cmd_logo, "show logo")

static xcmd_t cmds[] = {
#ifndef ENABLE_XCMD_EXPORT
    {"clear", cmd_clear, "clear screen", NULL},
    {"help", cmd_help, "show this list", NULL},
    {"keys", cmd_keys, "show keys", NULL},
    {"logo", cmd_logo, "show logo", NULL},
#endif
};

void default_cmds_init(xcmder_t* xcmder) {
    xcmd_cmd_register(xcmder, cmds, sizeof(cmds) / sizeof(xcmd_t));
}
