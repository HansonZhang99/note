# fstream类
在头文件fstream类中，包含了ifstream和ofstream类。派生于头文件iostream中的类，可以使用iostream中的类的方法。

写入一个文件：
1.创建一个ofstream对象管理输出流。
2.将该对象与特定文件关联起来。
3.以使用cout的方式使用该对象，唯一的区别是输入进入文件而不是屏幕。
```cpp
#include <fstream>
ofstream fout;
fout.open("test.txt");
//ofstream fout("test.txt");
fout << "hello world\n";
fout.close();
```
ofstream派生自ostream类，可以使用ostream重载的插入运算符，格式化方法和控制符。
ofstream使用被缓冲的输出，创建对象会创建缓冲区，缓冲区填满会写入文件。

读取一个文件：
1.创建一个ifstream对象管理输入流。
2.将该对象与特定文件关联。
3.以使用cin的方式使用该对象。
```cpp
#include <fstream>
ifstream fin;
fin.open("test.txt");
//ifstream fin("test.txt");
char ch;
fin >> ch;
char buf[128];
fin >> buf;
fin.getline(buf,128);
string line;
fin >> line;
getline(fin.line);
fin.close();
```
检查流的状态：`is_open()`
文件流从ios_base继承了一个流状态成员，存储流失败，文件尾，正常的状态。
当试图打开一个不存在的文件进行输入时，将设置failbit位，可使用如下方式检查：
```cpp
fin.open("test.txt");
if(fin.fail())//if(!fin)   if(!fin.is_open())  if(!fin.good())
{
    //...
}
```
打开多个文件，一个ifstream对象的多次复用：
```cpp
ifstream fin;
fin.open("test1.txt");
...
fin.close();
fin.open("test2.txt");
...
fin.close();
fin.open("test3.txt");
...
fin.close();
```

 文件打开方式：
 `ifstream fin("test.txt", mode);`
 `ofstream fout; fout.open("test.txt", mode);`
mode是一个bitmask类型的参数，类似fmtflags和iostate

|                  |                                |
| ---------------- | ------------------------------ |
| 常量             | 含义                           |
| ios_base::in     | 打开文件用于读取               |
| ios_base::out    | 打开文件用于写入               |
| ios_base::ate    | 打开文件，移动到文件尾         |
| ios_base::app    | 追加到文件尾                   |
| ios_base::trunc  | 如果文件存在，截短（清空）文件 |
| ios_base::binary | 二进制打开                     |

example：
```cpp
ifstream fin;
fin.open("test.txt",ios_base::in);
ofstream fout;
fout.open("test.txt",ios_base::out|ios_base::app);
```

二进制读取/写入和字符读取/写入：
```cpp  
ifstream fin;
fin.open("test1.txt", ios_base::in);
char temp[1024];
fin >> temp;
fin.read(temp, 1024);
ofstream fout;
fout.open("test2.txt", ios_base::out|ios_base::app);
fout << temp;
fout.write(temp, 1024);
fin.close();
fout.close();
```

定位到文件某一处：
```cpp
istream &seekg( streamoff, ios_base::seekdir);
istream &seekg( streampos);
ostream &seekp( streamoff, ios_base::seekdir);
ostream &seekp( streamoff);
```

`ios_base::beg`相对于文件开始处的偏移量
`ios_base::cur`相对当前位置的偏移量
`ios_base::end`相对文件尾偏移量
```cpp
fin.seekg(30, ios_base::beg);
fin.seekg(-1, ios_base::cur);
fin.seekg(-20, ios_base::end);
```

`tellg()`和`tellp()`返回当前文件指针的位置（从文件头开始算的字节数）
类似于lseek和fseek，但是lseek/fseek直接返回指针前的字节数

