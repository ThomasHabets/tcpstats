#include<stdio.h>
#include<dlfcn.h>
#include<string.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<arpa/inet.h>

static const int PORT = 12346;
static const int DEBUG = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int (*orig_close)(int fd);
static void *dlhandle = NULL;

/**
 *
 */
static void
send_data(const struct tcp_info *ti,
          const struct sockaddr *peer, socklen_t peerlen)
{
        int fd;
        struct sockaddr_in sa;
        char buf[sizeof(struct tcp_info) + peerlen];

        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(PORT);
        sa.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (-1 == (fd = socket(PF_INET, SOCK_DGRAM, 0))) {
                if (DEBUG) {
                        fprintf(stderr, "tcpstats: socket(): %m\n");
                }
                return;
        }
        memcpy(buf, ti, sizeof(struct tcp_info));
        memcpy(buf + sizeof(struct tcp_info), peer, peerlen);
        if (-1 == sendto(fd,
                         buf,
                         sizeof(struct tcp_info) + peerlen,
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
