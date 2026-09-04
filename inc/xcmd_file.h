#ifndef XCMD_FILE_H
#define XCMD_FILE_H

#include <stddef.h>
#include <stdint.h>

#include "xcmd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 输出重定向文件接口 (弱符号): xcmd 内置支持 `cmd > file` / `cmd >> file`,
 * 库内提供恒失败的缺省实现; 需要重定向功能的平台 (如 fs_cmds) 覆盖强符号。
 * fd 语义: >=0 有效文件句柄, (size_t)-1 表示无重定向。
 */

/**
 * @description: 打开文件 (is_write=0 只读; is_write=1 时 is_append 决定追加/截断)
 * @return {size_t} 成功返回文件句柄(>=0), 失败返回 (size_t)-1
 */
size_t file_open(char* name, int is_write, int is_append);

/**
 * @description: 关闭文件句柄, fd<0 时无操作
 */
void file_close(size_t fd);

/**
 * @description: 向文件写 NUL 结尾字符串
 * @return {int} 0 成功, -1 失败
 */
int file_write(size_t fd, const char* str);

/**
 * @description: 从文件读最多 buflen 字节到 buf
 * @return {int} 0 成功, -1 失败
 */
int file_read(size_t fd, char* buf, int buflen);

#ifdef __cplusplus
}
#endif

#endif /* XCMD_FILE_H */
