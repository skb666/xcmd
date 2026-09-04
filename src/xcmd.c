#include "xcmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xcmd_default_cmds.h"
#include "xcmd_default_keys.h"
#include "xcmd_file.h"
#include "xcmd_obj.h"

#define CMD_IS_END_KEY(c) (((c >= 'A') && (c <= 'D')) || ((c >= 'P') && (c <= 'S')) || \
                           (c == '~') || (c == 'H') || (c == 'F'))

#define CMD_IS_PRINT(c) ((c >= 32) && (c <= 126))

/* 恒失败的缺省实现; 需要重定向的平台 (如 fs_cmds) 覆盖强符号 (原型见 xcmd_file.h) */
__attribute__((weak)) size_t file_open(char* name, int is_write, int is_append) {
    (void)name;
    (void)is_write;
    (void)is_append;
    return (size_t)(-1);
}

__attribute__((weak)) void file_close(size_t fd) {
    (void)fd;
}

__attribute__((weak)) int file_write(size_t fd, const char* str) {
    (void)fd;
    (void)str;
    return -1;
}

__attribute__((weak)) int file_read(size_t fd, char* buf, int buflen) {
    (void)fd;
    (void)buf;
    (void)buflen;
    return -1;
}

/* 打印提示符 (含颜色), 供框架各重绘路径及 xcmd_enter 使用 */
void xcmd_display_prompt_print(xcmder_t* xcmder) {
#ifndef XCMD_DEFAULT_PROMPT_CLOLR
    xcmd_print(xcmder, "%s", xcmder->parser.prompt);
#else
    xcmd_print(xcmder, XCMD_DEFAULT_PROMPT_CLOLR "%s" TX_DEF, xcmder->parser.prompt);
#endif
}

static uint8_t xcmd_display_redraw_get(xcmder_t* xcmder); /* 查询重绘标志, 供 xcmd_cmd_match 收尾判断 */

static char* xcmd_strpbrk(char* s, const char* delim)  // 返回s1中第一个满足条件的字符的指针, 并且保留""号内的源格式
{
    uint8_t flag = 0;
    for (uint16_t i = 0; s[i]; i++) {
        for (uint16_t j = 0; delim[j]; j++) {
            if (0 == flag) {
                if (s[i] == '\"') {
                    for (uint16_t k = i; s[k]; k++) {
                        s[k] = s[k + 1];
                    }
                    flag = 1;
                    continue;
                }
            }

            if (flag) {
                if (s[i] == '\"') {
                    for (uint16_t k = i; s[k]; k++) {
                        s[k] = s[k + 1];
                    }
                    flag = 0;
                } else {
                    continue;
                }
            }

            if (s[i] == delim[j]) {
                return &s[i];
            }
        }
    }
    return NULL;
}

/* 字符串切割函数，使用:
        char s[] = "-abc-=-def";
        char *sp;
        x = strtok_r(s, "-", &sp);      // x = "abc", sp = "=-def"
        x = strtok_r(NULL, "-=", &sp);  // x = "def", sp = NULL
        x = strtok_r(NULL, "=", &sp);   // x = NULL
                // s = "abc/0-def/0"
*/
static char* xcmd_strtok(char* s, const char* delim, char** save_ptr) {
    char* token;

    if (s == NULL) s = *save_ptr;

    /* Scan leading delimiters.  */
    s += strspn(s, delim);  // 返回字符串中第一个不在指定字符串中出现的字符下标，去掉了以空字符开头的
                            // 字串的情况
    if (*s == '\0')
        return NULL;

    /* Find the end of the token.  */
    token = s;
    s = xcmd_strpbrk(token, delim);  //   返回s1中第一个满足条件的字符的指针
    if (s == NULL)
        /* This token finishes the string.  */
        *save_ptr = strchr(token, '\0');
    else {
        /* Terminate the token and make *SAVE_PTR point past it.  */
        *s = '\0';
        *save_ptr = s + 1;
    }
    return token;
}

