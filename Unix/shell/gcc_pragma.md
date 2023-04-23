# `gcc #pragma`

## <font color=#DC143C >`#pragema message`</font>

`#pragema message "string" `:在编译阶段打印此信息
`vim pragma_test1.c`

```cpp
#include <stdio.h>

#if defined TEST1
#pragma message "compile with test1..."
#elif defined TEST2
#pragma message "compile with test2..."
#endif

int main()
{
    return 0;
}

```

```
zh@zh-virtual-machine:~/anything_test$gcc pragma_test1.c -DTEST1
pragma_test1.c:17:9: note: #pragma message: compile with test1...
#pragma message "compile with test1..."
```

## <font color=#DC143C >`#pragma once`</font>

放在头文件中，保证头文件只被编译一次（个人想法：头文件通常会用`ifndef + define + endif`来保证头文件只被包含一次，貌似不需要这个once了）

## <font color=#DC143C >`#pragma pack`</font>

```
#pragma pack(n)
...
#pragma pack()
```

在两者之间的结构体以n字节对齐（需要在gcc编译器所支持的字节对齐范围内）

# `gcc __attribute__`

`__attribute__`可以设置函数属性(function attribute), 变量属性(variable attribute)和类型属性(type attribute)

## 函数属性(function attribute)

关键字`__attribute__`允许您在声明时指定特殊属性。此关键字后跟双括号内的属性说明。当前为所有目标上的函数定义了以下属性： `noreturn, noinline, always_inline, pure, const, nothrow, sentinel, format, format_arg, no_instrument_function, section, constructor, destructor, used, unused, deprecated, weak, malloc, alias,warn_unused_result和nonnull.` 为特定目标系统上的功能定义了其他几个属性。

### `alias`

```cpp
void __f(); /*do something*/
void f() __attribute__((weak, alias("__f")));
```

声明f()为__f()的一个弱别名。

### <font color=#DC143C >`always_inline/noinline`</font>

`inline`: 建议编译器内联，实际是否内联由编译器决定（根据优化等级）；
`__attribute__((always_inline))`：强制编译器将函数当做内联函数；

```cpp
inline void foo(int a) __attribute__((always_inline));
或者
__attribute__ ((always_inline)) void foo(int a);
```

### `const`

该属性只能用于带有数值类型参数的函数上。当重复调用带有数值参数的函数时，由于返回值是相同的，所以此时编译器可以进行优化处理，除第一次需要运算外，其它只需要返回第一次的结果就可以了，进而可以提高效率。
该属性主要适用于没有静态状态（static state）和副作用的一些函数，并且返回值仅仅依赖输入的参数。

### <font color=#DC143C >`constructor`</font>

### <font color=#DC143C >`destructor`</font>

声明为constructor的函数会在进入main函数前被调用，
声明为destructor的函数会在main函数结束或调用exit时被调用。(感觉用处不大...)

```cpp
#include<stdio.h>
__attribute__((constructor)) void before_main()
{
	printf("before main/n");
}

__attribute__((destructor)) void after_main()
{
	printf("after main/n");
}

int main()
{
	printf("in main/n");
	return 0;
}
```

```
$ gcc test.c -o test
$ ./test
before main
in main
after main
```

### `deprecated(过时的)`

被声明为deprecated的函数，类型，变量。如果在源文件中出现调用，在编译时，会弹出警告，警告会打印函数/变量/类型被声明的位置。

### `format`

```cpp
extern int my_printf (void *my_object, const char *my_format, ...)  __attribute__ ((format (printf, 2, 3)));
```

`__attribute__((format(printf/scanf..., 格式字符串在my_printf中的第几个参数，从my_printf的第几个参数开始检查)`

### `naked(裸函数)`

`__attribute__((naked))`对于这种函数，编译器不会生成任何函数入口代码和退出代码。这种函数一般应用在与操作系统内核相关的代码中，如中断处理函数、钩子函数等。
详细可见Linux pg打桩例子中代码。

### <font color=#DC143C >`nonull`</font>

该nonnull属性指定某些函数参数应该是非空指针。例如，声明

```cpp
extern void * my_memcpy (void *dest, const void *src, size_t len) __attribute__((nonnull (1, 2)));
```

导致编译器检查在对的调用中`my_memcpy`，参数`dest`和`src`是否为非空。如果编译器确定在标记为非空的参数槽中传递了空指针，并且`-Wnonnull`选项启用，发出警告。编译器还可以选择基于某些函数参数不会为空的知识进行优化。
如果没有为`nonnull`属性提供参数索引列表，则所有指针参数都标记为非空。为了说明，以下声明等效于前面的示例

