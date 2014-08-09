all:epoll_server client
epoll_server:epoll_test_tr.c
	gcc -o epoll_server epoll_test_tr.c
client:client.c
	gcc -o client client.c

	
