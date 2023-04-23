# iostream类
### **#派生关系**
iostream类基于istream和ostream类，而istream和ostream类派生于ios类（ios类包含一个指向streambuf的指针，streambuf用于管理输入输出缓冲区），ios类基于ios_base类
istream模板：`basic_istream<charT,traits<charT>>`，istream类是此模板基于char的一个typedef
ostream模板：`basic_ostream<charT,traits<<charT>>`,ostream类是此模板基于char的一个typdef
wistream和wostream是wchar_t的具体化，用来处理更宽的字符类型。

**iostream头文件中自动创建的8个对象**
cin：标准输入流，默认关联到标准输入设备（一般是键盘）,wcin类似但处理wchar_t类型，行缓冲
cout：标准输出流，默认关联到标准输出设备（一般是屏幕），wcout类似但处理wchar_t类型，行缓冲
cerr：标准错误流，默认关联到标准输出设备（一般是屏幕），wcerr类似但处理wchar_t类型，全缓冲
clog：标准错误流，wclog类似，相比wcerr这个流被缓冲

## **cout**
1.重载<<运算符
cout对象为ostream类对象，ostream类重载<<用于C++的所有基本类型，同时还包含字符串指针的输出重载，可通过`man std::ostream`查看。
可拼接输出，因为<<重载返回了ostream类对象的引用： `ostream& operator<<(type)`
2.常用ostream类方法：
```cpp
ostream &put(char)
cout.put('A').put('B')<<std::endl;
```
类似C中的putchar函数
```cpp
basic_ostream<charT,traits>& write(const char_types *s,streamsize n)
ostream& write(const char *str, size_t n)//char的具体化
```
类似C函数中的`fwrite`以及Linux中的`wirte`系统调用，输出str中的n个字节，显示为ASCII码

刷新输出缓冲区：（常用换行符和`std::endl`刷新输出缓冲区，将数据立刻显示到屏幕)
```cpp
flush(cout);
cout<<flush;
```
类似C语言中的:
```cpp
fflush(stdout);
```

cout的格式化输出：
1.浮点数显示6位有效位，后面的0不显示
2.根据继承关系，ostream类对象可以使用ios和ios_base类的方法以及公有成员
```cpp
//整型的cout显示--控制符hex，oct，dec：
cout<<hex;
hex(cout);//将以十六进制显示整型。
//控制符位于std中
```
3.调整字段宽度：
成员函数：
```cpp
int width();//返回当前设置
int width(int i);//设置字段宽度为i个空格，并返回以前的设置
//width方法只会影响到下一次输出，随后恢复默认值
```

4.填充字符：
```cpp
cout.fill(char ch);//通常结合width使用，将width填充的空格使用ch填充，一直生效
```
5.浮点数显示精度：
```cpp
//浮点数默认显示精度为6位，precision函数可改变精度，一直生效
cout.precision(2);//设置精度为2位。
```
6.打印浮点数后的0:
```cpp
cout.setf(ios_base::showpoint);
```

setf方法：
```cpp
fmtflags setf(fmtflags);
//返回原来的设置，传入值为一些位值常量（某一位为1），一般使用以下常量
```

|                     |                                     |
| ------------------- | ----------------------------------- |
| 常量                | 含义                                |
| ios_base::boolalpha | 输入输出bool值，可以是ture或false   |
| ios_base::showbase  | 输出使用c++基数前缀(0,0x)           |
| ios_base::showpoint | 显示末尾小数点和0                   |
| ios_base::uppercase | 对十六进制输出，使用大写字母E表示法 |
| ios_base::showpos   | 在正数前加上+                       |

```cpp
setf被重载后的第二种形式：
fmtflags setf(fmtflags, fmtflags);
返回原来的设置，第一个参数为想要设置的位，第二个参数为想要清除的位。
一般第一个参数一第二个参数的关系：第一个参数为一组bitmask类型常量中的一个，第二个参数清楚这组常量所设置的所有位
```
|                       |                      |                              |
| --------------------- | -------------------- | ---------------------------- |
| param2                | param1               | means                        |
| ios_base::basefield   | ios_base::dec        | 使用基数10，10进制打印       |
| ios_base::basefield   | ios_base::oct        | 使用基数8，8进制打印         |
| ios_base::base::field | ios_base::hex        | 使用基数16，16进制打印       |
| ios_base::floatfield  | ios_base::fixed      | 定点计数法                   |
| ios_base::floatfield  | ios_base::scientific | 科学计数法                   |
| ios_base::adjustfield | ios_base::left       | 左对齐                       |
| ios_base::adjustfield | ios_base::right      | 右对齐                       |
| ios_base::adjustfield | ios_base::internal   | 符号基数前缀左对齐，值右对齐 |

