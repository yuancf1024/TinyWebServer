#include <stdio.h>

#define PI 3.14

int main() {
    // 这是测试代码
    int sum = PI + 10;
    printf("Hello!\n");
    return 0;
}

/*
源代码 .h .c .cpp

预处理 -E -> .i文件
gcc test.c -E -o test.i

编译 -S
gcc test.i -S -o test.s

汇编
gcc test.s -c

链接
gcc test.o -o test

*/