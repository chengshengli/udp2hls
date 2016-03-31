#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
using namespace std;
#include "global.h"
#include "tsparser.h"

//#define DEBUG


TsParser::TsParser(uint8_t index)
{
    channel = index;
    bufSize = TS_PACKAGE_SIZE;
    buf = new uint8_t[bufSize*2];
    tsFileCount = 0;
    listSeq = 0;
    deltTime = 0;

    first = 1;
    patReady = 0;
    listM3u8.sList[listM3u8.listLen++] = "#EXTM3U\n";
    listM3u8.sList[listM3u8.listLen++] = "#EXT-X-VERSION:3\n";
    listM3u8.sList[listM3u8.listLen++] = "#EXT-X-ALLOW-CACHE:YES\n";
    listM3u8.sList[listM3u8.listLen++] = "#EXT-X-MEDIA-SEQUENCE:0\n";
    listM3u8.sList[listM3u8.listLen++] = "#EXT-X-TARGETDURATION:10\n";
    pidTs.PID_pat = 0;

}

TsParser::~TsParser()
{
    delete []buf;
}



void TsParser::startup(void)
{
    int err;
    err = pthread_create(&thid, NULL, this->ts_parser_thread, this);
    if(err!=0)
    {
        cout<<"create udp thread err.\n"<<endl;
    }
}

void TsParser::deal_IDR(void)
{
    uint32_t minPTS = videoInfo.PTS%5400000;
    uint32_t tmp;
    if(first)
    {
        initPTS = minPTS;
        new_tsfile();
        first = 0;
    }
    if(minPTS >= initPTS)
    {
        tmp = minPTS;
    }
    else
    {
        tmp = minPTS + 5400000;
    }

    if(tmp - initPTS > 900000)
    {
        if(minLastPTS < initPTS)
            minLastPTS += 5400000;
        deltTime = (float)(minLastPTS - initPTS)/90000;

        if(minLastPTS > 5400000)
            minLastPTS -= 5400000;
        initPTS = minLastPTS;
        if(deltTime<=10)
        {
            close_tsfile();
        }
        else
        {
            drop_tsfile();
        }
        new_tsfile();
    }
    minLastPTS = minPTS;
}

void TsParser::close_tsfile(void)
{
    if(fp_TS.is_open())
    {
        cout<<"close" + updateName<<endl;
        fp_TS.close();
        update_m3u8();

        delete_tsfile();

    }
}
void TsParser::drop_tsfile(void)
{
    if(fp_TS.is_open())
    {
        fp_TS.close();
        string path;
//        cout << "drop:" + updateName <<endl;
#ifdef DEBUG
        path = "./" + updateName;
#else
        path = "/usr/local/srs/trunk/objs/nginx/html/live/" + updateName;

#endif
        remove(path.c_str());

    }
}

void TsParser::new_tsfile(void)
{

    if(!fp_TS.is_open())
    {

        stringstream s;
        string path;
        s.str("");
        s << (channel + 1);
        updateName = "stream"+ s.str() + "-";
        s.str("");
        s << tsFileCount;
        updateName = updateName + s.str() + ".ts";
        cout << "open:" + updateName << endl;
#ifdef DEBUG
        path = "./" + updateName;
#else
        path = "/usr/local/srs/trunk/objs/nginx/html/live/" + updateName;
#endif
        fp_TS.open(path.c_str(), ofstream::out | ofstream::binary);
        if(fp_TS)
        {
            cout << "open success." << endl;
            fp_TS.write((char*)patData,TS_PACKAGE_SIZE);
            fp_TS.write((char*)pmtData,TS_PACKAGE_SIZE);
        }
    }
}

void TsParser::delete_tsfile(void)
{
    if(listM3u8.listLen >= (4 + LIST_TS_FILE_NUM *2) && tsFileCount > 5)
    {
#ifdef DEBUG
        string path("./");
#else
        string path("/usr/local/srs/trunk/objs/nginx/html/live/");
#endif
        stringstream s;
        s.str("");
        s << channel + 1;
        deleteName = "stream"+ s.str() + "-";
        s.str("");\
        s << tsFileCount-6;
        deleteName = deleteName + s.str() + ".ts";
        path = path + deleteName ;
        cout <<"delete:" + deleteName << endl;
        if(access(path.c_str(),0) != -1)
        {
            remove(path.c_str());
        }

    }
}

