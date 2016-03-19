#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <stdint.h>
#include <pthread.h>

#define UDP_BUF 1024 * 512
#define RECVBUF 188*4

class UdpDocket
{
public:
    UdpDocket();
    void startup(void);
    int udp_get_queue(uint8_t *buf,int size);

private:
    pthread_t thid;
    pthread_mutex_t locker;
    uint8_t* q_buf;
    int write_ptr;
    int max;
    int read_ptr;
    int bufsize;
    static void *udp_thread(void *arg);
    void put_queue(uint8_t* buf, int size);

};

#endif // UDPSOCKET_H