```cpp
extern void * my_memcpy (void *dest, const void *src, size_t len) __attribute__((nonnull));
```

### `noreturn`

一些标准库函数，例如`abort and exit`不能返回。`GCC` 自动知道这一点。

```cpp
extern void exit(int)   __attribute__((noreturn));
extern void abort(void) __attribute__((noreturn));
```

一些程序定义了自己的永远不会返回的函数。你可以声明它们 `noreturn`来告诉编译器这个事实。例如

```cpp
void fatal () __attribute__ ((noreturn));
void fatal (/* ... */)
{
    /* ... */ /* Print error message. */ /* ... */
    exit (1);
}
```

### `pure`

许多函数返回值只取决于参数或者全局变量，这样的函数可以像算术运算符一样进行公共子表达式消除和循环优化。这些函数应该用属性声明`pure`。例如

```cpp
int square (int) __attribute__ ((pure));
other: strlen, memcmp
```

### <font color=#DC143C >`unused`</font>

附加到函数的此属性意味着该函数可能未被使用。`GCC` 不会对此函数产生警告。

### <font color=#DC143C >`used`</font>

向编译器说明这段代码有用，即使在没有用到的情况下编译器也不会警告！

### <font color=#DC143C >`visibility (default, hidden, protected or internal )`</font>

在多个源文件被链接成动态库时，声明为`visibility("hidden")`的函数在使用`readelf`查看动态库时，该函数将会是`hidden`的，在外部调用此函数，即使链接此库，编译也无法通过。
被声明为`visibility("hidden")`的函数被用于库内部的相互调用，而不提供外部使用。
gcc 默认所有函数都是`default`状态`(gcc visibility=default *.c ...)`，即默认暴露所有函数接口给外部使用。
可以在编译时指定默认的可见性`gcc visibility=default/hidden... *.c ...`，在代码内部使用`__attribute__`定义的函数依然会生效
与static修饰的函数的区别：`static`作用域是文件而`visibility`作用于域是动态库。

### <font color=#DC143C >`weak`</font>

若两个或两个以上全局符号（函数或变量名）名字一样，而其中之一声明为weak属性，则这些全局符号不会引发重定义错误。链接器会忽略弱符号，去使用普通的全局符号来解析所有对这些符号的引用，但当普通的全局符号不可用时，链接器会使用弱符号。当有函数或变量名可能被用户覆盖时，该函数或变量名可以声明为一个弱符号。



## 类型属性(type attribute)

关键字`__attribute__`允许您在定义此类类型时指定特殊属性`struct`和`union`类型。此关键字后跟双括号内的属性说明。六个属性，目前的类型定义为： `aligned，packed，transparent_union，unused， deprecated和may_alias
`

### <font color=#DC143C >`aligned`</font>

`__attribute__((aligned(n)))`   采用n字节对齐，n的有效参数为2的幂值，32位最大为2 ^ 32,64位为2 ^ 64，这个时候编译器会将让n与默认的对齐字节数进行比较，取较大值为对齐字节数，与`#pragma pack(n)`恰好相反。
使用`__attribute__((aligned))`，
使用`__attribute__((aligned(n)))` 进行对齐受到系统位数影响：
在64位操作系统64位编译器的环境下，当n ≥ 8时，内存对齐的字节数是n，不然为8
在32位操作系统32位编译器的环境下，当n ≥ 4时，内存对齐的字节数是n，不然为4

结构体字节对齐受到系统位数影响，受到结构体中最大基本类型字节数影响，受到aligned属性影响。
1.在没有aligned限制条件下，结构体对齐受到系统位数和最大基本类型字节数影响。eg:
最大基本类型字节数大于等于系统位数时，使用系统位数对齐（double为8字节,32bit系统依然使用4字节对齐）
最大基本类型字节数小于等于系统位数时，使用最大基本类型字节数对齐（char为1字节，64/32为系统都使用1字节对齐）

```cpp
typedef struct __test1
{					//64bit    32bit
	int a;			//4 + 4		4
	double b;		//8			4 +4
}test1;
```

sizeof(test1) = 16byte (64bit), 12 (32bit);

```cpp
typedef struct __test2
{					//64bit		32bit
	test1 a;		//16		12
	int b;			//4 + 4		4
}test2;
```

sizeof(test2) = 24byte (64bit),  16byte(32bit)

