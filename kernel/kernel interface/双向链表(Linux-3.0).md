# 双向链表(Linux-3.0)

## 定义

linux-3.0/include/linux/list.h:

```cpp
struct list_head {
    struct list_head *next, *prev;
};
```

虽然名称list_head，但是它既是双向链表的表头，也代表表尾。

## 初始化

```cpp
#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}
```

LIST_HEAD的作用是定义表头(节点)：新建双向链表表头name，并设置name的前继节点和后继节点都是指向name本身。
LIST_HEAD_INIT的作用是初始化节点：设置name节点的前继节点和后继节点都是指向name本身。
INIT_LIST_HEAD和LIST_HEAD_INIT一样，是初始化节点：将list节点的前继节点和后继节点都是指向list本身。

## 添加节点

```cpp
static inline void __list_add(struct list_head *new,struct list_head *prev,struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}
```

当链表仅有一个节点head时:head的prev和next域都指向自己，调用list_add函数和list_add_tail函数，结果为head->next=new,head->prev=new,new->next=head,new->prev=head。

当链表中有多个节点时：list_add函数将new节点插入到head后，list_add_tail函数将new节点插入到head前。

## 删除节点

```cpp
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void __list_del_entry(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void list_del_init(struct list_head *entry)
{
    __list_del_entry(entry);
    INIT_LIST_HEAD(entry);
}
```

`__list_del(prev, next)` 和`__list_del_entry(entry)`都是linux内核的内部接口。
`__list_del(prev, next) `的作用是从双链表中删除prev和next之间的节点。
`__list_del_entry(entry)` 的作用是从双链表中删除entry节点。

list_del(entry) 和 list_del_init(entry)是linux内核的对外接口。
list_del(entry) 的作用是从双链表中删除entry节点。
list_del_init(entry) 的作用是从双链表中删除entry节点，并将entry节点的前继节点和后继节点都指向entry本身。

## replace

```cpp
static inline void list_replace(struct list_head *old,
                struct list_head *new)
{
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}
```

list_replace(old, new)的作用是用new节点替换old节点。

## 判空

```cpp
static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}
```

list_empty(head)的作用是判断双链表是否为空。它是通过区分"表头的后继节点"是不是"表头本身"来进行判断的。

## 获取节点

```cpp
#define list_entry(ptr, type, member) \    container_of(ptr, type, member)
```

list_entry(ptr, type, member) 实际上是调用的container_of宏。
它的作用是：根据"结构体(type)变量"中的"域成员变量(member)的指针(ptr)"来获取指向整个结构体变量的指针。

## 遍历

```cpp
#define list_for_each_entry(pos, head, member)              \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
         &pos->member != (head);    \
         pos = list_entry(pos->member.next, typeof(*pos), member))
```

```cpp
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)
```

以input子系统中handler与device匹配过程为例：

```cpp
list_for_each_entry(dev, &input_dev_list, node)
```

展开：

```cpp
for(dev=list_entry(&input_dev_list->next,dev,node) ;   
      &dev->node!=input_dev_list; 
      dev=list_entry(dev->node.next;dev;node))
```

根据container_of宏：
for(dev=根据node和input_dev_list链表中第一个节点找到对应dev结构体首地址；&dev->node!=input_dev_list；dev=dev->node.next)
即遍历链表。dev为每次遍历时的变量地址。

这个宏就是遍历head链表中所有的member,并得出其所在的pos结构体的地址。

```cpp
#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = list_entry((head)->next, typeof(*pos), member),  \
        n = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                    \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))
```

这个宏和list_for_each_entry原理上是一样的，其主要区别是：

```cpp
list_for_each_entry：
                for(ptr=head->next;ptr!=head;ptr=ptr->next)
list_for_each_entry_safe:
                for(ptr=head->next,str=ptr->next;ptr!=head;ptr=str,str=str->next)
```

