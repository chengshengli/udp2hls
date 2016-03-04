#ifndef TSPARSER_H
#define TSPARSER_H

#include <stdint.h>
#include <pthread.h>

#define TS_PACKAGE_SIZE 188

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
};

class TsParser
{
public:

    TsParser();
    void startup(void);

private:
    pthread_t thid;
    uint8_t *buf;
    uint8_t buf_size;
    uint16_t pid;
    PID_TS pid_ts;
    TS_Header ts_header;
    PAT_Info pat_info;
    PMT_Info pmt_info;
    VIDEO_Info video_info;
    int get_ts_packet();
    int parser_header(void);
    int parser_pat(void);
    int parser_pmt(void);
    int parser_vedio(void);
    int parser_audio(void);
    int parser_ts_packet(void);
    static void *ts_parser_thread(void *arg);
};

#endif // TSPARSER_H
