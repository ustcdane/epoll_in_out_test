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