程序示例：
```cpp
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>

inline void eatline()
{
    while(std::cin.get() != '\n')
        continue;
}

struct planet
{
    char name[20];
    double population;
    double g;
};

const char * file = "planets.dat";

int main()
{
    using namespace std;
    planet pl;
    cout << fixed << right;
    ifstream fin;
    fin.open(file, ios_base::in | ios_base::binary);
    if(fin.is_open())
    {
        cout << "Here are the current contents of the " << file << " file:\n";
        while(fin.read((char *)&pl, sizeof(pl)))
        {
            cout << setw(30) << pl.name << ": "
                << setprecision(0) << setw(20) << pl.population
                << setprecision(2) << setw(6) <<pl.g << endl;
        }
        if(fin.eof())
            fin.clear();
        fin.seekg(0, ios_base::end);
        cout << fin.tellg() << endl;
        fin.close();
    }
    ofstream fout(file, ios_base::out | ios_base::app | ios_base::binary);
    if(!fout.is_open())
    {
        cerr << "Can't open " << file << " file for output:\n";
        exit(EXIT_FAILURE);
    }

    cout << "Enter the planet's name(Enter a blank line to quit):\n";
    cin.get(pl.name, 20);
    while(pl.name[0] != '\0')
    {
        eatline();
        cout << "Enter the population of the planet: ";
        cin >> pl.population;
        cout << "Enter planet;s acceleation of gravity: ";
        cin >> pl.g;
                eatline();
        fout.write((char *)&pl, sizeof(pl));
        cout << "Enter the planet's name(Enter a blank line to quit):\n";
        cin.get(pl.name, 20);

    }

    fout.close();

    fin.clear();

    fin.open(file, ios_base::in | ios_base::binary);
    if(fin.is_open())
    {
        cout << "Here are the current contents of the " << file << " file:\n";
        while(fin.read((char *)&pl, sizeof(pl)))
        {
            cout << setw(30) << pl.name << ": "
                << setprecision(0) << setw(20) << pl.population
                << setprecision(2) << setw(6) <<pl.g << endl;
        }
        fin.close();
    }

    cout << "Done\n";
    return 0;
}

```
```cpp
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>

const int LIM = 20;
struct planet
{
    char name[LIM];
    double population;
    double g;
};

const char *file = "planets.dat";
inline void eatline()
{
    while(std::cin.get() != '\n')
        continue;
}

int main()
{
    using namespace std;
    planet pl;
    cout << fixed;

    fstream finout;
    finout.open(file, ios_base::in | ios_base::out | ios_base::binary);
    int ct = 0;
    if(finout.is_open())
    {
        finout.seekg(0);
        cout << "Here are the current contents of the " << file << " file:\n";
        while(finout.read((char *) &pl, sizeof(pl)))
        {
            cout << ct++ << ": " << setw(20) << pl.name << ": "
                << setprecision(0) << setw(20) << pl.population
                << setprecision(2) <<setw(6) << pl.g << endl;
        }
        if(finout.eof())
            finout.clear();
        else
        {
            cerr << "Error in reading " << endl;
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        cerr << "Error in open" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Enter the record number you wish to change: ";
    long rec;
    cin >> rec;
    eatline();
    if(rec < 0 || rec >=ct)
    {
                cerr << "Invailid record number" << endl;
        exit(EXIT_FAILURE);
    }

    streampos place = rec * sizeof(pl);
    finout.seekg(place);
    if(finout.fail())
    {
        cerr << "Error on attempted seek\n";
        exit(EXIT_FAILURE);
    }
    finout.read((char *)&pl, sizeof(pl));
    cout << "your selection:\n";
    cout << rec << ": " << setw(20) << pl.name << ": "
            << setprecision(0) << setw(20) << pl.population
            << setprecision(2) <<setw(6) << pl.g << endl;
    if(finout.eof())
        finout.clear();
    cout << "Enter the planet name: ";
    cin.get(pl.name, 20);
    eatline();
    cout << "enter population: ";
    cin >> pl.population;
    cout << "enter the gravity: ";
    cin >> pl.g;
    finout.seekp(place);
    finout.write((char *)&pl, sizeof(pl)) <<flush;
    if(finout.fail())
    {
        cerr << "error write\n";
        exit(EXIT_FAILURE);
    }
    ct = 0;
    if(finout.is_open())
    {
        finout.seekg(0);
        cout << "Here are the current contents of the " << file << " file:\n";
        while(finout.read((char *) &pl, sizeof(pl)))
        {
            cout << ct++ << ": " << setw(20) << pl.name << ": "
                << setprecision(0) << setw(20) << pl.population
                << setprecision(2) <<setw(6) << pl.g << endl;
        }
        if(finout.eof())
            finout.clear();
        else
        {
            cerr << "Error in reading " << endl;
            exit(EXIT_FAILURE);
        }
    }

    finout.seekg(28, ios_base::beg);
    finout.seekp(30, ios_base::beg);
    cout << finout.tellg() << " | " << finout.tellp() << endl;

    finout.close();
    return 0;
}

```