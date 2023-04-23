# container_of(Linux-3.0)

原型：

```cpp
#define container_of(ptr, type, member) ({              \         
const typeof( ((type *)0)->member ) *__mptr = (ptr);    \         
(type *)( (char *)__mptr - offsetof(type,member) );})
```



函数会使用到另一个宏

```cpp
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
```

测试offsetof宏：

```cpp
#include<stdio.h>
  
struct test_s
{
    int a;
    char b;
    int c;
}test_t;


int main()
{
    printf("&test_t=%p\n",&test_t);
    printf("&test_s.c=%p\n",&test_t.c);
    printf("(size_t&((struct test_s *)0)->c)=%d\n",((size_t) &((struct test_s *)0)->c));
    printf("&((struct test_s *)0)->c=%d\n",&((struct test_s *)0)->c);
    return 0;
}
```

输出：

```shell
&test_t=0x601050
&test_s.c=0x601058
(int&((struct test_s *)0)->c)=8
&((struct test_s *)0)->c=8
```

由于结构体的对齐，可以看出，&((struct test_s *)0)->c可以获取成员c到结构体首地址的长度（Byte)



结论：

**宏offsetof其作用就是获取成员MEMBER到其类型TYPE的首地址的字节数。**

再来看宏函数container_of：

第二行const typeof( ((type *)0)->member ) *__mptr = (ptr);将定义_mptr其类型为type中的member的类型，这里的作用其实是判断ptr类型和type的member类型是否一致，保证宏containe工作的安全性，如果_mptr和ptr类型不匹配，将会报错。

第三行就是获取该结构体的首地址。

**container_of(ptr, type, member)通过member的域成员变量指针ptr来获取整个结构体type的首地址**



示例：

```cpp
struct evdev *evdev = container_of(inode->i_cdev, struct evdev, cdev);
```

根据inode中类型为struct cdev的成员i_cdev的地址，找到包含i_cdev地址的结构体evdev的首地址