2.在使用`aligned`限制结构体对齐时，`__attribute__((aligned(n)))`中的n必须>=系统位数/8即系统对齐字节数时，n字节对齐才会生效。此时结构体的大小一定是n的整数倍。
内部的元素排列依然会以1中的描述进行对齐，当结构体整体大小不满足n的整数倍时，会向后补字节数。

```cpp
typedef struct __test
{				//64bit		32bit
	int a;		//4 + 4		4
	double b;	//8			4+4
	char c;		//1 + 15	1 + 3
}__attribute__((aligned(16))) test;
```

当test被嵌套到其他结构体时，其他结构体将至少以n字节对齐，此时test相当以一个基本元素。如果新结构体中出现比test还要大的对齐类型，则取最大对齐类型进行对齐。但是基础的对齐依然采用系统位数/8。

### <font color=#DC143C >`packed`</font>

`__attribute__((__packed__))`作用与aligned正好相反，使用此条件限制的结构体，会以结构体中最小类型对齐。

```cpp
typedef struct __test1
{
	int a;
    double b;
    char c;
}__attribute__((__packed__)) test1;

typedef struct __test2
{
	test1 a;
    int b;
    char c;
}test2;

typedef struct __test3
{
	test2 a;
    double b;
    char c;
}__attribute((__packed__))test3;

typedef struct __test4
{
	int a;
    short b;
    char c;
}test4;

typedef struct __test5
{
	test4 a;
    int b;
    double c;
}__attribute((__packed__)) test5;
```

