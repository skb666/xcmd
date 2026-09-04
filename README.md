# xcmd

#### 效果展示

![输入图片说明](https://images.gitee.com/uploads/images/2021/0922/220957_66faa768_1680380.gif "演示1.gif")

#### 介绍

xcmd是一款单片机命令行工具，移植十分方便，并且对flash与ram占用很小，旨在为单片机提供一个能够快速搭建且占用资源很小的命令行工具，可以大大加快单片机程序调试工作，它有一下几个优点。

1. **移植十分简单**
2. **资源占用很小**
3. **支持历史记录**
4. **支持命令自动补全**
5. **支持注册快捷键**
6. **支持多实例：一个工程可创建多个shell，每个实例绑定一路IO（如UART0/UART1），命令表/历史记录/显示缓存互相隔离**
7. **支持 `xcmd_cmd_register()/xcmd_key_register`方法注册命令或按键**
8. **支持 `XCMD_EXPORT_CMD()/XCMD_EXPORT_KEY()`方法直接导出命令或按键，不需要额外运行注册函数**

#### 支持的平台

1. linux
2. arduino
3. stm32
4. esp32
5. qemu-stm32

#### 测试过的串口软件

2. （win/linux推荐）putty
3. （win推荐）MobaXterm
4. （linux端推荐）cu
5. SecureCRT：请设置成Xterm模式
6. Xshell
7. nc(支持性很差)
8. minicom(支持性很差)

#### 支持的扩展功能

1. 类linux风格的快捷键：ctr+left、ctr+right、ctr+a、ctr+e、ctr+u、ctr+k、ctr+l快捷键
2. 支持字体、背景颜色显示
3. 支持类linux的文件操作（Fatfs文件系统）：ls、cd、rm、df、mv、mkdir、touch、rd、wr、cat
4. 支持基于socket的udp server、udp client测试程序

#### 移植

- 移植十分简单，可在example中查看各平台移植结果

1. 在初始化的时候提供字符输入输出函数即可
2. 从example中拷贝一个xcmd_confg.h到自己的项目

```C
/* 例如移植到Arduino平台 */
int cmd_get_char(uint8_t *ch)
{
    if(Serial.available())
    {
        *ch =  Serial.read();
        return 1;
    }
    else
    {
        return 0;
    }
}

int cmd_put_char(uint8_t ch)
{
    Serial.write(ch);
    return 1;
}

static xcmder_t xcmder; /* 声明一个xcmd实例 */

void setup() {
    Serial.begin(115200);
    xcmd_init(&xcmder, cmd_get_char, cmd_put_char);
}

void loop() {
  
    xcmd_task(&xcmder);
}
```

#### 多实例

xcmd 所有运行状态都收在 `xcmder_t` 实例内（见 `inc/xcmd_obj.h`），不占用全局变量。
需要多路 shell 时为每路 IO 各建一个实例即可，实例间的命令表、按键表、历史记录、
显示缓存完全隔离：

```C
static xcmder_t xcmder_uart0;
static xcmder_t xcmder_uart1;

xcmd_init(&xcmder_uart0, uart0_get_char, uart0_put_char);
xcmd_init(&xcmder_uart1, uart1_get_char, uart1_put_char);

test_cmd_init(&xcmder_uart0);   /* 扩展命令只注册到实例0 */
xcmd_set_prompt(&xcmder_uart1, "uart1>");  /* 实例1 用独立的提示符 */

while(1) {
    xcmd_task(&xcmder_uart0);
    xcmd_task(&xcmder_uart1);
}
```

多实例回归测试见 `test/test_multi_instance.c`（host 平台直接编译运行）。

**注意**：命令回调签名为 `int func(xcmder_t* xcmder, int argc, char* argv[])`，
按键回调通过 `void* pv` 参数接收实例指针（`pv` 即发起按键的实例）。
使能 `ENABLE_XCMD_EXPORT` 时导出段是链接期生成的全局单份表，所有实例共享同一张命令表。

#### 配置

配置xcmd_config.h

**注意**： 如果不知道如何修改连接脚本，建议不要使能ENABLE_XCMD_EXPORT。不使能ENABLE_XCMD_EXPORT的情况下有更好的兼容性移植更加简单

```C
#define XCMD_LINE_MAX_LENGTH          (128)           /* 命令行支持的最大字符数 */
#define XCMD_HISTORY_MAX_NUM          (16)            /* 支持的历史记录个数, 这个参数对内存的影响很大，建议arduino设置为0，0为不支持 */
#define XCMD_PARAM_MAX_NUM            (8)             /* 支持输入的参数个数 */
// #define XCMD_DEFAULT_PROMPT           "->"            /* 提示符 */
// #define XCMD_DEFAULT_PROMPT_CLOLR     TX_GREEN        /* 提示符颜色 */
// #define ENABLE_XCMD_EXPORT                            /* 需要修改lds，启用后支持XCMD_EXPORT_CMD和XCMD_EXPORT_KEY方法导出命令 */
```

#### 使用说明

##### xcmd_register()方法注册自定义命令

```C
static int cmd_echo(int argc, char* argv[])
{
    if(param_check(1, argc, argv))
	{
    	printf("%s\r\n", argv[1]);
	}
    return 0;
}

static xcmd_t cmds[] = 
{
    {"echo", cmd_echo, "print anything", NULL},
};

void test_cmd_init(void)
{
    xcmd_cmd_register(cmds, sizeof(cmds)/sizeof(xcmd_t));
}
```

##### xcmd_key_register() 方法注册自定义快捷键

```C
int cmd_ctr_a(void* pv)
{
    printf("this is ctr+a\n");
}

static xcmd_key_t user_keys[] = 
{
    {KEY_CTR_A, cmd_ctr_a, "ctr+a", NULL},
};

void test_keys_init(void)
{
    xcmd_key_register(user_keys, sizeof(user_keys)/sizeof(xcmd_key_t));
}
```

##### XCMD_EXPORT_CMD()方法注册自定义命令，需要使能宏ENABLE_XCMD_EXPORT并修改lds，见 `example/stm32/qemu/demos/main.ld`

```C
static int cmd_echo(int argc, char* argv[])
{
    if(param_check(1, argc, argv))
	{
    	printf("%s\r\n", argv[1]);
	}
    return 0;
}
XCMD_EXPORT_CMD(echo, cmd_echo, "print anything")
```

##### XCMD_EXPORT_KEY() 方法注册自定义快捷键, 需要使能宏ENABLE_XCMD_EXPORT并修改lds，见 `example/stm32/qemu/demos/main.ld`

```C
int cmd_ctr_a(void* pv)
{
    printf("this is ctr+a\n");
}
XCMD_EXPORT_KEY(KEY_CTR_A, cmd_ctr_a, "ctr+a")
```

#### 其他

流程图需要使用drawio来打开，可以使用vscode drawio的插件来打开
