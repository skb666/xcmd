#ifndef XCMD_OBJ_H
#define XCMD_OBJ_H

#include <stddef.h>
#include <stdint.h>

#include "xcmd.h"
#include "xcmd_default_confg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 历史记录节点 (环形链表, 节点池内建在实例里)
 */
typedef struct __history {
    char line[XCMD_LINE_MAX_LENGTH + 1];
    struct __history* next;
    struct __history* prev;
} xcmd_history_t;

/**
 * xcmd 解释器实例: 所有运行状态都在实例内, 不占用任何全局变量,
 * 一个工程可创建多个实例各自绑定一路 IO 并行运行。
 * 实例由调用方分配 (静态/动态), 零初始化后交给 xcmd_init() 即可用。
 */
struct xcmder {
    struct
    {
        size_t write_fd;
        int (*get_c)(uint8_t*);
        int (*put_c)(uint8_t);
    } io;

    struct
    {
        xcmd_t head;
    } cmd_list;

    struct
    {
        xcmd_key_t head;
    } key_list;

    struct
    {
#if XCMD_HISTORY_MAX_NUM
        struct
        {
            xcmd_history_t pool[XCMD_HISTORY_MAX_NUM];
            uint16_t index;
        } history_pool;
        struct
        {
            uint16_t len;
            xcmd_history_t* head;   /* 最旧一条 */
            xcmd_history_t* tail;   /* 最新一条 */
            xcmd_history_t* slider; /* 历史浏览位置, NULL 表示未在浏览 */
        } history_list;
#endif

        char display_line[XCMD_LINE_MAX_LENGTH + 1]; /* 显示区的缓存 */
        const char* prompt;                          /* 显示区的提示 */
        uint16_t byte_num;                           /* 当前行的字符个数 */
        uint16_t cursor;                             /* 光标所在位置 */
        uint8_t encode_case_stu;
        char encode_buf[7];
        uint8_t encode_count;
        uint32_t key_val;
        uint8_t (*recv_hook_func)(char*); /* 解释器接收钩子函数，返回0则接收到的数据会返回给解释器，返回1则不会 */
        uint8_t redraw_pending;           /* 命令已自行重绘命令行 (如 clear), 框架收尾时跳过补行+提示符 */
    } parser;
#ifdef XCMD_END_LINE_MODE
    uint8_t cmd_executing; /* 当前是否正在执行命令 (对应 letter-shell status.isActive) */
#endif
    uint8_t _initOK;
};

#ifdef __cplusplus
}
#endif

#endif /* XCMD_OBJ_H */
