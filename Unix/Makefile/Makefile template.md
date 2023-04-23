# Makefile template

```makefile
#--------------------------------------------------------标明交叉编译器平台，编译所需的交叉编译工具链
COMPILE_PREFIX =
AR = $(COMPILE_PREFIX)ar
RANLIB = $(COMPILE_PREFIX)ranlib
CC = $(COMPILE_PREFIX)gcc
CXX = $(COMPILE_PREFIX)g++
LINK = $(COMPILE_PREFIX)gcc
#--------------------------------------------------------集合所有目标文件包括C文件和头文件
#SOURCES = $(wildcard ./*.c)
#VAR = $(shell ls -l | grep ^d | awk ' {print $9}')
#进行深度为3的目录遍历，找到所有目录
VAR = $(shell find . -maxdepth 3 -type d)
#VAR = $(shell find . -maxdepth 3 -type f)


#获取所有目录下的C文件
SOURCES = $(foreach file, $(VAR),$(wildcard $(file)/*.c))
#$(warning $(SOURCES))


#获取所有目录下的头文件
HEADERS = $(wildcard ./*.h)
HEADERS += $(foreach header, $(VAR), $(wildcard $(header)/*.h))


#指定头文件所在目录
INCLUDES = $(foreach includes, $(VAR), -I$(includes))
#中间目标.o文件集合
OBJS = $(patsubst %.c,%.o,$(SOURCES))
INCLUDE = $(patsubst %.c,%.d,$(SOURCES))
#移除main.o
#OBJS := $(subst main.o ,,$(OBJS))
#--------------------------------------------------------设置环境变量
DFLAG = -g -O2 -Wall
CFLAG = -DCDEFINE


CFLAGS += $(DFLAG) $(CFLAG) $(INCLUDES)


#--------------------------------------------------------设置其他变量
DYNAMICLIB = libtest.a
SHAREDLIB = libtest.so
TARGET = main
LIB_DIR_NAME = output_lib
LIB_FILES_PLACE = $(shell pwd)/$(LIB_DIR_NAME)
#--------------------------------------------------------编译提示信息来自Linux内核
squote := \'
cmd = @$(echo-cmd) $(cmd_$(1))
escsq = $(subst $(squote),'$(squote)',$1)
#echo-cmd = $(if $($(quiet)cmd_$(1)),\
        echo '  $(call escsq,$($(quiet)cmd_$(1)))$(echo-why)';)
echo-cmd = $(if $(quiet_cmd_$(1)),\
        echo '  $(call escsq,$(quiet_cmd_$(1)))$(echo-why)';)
#echo_cmd = echo $(quiet_cmd_$(1))


#之前我把cmds命名为VAR会出现很奇怪的错误？？？
cmds = $(info $(1)  $(2))
#.PHONY: clean
# $@代表目标文件 $^代表所有的依赖文件 $<代表第一个依赖文件 $?代表当前目标所依赖的文件列表中比当前目标文件还要新的文件 $*不包括后缀名的当前依赖文件的名字
#all: create_dir lib.a($(OBJS)) 后者的出现Makefile会执行隐含规则：.c -> .o ; $(AR) rv lib.a $*.o;
#-忽略错误,@取消打印
#--------------------------------------------------------设置总目标:可执行程序
all: $(TARGET)




#dynamic_lib: $(OBJS)
#    @$(shell mkdir $(LIB_FILES_PLACE))
#    $(AR) rcs $(LIB_FILES_PLACE)/$(DYNAMICLIB) $^
#    $(RANLIB) $(LIB_FILES_PLACE)/$(DYNAMICLIB)


$(TARGET): $(OBJS)
    $(call cmds,hello,hi)
    $(call cmd,link_link)


#shared_lib: CFLAGS += -fPIC
#shared_lib: $(OBJS)
#    @$(shell mkdir $(LIB_FILES_PLACE))
#    $(CC) -shared $^ -o $(LIB_FILES_PLACE)/$(SHAREDLIB)


$(OBJS):%.o:%.c
    $(call cmd,cc_o_c)


quiet_cmd_cc_o_c = CC      $@
      cmd_cc_o_c = $(CC) $(CFLAGS) -c $< -o $@




quiet_cmd_link_link = LD      $@
      cmd_link_link = $(LINK) -o $@ $^ $(CFLAGS)


%d:%c
    set -e; rm -f $@; \
    $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
    rm -f $@.$$$$


#这个要放下面
include $(SOURCES:.c=.d)
#当Makefile遇到依赖为.o时，默认会执行：
#在.o文件对应路径找到.c文件，执行$(CC) -c $(CPPFLAGS) $(CFLAGS)"


#将所有.c文件编译成.o文件
#$(OBJS):%.o:%.c
#    $(CC) $(CFLAGS) -c $< -o $@
#    $(foreach files,$(SOURCES),$(shell $(CC) $(CFLAGS)  -c $(files) -o $(patsubst %.c,%.o,$(files))))


#$(OBJS):%.o:%.c
#    $(CC) $(CFLAGS) -fPIC -c $< -o $@
#    #$(foreach files,$(SOURCES),$(shell $(CC) $(CFLAGS) -fPIC  -c $(files) -o $(patsubst %.c,%.o,$(files))))


#define clean_dir
#    $(shell if [ -d $(LIB_FILES_PLACE) ] ; then rm -rf $(LIB_FILES_PLACE) ; fi;)
#endef


clean:
    if [ -d $(LIB_FILES_PLACE) ] ; then rm -rf $(LIB_FILES_PLACE) ; fi;
    -rm $(OBJS) $(INCLUDE) $(TARGET)
#   $(call clean_dir)



```

