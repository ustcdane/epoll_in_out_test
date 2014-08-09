#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define MAX 64

int make_non_blocking(int fd)
{
		int flag,s;
		flag=fcntl(fd, F_GETFL,0);
		if(-1==flag)
		{
				perror("fcntl get");
				return -1;
		}
		flag |= O_NONBLOCK;
		s=fcntl(fd, F_SETFL, flag);
		if(-1==s)
		{
				perror("fcntl set");
				return -1;
		}
		return 0;
}

int create_bind(char *port)
{
	struct addrinfo h;
	struct addrinfo *re,*rp;
	int s, fd;
	memset(&h,0,sizeof(h));
	h.ai_family=AF_UNSPEC;
	h.ai_socktype=SOCK_STREAM;
	h.ai_flags=AI_PASSIVE;

	s=getaddrinfo(NULL, port, &h, &re);
	if(s!=0)
	{
			fprintf(stderr, "getaddrinfo\n");
			return -1;
	}
	for(rp=re; rp!=NULL; rp=rp->ai_next)
	{
		fd=socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(fd==-1)
			continue;
		s=bind(fd, rp->ai_addr, rp->ai_addrlen);
		if(s==0)
			break;
		close(fd);
	}
	if(rp==NULL)
	{
		fprintf(stderr, "could no bind\n");
		return -1;
	}
	freeaddrinfo(re);
	return fd;
}

void fdmod(int epoll_fd, int fd, int ev)
{
	struct epoll_event event;
	event.data.fd=fd;
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		printf("Usage:%s [port]\n", argv[0]);
		return -1;
	}
	struct epoll_event ev;
	struct epoll_event *evs;
	
	/// not a good idea.
	char write_buf[1024];
	int write_bytes;
	int efd;
	int fd=create_bind(argv[1]);
	make_non_blocking(fd);
	int s=listen(fd, SOMAXCONN);
	if(s==-1)
	{
		printf("listen\n");
		return -1;
	}
    efd=epoll_create1(0);// efd
	ev.data.fd=fd;
	ev.events = EPOLLIN | EPOLLET;
	s=epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
	evs=calloc(MAX, sizeof(ev));
	// event loop
	while(1)
	{
		int n,i;
		n=epoll_wait(efd, evs, MAX, -1);
		for(i=0; i<n; i++)
		{
			if((evs[i].events & EPOLLERR))
			{
					printf("epoll error\n");
					close(evs[i].data.fd);
					continue;
			}
			else if(fd==evs[i].data.fd)// fd is listenfd
			{
				//incoming connection
				while(1)
				{
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[512],sbuf[512];
					in_len=sizeof(in_addr);
					infd=accept(fd, &in_addr, &in_len);
					if(infd==-1)
					{
						if(errno == EAGAIN)
							break;
						else
						{
							perror("accept");
							break;
						}
					}
					s=getnameinfo(&in_addr, in_len, hbuf,sizeof(hbuf), sbuf,sizeof(sbuf),
							NI_NUMERICHOST|NI_NUMERICSERV);
					if(s==0)
					{
						fprintf(stdout, "Accept connection from %d %s %s",infd, hbuf, sbuf);
					}
					make_non_blocking(infd);
					ev.data.fd=infd;
					ev.events= EPOLLIN | EPOLLET;
					epoll_ctl(efd, EPOLL_CTL_ADD, infd, &ev);
				}
			}
			else if(evs[i].events & EPOLLIN) // read
			{
				int done=0;// complete reading.
				while(1)
				{
					int cnt;
					char buf[1024];
					cnt=read(evs[i].data.fd, buf, sizeof(buf));
					if(cnt==-1)
					{
						if(errno!=EAGAIN)
						{
							perror("read");
							done=1;
						}
								break;
						}
						else if(cnt==0)// close socket
						{
								done=1;
								break;
						}
						s=write(1, buf, cnt);// print to stdout
						if(s==-1)
						{
								printf("err\n");
								return -1;
						}
						if(done)
						{
								printf("Close connection on %d\n", evs[i].data.fd);
								close(evs[i].data.fd);
						}
						else// import, learn how EPOLL be triggered
						{
							printf("\n Input some data to the client:\n");
							scanf("%s", write_buf);
							write_bytes = strlen(write_buf);
							if(write_bytes>0)// trigger EPOLLOUT 
							{
								fdmod(efd, evs[i].data.fd, EPOLLOUT);
							}
						}
					}// if cnt==-1
				}
				else if(evs[i].events & EPOLLOUT) // can write
				{
					printf("%d EPOLLOUT event.\n", evs[i].data.fd);
					int client_fd = evs[i].data.fd;
					if(write_bytes==0)// have nothing to send
					{
						fdmod(efd, client_fd, EPOLLIN);
						continue;
					}
					int nums = 0;
					int tmp;
					while(1) // ET
					{
						tmp=write(client_fd, write_buf, write_bytes);
						if(tmp <= -1)
						{
							if(errno==EAGAIN)
							{
								fdmod(efd, client_fd, EPOLLOUT);
								break;
							}
						 	break;
						}
						write_bytes-=tmp;
						//printf("write_bytes %d\n", write_bytes);
						if(write_bytes<1)
						{
							fdmod(efd, client_fd, EPOLLIN);
							break;// 注意循环读写 别忘了 break
						}
					}
				}
			}
	}
	free(evs);
	close(fd);
	return 0;
}
