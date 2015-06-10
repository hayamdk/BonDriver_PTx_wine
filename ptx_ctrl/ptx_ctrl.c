#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "pt3_ioctl.h"
#include "ptx_ctrl.h"

#define MAX_DEV_NUM 99

static int gettime_ms(uint64_t *ms)
{
    struct timeval tv;
    if(gettimeofday(&tv, NULL) == 0) {
        *ms = (uint64_t)tv.tv_sec*1000 + tv.tv_usec/1000;
        return 0;
    } else {
        return -1;
    }
}

static int ptx_open(const char *devfmt, tuner_type_t tuner_type)
{
    int i, fd;
    char dev[128];
    
    if(tuner_type == ISDB_T) {
        i = 2;
    } else {
        i = 0;
    }
    
    while(i <= MAX_DEV_NUM) {
        sprintf(dev, devfmt, i);
        fd = ptx_open_device(dev);
        if(fd < 0) {
            if( errno == ENOENT ) {
                return -1;
            }
        } else {
            //printf("open: %s\n", dev);
            return fd;
        }
        
        if(i%2 == 0) {
            i++;
        } else {
            i+=3;
        }
    }
    return -1;
}

PTXCTRL_FUNC int pt3_open(tuner_type_t tuner_type)
{
    return ptx_open("/dev/pt3video%d", tuner_type);
}

PTXCTRL_FUNC int pt1_open(tuner_type_t tuner_type)
{
   return ptx_open("/dev/pt1video%d", tuner_type);
}

PTXCTRL_FUNC int ptx_open_device(const char *dev)
{
    return open(dev, O_RDONLY);
}

PTXCTRL_FUNC int ptx_close_device(int fd)
{
    return close(fd);
}

PTXCTRL_FUNC int ptx_select(int fd, int timeout_ms)
{
    struct timespec ts = {0, 0};
    fd_set fds;
    
    if(timeout_ms > 0) {
        ts.tv_sec = timeout_ms/1000;
        ts.tv_nsec = (timeout_ms%1000)*1000*1000;
    }
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    return pselect(fd+1, &fds, NULL, NULL, &ts, NULL);
}

PTXCTRL_FUNC int ptx_read(int fd, uint8_t *buf, int maxsize)
{
    int ret = read(fd, buf, maxsize);
    //printf("read %d bytes\n", ret);
    return ret;
}

PTXCTRL_FUNC void ptx_purge(int fd)
{
    uint8_t tmp[188*256];
    uint64_t t1, t;
    
    if(gettime_ms(&t1) < 0) {
        return;
    }
    
    while(ptx_select(fd, 0) > 0) {
        //printf("purge read! ...\n");
        if(ptx_read(fd, tmp, 188*256) <= 0) {
            //printf("read 0\n");
            return;
        }
        
        if(gettime_ms(&t) < 0 ) {
            return;
        }
        if(t-t1 >= 1000) {
            /* 1000ms timeout */
            //printf("1000ms timeout!\n");
            return;
        }
        //printf("end\n");
    }
}

PTXCTRL_FUNC int ptx_tune(int fd, FREQUENCY *freq)
{
    //printf("tune: %d %d\n", freq->frequencyno, freq->slot);
    //return ioctl(fd, SET_CHANNEL, freq);
    int ret = ioctl(fd, SET_CHANNEL, freq);
    ptx_getlevel_t(fd);
    return ret;
}

PTXCTRL_FUNC double ptx_getlevel_t(int fd)
{
    int rc;
    double p;
    if( ioctl(fd, GET_SIGNAL_STRENGTH, (uint64_t)(&rc)) < 0 ) {
        return 0.0;
    }
    
    //printf("rc=%d\n", rc);

    p = log10(5505024/(double)rc) * 10;
    return (0.000024 * p * p * p * p) - (0.0016 * p * p * p) +
                (0.0398 * p * p) + (0.5491 * p)+3.0965;
}

PTXCTRL_FUNC double ptx_getlevel_s(int fd)
{
    int rc;
    unsigned char sigbuf[4];
    float fMixRate;
    
    /* apply linear interpolation */
    static const float afLevelTable[] = {
        24.07f,    // 00    00    0        24.07dB
        24.07f,    // 10    00    4096     24.07dB
        18.61f,    // 20    00    8192     18.61dB
        15.21f,    // 30    00    12288    15.21dB
        12.50f,    // 40    00    16384    12.50dB
        10.19f,    // 50    00    20480    10.19dB
        8.140f,    // 60    00    24576    8.140dB
        6.270f,    // 70    00    28672    6.270dB
        4.550f,    // 80    00    32768    4.550dB
        3.730f,    // 88    00    34816    3.730dB
        3.630f,    // 88    FF    35071    3.630dB
        2.940f,    // 90    00    36864    2.940dB
        1.420f,    // A0    00    40960    1.420dB
        0.000f     // B0    00    45056    -0.01dB
    };
    
    if( ioctl(fd, GET_SIGNAL_STRENGTH, (uint64_t)(&rc)) < 0 ) {
        return 0.0;
    }

    memset(sigbuf, '\0', sizeof(sigbuf));
    sigbuf[0] =  (((rc & 0xFF00) >> 8) & 0XFF);
    sigbuf[1] =  (rc & 0xFF);

    /* calculate signal level */
    if(sigbuf[0] <= 0x10U) {
        /* clipped maximum */
        return 24.07f;
    } else if (sigbuf[0] >= 0xB0U) {
        /* clipped minimum */
        return 0.0f;
    } else {
        /* linear interpolation */
        fMixRate =
            (float)(((unsigned short)(sigbuf[0] & 0x0FU) << 8) |
                    (unsigned short)sigbuf[0]) / 4096.0f;
        return (double)( afLevelTable[sigbuf[0] >> 4] * (1.0f - fMixRate) +
            afLevelTable[(sigbuf[0] >> 4) + 0x01U] * fMixRate );
    }
}

PTXCTRL_FUNC int ptx_start(int fd)
{
    return ioctl(fd, START_REC, 0);
}

PTXCTRL_FUNC int ptx_stop(int fd)
{
    return ioctl(fd, STOP_REC, 0);
}

/*
int main()
{
    int fd = pt3_open(ISDB_T);
    if(fd < 0) {
        printf("open failed!\n");
        return 0;
    }
    ptx_start(fd);
    sleep(10);
    ptx_stop(fd);
    ptx_close_device(fd);
    return 0;
}
*/
