#include<stdio.h>
#include<dlfcn.h>
#include<string.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<arpa/inet.h>

#include"data.h"

static const int PORT = 12346;
static const int DEBUG = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int (*orig_close)(int fd);
static void *dlhandle = NULL;
static uint64_t serial = 0;

/**
 *
 */
static void
send_data(const struct tcp_info *ti,
          const struct sockaddr *peer, socklen_t peerlen)
{
        int fd;
        struct sockaddr_in sa;
        struct tcp_info tcpi;

        memcpy(&tcpi, ti, sizeof(tcpi));
        tcpi.tcpi_rtt = htonl(tcpi.tcpi_rtt);
        tcpi.tcpi_rto = htonl(tcpi.tcpi_rto);

        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(PORT);
        sa.sin_addr.s_addr = inet_addr("224.1.2.3");

        if (-1 == (fd = socket(PF_INET, SOCK_DGRAM, 0))) {
                if (DEBUG) {
                        fprintf(stderr, "tcpstats: socket(): %m\n");
                }
                return;
        }

        char buf[1024];
        char tmpbuf[1024];
        char *p = buf;
#pragma pack(1)
        struct tlv {
                uint16_t type;
                uint16_t length;
                char val[1];
        };
#pragma pack()
        const int HEAD_SIZE = 4;
        struct tlv *pt;

        // version
        pt = (void*)p;
        pt->type = htons(DATA_VERSION);
        pt->length = htons(1);
        pt->val[0] = 1;
        p += HEAD_SIZE + htons(pt->length);

        // serial
        {
                uint64_t s = htonl(serial);
                pt = (void*)p;
                pt->type = htons(DATA_SERIAL);
                pt->length = htons(8);
                memcpy(pt->val, &s, 8);
                p += HEAD_SIZE + htons(pt->length);
        }

        // peer v4 address
        if (peer->sa_family == AF_INET) {
                struct sockaddr_in *sin = (void*)peer;
                pt = (void*)p;
                pt->type = htons(DATA_PEERV4);
                pt->length = htons(4);
                memcpy(pt->val, &sin->sin_addr, 4);
                p += HEAD_SIZE + htons(pt->length);
        }

        // RTT
        pt = (void*)p;
        pt->type = htons(DATA_RTT);
        pt->length = htons(4);
        memcpy(pt->val, &tcpi.tcpi_rtt, 4);
        p += HEAD_SIZE + htons(pt->length);

        // RTO
        pt = (void*)p;
        pt->type = htons(DATA_RTO);
        pt->length = htons(4);
        memcpy(pt->val, &tcpi.tcpi_rto, 4);
        p += HEAD_SIZE + htons(pt->length);

        if (-1 == sendto(fd,
                         buf,
                         p - buf,
                         0,
                         (struct sockaddr*)&sa,
                         sizeof(sa))) {
                if (DEBUG) {
                        fprintf(stderr, "tcpstats: sendto(): %m\n");
                }
        }

        orig_close(fd);
}

/**
 *
 */
int
close(int fd)
{
        struct tcp_info ti;
        socklen_t tilen = sizeof(ti);
        struct sockaddr_storage ss;
        socklen_t sslen = sizeof(ss);

        memset(&ti, 0, sizeof(ti));
        memset(&ss, 0, sizeof(ss));

        /* *** */
        if (DEBUG) {
                fprintf(stderr, "tcpstats: close(%d)\n", fd);
        }

        /* get orig_close pointer */
        pthread_mutex_lock(&mutex);
        serial++;
        if (!dlhandle) {
                dlhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
                if (!dlhandle) {
                        if (DEBUG) {
                                fprintf(stderr, "tcpstats: dlopen(): %m\n");
                        }
                        goto errout_unlock;
                }
                orig_close = dlsym(dlhandle, "close");
        }
        pthread_mutex_unlock(&mutex);

        /* *** */
        if (!getsockopt(fd,
                        IPPROTO_TCP,
                        TCP_INFO,
                        &ti,
                        &tilen)
            && !getpeername(fd,
                            (struct sockaddr*)&ss,
                            &sslen)) {
                if (DEBUG) {
                        fprintf(stderr, "tcpstats: sizeof %d\n", sizeof(ti));
                        fprintf(stderr, "tcpstats: options: %.2x\n", ti.tcpi_options);
                        fprintf(stderr, "tcpstats: snd_wscale: %.1x\n", ti.tcpi_snd_wscale);
                        fprintf(stderr, "tcpstats: rcv_wscale: %.1x\n", ti.tcpi_rcv_wscale);
                        fprintf(stderr, "tcpstats: rto: %.8x\n", ti.tcpi_rto);
                        fprintf(stderr, "tcpstats: pmtu: %.8x\n", ti.tcpi_pmtu);
                }
                send_data(&ti, (struct sockaddr*)&ss, sslen);
        }

        /* *** */
        return orig_close(fd);

 errout_unlock:
        pthread_mutex_unlock(&mutex);
        return -1;
}

/**
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * fill-column: 79
 * End:
 */