example:
```cpp
ios_base::fmtflags old = cout.setf(ios_base::left, ios_base::adjustfield);//set
cout.setf(old, ios_base::adjustfield);//recover
```
调用setf后可以使用unsetf恢复，
```cpp
void unsetf(fmtflags mask);
cout.setf(ios_base::showpoint);//set
cout.unsetf(ios_base::showpoint);//unset
cout.setf(0, ios_base::floatfield);//恢复fixed,scientfic的默认
cout.unsetf(ios_base::floatfield);//同上
```
标准控制符：
可以直接使用控制符与cout << 连接使用达到setf的效果，如：
```cpp
cout << hex << left;
```
|             |                                                  |
| ----------- | ------------------------------------------------ |
| control     | setf/unsetf                                      |
| boolalpha   | setf(ios_base::boolalpha)                        |
| noboolalpha | unsetf(ios_base::boolalpha)                      |
| showbase    | setf(ios_base::showbase)                         |
| noshowbase  | unsetf(ios_base::showbase)                       |
| showpoint   | setf(ios_base::showpoint)                        |
| noshowpoint | unsetf(ios_base::showpoint)                      |
| showpos     | setf(ios_base::showpos)                          |
| noshowpos   | unsetf(ios_base::showpos)                        |
| uppercase   | setf(ios_base::uppercase)                        |
| nouppercase | unsetf(ios_base::uppercase)                      |
| internal    | setf(ios_base::internal, ios_base::adjustfield)  |
| left        | setf(ios_base::left, ios_base::adjustfield)      |
| right       | setf(ios_base::right, ios_base::adjustfield)     |
| dec         | setf(ios_base::dec, ios_base::basefield)         |
| hex         | setf(ios_base::hex, ios_base::basefield)         |
| oct         | set(ios_base::oct, ios_base::basefield)          |
| fixed       | set(ios_base::fixed, ios_base::floatfield)       |
| scientific  | setf(ios_base::scientific, ios_base::floatfield) |


其他控制：
```cpp
#include<iomanip>
setprecision(3);//精度
setfill('h');//填充字符
setw(12);//字段宽度
g++ -lm//链接数学库
```
## cin
```cpp
istream & operator>>(int &)
```
istream类重载所有基本类型。另外同cout，如`char *, unsigned char *, signed char *`也重载了>>抽取运算符
控制符连用：
```cpp
cin >> hex ;//接下来的输入被认为十六进制
```
输入流状态：
cin和cout包括一个描述流状态的数据成员（继承于ios_base类，类型位iostate类型，实际是bitmask类型）
流状态由三个ios_base元素组成：eofbit, failbit, badbit。
cin读取到文件尾：eofbit.
cin未能读取到预期字符如`int a, cin >> a;//输入abc回车`：failbit.
cin出现无法诊断的错误：badbit.

流状态相关:

|                        |                                                              |
| ---------------------- | ------------------------------------------------------------ |
| member                 | description                                                  |
| eofbit                 | 达到文件尾，设置为1                                          |
| badbit                 | 流被破坏，设置为1，如读取文件错误                            |
| failbit                | 输入操作未能读取到预期字符，或输出操作没有写入预期字符，设置为1 |
| goodbit                | 正常情况下，用于表示流状态正常                               |
| good()                 | 如果流可以使用（所有位都被清除），则返回true                 |
| eof()                  | eofbit被设置返回true                                         |
| bad()                  | badbit被设置返回true                                         |
| fail()                 | failbit或badbit被设置返回true                                |
| rdstate()              | 返回流状态                                                   |
| exceptions()           | 返回一个位掩码，指出那些标记导致异常被引发                   |
| exceptions(iostate ex) | 设置哪些状态将导致clear()引发异常；例如，如果ex是eofbit，则如果eofbit被设置，clear()将引发异常 |
| clear(iostate s)       | 将流状态设置为s，s的默认值为0(goodit)，如果rdstate() & exceptions() != 0 ，则引发异常basic_ios::failure |
| setstate(iostate s)    | 调用clear(rdstate() 按位或 s)                                |

clear不带任何参数将清楚所有位：`clear()`
设置eofbit而清除其他位：`clear(eofbit)`
setate只影响设其参数中已经设置的位：`setstate(eofbit)`，将eofbit置1，但是不影响其他位