void TsParser::update_m3u8(void)
{
    stringstream s;
    s.str("");
#ifdef DEBUG
    s << channel + 1;
    string path = "./stream"+ s.str() + ".m3u8";
#else
    s << channel + 1;
    string path = "/usr/local/srs/trunk/objs/nginx/html/live/stream"+ s.str() + ".m3u8";
//    string path("/usr/local/srs/trunk/objs/nginx/html/live/stream1.m3u8");

#endif



        if(listSeq >= 5)
        {
            s.str("");
            s << (listSeq - 4);
            listM3u8.sList[3] = "#EXT-X-MEDIA-SEQUENCE:" + s.str() + "\n";
        }
        listSeq++;
        if(listM3u8.listLen < (4 + LIST_TS_FILE_NUM *2))
        {
            s.str("");
            s << deltTime;
            listM3u8.sList[listM3u8.listLen++] = "#EXTINF:" + s.str()+ "\n";
            s.str("");
            s << channel + 1;
            s << "-";
            s << tsFileCount++;
            listM3u8.sList[listM3u8.listLen++] = "stream" + s.str() + ".ts\n";
        }
        else
        {
            for(int i=0; i < LIST_TS_FILE_NUM - 1; i++)
            {
                listM3u8.sList[5+2*i] = listM3u8.sList[7+2*i];
                listM3u8.sList[6+2*i] = listM3u8.sList[8+2*i];
            }
            s.str("");
            s << deltTime;
            listM3u8.sList[5+(LIST_TS_FILE_NUM-1)*2] = "#EXTINF:" + s.str()+ "\n";
            s.str("");
            s << channel + 1;
            s << "-";
            s << tsFileCount++;
            listM3u8.sList[6+(LIST_TS_FILE_NUM-1)*2] = "stream" + s.str() + ".ts\n";
        }


    ofstream fpM3u8(path.c_str(),ofstream::out);
    if(fpM3u8.is_open())
    {
        for (int i = 0; i < listM3u8.listLen; i++)
        {
            fpM3u8 << listM3u8.sList[i];
        }
        fpM3u8.close();
    }
}



int TsParser::parser_header(void)
{
    if(buf[0] != 0x47 || (buf[1]& 0x80) == 0x80)
        return 1;
    uint16_t tmp = (buf[1]<<8) | buf[2];

    tsHeader.this_pid = (tmp & 0x1fff);

    tsHeader.indicator = (buf[1] & 0x40)>>6;
    if(tsHeader.indicator)
        tsHeader.tableSkew = buf[4];
    else
        tsHeader.tableSkew = 0;

    if((buf[3]&0x20) !=0)
        tsHeader.adaptation_filed = 1;
    else
        tsHeader.adaptation_filed = 0;


    return 0;
}

int TsParser::parser_pat(void)
{
    uint8_t pos = 4 + tsHeader.indicator + tsHeader.tableSkew;
    uint8_t version = (*(buf + pos + 5) & 0x3e)>>1;
    if(patInfo.PAT_version != version)
    {
        patInfo.PAT_version = version;
        patInfo.PAT_change = 1;
    }
    if(*(uint16_t*)(buf + pos + 8)!=0)
    {
        pidTs.PID_pmt = ((uint16_t)(*(buf + pos + 10)&0x1f) << 8) | *(buf + pos + 11);
    }

    return 0;
}