sizeof(test1) = 4 + 8 + 1 = 13byte;(以char即1字节对齐）
sizeof(test2) = 13 + (3) + 4 + 1 +(3) = 24byte;（test1作为整体以int即4字节对齐）
sizeof(test3) = 24 + 8 + 1 = 33byte;（test2作为整体，以char对齐）
sizeof(test4) = 4 + 2 + 1 + (1) = 8byte;（64位系统默认最小8字节对齐）
sizeof(test5) = 8 + 4 + 8 = 20byte;（test4作为整体，以int/char对齐（这个不清楚））

### <font color=#DC143C >`unused`</font>

When attached to a type (including a union or a struct), this attribute means that variables of that type are meant to appear possibly unused. GCC will not produce a warning for any variables of that type, even if the variable appears to do nothing. This is often the case with lock or thread classes, which are usually defined and then not referenced, but contain constructors and destructors that have nontrivial bookkeeping functions.

### `transparent_union`

...感觉没啥用

### `deprecated`

The deprecated attribute results in a warning if the type is used anywhere in the source file. This is useful when identifying types that are expected to be removed in a future version of a program. If possible, the warning also includes the location of the declaration of the deprecated type, to enable users to easily find further information about why the type is deprecated, or what they should do instead. Note that the warnings only occur for uses and then only if the type is being applied to an identifier that itself is not being declared as deprecated.

```cpp
typedef int T1 __attribute__ ((deprecated));
T1 x;
typedef T1 T2;
T2 y;
typedef T1 T3 __attribute__ ((deprecated));
T3 z __attribute__ ((deprecated));
```

results in a warning on line 2 and 3 but not lines 4, 5, or 6. No warning is issued for line 4 because T2 is not explicitly deprecated. Line 5 has no warning because T3 is explicitly deprecated. Similarly for line 6.

### `may_alias`

...感觉没啥用

## 变量属性(variable attribute)

### `vector_size`

```cpp
typedef int v4si __attribute__ ((vector_size (16)));

v4si a = {1,2,3,4};
v4si b = {3,2,1,4};
v4si c;

c = a >  b;     /* The result would be {0, 0,-1, 0}  */
c = a == b;     /* The result would be {0,-1, 0,-1}  */
```

### `aligned`

...

### <font color=#DC143C >`cleanup`</font>

The cleanup attribute runs a function when the variable goes out of scope. This attribute can only be applied to auto function scope variables; it may not be applied to parameters or variables with static storage duration. The function must take one parameter, a pointer to a type compatible with the variable. The return value of the function (if any) is ignored.

使用cleanup实现c++的<font color=#DC143C >智能指针</font>：

```cpp
#include <stdlib.h>
#include <stdio.h>

#define smart_ptr __attribute__((cleanup(auto_free)))

static void auto_free(void *ptr)
{
    void **pptr = (void **)ptr;
    if(*pptr != NULL)
    {
        printf("free heap mem\n");
        free(*pptr);
    }
}

int main()
{
    smart_ptr int *p = (int *)malloc(100);
    return 0;
}

```

```
zhanghang22@Cpl-MT-General-13-131:~/work/anything_test$ gcc test1.c
zhanghang22@Cpl-MT-General-13-131:~/work/anything_test$ valgrind  --tool=memcheck  --leak-check=full  --leak-check=full --show-reachable=yes ./a.out
==2839== Memcheck, a memory error detector
==2839== Copyright (C) 2002-2011, and GNU GPL'd, by Julian Seward et al.
==2839== Using Valgrind-3.7.0 and LibVEX; rerun with -h for copyright info
==2839== Command: ./a.out
==2839==
free heap mem
==2839==
==2839== HEAP SUMMARY:
==2839==     in use at exit: 0 bytes in 0 blocks
==2839==   total heap usage: 1 allocs, 1 frees, 100 bytes allocated
==2839==
==2839== All heap blocks were freed -- no leaks are possible
==2839==
==2839== For counts of detected and suppressed errors, rerun with: -v
==2839== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 2 from 2)

```

### <font color=#DC143C >`common nocommon`</font>

使用__attribute__((common))的变量将被放在common段，成为一个弱符号，
相反__attribute__((nocommon))将变量定义为强符号。
也可以在gcc编译时使用-fcommon 和-fno-common来指定未赋值的全局变量为强/弱符号。
当两个文件存在未初始化的和初始化的同名全局变量时，两文件编译链接并不会报错，这是因为未初始化的全局变量是弱符号，位于bss段的common段，会被强符号覆盖，为了防止这种覆盖问题，可以在编译时加入-fno-common选项或者在声明时，使用__attribute限制，这样会在编译时报错。
COMMON段：http://blog.chinaunix.net/uid-23629988-id-2888209.html

### `deprecated`

同函数和类型属性中的意思，在被声明为此属性的变量，如果被使用，编译器会在编译时报出警告，包含使用此变量的具体文件和行数。



### <font color=#DC143C >`packed aligned `</font>

This attribute specifies a minimum alignment for the variable or structure field, measured in bytes. For example, the declaration:

          int x __attribute__ ((aligned (16))) = 0;

causes the compiler to allocate the global variable x on a 16-byte boundary. On a 68040, this could be used in conjunction with an asm expression to access the move16 instruction which requires 16-byte aligned operands.

You can also specify the alignment of structure fields. For example, to create a double-word aligned int pair, you could write:

          struct foo { int x[2] __attribute__ ((aligned (8))); };

This is an alternative to creating a union with a double member that forces the union to be double-word aligned.

As in the preceding examples, you can explicitly specify the alignment (in bytes) that you wish the compiler to use for a given variable or structure field. Alternatively, you can leave out the alignment factor and just ask the compiler to align a variable or field to the maximum useful alignment for the target machine you are compiling for. For example, you could write:

          short array[3] __attribute__ ((aligned));

Whenever you leave out the alignment factor in an aligned attribute specification, the compiler automatically sets the alignment for the declared variable or field to the largest alignment which is ever used for any data type on the target machine you are compiling for. Doing this can often make copy operations more efficient, because the compiler can use whatever instructions copy the biggest chunks of memory when performing copies to or from the variables or fields that you have aligned this way.

The aligned attribute can only increase the alignment; but you can decrease it by specifying packed as well. See below.

Note that the effectiveness of aligned attributes may be limited by inherent limitations in your linker. On many systems, the linker is only able to arrange for variables to be aligned up to a certain maximum alignment. (For some linkers, the maximum supported alignment may be very very small.) If your linker is only able to align variables up to a maximum of 8 byte alignment, then specifying aligned(16) in an __attribute__ will still only provide you with 8 byte alignment. See your linker documentation for further information.

The **packed** attribute specifies that a variable or structure field should have the smallest possible alignment—one byte for a variable, and one bit for a field, unless you specify a larger value with the aligned attribute.
Here is a structure in which the field x is packed, so that it immediately follows a:

          struct foo
          {
            char a;
            int x[2] __attribute__ ((packed));
          };

### `section `

https://dengjin.blog.csdn.net/article/details/107223168?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ELandingCtr%7Edefault-1.queryctrv2&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ELandingCtr%7Edefault-1.queryctrv2&utm_relevant_index=1

### `unused`

附加到变量的此属性意味着该变量可能未被使用。GCC 不会对此变量产生警告。

### `weak`

与函数属性中的weak一致

...





ref:
http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Function-Attributes.html

http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Type-Attributes.html#Type-Attributes

http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Variable-Attributes.html#Variable-Attributes

https://nshipster.com/__attribute__/