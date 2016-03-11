#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "global.h"
#include "tsparser.h"

TsParser::TsParser()
{
    bufSize = TS_PACKAGE_SIZE;
    buf = new uint8_t[bufSize*2];
    fp_TS = NULL;
    tsFileCount = 0;
    listSeq = 0;
    deltTime = 0;

    first = 1;
    patReady = 0;
    sprintf(listM3u8.sList[listM3u8.listLen++],"#EXTM3U\n");
    sprintf(listM3u8.sList[listM3u8.listLen++],"#EXT-X-VERSION:3\n");
    sprintf(listM3u8.sList[listM3u8.listLen++],"#EXT-X-ALLOW-CACHE:YES\n");
    sprintf(listM3u8.sList[listM3u8.listLen++],"#EXT-X-MEDIA-SEQUENCE:0\n");
    sprintf(listM3u8.sList[listM3u8.listLen++],"#EXT-X-TARGETDURATION:10\n");


    pidTs.PID_pat = 0;

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

void TsParser::deal_IDR(void)
{
    uint32_t minPTS = videoInfo.PTS%5400000;
    uint32_t tmp;
 //   printf("minPTS:%d\n",minPTS);
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

//printf("3:%x %x %x %x %x %x \n",patData[0],patData[1],patData[2],patData[3],patData[4],patData[5]);
        new_tsfile();
    }
 //   printf("initPTS:%d\n",initPTS);
    minLastPTS = minPTS;
}

void TsParser::close_tsfile(void)
{
    if(fp_TS)
    {
        fclose(fp_TS);
        fp_TS = NULL;
        update_m3u8();

        delete_tsfile();

    }
}
void TsParser::drop_tsfile(void)
{
    if(fp_TS)
    {
        fclose(fp_TS);
        fp_TS = NULL;
        char path[512]={0};
        sprintf(updateName,"stream1-%d.ts",tsFileCount);
        printf("drop:%s\n",updateName);
        sprintf(path,"/usr/local/srs/trunk/objs/nginx/html/live/%s",updateName);

//        update_m3u8();

        remove(path);

    }
}

void TsParser::new_tsfile(void)
{

    if(!fp_TS)
    {
        char path[512]={0};
        sprintf(updateName,"stream1-%d.ts",tsFileCount);
        printf("open:%s\n",updateName);
        sprintf(path,"/usr/local/srs/trunk/objs/nginx/html/live/%s",updateName);
//        sprintf(path,"./%s",updateName);
        fp_TS = fopen(path,"wb");
 //       printf("%x %x %x %x %x %x \n",patData[0],patData[1],patData[2],patData[3],patData[4],patData[5]);
  //      if(patData[0]==0)patData[0]=0x47;
        printf("%x %x %x %x %x %x \n",patData[0],patData[1],patData[2],patData[3],patData[4],patData[5]);
        fwrite(patData,TS_PACKAGE_SIZE,1,fp_TS);
        fwrite(pmtData,TS_PACKAGE_SIZE,1,fp_TS);
    }
}

void TsParser::delete_tsfile(void)
{
    if(listM3u8.listLen >= (4 + LIST_TS_FILE_NUM *2))
    {

        char path[512]={0};
//printf("1:%x %x %x %x %x %x \n",patData[0],patData[1],patData[2],patData[3],patData[4],patData[5]);
        sprintf(deleteName,"stream1-%d.ts",tsFileCount-6);
 //       sprintf(path,"./%s",deleteName);
 //       printf("1.5:%x %x %x %x %x %x \n",patData[0],patData[1],patData[2],patData[3],patData[4],patData[5]);
        sprintf(path,"/usr/local/srs/trunk/objs/nginx/html/live/%s",deleteName);
 //  printf("2:%x %x %x %x %x %x \n",patData[0],patData[1],patData[2],patData[3],patData[4],patData[5]);
        remove(path);

    }
}

void TsParser::update_m3u8(void)
{
    FILE* fpM3u8 = NULL;
    char path[512]={0};
    sprintf(path,"/usr/local/srs/trunk/objs/nginx/html/live/stream1.m3u8");
//    sprintf(path,"./livestream.m3u8");
    fpM3u8 = fopen(path,"w");

    if(fpM3u8)
    {

        if(listSeq >= 5)
            sprintf(listM3u8.sList[3]+22,"%u\n",listSeq-4);
        listSeq++;
        if(listM3u8.listLen < (4 + LIST_TS_FILE_NUM *2))
        {
            sprintf(listM3u8.sList[listM3u8.listLen++],"#EXTINF:%.3f\n",deltTime);
            sprintf(listM3u8.sList[listM3u8.listLen++],"stream1-%d.ts\n",tsFileCount++);
        }
        else
        {
            for(int i=0; i < LIST_TS_FILE_NUM - 1; i++)
            {
                strcpy(listM3u8.sList[5+2*i],listM3u8.sList[7+2*i]);
                strcpy(listM3u8.sList[6+2*i],listM3u8.sList[8+2*i]);
            }
            sprintf(listM3u8.sList[5+(LIST_TS_FILE_NUM-1)*2],"#EXTINF:%.3f\n",deltTime);
            sprintf(listM3u8.sList[6+(LIST_TS_FILE_NUM-1)*2],"stream1-%d.ts\n",tsFileCount++);
        }

    }

    for (int i = 0; i < listM3u8.listLen; i++)
    {
        fputs(listM3u8.sList[i],fpM3u8);
    }
    fclose(fpM3u8);
}



