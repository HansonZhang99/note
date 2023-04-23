# exit, _exit和return



`exit(0)`进程正常退出`exit(1)`进程异常退出正常异常是返回给操作系统的
`_exit`：不做清理工作，进程直接返回

main函数中的return和exit相同