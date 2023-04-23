# vector模板和迭代器
```cpp
#include <vector>
template <class T, class Allocator = allocator<T>>
class vector  
{
    ...
}
```
动态创建一个含有5和int数的数组
`vector<int> vt(5);`

vector也属于STL容器，STL容器一般都有一些相同的基本方法，包括：
`size()`返回容器中元素数目
`swap()`交换量容器的内容
`begin()`返回一个指向容器第一个元素的迭代器
`end()`返回一个指向容器最后一个元素后一个元素的超尾迭代器

迭代器：一个广义指针。可以是一个指针，也可以是一个可对其指向元素执行类似指针的操作如解除引用(`*operator()`)和递增(`operator++()`)的对象。
每个容器都定义了一个迭代器，该迭代器为名为`iterator`的typedef，作用域为整个类。

为vector的double声明一个迭代器：
`vector<double>::iterator pd;`
以下为一个vector对象：
`vector<double> scores(10);`
使用迭代器访问vector中的元素：
`pd = scores.begin();`
`*pd = 15.6;`
`++pd;`

C++11的auto创建指向容器的迭代器：
```cpp
vector<double>::iterator pd = scores.begin();
auto pd = scores.begin();
```
使用迭代器进行遍历：
```cpp
vector<double> scores(10);
for(auto pd = scores.begin(); pd != scores.end(); pd++)
{
    cout << *pd << endl;
}
```
向迭代器尾部插入元素：
```cpp
for(auto pd = scores.begin(); pd != scores.end(); pd++)
{
    another_scores.push_back(*pd);
}
```
擦除容器中元素：[0,2)
```cpp
scores.erase(scores.begin(), scores.begin() + 2);
```
向容器中插入元素：将another_scores的所有元素插入到scores的开头
```cpp
scores.insert(scores.begin(), another_scores.begin(), another.end());
```
容器拥有与非成员函数同名函数时，应使用容器提供额函数，效率会更高，如find。

for_each函数：
```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
void show(const double a)
{
    cout << a <<endl;
}
int main()
{
    vector<double> vt(10);
    for(int i = 0; i < 10; i++)
        vt[i] = i * 2 + 1;
    for(auto pd = vt.begin(); pd != vt.end(); pd++)
        show(*pd);
    for_each(vt.begin(), vt.end(), show);
    double *p = &vt[0];
    for_each(p, p+10, show);
}
```
for_each接受三个参数，前两个为区间迭代器，最后一个是调用函数，函数不能修改容器内元素，函数参数为迭代器的解除引用。

random_shuffle随机排列区间元素：
```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
void show(const double a)
{
    cout << a << "  ";
}
int main()
{
    vector<double> vt(10);
    for(int i = 0; i < 10; i++)
        vt[i] = i * 2 + 1;
    for(auto pd = vt.begin(); pd != vt.end(); pd++)
        show(*pd);
    cout << endl;
    double *p = &vt[0];
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    for_each(p, p+10, show);
    cout << endl;
    random_shuffle(p, p+10);
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    random_shuffle(vt.begin(), vt.end());
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
}
```
sort函数有两个版本，第一个接受两个迭代器区间参数，并使用容器中元素对象的operator<对容器中元素从小到大排序，基本类型不需要定义operator<，但是如果元素类型为用户自定义类型，需要类重载operator<：
```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
void show(const double a)
{
    cout << a << "  ";
}
int main()
{
    vector<double> vt(10);
    for(int i = 0; i < 10; i++)
        vt[i] = i * 2 + 1;
    for(auto pd = vt.begin(); pd != vt.end(); pd++)
        show(*pd);
    cout << endl;
    double *p = &vt[0];
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    for_each(p, p+10, show);
    cout << endl;
    random_shuffle(p, p+10);
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    random_shuffle(vt.begin(), vt.end());
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    sort(vt.begin(), vt.end());
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
}
```
sort的第二个版本接受第三个参数，为一个函数对象，返回值为bool类型，接受两个类型为迭代器解除引用的参数：
```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
void show(const double a)
{
    cout << a << "  ";
}
bool ll(const double & d1, const double & d2)
{
    if(d1 < d2)
        return false;
    return true;
}
int main()
{
    vector<double> vt(10);
    for(int i = 0; i < 10; i++)
        vt[i] = i * 2 + 1;
    for(auto pd = vt.begin(); pd != vt.end(); pd++)
        show(*pd);
    cout << endl;
    double *p = &vt[0];
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    for_each(p, p+10, show);
    cout << endl;
    random_shuffle(p, p+10);
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    random_shuffle(vt.begin(), vt.end());
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    sort(vt.begin(), vt.end());
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
    sort(vt.begin(), vt.end(), ll);
    for_each(vt.begin(), vt.end(), show);
    cout << endl;
}
```
C++提供基于范围的for循环：
```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
int main()
{
    vector<double> vt(10);
    for(int i = 0; i < 10; i++)
        vt[i] = i * 2 + 1;

    for(double x : vt)//for(auto x : vt)
        show(x);
    cout << endl;
}
```
模板使算法独立于存储的数据类型
迭代器使算法独立于使用的容器类型