static int xcmd_get_param(char* msg, char* delim, char* get[], int max_num) {
    int i, ret;
    char* ptr = NULL;
    char* sp = NULL;
    ptr = xcmd_strtok(msg, delim, &sp);
    for (i = 0; ptr != NULL && i < max_num; i++) {
        get[i] = ptr;
        ptr = xcmd_strtok(NULL, delim, &sp);
    }
    ret = i;
    return ret;
}

static int xcmd_redirected(xcmder_t* xcmder, int argc, char* argv[]) {
    for (int i = 1; i < (argc - 1); i++) {
        if (argv[i][0] == '>') {
            argc = i;
            if (argv[i][1] == '>') {
                xcmder->io.write_fd = file_open(argv[i + 1], 1, 1);
            } else {
                xcmder->io.write_fd = file_open(argv[i + 1], 1, 0);
            }
            break;
        }
    }
    return argc;
}

static int xcmd_cmd_match(xcmder_t* xcmder, int argc, char* argv[]) {
    uint8_t flag = 0;
    int ret = -1;
    xcmd_t* p = NULL;
    XCMD_CMD_FOR_EACH(xcmder, p) {
        if (strcmp(p->name, argv[0]) == 0) {
            flag = 1;
            if (argc > 1) {
                if ((strcmp(argv[1], "?") == 0) ||
                    (strcmp(argv[1], "-h") == 0)) {
                    xcmd_print(xcmder, "%s\r\n", p->help);
                    break;
                }
            }
            argc = xcmd_redirected(xcmder, argc, argv);
#ifdef XCMD_END_LINE_MODE
            xcmder->cmd_executing = 1; /* 命令执行期间的日志走原始输出, 不触碰命令行重绘 (letter-shell isActive) */
#endif
            ret = p->func(xcmder, argc, argv);
#ifdef XCMD_END_LINE_MODE
            xcmder->cmd_executing = 0;
#endif
            file_close(xcmder->io.write_fd);
            xcmder->io.write_fd = (size_t)(-1);
            break;
        }
    }
    if (flag) {
        if (!xcmd_display_redraw_get(xcmder)) {
            xcmd_print(xcmder, "\r\n");
        }
    } else {
        xcmd_print(xcmder, "cmd \"%s\" does not exist\r\n", argv[0]);
    }
    return ret;
}

static void xcmd_key_match(xcmder_t* xcmder, char* key) {
    xcmd_key_t* p = NULL;
    XCMD_KEY_FOR_EACH(xcmder, p) {
        if (strcmp(key, p->key) == 0) {
            if (p->func(xcmder) == 0) {
                break;
            }
        }
    }
}

static void xcmd_key_exec(xcmder_t* xcmder, char* key) {
    xcmd_key_match(xcmder, key);
    /* 按键处理完消耗重绘标志: Ctrl+L 等按键路径重绘过命令行且不在回车流程内,
     * 必须在此了结标志, 否则泄漏到下一次回车, 会误跳过那条命令的补行+提示符 */
    (void)xcmd_display_redraw_take(xcmder);
}

static uint8_t xcmd_rcv_encode(xcmder_t* xcmder, uint8_t byte) {
    uint8_t ret = 0;

    switch (xcmder->parser.encode_case_stu) {
        case 0:
            xcmder->parser.encode_count = 0;
            if (byte == 0x1B)  // ESC
            {
                xcmder->parser.encode_buf[xcmder->parser.encode_count++] = byte;
                xcmder->parser.encode_case_stu = 1;
                xcmder->parser.key_val = byte;
            } else {
                xcmder->parser.encode_buf[xcmder->parser.encode_count++] = byte;
                xcmder->parser.encode_buf[xcmder->parser.encode_count] = '\0';
                xcmder->parser.encode_count = 0;
                ret = 1;
            }
            break;
        case 1:
            if (CMD_IS_END_KEY(byte)) {
                xcmder->parser.encode_buf[xcmder->parser.encode_count++] = byte;
                xcmder->parser.encode_buf[xcmder->parser.encode_count] = '\0';
                ret = xcmder->parser.encode_count;
                xcmder->parser.encode_case_stu = 0;
            } else {
                xcmder->parser.encode_buf[xcmder->parser.encode_count++] = byte;
                if (xcmder->parser.encode_count >= 6) {
                    xcmder->parser.encode_case_stu = 0;
                    ret = 0;
                }
            }
            break;
        default:
            break;
    }
    return ret;
}

