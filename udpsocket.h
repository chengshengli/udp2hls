#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <stdint.h>
#include <pthread.h>

#define UDP_BUF 1024 * 188 *12
#define RECVBUF 188*4
#define UDP_SOCKET_NUM 16

class UdpDocket
{
public:
    UdpDocket();
    ~UdpDocket();
    void startup(void);
    int udp_get_queue(uint8_t index, uint8_t *buf,int size);

private:
    pthread_t thid;
    pthread_mutex_t locker[UDP_SOCKET_NUM];
    uint8_t* q_buf[UDP_SOCKET_NUM];
    int write_ptr[UDP_SOCKET_NUM];
    int max[UDP_SOCKET_NUM];
    int read_ptr[UDP_SOCKET_NUM];
    int bufsize;
    static void *udp_thread(void *arg);
    void put_queue(uint8_t index,uint8_t* buf, int size);

};

#endif // UDPSOCKET_H
