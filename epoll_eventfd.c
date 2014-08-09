/*
eventfd test. int eventfd(unsigned int initval,int flags)
eventfd：实现了线程之间事件通知的方式，eventfd的缓冲区大小是sizeof(uint64_t)；向其write可以递增这个计数器，
read操作可以读取，并进行清零；eventfd也可以放到监听队列中，当计数器不是0时，有可读事件发生，可以进行读取。

这个函数会创建一个 事件对象 (eventfd object), 用来实现，进程(线程)间的等待/通知(wait/notify) 机制. 内核会
为这个对象维护一个64位的计数器(uint64_t)。并且使用第一个参数(initval)初始化这个计数器。调用这个函数就会返
回一个新的文件描述符(event object)。
创建这个对象后，可以对其做如下操作：
write 将缓冲区写入的8字节整形值加到内核计数器上，计数器能存储的最大值是最大的无符号64位整型值1（0xfffffffffffffffe)。
如果增加导致计时器的值超过了最大值，write要么阻塞直到一个read在这个文件描述符上执行，要么返回EAGAIN，如果文件
描述法是非阻塞的；read 读取8字节值， 并把计数器重设为0. 如果调用read的时候计数器为0， 要是eventfd是阻塞的， read
就一直阻塞在这里，否则就得到 一个EAGAIN错误。如果buffer的长度小于8那么read会失败， 错误代码被设置成 EINVAL;
具体参考：
http://www.man7.org/linux/man-pages/man2/eventfd.2.html

通过这个实验可以发现 多线程环境下不能简单的通过read write 来进行线程间事件文件描述符的传送(有可能连续2次的write，
这样read的值会是连续两次文件描述符之和了， 出现混乱了！！)。
*/


//gcc eventfd.c -o eventfd -lpthread
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

int efd = -1;

void *read_thread(void *arg)
{
    int ret = 0;
    int ep_fd = -1;
    struct epoll_event events[100];

    if (efd < 0)
    {
        printf("efd not inited.\n");
        return NULL;
    }

    ep_fd = epoll_create1(0);
    if (ep_fd < 0)
    {
        perror("epoll_create fail: ");
        return NULL;
    }


    struct epoll_event read_event={0};

    read_event.events = EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLET;
    read_event.data.fd = efd;

    ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, efd, &read_event);
    if (ret < 0)
    {
        perror("epoll ctl failed:");
		close(ep_fd);
        return NULL;
    }
    
    while (1)
    {
        ret = epoll_wait(ep_fd, events, 100, -1);
		//printf("epoll return %d\n", ret);
        if (ret > 0)
        {
            int i = 0;
            for (; i < ret; i++)
            {
                if (events[i].events & EPOLLHUP)
                {
                    printf("epoll eventfd has epoll hup.\n");
                    close(ep_fd);
					return NULL;
                }
                else if (events[i].events & EPOLLERR)
                {
                    printf("epoll eventfd has epoll error.\n");
                    close(ep_fd);
					return NULL;
                }
                else if (events[i].events & EPOLLIN)
                {
                    int event_fd = events[i].data.fd;
					if(event_fd != efd)// is eventfd
						continue;
					
					
					uint64_t count = 0;
	                ret = read(event_fd, &count, sizeof(count));
                    if (ret < 0)
                    {
						if(ret==-1 && errno==EAGAIN)
							continue;
                        perror("read fail:");
                        close(ep_fd);
						return NULL;
                    }
                    else
                    {
                        printf("success read from efd, read %d bytes(%llu)\n", ret, count);
                    }
                }
            }
        }
        else if (ret == 0)
        {
            /* time out */
            printf("epoll wait timed out.\n");
            break;
        }
        else
        {
            perror("epoll wait error:");
            close(ep_fd);
			return NULL;
        }
    }
	return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t pid = 0;
    uint64_t count = 0;
    int ret = 0;
    int i = 0;

    efd = eventfd(0, EFD_NONBLOCK);
    if (efd < 0)
    {
        perror("eventfd failed.");
        return ret;
    }

    ret = pthread_create(&pid, NULL, read_thread, NULL);
    if (ret < 0)
    {
        perror("pthread create:");
        return ret;
    }

    for (i = 0; i < 1000; i++)
    {
        count = i;
        ret = write(efd, &count, sizeof(count));
        if (ret < 0)
        {
			if(errno==EAGAIN)
			{
				--i;
				continue;
			}
            perror("write event fd fail:");
            break ;
        }
        else
        {
            printf("success write to efd, write %d bytes(%llu)\n", ret, count);
        }
    }


    pthread_join(pid, NULL);
    close(efd);
    return ret;
}
