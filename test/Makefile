# 多实例回归测试 (host 平台, gcc/mingw 或 linux 均可)
CFLAGS = -g -O0 -Wall -I./ -I../inc -I../extensions/test -I../extensions/ex_cmds

test_multi: test_multi_instance.c ../src/xcmd.c ../src/xcmd_default_cmds.c ../src/xcmd_default_keys.c \
           ../extensions/test/test.c ../extensions/ex_cmds/ex_cmds.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: run clean
run: test_multi
	./test_multi$(if $(filter Windows_NT,$(OS)),.exe,)

clean:
	rm -f test_multi test_multi.exe