STL遵循规则：
每个容器类（vector，list，deque等）定义了相应的迭代器类型。对于其中某个类，迭代器可能是指针（如string类的迭代器是`char *`，尽管string类不是STL容器），也可能是对象。
不管实现方式如何，迭代器都将提供所需的操作，如接触解除引用operator*，和自增operator++。
其次，每个容器类都有一个超尾标记。当迭代器递增到超越容器的最后一个值后，这个值将被赋值给迭代器。每个容器类都有begin和end方法分别返回指向容器第一个和超尾的迭代器。每个容器都使用++操作，从而遍历容器中每一个元素。


迭代器的类型：
输入迭代器：单向迭代器，只能递增，不能倒退。可读取容器中的元素，从头至尾遍历一次容器所有元素（operator++）。
输出迭代器：单向迭代器，只能递增，不能倒退。只能修改容器中元素但是不能读取。
正向迭代器：可以多次正向遍历容器中的所有元素，可读可写。
双向迭代器：支持正向迭代器所有特征，可双向遍历容器中所有元素。
随机访问迭代器：支持双向迭代器所有操作，并支持随机访问（指针增加和减少等）。

随机访问迭代器特点：
a,b为迭代器，n为整数，r为迭代器引用

|        |                          |
| ------ | ------------------------ |
| 表达式 | description              |
| a+n    | 指向a所指元素后第n个元素 |
| n+a    | 同上                     |
| a-n    | 指向a所指元素第n个元素   |
| r+=n   | 等价于r = r + n          |
| r-=n   | 等价于r = r - n          |
| a[n]   | 等价于*(a+n)             |
| b-a    | 结果为这样的n:b = a+n    |
| a<b    | b-a>0为真                |
| a>b    | b-a<0为真                |
| a>=b   | b-a>=0为真               |
| a<=b   | b-a>=0为真               |

迭代器层次结构：从正向迭代器开始，后者都包含前者功能，并增加新功能。

|                      |      |      |      |      |      |
| -------------------- | ---- | ---- | ---- | ---- | ---- |
| 迭代器功能           | 输入 | 输出 | 正向 | 双向 | 随机 |
| 解除引用读取`n = *a` | 1    | 0    | 1    | 1    | 1    |
| 解除引用写入`*a = n` | 0    | 1    | 1    | 1    | 1    |
| 固定和可重复排序     | 0    | 0    | 1    | 1    | 1    |
| ++i和i++             | 1    | 1    | 1    | 1    | 1    |
| --i和i--             | 0    | 0    | 0    | 1    | 1    |
| i[n]                 | 0    | 0    | 0    | 0    | 1    |
| i+n                  | 0    | 0    | 0    | 0    | 1    |
| i-n                  | 0    | 0    | 0    | 0    | 1    |
| i+=n                 | 0    | 0    | 0    | 0    | 1    |
| i-=n                 | 0    | 0    | 0    | 0    | 1    |

vector使用随机访问迭代器，list使用双向迭代器。指针是随机访问迭代器

非成员sort函数对迭代器区间的元素进行排序：
`sort(begin,end,compare)`使用第三个参数的函数对象进行排序，类似Vector的成员函数sort
`sort(begin,end)`默认从小到大排序，需容器中的元素支持operator<。

copy函数：
```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
void show(const double a)
{
    cout << a << "  ";
}
int main()
{
    vector<double> vt(10);
    for(int i = 0; i < 10; i++)
        vt[i] = i * 2 + 1;
    vector<double> vt2(10);

    for(double x : vt)
        show(x);
    cout << endl;
    for(double x : vt2)
        show(x);
    cout << endl;
    copy(vt.begin(),vt.end(),vt2.begin());
    for(double x : vt2)
        show(x);
    cout << endl;
}
```