# string类
C++标准的string类是来自于basic_string模板类的一个typdef

```cpp
template<class charT, class traits = char _traits<charT>, class Allocator = allocator<charT>>
basic string
{
    ...
};
```
`typdedef basic_string<char> string`内存分配与释放默认使用new和delete
字符串长度的最大值：`string::npos`通常为`unsigned int`的最大值

string类的8个构造函数：
表格中使用NBTS表示空字符结尾的字符串

|                                                              |                                                              |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| constructor                                                  | description                                                  |
| `string(const char * s)`                                     | string初始化为s指向的NBTS                                    |
| `string(size_type n, char c)`                                | string初始化为包含n个c字符的字符串                           |
| `string(const string &str)`                                  | 拷贝构造函数                                                 |
| `string()`                                                   | 默认构造函数，长度为0                                        |
| `string(const char *s, size_type n)`                         | string初始化为s中的前n个字符组成的字符串                     |
| `template<class Iter> string(Iter begin, Iter end)`          | string初始化为[begin, end)区间的字符组成的字符串，Iter为指针或迭代器 |
| `string(const string &str, size_type pos = 0,size_type n = pos)` | string初始化为str的[0,pos]或[pos,n]区间组成的字符串          |
| `string(string &&str) noexcept`                              | 移动构造函数                                                 |
| `string(initializer_list<char> il)`                          | string初始化为il中的字符                                     |

eg:
```cpp
#include <string>
#include <iostream>
using namespace std;
int main()
{
    const char* cStr = "abcdefghijklmn";
    string strmove = "ABCDEFGHIJKLMN";
    string str1("ABCDEFGHIJKLMN");//1
    string str2(10, 'c');//2
    string str3(str1);//3
    string str4;//4
    string str5(cStr, 5);//5
    string str6(&cStr[1], &cStr[8]);//6
    string str7(str1, 1, 8);//7
    string str8(str1, 3);//7
    string str9(std::move(strmove));//8
    string str10{'a','b','c','d','e'};//9
    cout << str1 << endl;
    cout << str2 << endl;
    cout << str3 << endl;
    cout << str4 << endl;
    cout << str5 << endl;
    cout << str6 << endl;
    cout << str7 << endl;
    cout << str8 << endl;
    cout << str9 << endl;
    cout << str10 << endl;
}
./a.out:
ABCDEFGHIJKLMN
cccccccccc
ABCDEFGHIJKLMN

abcde
bcdefgh
BCDEFGHI
DEFGHIJKLMN
ABCDEFGHIJKLMN
abcde
```

