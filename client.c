#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
    int sockfd;
	int numbytes;
    char buf[1024];
	char str[1024];
    struct hostent *he;
    struct sockaddr_in their_addr; // connector'saddress information


     if (( argc == 1) || (argc==2) ) {
        fprintf(stderr,"usage: client hostname\nEx:\n$./client ip port\n");
        exit(1);
    }

    if ((he=gethostbyname(argv[1])) == NULL) { // get the host info

        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET; // host byte order

    their_addr.sin_port = htons(atoi(argv[2])); // short, network byte order

    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
    if (connect(sockfd, (struct sockaddr *)&their_addr,
                                          sizeof their_addr) == -1) {
        perror("connect");
        exit(1);
    }
   
	strcpy(str, "hello server.\n");
    while( 1 )
    {
	
	if( strlen(str)>0)// have data then send  
    {
		if(send(sockfd, str, strlen(str), 0)==-1)
		{
			perror("send");
			continue;
		}
		buf[0]='\0';
    }

	if ((numbytes=recv(sockfd, buf, 1023, 0)) == -1)// recv data
	{
		continue;
   	}
    else if( numbytes == 0 )
    {
		printf("Remote server has shutdown!\n");
        break;
    }
		buf[numbytes] = '\0';
		printf("Received: %s \n",buf);
		printf("---------------------\n");
		printf("Input data to server:\n");
		scanf("%s", str);
	}

    close(sockfd);
    return 0;
}
