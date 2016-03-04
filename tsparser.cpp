#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "global.h"
#include "tsparser.h"

TsParser::TsParser()
{
    buf_size = TS_PACKAGE_SIZE;
    buf = new uint8_t[buf_size];
 //   pid_ts = new PID_TS();
 //   ts_header = new TS_Header();
//    pat_info = new PAT_Info();
//    pmt_info = new PMT_Info();
//    if(buf==NULL || pid_ts==NULL || ts_header==NULL || pat_info==NULL || pmt_info==NULL)
//        printf("malloc the memory for tsparser err.\n");
    pid_ts.PID_pat = 0;

}



void TsParser::startup(void)
{
    int err;
    err = pthread_create(&thid, NULL, this->ts_parser_thread, this);
    if(err!=0)
    {
        printf("create udp thread err.\n");
    }
}

int TsParser::parser_header(void)
{
    if(buf[0] != 0x47 || (buf[1]& 0x80) == 0x80)
        return 1;
    uint16_t tmp = (buf[1]<<8) | buf[2];

    ts_header.this_pid = (tmp & 0x1fff);

    ts_header.indicator = (buf[1] & 0x40)>>6;

    return 0;
}

int TsParser::parser_pat(void)
{
    uint8_t pos = 3 + ts_header.indicator;
    if((*(buf + pos + 6) & 0x01)==0)
        return 1;
    uint8_t version = (*(buf + pos + 6) & 0x3e)>>1;
    if(pat_info.PAT_version != version)
    {
        pat_info.PAT_version = version;
        pat_info.PAT_change = 1;
    }
    if(*(uint16_t*)(buf + pos + 9)!=0)
    {
        pid_ts.PID_pmt = ((uint16_t)(*(buf + pos + 11)&0x1f) << 8) | *(buf + pos + 12);
    }

    return 0;
}

int TsParser::parser_pmt(void)
{
    uint8_t pos = 3 + ts_header.indicator;
    uint16_t len = ((uint16_t)(*(buf + pos + 2)&0x0f) << 8) | *(buf + pos + 3);
    uint8_t version = (*(buf + pos + 6) & 0x3e)>>1;
    if(pmt_info.PMT_version != version)
    {
        pmt_info.PMT_version = version;
        pmt_info.PMT_change = 1;
    }
    pid_ts.PID_pcr = ((uint16_t)(*(buf + pos + 9)&0x1f) << 8) | *(buf + pos + 10);
    int index = 13+pos;
    int times = (len-9-4)/5;
    while(times--)
    {
        switch(*(buf+index))
        {
        case 0x1B:
            pid_ts.PID_vedio = ((uint16_t)(*(buf + index + 1)&0x1f) << 8) | *(buf + index + 2);
            break;
        case 0x0f:
            pid_ts.PID_audio = ((uint16_t)(*(buf + index + 1)&0x1f) << 8) | *(buf + index + 2);
            break;
        default:
            break;
        }
        index+=5;
    }
//    printf("%x %x %x %x\n",pid_ts.PID_pmt,pid_ts.PID_pcr,pid_ts.PID_vedio,pid_ts.PID_audio);
    return 0;
}

int TsParser::parser_vedio(void)
{
    uint8_t tmp = *(buf+3) & 0xf;
    if(((video_info.counter+1)&0xf) != tmp)
        printf("lose a packet.\n");
    video_info.counter = tmp;
    return 0;
}

int TsParser::parser_audio(void)
{
    return 0;
}

int TsParser::parser_ts_packet(void)
{
    if(parser_header()!=0)
        return 1;
    if(ts_header.this_pid == pid_ts.PID_pat)
        parser_pat();
    else if(ts_header.this_pid == pid_ts.PID_pmt)
        parser_pmt();
    else if(ts_header.this_pid == pid_ts.PID_vedio)
        parser_vedio();
    else if(ts_header.this_pid == pid_ts.PID_audio)
        parser_audio();
    return 0;
}

int TsParser::get_ts_packet()
{
    return udpSocket->udp_get_queue(buf,buf_size);

}

void *TsParser::ts_parser_thread(void *args)
{
    TsParser * myts = static_cast<TsParser *>(args);
    int cnt = 0;
    while(1)
    {
        if(myts->get_ts_packet()==TS_PACKAGE_SIZE)
        {
           // printf("%d\n",cnt++);
            if(myts->parser_ts_packet() != 0)
                continue;

        }
        else
            usleep(1000);
    }

}