自实现string类部分内容：
```cpp
#ifndef __STRING__H__
#define __STRING__H__

#include <cstring>
#include <iostream>
namespace st
{
    using namespace std;
    class string
    {
        //private
        private:
            char *str;
            size_t length;

        public:
            string();//
            string(const string &);//
            string(const char *);
            string(const size_t,const char);
            string(string&& st);
            ~string();
            size_t Length() const;// 字符串长度
            bool isEmpty();// 返回字符串是否为空
            const char* c_str();// 返回c风格的trr的指针
            friend ostream& operator<< (ostream&, const string&);
            friend istream& operator>> (istream&, string&);
            friend string operator+(const string&,const string&);
            friend bool operator==(const string&,const string&);
            friend bool operator!=(const string&,const string&);
            friend bool operator<(const string&,const string&);
            friend bool operator>(const string&,const string&);
            char& operator[](const size_t);
            const char& operator[](const size_t)const;
            string& operator=(const string&);
            string& operator=(string &&);
            string& operator+=(const string&);
            string substr(const size_t pos,const size_t n);
            string& append(const string&);
            string& insert(size_t,const string&);
            string& assign(string&,size_t,size_t);
            string& erase(size_t,size_t);
            int find_first_of(const char* str,size_t index=0);
            int find_first_of(const char ch,size_t index=0);
            int find_first_of(const string &,size_t index=0);
            int find_first_not_of(const char* str,size_t index=0);
            int find_first_not_of(const char ch,size_t index=0);
            int find_first_not_of(const string&,size_t index=0);
            void swap(string& lhs,string& rhs);
    };
    class out_of_bounds : public exception
    {
        public:
            virtual const char *what() const noexcept
            {
                cout << "out of bounds\n";
            }
    };
}
```
源文件：
```cpp
#include "string.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <exception>
using namespace std;
const int npos = -1;
namespace st
{
    //default constructor
    string::string() : str(nullptr), length(0)
    {
        cout << "calling default constructor\n";
    }

    //constructor
    string::string(const char *s)
    {
        cout << "call string(const char *s)\n";
        if(nullptr == s)
        {
            str = nullptr;
            length = 0;
            return ;
        }
        length = strlen(s);
        str = new char[length + 1];
        for(int i = 0; i < length; i++)
            str[i] = s[i];
        str[length] = '\0';
    }
    //constructor
    string::string(size_t len, const char ch)
    {
        cout << "calling string(size_t, const char)\n";
        length = len;
        str = new char[length + 1];
        for(int i = 0; i < length; i++)
            str[i] =ch;
        str[length] = '\0';
    }

    //copy constructor
    string::string(const string& st)
    {
        cout << "calling copy constructor\n";
        if(nullptr == st.str)
        {
            str = nullptr;
            length = 0;
            return ;
        }
        length = st.length;
        str = new char[length + 1];
        for(int i = 0; i < length; i++)
            str[i] = st.str[i];
        str[length] = '\0';
    }
    //move constructor
    string::string(string&& st)
    {
        cout << "calling move constructor\n";
        str = st.str;
        length = st.length;
        st.str = nullptr;
        st.length = 0;
    }

    //delete constructor
    string::~string()
    {
        cout << "calling delete constructor\n";
        delete[] str;
        length = 0;
    }

    size_t string::Length() const
    {
        return length;
    }

    bool string::isEmpty()
    {
        return length == 0;
    }

    const char* string::c_str()
    {
        return str;
    }

    ostream& operator<<(ostream& os,const string& st)
    {
        cout << "call operator<<(ostream& os,const string& st)\n";
        if(nullptr != st.str)
        {
            os << st.str;
        }
        return os;
    }

    istream& operator>>(istream& is, string& st)
    {
        char tmp[1024];
        if(is >> tmp)
        {
            st.length = strlen(tmp);
            delete[] st.str;
            st.str = new char[st.length + 1];
            for(int i = 0; i < st.length; i++)
                st.str[i] = tmp[i];
            st.str[st.length] = '\0';
        }
        return is;
        }

    string operator+(const string& st1, const string& st2)
    {
        cout << "calling operator+(const string& st1, const string& st2)\n";
        string s;
        s.length = st1.length + st2.length;
        s.str = new char[s.length + 1];
        int i;
        for(i = 0; i < st1.length; i++)
            s.str[i] = st1.str[i];
        for( ; i < s.length; i++)
            s.str[i] = st2.str[i - st1.length];
        s.str[i] = '\0';
        return s;
    }

    bool operator==(const string& st1, const string& st2)
    {
        return strcmp(st1.str, st2.str) == 0;
    }

    bool operator!=(const string& st1, const string& st2)
    {
        return !(st1 == st2);
    }

    bool operator<(const string& st1, const string& st2)
    {
        return strcmp(st1.str, st2.str) < 0;
    }

    bool operator>(const string& st1, const string& st2)
    {
        return !(st1.str < st2.str);
    }

    char& string::operator[](size_t index)
    {
        if(index < 0 || index >= length)
        {
            throw out_of_bounds();
        }
        return str[index];
    }

    const char& string::operator[](size_t index) const
    {
        if(index < 0 || index >= length)
        {
            throw out_of_bounds();
        }
        return str[index];
    }
    string& string::operator=(const string& st)
    {
        cout << "calling assignment operator\n";
        if(this == &st)
            return *this;
        delete[] str;
        length = st.length;
        str = new char[length + 1];
        for(int i = 0; i < length; i++)
            str[i] =st.str[i];
        str[length] = '\0';
        return *this;
    }

    string& string::operator=(string&& st)
    {
        cout << "calling move assignment operator\n";
        if(this == &st)
            return *this;
        str = st.str;
        length = st.length;
        st.str = nullptr;
        st.length = 0;
        return *this;
    }

    string& string::operator+=(const string& st)
    {
        cout << "call string::operator+=(const string& st)\n";
        if(st.str == nullptr)
            return *this;
        char *tmp = new char[length + st.length + 1];
        int i;
        for(i = 0; i < length; i++)
            tmp[i] = str[i];
        for( ; i < length + st.length; i++)
            tmp[i] = st.str[i - length];
        tmp[i] = '\0';
        delete[] str;
        str = tmp;
        tmp = nullptr;
        length += st.length;
        return *this;
    }

    string string::substr(const size_t pos, const size_t n)
    {
        if( n + pos >= length)
            throw out_of_bounds();
#if 0
        string st;
        st.str = new char[n + 1];
        st.length = n;
        for(int i = pos; i < pos + n; i++)
            st.str[i - pos] = str[i];
        st.str[st.length] = '\0';
        return st;
#endif
        char *tmp = new char[n + 1];
        for(int i = 0; i < n; i++)
            tmp[i] = str[i + pos];
        tmp[n] = '\0';
        return string(tmp);
    }

    string& string::append(const string& st)
    {
        *this += st;
        return *this;
    }

    string& string::insert(size_t pos, const string& st)
    {
        if(pos < 0 || pos >= length)
            throw out_of_bounds();
        if(nullptr == st.str)
            return *this;
#if 0
        char *tmp = str;
        size_t len = length;
        length = len + st.length;
        str = new char[st.length + len + 1];
        int i;
        for(i = 0 ; i < pos; i ++)
            str[i] = tmp[i];
        for(; i < st.length; i++)
            str[i] = st.str[i];
        for(; i < length; i++)
            str[i] = tmp[i - st.length];
        delete[] tmp;
        return *this;
#endif
        string st1 = substr(0, pos);
        string st2 = substr(pos, length - pos);
        *this = st1 + st + st2;
        return *this;
    }
    string& string::assign(string& st, size_t pos, size_t n)
    {
        assert(pos > 0 && pos < length);
        assert(pos + n < st.length);

        if( n > length - pos)
        {
            char *tmp = str;
            str = new char[n + pos + 1];
            for(int i = 0; i < length; i ++)
                str[i] = tmp[i];
            length = n + pos;
            delete[] tmp;
        }
        for(int i = pos; i < pos + n; i++)
            str[i] = st.str[i];
        str[length] = '\0';
        return *this;
    }

    string& string::erase(size_t pos, size_t n)
    {
        assert( pos < length);
        size_t erase_len = n + pos > length ? length - pos : n;
        char * tmp = str;
        str = new char[length - erase_len + 1];
        int i;
        for(i = 0; i < pos; i++)
        {
            str[i] = tmp[i];
        }
        for(i = pos + erase_len; i < length; i++)
            str[i] = tmp[length - (pos + erase_len)];
        length -= erase_len;
        str[length] = '\0';
    }

    int string::find_first_of(const char* st, size_t index)
    {
        if(nullptr == st || index >= length || strlen(st) + index >= length)
            return npos;
        size_t len = strlen(st);
        bool flag = false;
        size_t i = 0, j = 0, k = 0;
        i = index;
        while(i < length - strlen(st))
        {
            k = i;
            j = 0;
            while(1)
            {
                if(str[k++] == st[j++])
                {
                    if(j == len)
                    {
                        flag = true;
                        break;
                    }
                    continue;
                }
                else
                    break;
            }
            if(flag == false)
                i++;
            else
                break;
        }
        if(flag == true)
            return i;
        return npos;
    }

    int string::find_first_of(const char ch, size_t index)
    {
        if(index >= length)
            return npos;
        for(int i = 0; i < length; i++)
        {
            if(str[i] == ch)
                return i;
        }
        return npos;
    }

    int string::find_first_of(const string& st, size_t index)
    {
        return find_first_of(st.str, index);
    }
    void string::swap(string &st1, string& st2)
    {
        st1.length ^= st2.length;
        st2.length ^= st1.length;
        st1.length ^= st2.length;
        char *tmp;
        tmp = st1.str;
        st1.str = st2.str;
        st2.str = tmp;
    }
}

```
```cpp
#include "string.h"
#include <iostream>
using namespace std;
int main()
{
    st::string str1;
    cout << "=======" << str1 <<endl;
    st::string str2("hello world");
    cout << "=======" << str2 << endl;
    st::string str3(str2);
    cout << "=======" << str3 << endl;
    st::string str4(10,'a');
    cout << "=======" << str4 << endl;
    st::string str5(std::move(str2 + str3));
    cout << "=======" << str5 <<endl;
    const char *h = "hello";
    const char *w = " world";
    st::string str6 = st::string(h) + st::string(w);
    cout << "=======" << str6 << endl;
    st::string str7;
    str7 = str2;
    cout << "=======" << str2 << endl;
    st::string str8(str2+str3);
    cout << "=======" << str8 << endl;
}

```

标准string类的find方法时间复杂度较低：

|                                                              |                                                              |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| prototype                                                    | description                                                  |
| `size_type find(const string &str, size_type pos = 0) const` | 从pos开始查找子串str，找到返回首次出现首字符的索引，否则返回`string::nopos` |
| `size_type find(const char *s, size_type pos = 0) const`     | 同上，只是string对象变为`char *`对象                         |
| `size_type find(const char *s, size_type pos = 0, size_type n) const` | 从pos开始查找s的前n个字符组成的子字符串，找到返回首次出现首字符的索引，否则返回`string::nopos` |
| `size_type find(char ch, size_type pos = 0)`                 | 从pos开始查找字符ch，找到返回首次出现字符的索引，否则返回`string::nopos` |