#include "xcmd.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static xcmder_t *g_cmder = NULL;

#define EXIT_MESSAGE() xcmd_print(g_cmder, "press \"q\" or \"Q\" to exit!\r\n")

#define EXIT_CHECK()                 \
    do                               \
        (toupper(GET_CHAR()) == 'Q') \
        {                            \
            uint8_t c;               \
            if (GET_CHAR(&c))        \
            {                        \
                switch (c)           \
                case 'q':            \
                case 'Q':            \
                case 0x1B:           \
                    return;          \
            }                        \
        }                            \
    while (0);

static uint8_t param_check(int need, int argc, char*argv[])
{
    uint8_t i,ret = 0;
    if(need<(argc))
    {
        ret = 1;
    }
    else
    {
        xcmd_print(g_cmder, "err need %d but input %d:\r\n", need, argc-1);
    	xcmd_print(g_cmder, "input= ");
	    for(i=0; i<argc; i++)
	    {
	        if(argv[i] != NULL)
	        {
	            xcmd_print(g_cmder, "%s ", argv[i]);
	        }
	    }
	    xcmd_print(g_cmder, "\r\n");
        ret = 0;
    }
    return ret;
}

static void cmd_echo(int argc, char* argv[])
{
    if(param_check(1, argc, argv))
	{
    	xcmd_print(g_cmder, "%s\r\n", argv[1]);
	}
}

static void cmd_example(int argc, char* argv[])
{
    uint8_t i;
    if(param_check(1, argc, argv))
	{
		if(strcmp(argv[1], "str") == 0)
		{
			for(i=2; i<argc; i++)
		    {
		    	xcmd_print(g_cmder, "%s\r\n", argv[i]);
			}
		}
		if(strcmp(argv[1], "int") == 0)
		{
			for(i=2; i<argc; i++)
		    {
		    	xcmd_print(g_cmder, "%d\r\n", atoi(argv[i]));
			}
		}
		if(strcmp(argv[1], "float") == 0)
		{
			for(i=2; i<argc; i++)
		    {
		    	xcmd_print(g_cmder, "%f\r\n", atof(argv[i]));
			}
		}
	}
}

static xcmd_t cmds[] = 
{
    {"echo", cmd_echo, "echo anything", NULL},
    {"example", cmd_example, "example [-f|-i|-s] [val]", NULL},
};

void test_cmd_init(xcmder_t *cmder)
{
    xcmd_register(cmder, cmds, sizeof(cmds)/sizeof(xcmd_t));
    g_cmder = cmder;
}