int TsParser::parser_header(void)
{
    if(buf[0] != 0x47 || (buf[1]& 0x80) == 0x80)
        return 1;
    uint16_t tmp = (buf[1]<<8) | buf[2];

    tsHeader.this_pid = (tmp & 0x1fff);

    tsHeader.indicator = (buf[1] & 0x40)>>6;

    if((buf[3]&0x20) !=0)
        tsHeader.adaptation_filed = 1;
    else
        tsHeader.adaptation_filed = 0;


    return 0;
}

int TsParser::parser_pat(void)
{
    uint8_t pos = 4 + tsHeader.indicator;
    if((*(buf + pos + 5) & 0x01)==0)
        return 1;
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
    uint8_t pos = 4 + tsHeader.indicator;
    uint16_t len = ((uint16_t)(*(buf + pos + 1)&0x0f) << 8) | *(buf + pos + 2);
    uint8_t version = (*(buf + pos + 5) & 0x3e)>>1;
    if(pmtInfo.PMT_version != version)
    {
        pmtInfo.PMT_version = version;
        pmtInfo.PMT_change = 1;
    }
    pidTs.PID_pcr = ((uint16_t)(*(buf + pos + 8)&0x1f) << 8) | *(buf + pos + 9);
    int index = 12+pos;
    int times = (len-9-4)/5;
    while(times--)
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
        index+=5;
    }
//    printf("%x %x %x %x\n",pid_ts.PID_pmt,pid_ts.PID_pcr,pid_ts.PID_vedio,pid_ts.PID_audio);
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

   //             printf("B:%d\n",cnt_B);
   //             printf("video:%d\n",cnt_a);
                printf("IDR \n");
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
   //             printf("B:%d\n",cnt_B);
    //            printf("P \n");
                cnt_B = 0;
                return 0;
                break;
            case 0x01:
               // printf("B\n");
                cnt_B++;
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

int TsParser::parser_vedio(void)
{
 //   printf(" tag is: %x %x %x %x %x %x %x %x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
    uint8_t tmp = *(buf+3) & 0xf;
    uint8_t index=0;
    if(((videoInfo.counter+1)&0xf) != tmp)
        printf("lose a packet.\n");
    videoInfo.counter = tmp;
    if(tsHeader.indicator)
    {
        int pes_header_len=0;
        bool pts_be = false,dts_be = false;

        index = 4 + tsHeader.adaptation_filed *(buf[4] + 1);
 //       printf("the PES tag is: %x %x %x %x %x %x %x %x %x %x\n",buf[index],buf[index+1],buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
        if((buf[index]==0) && (buf[index+1] == 0) && (buf[index+2]==1) && (buf[index+3]==0xE0))
        {cnt_a++;
            //printf("get the PES.\n");
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
 //       printf("%x %x %x %x %x\n",buf[index],buf[index+1],buf[index+2],buf[index+3],buf[index+4]);
        }
        //printf("%x %x %x %x %x\n",buf[index+9],buf[index+10],buf[index+11],buf[index+12],buf[index+13]);
  //      printf("PTS:%ld\n",video_info.PTS);
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
 //       if(!patReady)
  //      {
            patReady = 1;
            memcpy(patData,buf,TS_PACKAGE_SIZE);
  //      }
        return 0;
    }
    else if(tsHeader.this_pid == pidTs.PID_pmt)
    {
        parser_pmt();
        memcpy(pmtData,buf,TS_PACKAGE_SIZE);
        return 0;
    }
    else if(tsHeader.this_pid == pidTs.PID_pcr)
    {
 //       return 0;
    }
    else if(tsHeader.this_pid == pidTs.PID_vedio)
        parser_vedio();
    else if(tsHeader.this_pid == pidTs.PID_audio)
        parser_audio();
    if(fp_TS)
    {
        fwrite(buf,TS_PACKAGE_SIZE,1,fp_TS);
    }
    return 0;
}

int TsParser::get_ts_packet()
{
    return udpSocket->udp_get_queue(buf,bufSize);

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
