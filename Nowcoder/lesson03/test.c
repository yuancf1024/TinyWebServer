#include <stdio.h>

int main() {
    int a = 10;
#ifdef DEBUG
    printf("我是一个程序猿，我不会爬树\n");
#endif
    for (int i = 0; i < 3; ++i) {
        printf("hello, GCC!\n");
    }

    int b, c, d, f;
    b = 10;
    c = b;
    d = c;
    f = d;

    /*
    程序会优化 -> 能够提高逆向的难度
    int b, c, d, f;
    b = 10;
    c = 10;
    d = 10;
    f = 10;
    */

   // C89 变量定义必须最上面
    int a = 10;
    fun();
    int d = 10;

    for (int i = 0; i < 5; ++i){} // C89要求int定义放在()外面
    return 0;
}

/*
$ gcc -o test test.c
$ ./test
hello, GCC!
hello, GCC!
hello, GCC!


$ gcc -o test test.c -D DEBUG # -D 使用指定的宏
$ ./test
我是一个程序猿，我不会爬树
hello, GCC!
hello, GCC!
hello, GCC!
*/