int TsParser::parser_pmt(void)
{
    uint8_t pos = 4 + tsHeader.indicator + tsHeader.tableSkew;
    uint16_t len = ((uint16_t)(*(buf + pos + 1)&0x0f) << 8) | *(buf + pos + 2);
    uint8_t version = (*(buf + pos + 5) & 0x3e)>>1;
    if(pmtInfo.PMT_version != version)
    {
        pmtInfo.PMT_version = version;
        pmtInfo.PMT_change = 1;
    }
    pidTs.PID_pcr = ((uint16_t)(*(buf + pos + 8)&0x1f) << 8) | *(buf + pos + 9);
    int index = 12+pos;

    while(index+4<188)
    {
        switch(*(buf+index))
        {
        case 0x1B:
            pidTs.PID_vedio = ((uint16_t)(*(buf + index + 1)&0x1f) << 8) | *(buf + index + 2);

            break;
        case 0x0f:
            pidTs.PID_audio = ((uint16_t)(*(buf + index + 1)&0x1f) << 8) | *(buf + index + 2);
            break;
        default:
            break;
        }
        index+=(5+(*(buf + index + 4)));
    }
    return 0;
}
int cnt_B=0;
int cnt_a=0;
int TsParser::parser_nal(uint8_t index)
{

    while(index<TS_PACKAGE_SIZE)
    {
        if(buf[index]==0 && buf[index+1]==0 && buf[index+2]==1)
        {
            switch(buf[index+3])
            {
            case 0x25:
           //     printf("IDR \n");
                if(patReady)
                    deal_IDR();
   //             videoInfo.second = videoInfo.PTS%5400000;
   //             videoInfo.msecond = videoInfo.second % 90000 /90;
   //             printf("second:%d.%ds\n",videoInfo.second/90000,videoInfo.msecond);

                cnt_B = 0;
                cnt_a = 0;
                return 0;
                break;
            case 0x21:
    //            printf("P \n");
                cnt_B = 0;
                return 0;
                break;
            case 0x01:

                return 0;
                break;
            case 0x09:
            //    printf("aud\n");
                break;
            case 0x06:
             //   printf("SEI\n");
                break;
            case 0x27:
            //    printf("SPS\n");
                break;
            case 0x28:
            //    printf("PPS\n");
                break;
            default:
                printf("unkown.\n");
                break;
            }
        }
        index++;
    }
    printf("reach the end.\n");
    return 0;
}
int cnt_lose=0;
int TsParser::parser_vedio(void)
{
    uint8_t tmp = *(buf+3) & 0xf;
    uint8_t index=0;
//    if(((videoInfo.counter+1)&0xf) != tmp)
//        printf("lose a packet:%d\n",cnt_lose++);
    if(pidTs.PID_pcr == pidTs.PID_vedio)
    {
        if((*(buf+3) & 0x30) == 0x30)
        {
            index+=*(buf+4);
        }
    }
    videoInfo.counter = tmp;
    if(tsHeader.indicator)
    {
        int pes_header_len=0;
        bool pts_be = false,dts_be = false;

        index = 4 + tsHeader.adaptation_filed *(buf[4] + 1);
       if((buf[index]==0) && (buf[index+1] == 0) && (buf[index+2]==1) && (buf[index+3]==0xE0))
        {
            cnt_a++;
            pes_header_len = buf[index+8];
            pts_be = buf[index+7] & 0x80;
            dts_be = buf[index+7] & 0x40;
            if(pts_be && dts_be)
            {
                if((buf[index+9] & 0xf0) == 0x30)
                {
                    videoInfo.PTS = ((uint64_t)(buf[index+9]&0x0E)<<29) | ((uint64_t)(buf[index+10])<<22) | \
                            ((uint64_t)(buf[index+11]&0xFE)<<14) | ((uint64_t)(buf[index+12])<<7) | ((buf[index+13]&0xFE)>>1);
                    videoInfo.DTS = ((uint64_t)(buf[index+14]&0x0E)<<29) | ((uint64_t)(buf[index+15])<<22) | \
                            ((uint64_t)(buf[index+16]&0xFE)<<14) | ((uint64_t)(buf[index+17])<<7) | ((buf[index+18]&0xFE)>>1);
                }


            }
            else if(pts_be && !dts_be)
            {
                if((buf[index+9] & 0xf0) == 0x20)
                {
                    videoInfo.PTS = ((uint64_t)(buf[index+9]&0x0E)<<29) | ((uint64_t)(buf[index+10])<<22) | \
                            ((uint64_t)(buf[index+11]&0xFE)<<14) | ((uint64_t)(buf[index+12])<<7) | ((buf[index+13]&0xFE)>>1);
                }
            }
            else
                return 1;
            index += pes_header_len + 9;

            parser_nal(index);
        }
    }
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
    if(tsHeader.this_pid == pidTs.PID_pat)
    {
        parser_pat();
        patReady = 1;
        memcpy(patData,buf,TS_PACKAGE_SIZE);
        return 0;
    }
    else if(tsHeader.this_pid == pidTs.PID_pmt)
    {
        parser_pmt();
        memcpy(pmtData,buf,TS_PACKAGE_SIZE);
        return 0;
    }
//    else if(tsHeader.this_pid == pidTs.PID_pcr)
//    {

//    }
    else if(tsHeader.this_pid == pidTs.PID_vedio)
        parser_vedio();
    else if(tsHeader.this_pid == pidTs.PID_audio)
        parser_audio();
    if(fp_TS)
    {
        fp_TS.write((char*)buf,TS_PACKAGE_SIZE);
    }
    return 0;
}

int TsParser::get_ts_packet()
{
    return udpSocket->udp_get_queue(channel,buf,bufSize);

}

void *TsParser::ts_parser_thread(void *args)
{
    TsParser * myts = static_cast<TsParser *>(args);
    while(1)
    {
        if(myts->get_ts_packet()==TS_PACKAGE_SIZE)
        {

            if(myts->parser_ts_packet() != 0)
                continue;

        }
        else
            usleep(1000);
    }

}
