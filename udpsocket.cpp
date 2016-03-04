#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <fcntl.h>
#include <netinet/in.h>
#include "udpsocket.h"
UdpDocket::UdpDocket()
{
    bufsize = UDP_BUF;
    q_buf = (uint8_t*)malloc(sizeof(uint8_t)*bufsize);
    pthread_mutex_init(&locker, NULL);
    write_ptr = 0;
    read_ptr = 0;
}

void UdpDocket::startup(void)
{
    int err;
    err = pthread_create(&thid, NULL, this->udp_thread, this);
    if(err!=0)
    {
        printf("create udp thread err.\n");
    }
}

void *UdpDocket::udp_thread(void *args)
{
    UdpDocket * myudp = static_cast<UdpDocket *>(args);
    int port = 10000;
    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    /*create the UDP socket.*/
    struct sockaddr_in server_addr;
    uint8_t rec_buf[RECVBUF];
//    int fd,
    int cnt=0;
//    fd = creat("./test.ts",S_IWUSR | S_IRUSR);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(port);
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket_fd == -1)
    {
         perror("Create Socket Failed:");
         exit(1);
    }
    bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    while(1)
    {
        int len = recvfrom(server_socket_fd, rec_buf, RECVBUF,0,(struct sockaddr*)&client_addr, &client_addr_length);
//        printf("%d\n",len);
        myudp->put_queue(rec_buf,len);
//        sleep(1);
//        write(fd,myudp->q_buf,len);
//       printf("the cnt is %d.\n",cnt++);

    }
//    close(fd);
}

void UdpDocket::put_queue(uint8_t* buf, int size)
{
    pthread_mutex_lock(&locker);
    if(write_ptr + size > bufsize)
    {
        memcpy(q_buf+write_ptr,buf,bufsize - write_ptr);
        memcpy(q_buf,buf + bufsize - write_ptr,write_ptr + size - bufsize);

    }
    else
    {
        memcpy(q_buf+write_ptr,buf,size);
    }
    write_ptr = (write_ptr + size) % bufsize;
    pthread_mutex_unlock(&locker);
}
int max=0;
int UdpDocket::udp_get_queue(uint8_t *buf,int size)
{
    int pos = write_ptr;
    pthread_mutex_lock(&locker);
    if(pos < read_ptr)
    {
        pos += bufsize;
    }
    if(read_ptr + size > pos)
    {
        pthread_mutex_unlock(&locker);
        return 0;
    }
    if(read_ptr + size > bufsize)
    {
        memcpy(buf, q_buf + read_ptr, bufsize - read_ptr);
        memcpy(buf + bufsize - read_ptr, q_buf, size - (bufsize - read_ptr));
    }
    else
        memcpy(buf, q_buf + read_ptr, size);
    read_ptr = (read_ptr + size) % bufsize;
    pthread_mutex_unlock(&locker);
    if(write_ptr - read_ptr>max)
    {
        max = write_ptr - read_ptr;
        printf("size=%d\n",max);
    }
    return size;
}
