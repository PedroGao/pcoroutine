# ucontext 使用

简单说来，getcontext 获取当前上下文，setcontext 设置当前上下文，
swapcontext 切换上下文，makecontext 创建一个新的上下文。

实现用户线程的过程是：

1. 我们首先调用getcontext获得当前上下文
2. 修改当前上下文ucontext_t来指定新的上下文，如指定栈空间极其大小，设置用户线程执行完后返回的后继上下文（即主函数的上下文）等
3. 调用makecontext创建上下文，并指定用户线程中要执行的函数
4. 切换到用户线程上下文去执行用户线程（如果设置的后继上下文为主函数，则用户线程执行完后会自动返回主函数）。

## 资料

* https://blog.csdn.net/qq910894904/article/details/41911175
