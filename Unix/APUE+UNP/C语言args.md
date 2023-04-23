# C语言args

```c
__FUNCTION__函数宏，打印函数名%s
__FILE__文件宏，打印文件名%s
__LINE__行数宏，打印行数%d
```

可变参数及其宏定义：

>#define printf_dbg(fmt, ...) printf(fmt, ##__VA_AGRS__)
>// ...和__##VA_ARGS__等价
>#define printf_dbgs(fmt, arg...)    printf(fmt,##arg)
>//fmt可为格式化字符串,##arg的双##号用来连接arg...中的多个参数，当参数个数为0时，还可以清除printf(fmt,##args)中的','防止printf多余逗号报错
>两个宏定义此处可以等价，##用来在参数为0时清除
>#define a(arg...) printf(arg)
>#define b(...) printf(__VA_ARGS__)
>上面两者等价arg...和...可以为字符串和格式化字符串


>说一下#和##在宏中的作用：
>#: #NAME将会生成"NAME"字符串
>##: NAME1##__NAME2用来连接前后两者,生成NAME1__NAME2
>NAME会在#合##生效前先展开
>在上面##还可以在arg和__VA_ARGS__为空时清除##前的','
>#号的使用：生成字符串
```c
#define SQUARE(x) printf("The square of "#x" is %d.\n", ((x)*(x)));
SQUARE(8) -->   The square of 8 is 64

#define a(format, args...) printf("==>Debug: %s-%d |" #format, __FUNCTION__, __LINE__, ## args)
a(abc)在预处理时会被替换为：printf("==>Debug: %s-%d |" "abc", __FUNCTION__, 34);

#define b(format, args...) printf("==>Debug: %s-%d "#format"|" , __FUNCTION__, __LINE__, ## args)
b(abc)预处理：printf("==>Debug: %s-%d ""abc""|", __FUNCTION__, 33);
运行结果：==>Debug: main-33 abc|
```
##的使用：
```
#define XNAME(n) x ## n
XNAME(8)  -->   x8
```


其他：
```c
#define f(fmt,...)   printf(fmt,##__VA_ARGS__)
#define g(...) f(__VA_ARGS__)

void g(const char *format,...)
{
        printf("======================%s\n",format);
        char buf[1024];
        memset(buf,0,1024);
        va_list ap;
        memset(&ap,0,sizeof(ap));
        va_start(ap,format);
        vsnprintf(buf,1024,format,ap);
        va_end(ap);
        printf("%s\n",buf);

}
int main(int argc ,char **argv)
{
    g("this is a good class:%s   %s\n","a","b");
}

运行结果：
======================this is a good class:%s   %s

this is a good class:a   b
```


```c
void print1(int count, ...)
{
    va_list valist;
    int value = 0.0;
    int i;
    printf("count:%d\n", count);

    va_start(valist, count);

    for (i = 0; i < count; i++)
    {
        value = va_arg(valist, int);
        printf("%d ", value);
    }
    printf("\n");

    va_end(valist);
}
int main(int argc ,char **argv)
{
        print1(5,2,3,4,5,6);
}
运行结果：
count:5
2 3 4 5 6

```

```c
#include <stdio.h>
#include <stdarg.h>

void print(int integer_or_string,...)
{
        va_list arg;
        int integer;
        char *str;
        int index = 0;
        if(integer_or_string == 1)
        {
                va_start(arg,(int *)integer_or_string);
        }
        else
                va_start(arg,(char *)integer_or_string);
        while(1)
        {
                if(integer_or_string == 1)
                {
                        integer = va_arg(arg,int);
                        if(integer == 0)
                               break;
                        printf("param[%d] : [%d]\n",++index,integer);
                }
                else
                {
                        str = va_arg(arg,char *);
                        if(strcmp(str, "") == 0)
                               break;
                        printf("param[%d] : [%s]\n",++index,str);
                }
        }
        va_end(arg);
        return ;
}


int main()
{
        print(1,1,2,3,4,5,0);
        print(2,"this","is","a","good","class","");
}


运行结果：
root@zhanghang-virtual-machine:~# ./a.out
param[1] : [1]
param[2] : [2]
param[3] : [3]
param[4] : [4]
param[5] : [5]
param[1] : [this]
param[2] : [is]
param[3] : [a]
param[4] : [good]
param[5] : [class]

```

printf的简单实现：
```c
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
int foo(char *fmt, ...);
int main()
{
        char *a = "hello";
        int b = 224;
        char c = 'x';

        foo("%s,%d,%c\n",a,b,c);
        return 0;
}
int foo(char *fmt, ...)
{
        va_list ap;
        int d;
        char c, *s;
        va_start(ap, fmt);
        while (*fmt != '\n')
                switch(*fmt++)
                {
                        case 's':           /* string */
                               s = va_arg(ap, char *);
                               printf("string %s\n", s);
                               break;
                        case 'd':           /* int */
                               d = va_arg(ap, int);
                               printf("int %d\n", d);
                               break;
                        case 'c':           /* char */
                               /* need a cast here since va_arg only
                                  takes fully promoted types */
                               c = (char) va_arg(ap, int);
                               printf("char %c\n", c);
                               break;
                        default:
                               break;
                }
        va_end(ap);
        return 0;
}
运行结果：
root@zhanghang-virtual-machine:~# ./a.out
string hello
int 224
char x

```

```c
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
void print(const char *format,...)
{
        FILE *fp;
        va_list arg;
        char buf[1024];
        memset(buf, 0, 1024);
        fp = fopen("./test.txt","a+");
        if(NULL == fp)
        {
                printf("fopen error:%s\n",strerror(errno));
                return ;
        }
        va_start(arg,format);
        vprintf(format,arg);
        va_end(arg);
        va_start(arg,format);
        vsnprintf(buf,1024,format,arg);
        printf("%s\n",buf);
        va_end(arg);
        va_start(arg,format);
        memset(buf, 0, 1024);
        vsprintf(buf,format,arg);
        printf("%s\n",buf);
        va_end(arg);
        va_start(arg,format);
        vfprintf(fp,format,arg);
        va_end(arg);
}

int main()
{
        print("%s,%d,%c\n","hello",100,'z');
}

运行结果：
root@zhanghang-virtual-machine:~# ./a.out
hello,100,z
hello,100,z

hello,100,z

root@zhanghang-virtual-machine:~# cat test.txt
hello,100,z

```