char* xcmd_end_of_input(xcmder_t* xcmder) {
    char* ret = xcmder->parser.display_line;
    if (xcmder->parser.byte_num) {
#if XCMD_HISTORY_MAX_NUM
        if (xcmder->parser.history_list.head == NULL) {
            xcmd_history_insert(xcmder, ret);
        } else {
            char* tail_line = xcmder->parser.history_list.tail->line;
            if (strcmp(tail_line, ret) != 0) {
                xcmd_history_insert(xcmder, ret);
            }
        }
#endif
        xcmder->parser.byte_num = 0;
        xcmder->parser.cursor = 0;
        xcmd_history_slider_reset(xcmder);
    }
    return ret;
}

static void xcmd_parser(xcmder_t* xcmder, uint8_t byte) {
    uint32_t num = 0;

    num = xcmd_rcv_encode(xcmder, byte);

    if (num > 0) {
        if (!(xcmder->parser.recv_hook_func && !xcmder->parser.recv_hook_func(xcmder->parser.encode_buf))) {
            if (CMD_IS_PRINT(xcmder->parser.encode_buf[0])) {
                xcmd_display_insert_char(xcmder, xcmder->parser.encode_buf[0]);
                return;
            } else {
                xcmd_key_exec(xcmder, xcmder->parser.encode_buf);
            }
        }
    }
}

/**
 * @description: 按精确长度输出字节, 不依赖字符串 NUL 结尾 (供尾行模式使用,
 *              避免读到日志缓冲区的历史残留)
 */
#ifdef XCMD_END_LINE_MODE
static void xcmd_put_buf(xcmder_t* xcmder, const char* buf, uint16_t len) {
    if (xcmder->io.write_fd != (size_t)(-1)) {
        /* 重定向时按长度输出 */
        if (len > 0) {
            char tmp[XCMD_LINE_MAX_LENGTH + 1];
            uint16_t n = (len > XCMD_LINE_MAX_LENGTH) ? XCMD_LINE_MAX_LENGTH : len;
            memcpy(tmp, buf, n);
            tmp[n] = '\0';
            if (file_write(xcmder->io.write_fd, tmp) != -1) {
                return;
            }
        }
    }
    for (uint16_t i = 0; i < len; i++) {
        xcmder->io.put_c((uint8_t)buf[i]);
    };
}
#endif /* XCMD_END_LINE_MODE */

void xcmd_put_str(xcmder_t* xcmder, const char* str) {
    if (xcmder->io.write_fd != (size_t)(-1)) {
        if (file_write(xcmder->io.write_fd, str) != -1) {
            return;
        }
    }
    for (uint16_t i = 0; str[i]; i++) {
        xcmder->io.put_c(str[i]);
    };
}

void xcmd_print(xcmder_t* xcmder, const char* fmt, ...) {
    char ucstring[XCMD_PRINT_BUF_MAX_LENGTH] = {0};
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(ucstring, XCMD_PRINT_BUF_MAX_LENGTH, fmt, arg);
    va_end(arg);
    xcmd_put_str(xcmder, ucstring);
}

#ifdef XCMD_END_LINE_MODE
/**
 * @description: 尾行模式输出, 语义对齐 letter-shell 的 shellWriteEndLine:
 * - 先 \x1b[2K\r 擦除当前行并回到行首, 日志从干净行首开始打
 * - 日志本身以 \r\n 结尾, 打完后光标落在下一行(命令行的新位置)
 * - xcmd 空闲 (非命令执行中) 时重绘提示符+已输入内容, 并把光标放回输入位置;
 *   命令执行期间不动命令行 (执行完后 xcmd 会重新提示)
 *
 * 日志体按 len 精确输出, 不依赖字符串 NUL 结尾——elog 的 log_buf 是复用
 * 缓冲区, 较短的日志之后会残留上一条较长的尾部, 用 strlen 会把这些残留
 * 一并打印出来 (表现为日志末尾重叠粘贴)。由调用方 (elog_port) 传入精确 len。
 */
