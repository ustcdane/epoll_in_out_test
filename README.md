epoll_in_out_test
=================

 epoll各事件触发条件

简述Linux Epoll ET模式EPOLLOUT和EPOLLIN触发时刻:
ET模式称为边缘触发模式，顾名思义，不到边缘情况，是死都不会触发的。
EPOLLOUT事件：
EPOLLOUT事件只有在连接时触发一次，表示可写，其他时候想要触发，那你要先准备好下面条件：
1.某次write，写满了发送缓冲区，返回错误码为EAGAIN。
2.对端读取了一些数据，又重新可写了，此时会触发EPOLLOUT。

简单地说：EPOLLOUT事件只有在不可写到可写的转变时刻，才会触发一次，所以叫边缘触发，这叫法没错的！
其实，如果你真的想强制触发一次，也是有办法的，直接调用epoll_ctl重新设置一下event就可以了，event跟原来的设置一模一样都行（但必须包含EPOLLOUT），关键是重新设置，就会马上触发一次EPOLLOUT事件。
EPOLLIN事件：
EPOLLIN事件则只有当对端有数据写入时才会触发，所以触发一次后需要不断读取所有数据直到读完EAGAIN为止。否则剩下的数据只有在下次对端有写入时才能一起取出来了。
现在明白为什么说epoll必须要求异步socket了吧？如果同步socket，而且要求读完所有数据，那么最终就会在堵死在阻塞里。

If you want to read data, set EPOLLIN , and read data when it returns true, if reading returns EAGAIN, wait till next time.
If you have data to send, send it. If sending returns EAGAIN and/or EWOULDBLOCK, you turn on EPOLLOUT and resume sending 
when EPOLLOUT becomes true. If you do not have anything to send you turn off EPOLLOUT.
EPOLLOUT事件只有在不可写到可写的转变时刻，才会触发一次，所以叫边缘触发，注意无发送数据时要关闭EPOLLOUT.

http://stackoverflow.com/questions/13568858/epoll-wait-always-sets-epollout-bit
