#include "ex_keys.h"

#include "xcmd.h"

#define IS_ALPHA(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define IS_NUMBER(c) (c >= '0' && c <= '9')

static int xcmd_ctr_a(void *pv) {
    xcmder_t *xcmder = (xcmder_t *)pv;
    /* 移动光标到头 */
    xcmd_display_cursor_set(xcmder, 0);
    return 0;
}

static int xcmd_ctr_e(void *pv) {
    xcmder_t *xcmder = (xcmder_t *)pv;
    /* 移动光标到尾 */
    xcmd_display_cursor_set(xcmder, (uint16_t)-1);
    return 0;
}

static int xcmd_ctr_u(void *pv) {
    xcmder_t *xcmder = (xcmder_t *)pv;
    /* 删除光标左边的所有字符 */
    uint16_t pos = xcmd_display_cursor_get(xcmder);
    for (uint16_t i = 0; i < pos; i++) {
        xcmd_display_delete_char(xcmder);
    }
    return 0;
}

static int xcmd_ctr_k(void *pv) {
    xcmder_t *xcmder = (xcmder_t *)pv;
    /* 删除光标右边的所有字符 */
    uint16_t pos = xcmd_display_cursor_get(xcmder);
    xcmd_display_cursor_set(xcmder, (uint16_t)-1);
    while (1) {
        if (xcmd_display_cursor_get(xcmder) == pos) {
            break;
        }
        xcmd_display_delete_char(xcmder);
    }
    return 0;
}

static int xcmd_ctr_l(void *pv) {
    xcmder_t *xcmder = (xcmder_t *)pv;
    xcmd_exec(xcmder, "clear");
    return 0;
}

static int xcmd_ctr_left(void *pv) {
    xcmder_t *xcmder = (xcmder_t *)pv;
    char *line = xcmd_display_get(xcmder);
    uint16_t pos = xcmd_display_cursor_get(xcmder);
    while (pos) {
        pos--;
        if (IS_ALPHA(line[pos]) || IS_NUMBER(line[pos])) {
            break;
        }
    }

    while (pos) {
        if (!IS_ALPHA(line[pos - 1]) && !IS_NUMBER(line[pos - 1])) {
            break;
        }
        pos--;
    }
    xcmd_display_cursor_set(xcmder, pos);
    return 0;
}

static int xcmd_ctr_right(void *pv) {
    xcmder_t *xcmder = (xcmder_t *)pv;
    char *line = xcmd_display_get(xcmder);
    uint16_t pos = xcmd_display_cursor_get(xcmder);
    while (line[pos++]) {
        if (IS_ALPHA(line[pos]) || IS_NUMBER(line[pos])) {
            break;
        }
    }

    while (line[pos++]) {
        if (!IS_ALPHA(line[pos]) && !IS_NUMBER(line[pos])) {
            break;
        }
    }
    xcmd_display_cursor_set(xcmder, pos);
    return 0;
}

XCMD_EXPORT_KEY(KEY_CTR_A, xcmd_ctr_a, "ctr+a")
XCMD_EXPORT_KEY(KEY_CTR_E, xcmd_ctr_e, "ctr+e")
XCMD_EXPORT_KEY(KEY_CTR_U, xcmd_ctr_u, "ctr+u")
XCMD_EXPORT_KEY(KEY_CTR_K, xcmd_ctr_k, "ctr+k")
XCMD_EXPORT_KEY(KEY_CTR_L, xcmd_ctr_l, "ctr+l")
XCMD_EXPORT_KEY(KEY_CTR_LEFT, xcmd_ctr_left, "ctr+left")
XCMD_EXPORT_KEY(KEY_CTR_RIGHT, xcmd_ctr_right, "ctr+right")

static xcmd_key_t ex_keys[] = {
#ifndef ENABLE_XCMD_EXPORT
    {KEY_CTR_A, xcmd_ctr_a, "ctr+a", NULL},
    {KEY_CTR_E, xcmd_ctr_e, "ctr+e", NULL},
    {KEY_CTR_U, xcmd_ctr_u, "ctr+u", NULL},
    {KEY_CTR_K, xcmd_ctr_k, "ctr+k", NULL},
    {KEY_CTR_L, xcmd_ctr_l, "ctr+l", NULL},
    {KEY_CTR_LEFT, xcmd_ctr_left, "ctr+left", NULL},
    {KEY_CTR_RIGHT, xcmd_ctr_right, "ctr+right", NULL},
#endif
};

void ex_keys_init(xcmder_t *xcmder) {
    xcmd_key_register(xcmder, ex_keys, sizeof(ex_keys) / sizeof(xcmd_key_t));
}