void xcmd_put_str_end_line_len(xcmder_t* xcmder, const char* str, uint16_t len) {
    if (!xcmder || !xcmder->_initOK || !xcmder->io.put_c || !str) {
        return;
    }
    /* 1. 擦除当前光标的整行并回行首 */
    xcmd_print(xcmder, "\x1b[2K\r");
    /* 2. 输出日志内容 (精确 len 字节) */
    xcmd_put_buf(xcmder, str, len);
    /* 3. 空闲时重绘命令行 (正在执行命令时不渲染) */
    if (!xcmder->cmd_executing) {
        xcmd_display_prompt_print(xcmder);
        if (xcmder->parser.byte_num > 0) {
            xcmd_print(xcmder, "%s", xcmder->parser.display_line);
        }
        /* 光标绝对定位到输入位置: 用 CHA(列号) 精确放置, 避免相对退格
         * (\x1b[nD) 在 prompt/颜色码存在时产生列偏移. 列号 = 提示符可见
         * 宽度 + 当前输入光标位置 (1 基) */
        xcmd_print(xcmder, CHA(strlen(xcmder->parser.prompt) + xcmder->parser.cursor + 1));
    }
}

void xcmd_put_str_end_line(xcmder_t* xcmder, const char* str) {
    xcmd_put_str_end_line_len(xcmder, str, (str) ? (uint16_t)strlen(str) : 0);
}

/**
 * @description: 查询实例是否已初始化完毕 (含 get_c/put_c 桥接已就绪)
 * @return {uint8_t} 1=已就绪; 0=未初始化
 */
uint8_t xcmd_ready(xcmder_t* xcmder) {
    return (xcmder && xcmder->_initOK && xcmder->io.put_c) ? 1 : 0;
}

void xcmd_print_end_line(xcmder_t* xcmder, const char* fmt, ...) {
    char ucstring[XCMD_PRINT_BUF_MAX_LENGTH] = {0};
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(ucstring, XCMD_PRINT_BUF_MAX_LENGTH, fmt, arg);
    va_end(arg);
    xcmd_put_str_end_line(xcmder, ucstring);
}
#endif /* XCMD_END_LINE_MODE */

void xcmd_display_write(xcmder_t* xcmder, const char* buf, uint16_t len) {
    if (len > XCMD_LINE_MAX_LENGTH) {
        len = XCMD_LINE_MAX_LENGTH;
    }
    for (uint16_t i = 0; i < len; i++) {
        xcmd_display_insert_char(xcmder, buf[i]);
    }
}

void xcmd_display_print(xcmder_t* xcmder, const char* fmt, ...) {
    char ucstring[XCMD_PRINT_BUF_MAX_LENGTH] = {0};
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(ucstring, XCMD_PRINT_BUF_MAX_LENGTH, fmt, arg);
    va_end(arg);
    xcmd_display_write(xcmder, ucstring, strlen(ucstring));
}

char* xcmd_display_get(xcmder_t* xcmder) {
    char* line = xcmder->parser.display_line;
    return line;
}

void xcmd_display_clear(xcmder_t* xcmder) {
    char* line = xcmd_display_get(xcmder);
    xcmd_print(xcmder, DL(1));
    xcmd_display_prompt_print(xcmder);
    xcmder->parser.byte_num = 0;
    xcmder->parser.cursor = 0;
    line[0] = '\0';
}

/* 供外部 (或 shell 就绪处) 重绘命令行: 擦当前行并打印提示符 */
void xcmd_display_redraw(xcmder_t* xcmder) {
    xcmd_print(xcmder, EL_CURRENT_LINE "\r");
    xcmd_display_prompt_print(xcmder);
}

