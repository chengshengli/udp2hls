#ifndef TSPARSER_H
#define TSPARSER_H

#include <stdint.h>
#include <pthread.h>
#include <fstream>
#define TS_PACKAGE_SIZE 188
#define LIST_TS_FILE_NUM 5

struct PID_TS{
    uint16_t PID_pat;
    uint16_t PID_pmt;
    uint16_t PID_audio;
    uint16_t PID_vedio;
    uint16_t PID_pcr;
};
struct TS_Header{
    uint8_t indicator;
    uint16_t this_pid;
    uint8_t adaptation_filed;

    uint8_t counter;
};
struct PAT_Info{
    uint8_t PAT_version;
    uint8_t PAT_change;
    uint8_t PAT_enble;
};
struct PMT_Info{
    uint8_t PMT_version;
    uint8_t PMT_change;
};
struct VIDEO_Info{
    uint8_t counter;
    uint64_t PTS;
    uint64_t DTS;
    uint32_t second;
    uint32_t msecond;
};

struct List_Info
{
  //  char sList[16][64];
    string sList[16];
    int listLen;
    List_Info()
    {
        listLen = 0;
    }
};

class TsParser
{
public:

    TsParser();
    void startup(void);

private:
    pthread_t thid;
    uint8_t *buf;
    uint8_t bufSize;
    uint16_t pid;
    PID_TS pidTs;
    TS_Header tsHeader;
    PAT_Info patInfo;
    PMT_Info pmtInfo;
    VIDEO_Info videoInfo;
    float deltTime;
    uint32_t minLastPTS;
    uint32_t initPTS;
    bool first;
    bool patReady;

    List_Info listM3u8;
    uint32_t listSeq;
    ofstream fp_TS;
    uint32_t tsFileCount;
    string updateName;
    //char updateName[32];
    string deleteName;
    //string deleteName;
    uint8_t patData[188];
    uint8_t pmtData[188];

    void close_tsfile(void);
    void new_tsfile(void);
    void delete_tsfile(void);
    void update_m3u8(void);
    void deal_IDR(void);
    void drop_tsfile(void);

    int get_ts_packet();
    int parser_header(void);
    int parser_pat(void);
    int parser_pmt(void);
    int parser_vedio(void);
    int parser_nal(uint8_t index);
    int parser_audio(void);
    int parser_ts_packet(void);
    static void *ts_parser_thread(void *arg);
};

#endif // TSPARSER_H