捕获异常：
```cpp
#incude <iostream>
#include <exception>

int main()
{
    using namespace std;
    cin.exceptions(ios_base::failbit);
    cout << "Enter numbers";
    int sum = 0;
    int input;
    try
    {
        while(cin >> input)//只有流状态良好时，才返回ture
        {
            sum += input;
        }
    }
    catch(ios_base::failure & bf)
    {
        cout << bf.what() << endl;
        cout << "input error\n";
    }
    cout << "Last value entered = " << input << endl;
    cout << "Sum = " << sum << endl;
    return 0;
}
```
上面程序捕获cin出现ios_base::failbit时的ios_base::failure异常。
流状态被设置后，流将对后面的输入输出关闭，直到位被清除：
```cpp
cin.clear();
while(cin.get() != '\n')//丢弃当前行所有输入
    continue;
while(cin.get() != ' ')
    continue;//仅仅丢弃下一个错误输入
```
处理流错误或文件尾导致的cin异常：
```cpp
if(cin.fail() && !cin.eof())//failbit错误
else if(cin.eof() && !cin.fail())//eofbit错误
```

其他istream类方法：
1.`istream & get(char &ch)`
从输入流中读取一个字符，赋值给ch，返回istream类对象，捕捉所有字符包括回车，空格。读取到eof或者ctrl+d(unix)时，`cin.get(ch)`返回false，同时failbit被置位
对比抽取运算符`>>`：
```cpp
char input;
cin >> input;
while(input != '\n')//由于>>会跳过空格和回车，while将无法退出
```
连用：
```cpp
char c1, c2, c3;
cin.get(c1).get(c2) >> c3;
```
2.`int get(void)`
`get()`从输入流中取出一个字符并返回，示例：
```cpp
char ch;
while((ch = cin.get()) != EOF)
{
    //input
}
```
两函数区别：

|              |             |                   |
| ------------ | ----------- | ----------------- |
| character    | cin.get(ch) | ch = cin.get()    |
| 返回值       | istream对象 | 字符编码(int类型) |
| 文件尾返回值 | false       | EOF               |

类似于C语言中的`ch = getchar()`
字符串输入： `getline(),get(),ignore()`
重载模板：
```cpp
istream & get(char *str, int n, char ch);//用分界符ch代替\n，分界符留在输入流中
istream & get(char *str, int n);//get将\n留在输入流中
istream & getline(char *ch, int n, char ch);//分界符ch代替\n，分界符留在输入流中
istream & getline(char *str, int n);//getline丢弃出入流中的\n
istream & ignore(int n1 = 1, int n2 = EOF)//从输入流中丢弃n1个字符或者遇到n2停止丢弃
```
example:
```cpp
char str[128];
cin.get(str, 20, 'a')//输入流中读取20个字符，或遇到字符a停止读取，但是a将留在输入流中
cin.get(str, 20);//输入流中读取20个字符，或遇到字符\n停止读取，但是\n将留在输入流中
cin.getline(str, 20, 'a')//输入流中读取20个字符，或遇到字符a停止读取，但是a将被丢弃，a既不在输入流也不在str中
cin.getline(str, 20)//输入流中读取20个字符，或遇到字符\n停止读取，但是\n将被丢弃，\n既不在输入流也不在str中
cin.ignore(255, '\n')//丢弃输入流中255个字符，或者遇到\n停止丢弃
```
`get`和`getline`函数：
遇到文件尾将设置eofbit，遇到流错误设置failbit。
如果不能抽取字符，将把空字符放到输入字符串，并使用setstate()设置failbit。
对于get，不能抽取字符可能是因为：1.输入方法立刻到达文件尾 2.输入一个空行
对getline，不能抽取字符可能因为：1.输入立刻到达文件尾 2.空行不会导致getline设置failbit，因为getline会抽取\n，虽然不保存。
getline空行终止：
```cpp
char temp[128];
while(cin.getline(tempp,128) && temp[0] != '\0')
```

|         |                                                              |
| ------- | ------------------------------------------------------------ |
| method  | behavior                                                     |
| getline | 没有读取任何字符（但是换行符被视为读取了一个字符），设置failbit。读取了最大数目的字符，但是行中还有字符，设置failbit。 |
| get     | 没有读取任何字符，设置failbit。读取最大数目字符后输入流还有其他字符不会导致failbit |

其他istream方法：
`read`从输入流读取128个字节而不是字符，并放入temp中。
```cpp
char temp[128];
cin.read(temp,128);
istream &read(char *,int n);
```
`peek`返回输入流下一个字符，但是字符仍存在于输入流中：
```cpp
int peek(void);
ch = cin.peek();
```
`putback`放入一个字符到输入流的开头：
```cpp
istream & putback(char ch);
//peek相当于如下：
int peek(void)
{
    char ch = cin.get();
    cin.putback(ch);
    return ch;
}
```