void xcmd_display_redraw_set(xcmder_t* xcmder) {
    xcmder->parser.redraw_pending = 1;
}

uint8_t xcmd_display_redraw_take(xcmder_t* xcmder) {
    uint8_t pending = xcmder->parser.redraw_pending;
    xcmder->parser.redraw_pending = 0;
    return pending;
}

static uint8_t xcmd_display_redraw_get(xcmder_t* xcmder) {
    return xcmder->parser.redraw_pending;
}

void xcmd_display_insert_char(xcmder_t* xcmder, char c) {
    char* line = xcmd_display_get(xcmder);
    if (xcmder->parser.byte_num < XCMD_LINE_MAX_LENGTH - 1) {
        for (uint16_t i = xcmder->parser.byte_num; i > xcmder->parser.cursor; i--) {
            line[i] = line[i - 1];
        }
        xcmder->parser.byte_num++;
        line[xcmder->parser.byte_num] = '\0';
        line[xcmder->parser.cursor++] = c;
        xcmd_print(xcmder, ICH(1));
        xcmd_print(xcmder, "%c", c);
    }
}

void xcmd_display_delete_char(xcmder_t* xcmder) {
    char* line = xcmd_display_get(xcmder);
    if (xcmder->parser.cursor > 0) {
        for (uint16_t i = xcmder->parser.cursor - 1; i < xcmder->parser.byte_num - 1; i++) {
            line[i] = line[i + 1];
        }
        xcmder->parser.byte_num--;
        xcmder->parser.cursor--;
        line[xcmder->parser.byte_num] = '\0';
        xcmd_print(xcmder, CUB(1));
        xcmd_print(xcmder, DCH(1));
    }
}

uint8_t xcmd_display_current_char(xcmder_t* xcmder, char* cha) {
    if (xcmder->parser.cursor < xcmder->parser.byte_num) {
        char* line = xcmd_display_get(xcmder);
        *cha = line[xcmder->parser.cursor];
        return 1;
    }
    return 0;
}

void xcmd_display_cursor_set(xcmder_t* xcmder, uint16_t pos) {
    if (pos > xcmder->parser.byte_num) {
        pos = xcmder->parser.byte_num;
    }
    xcmder->parser.cursor = pos;
    xcmd_print(xcmder, CHA(xcmder->parser.cursor + strlen(xcmder->parser.prompt) + 1));
}

uint16_t xcmd_display_cursor_get(xcmder_t* xcmder) {
    return xcmder->parser.cursor;
}

void xcmd_history_insert(xcmder_t* xcmder, char* str) {
#if XCMD_HISTORY_MAX_NUM
    if (xcmder->parser.history_list.len < XCMD_HISTORY_MAX_NUM) {
        xcmd_history_t* new_p = &(xcmder->parser.history_pool.pool[xcmder->parser.history_pool.index++]);
        if (xcmder->parser.history_list.len == 0) /* 首个节点自环 */
        {
            strncpy(new_p->line, str, XCMD_LINE_MAX_LENGTH);
            new_p->line[XCMD_LINE_MAX_LENGTH] = '\0';
            new_p->prev = new_p;
            new_p->next = new_p;
            xcmder->parser.history_list.head = new_p;
            xcmder->parser.history_list.tail = new_p;
            xcmder->parser.history_list.slider = NULL;
            xcmder->parser.history_list.len++;
        } else {
            strncpy(new_p->line, str, XCMD_LINE_MAX_LENGTH);
            new_p->line[XCMD_LINE_MAX_LENGTH] = '\0';
            new_p->prev = xcmder->parser.history_list.tail;
            new_p->next = xcmder->parser.history_list.head;
            xcmder->parser.history_list.head->prev = new_p;
            xcmder->parser.history_list.tail->next = new_p;
            xcmder->parser.history_list.tail = new_p;
            xcmder->parser.history_list.slider = NULL;
            xcmder->parser.history_list.len++;
        }
    } else {
        /* 池满: 覆写最旧节点(head)并将其旋转为最新(tail), head 后移 */
        strncpy(xcmder->parser.history_list.head->line, str, XCMD_LINE_MAX_LENGTH);
        xcmder->parser.history_list.head->line[XCMD_LINE_MAX_LENGTH] = '\0';
        xcmder->parser.history_list.tail = xcmder->parser.history_list.head;
        xcmder->parser.history_list.head = xcmder->parser.history_list.head->next;
        xcmder->parser.history_list.slider = NULL;
    }
#endif
}

