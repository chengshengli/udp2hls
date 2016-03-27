#include <cstdio>
#include <stdlib.h>
#include <cstring>
#include <iostream>
//#include <unistd.h>
//#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include "udpsocket.h"

using namespace std;

fd_set m_fdset_read;

UdpDocket::UdpDocket()
{
    bufsize = UDP_BUF;
//    q_buf = (uint8_t*)malloc(sizeof(uint8_t)*bufsize);

    for(int i=0;i<UDP_SOCKET_NUM;i++)
    {
        pthread_mutex_init(&locker[i], NULL);
        q_buf[i] = new uint8_t[UDP_BUF];
        write_ptr[i] = 0;
        read_ptr[i] = 0;
        max[i] = 0;
    }
}
UdpDocket::~UdpDocket()
{
    delete []q_buf;
}

void UdpDocket::startup(void)
{
    int err;
    err = pthread_create(&thid, NULL, this->udp_thread, this);
    if(err!=0)
    {
        perror("create udp thread err.");
        exit(1);
    }
}

void *UdpDocket::udp_thread(void *args)
{
    UdpDocket * myudp = static_cast<UdpDocket *>(args);
    int port[UDP_SOCKET_NUM] = {0};
    int server_socket_fd[UDP_SOCKET_NUM];

    struct sockaddr_in client_addr[UDP_SOCKET_NUM];
    socklen_t client_addr_length = sizeof(client_addr[0]);
    /*create the UDP socket.*/
    struct sockaddr_in server_addr[UDP_SOCKET_NUM];
    uint8_t rec_buf[RECVBUF];

    for(int i=0;i<UDP_SOCKET_NUM;i++)
    {
        port[i] = 10000+i;
        bzero(&server_addr[i], sizeof(server_addr[i]));
        server_addr[i].sin_family = AF_INET;
        server_addr[i].sin_addr.s_addr = htons(INADDR_ANY);
        server_addr[i].sin_port = htons(port[i]);
        server_socket_fd[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if(server_socket_fd[i] == -1)
        {
             perror("Create Socket Failed:");
             exit(1);
        }
        bind(server_socket_fd[i],(struct sockaddr*)&server_addr[i],sizeof(server_addr[i]));
    }


    while(1)
    {
        FD_ZERO( &m_fdset_read );
        int nMaxFd = 0;   // wangding, find the max fd

        int nClientCount = 0;
        for(int i=0; i<UDP_SOCKET_NUM; i++ )
        {
            FD_SET( server_socket_fd[i], &m_fdset_read );
            nClientCount ++;
            if( server_socket_fd[i] > nMaxFd )
                nMaxFd = server_socket_fd[i];
        }

        struct timeval TimeOut;   // Linux struct
        TimeOut.tv_sec = 0;
        TimeOut.tv_usec= 1000;
        select( nMaxFd+1, &m_fdset_read, NULL, NULL, &TimeOut);
        for(int i=0; i<UDP_SOCKET_NUM; i++)
        {
            if(FD_ISSET(server_socket_fd[i],&m_fdset_read))
            {
                int len = recvfrom(server_socket_fd[i], rec_buf, RECVBUF,0,(struct sockaddr*)&client_addr[i], &client_addr_length);
                myudp->put_queue(i,rec_buf,len);
            }

        }



 //
//        printf("%d\n",len);
 //
//        sleep(1);
//        write(fd,myudp->q_buf,len);
//       printf("the cnt is %d.\n",cnt++);

    }
//    close(fd);
}

void UdpDocket::put_queue(uint8_t index, uint8_t* buf, int size)
{
    uint8_t* thisbuf=static_cast<uint8_t *>(q_buf[index]);

    pthread_mutex_lock(&locker[index]);
    if(write_ptr[index] + size > bufsize)
    {
        memcpy(thisbuf + write_ptr[index],buf,bufsize - write_ptr[index]);
        memcpy(thisbuf,buf + bufsize - write_ptr[index],write_ptr[index] + size - bufsize);

    }
    else
    {
        memcpy(thisbuf+write_ptr[index],buf,size);
    }
    write_ptr[index] = (write_ptr[index] + size) % bufsize;
    pthread_mutex_unlock(&locker[index]);
}

int UdpDocket::udp_get_queue(uint8_t index, uint8_t *buf,int size)
{
    int pos = write_ptr[index];
    uint8_t* thisbuf=static_cast<uint8_t *>(q_buf[index]);
    pthread_mutex_lock(&locker[index]);
    if(pos < read_ptr[index])
    {
        pos += bufsize;
    }
    if(read_ptr[index] + size > pos)
    {
        pthread_mutex_unlock(&locker[index]);
        return 0;
    }
    if(read_ptr[index] + size > bufsize)
    {
        memcpy(buf, thisbuf + read_ptr[index], bufsize - read_ptr[index]);
        memcpy(buf + bufsize - read_ptr[index], thisbuf, size - (bufsize - read_ptr[index]));
    }
    else
        memcpy(buf, thisbuf + read_ptr[index], size);
    read_ptr[index] = (read_ptr[index] + size) % bufsize;
    pthread_mutex_unlock(&locker[index]);
    if((write_ptr[index] - read_ptr[index]) > max[index])
    {
        max[index] = write_ptr[index] - read_ptr[index];
 //       printf("size=%d\n",max);
    }
    return size;
}
