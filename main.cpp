//#include <stdio.h>
//#include <pthread.h>
#include <unistd.h>
//#include <stdint.h>
#include "udpsocket.h"
#include "tsparser.h"
UdpDocket *udpSocket;
int main(int argc, char *argv[])
{
    udpSocket = new UdpDocket();
    udpSocket->startup();

    TsParser tsParser1(0);
    tsParser1.startup();
    TsParser tsParser2(1);
    tsParser2.startup();
    TsParser tsParser3(2);
    tsParser3.startup();
    TsParser tsParser4(3);
    tsParser4.startup();
    TsParser tsParser5(4);
    tsParser5.startup();
    TsParser tsParser6(5);
    tsParser6.startup();
    TsParser tsParser7(6);
    tsParser7.startup();
    TsParser tsParser8(7);
    tsParser8.startup();
    TsParser tsParser9(8);
    tsParser9.startup();
    TsParser tsParser10(9);
    tsParser10.startup();
    TsParser tsParser11(10);
    tsParser11.startup();
    TsParser tsParser12(11);
    tsParser12.startup();
    TsParser tsParser13(12);
    tsParser13.startup();
    TsParser tsParser14(13);
    tsParser14.startup();
    TsParser tsParser15(14);
    tsParser15.startup();
    TsParser tsParser16(15);
    tsParser16.startup();

    while(1)
    {
        sleep(10);
    }
}