char* xcmd_history_next(xcmder_t* xcmder) {
    char* line = NULL;
#if XCMD_HISTORY_MAX_NUM
    if (xcmder->parser.history_list.len) {
        /* 向更新的方向移动, 移动后返回; 到达最新(tail)则结束浏览 */
        if (xcmder->parser.history_list.slider) {
            if (xcmder->parser.history_list.slider != xcmder->parser.history_list.tail) {
                xcmder->parser.history_list.slider = xcmder->parser.history_list.slider->next;
                line = xcmder->parser.history_list.slider->line;
            } else {
                xcmder->parser.history_list.slider = NULL;
            }
        }
    }
#endif
    return line;
}

char* xcmd_history_prev(xcmder_t* xcmder) {
    char* line = NULL;
#if XCMD_HISTORY_MAX_NUM
    if (xcmder->parser.history_list.len) {
        /* 向更旧的方向移动, 移动后返回; 未浏览时从最新(tail)开始, 到最旧(head)后停住 */
        if (!xcmder->parser.history_list.slider) {
            xcmder->parser.history_list.slider = xcmder->parser.history_list.tail;
        } else if (xcmder->parser.history_list.slider != xcmder->parser.history_list.head) {
            xcmder->parser.history_list.slider = xcmder->parser.history_list.slider->prev;
        }
        line = xcmder->parser.history_list.slider->line;
    }
#endif
    return line;
}

char* xcmd_history_current(xcmder_t* xcmder) {
    char* line = NULL;
#if XCMD_HISTORY_MAX_NUM
    if (xcmder->parser.history_list.len) {
        if (xcmder->parser.history_list.slider) {
            line = xcmder->parser.history_list.slider->line;
        }
    }
#endif
    return line;
}

uint16_t xcmd_history_len(xcmder_t* xcmder) {
#if XCMD_HISTORY_MAX_NUM
    return xcmder->parser.history_list.len;
#else
    (void)xcmder;
    return 0;
#endif
}

char* xcmd_history_slider_head(xcmder_t* xcmder) {
    char* line = NULL;
#if XCMD_HISTORY_MAX_NUM
    if (xcmder->parser.history_list.len) {
        xcmder->parser.history_list.slider = xcmder->parser.history_list.head;
        line = xcmder->parser.history_list.slider->line;
    }
#endif
    return line;
}

void xcmd_history_slider_reset(xcmder_t* xcmder) {
#if XCMD_HISTORY_MAX_NUM
    xcmder->parser.history_list.slider = NULL;
#endif
}

int xcmd_exec(xcmder_t* xcmder, char* str) {
    int param_num = 0;
    char* cmd_param_buff[XCMD_PARAM_MAX_NUM];
    char temp[XCMD_LINE_MAX_LENGTH + 1];
    strncpy(temp, str, XCMD_LINE_MAX_LENGTH);
    temp[XCMD_LINE_MAX_LENGTH] = '\0'; /* str 达到上限长度时 strncpy 不保证终止 */
    param_num = xcmd_get_param(temp, " ", cmd_param_buff, XCMD_PARAM_MAX_NUM);
    if (param_num > 0) {
        return xcmd_cmd_match(xcmder, param_num, cmd_param_buff);
    }
    return -1;
}

