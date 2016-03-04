//#include <stdio.h>
//#include <pthread.h>
#include <unistd.h>
//#include <stdint.h>
#include "udpsocket.h"
#include "tsparser.h"
UdpDocket *udpSocket;
TsParser *tsParser;
int main(int argc, char *argv[])
{
    udpSocket = new UdpDocket();
    udpSocket->startup();
    tsParser = new TsParser();
    tsParser->startup();
    while(1)
    {
        sleep(10);
    }
}