int xcmd_key_register(xcmder_t* xcmder, xcmd_key_t* keys, uint16_t number) {
#ifndef ENABLE_XCMD_EXPORT
    uint16_t i = 0;
    xcmd_key_t* temp;

    while (i < number) {
        temp = xcmder->key_list.head.next;
        xcmder->key_list.head.next = &keys[i];
        keys[i].next = temp;
        ++i;
    }
#endif
    return 0;
}

int xcmd_cmd_register(xcmder_t* xcmder, xcmd_t* cmds, uint16_t number) {
#ifndef ENABLE_XCMD_EXPORT
    xcmd_t* temp;
    uint16_t i = 0;
    while (i < number) {
        temp = xcmder->cmd_list.head.next;
        xcmder->cmd_list.head.next = &cmds[i];
        cmds[i].next = temp;
        ++i;
    }
#endif
    return 0;
}

xcmd_key_t* xcmd_keylist_get(xcmder_t* xcmder) {
#ifndef ENABLE_XCMD_EXPORT
    return xcmder->key_list.head.next;
#else
    (void)xcmder;
    return NULL;
#endif
}

xcmd_t* xcmd_cmdlist_get(xcmder_t* xcmder) {
#ifndef ENABLE_XCMD_EXPORT
    return xcmder->cmd_list.head.next;
#else
    (void)xcmder;
    return NULL;
#endif
}

int xcmd_unregister_cmd(xcmder_t* xcmder, char* cmd) {
#ifndef ENABLE_XCMD_EXPORT
    xcmd_t* bk = &xcmder->cmd_list.head;
    xcmd_t* p = NULL;
    XCMD_CMD_FOR_EACH(xcmder, p) {
        if (strcmp(cmd, p->name) == 0) {
            bk->next = p->next;
            return 0;
        }
        bk = p;
    }
#else
    (void)xcmder;
    (void)cmd;
#endif
    return -1;
}

int xcmd_unregister_key(xcmder_t* xcmder, char* key) {
#ifndef ENABLE_XCMD_EXPORT
    xcmd_key_t* bk = &xcmder->key_list.head;
    xcmd_key_t* p = NULL;
    XCMD_KEY_FOR_EACH(xcmder, p) {
        if (strcmp(key, p->key) == 0) {
            bk->next = p->next;
            return 0;
        }
        bk = p;
    }
#else
    (void)xcmder;
    (void)key;
#endif
    return -1;
}

void xcmd_set_prompt(xcmder_t* xcmder, const char* prompt) {
    if (prompt) {
        xcmder->parser.prompt = prompt;
    }
}

const char* xcmd_get_prompt(xcmder_t* xcmder) {
    return xcmder->parser.prompt;
}

void xcmd_register_rcv_hook_func(xcmder_t* xcmder, uint8_t (*func_p)(char*)) {
    xcmder->parser.recv_hook_func = func_p;
}

void xcmd_init(xcmder_t* xcmder, int (*get_c)(uint8_t*), int (*put_c)(uint8_t)) {
    if (xcmder && get_c && put_c) {
        xcmder->io.get_c = get_c;
        xcmder->io.put_c = put_c;
        xcmder->io.write_fd = (size_t)(-1);

        xcmder->parser.prompt = XCMD_DEFAULT_PROMPT;
        xcmder->parser.byte_num = 0;
        xcmder->parser.cursor = 0;
        xcmder->parser.encode_case_stu = 0;
        xcmder->parser.redraw_pending = 0;
#ifndef ENABLE_XCMD_EXPORT
        xcmder->cmd_list.head.next = NULL;
        xcmder->key_list.head.next = NULL;
#endif

        if (xcmder->_initOK == 0) {
            default_cmds_init(xcmder);
            default_keys_init(xcmder);
            xcmd_exec(xcmder, "logo");
            xcmd_unregister_cmd(xcmder, "logo");
        }
        xcmder->_initOK = 1;
    }
}

void xcmd_task(xcmder_t* xcmder) {
    uint8_t c;
    if (xcmder && xcmder->_initOK) {
        if (xcmder->io.get_c(&c)) {
            xcmd_parser(xcmder, c);
        }
    }
}
