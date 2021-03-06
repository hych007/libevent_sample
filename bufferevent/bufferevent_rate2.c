#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <assert.h>

#define SVRADDR "127.0.0.1"
#define PORT 8080

struct event_base *p_base;

static void
buff_input_cb (struct bufferevent *bev,
               void *ctx)
{
    printf("***in %s\n", __func__);

    int len = evbuffer_get_length(bufferevent_get_input(bev));
    printf("len=%d\n", len);

    char buff[1024];
    /* first read data, second make READ event pending */
    bufferevent_read(bev, buff, 1024);
    len = evbuffer_get_length(bufferevent_get_input(bev));
    printf("len=%d\n", len);

    return;
}

int
main ()
{
    int sockfd;
    struct bufferevent *p_event;
    struct sockaddr_in addr;
    int cc;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SVRADDR, &addr.sin_addr) <= 0) {
        printf("inet_pton");
        exit(1);
    }

    if ((p_base = event_base_new()) == NULL) {
        printf("event_base_new ");
        return 1;
    }

    /* we do not need invoke socket function to create socket */
    if ((p_event = bufferevent_socket_new(p_base, -1, BEV_OPT_CLOSE_ON_FREE)) == NULL) {
        printf("bufferevent_socket_new ");
        return 1;
    }

    /* client actually connecte to server at this time. */
    if ((sockfd = bufferevent_socket_connect(p_event, (struct sockaddr *) &addr,
                                             sizeof(addr))) < 0) {
        printf("bufferevent_socket_connect ");
        return 1;
    }

    /* EV_WRITE is default enabled, EV_READ is default disabled */
    /* So If we disable READ, evbuffer callback will not be added to base (read and write) */
    bufferevent_setcb(p_event, buff_input_cb, NULL, NULL, p_base);
    bufferevent_enable(p_event, EV_WRITE | EV_READ);

    /* you should not free cfg immediately, after call bufferevent_set_rate_limit. */
    /* you should not set the burst rate little than average rate */
    struct ev_token_bucket_cfg *cfg = ev_token_bucket_cfg_new (EV_RATE_LIMIT_MAX, EV_RATE_LIMIT_MAX, 1, 1, NULL);
    assert (NULL != cfg);

    /* If Server send N bytes, every time, 
        * read callback will be invoked when 2 bytes read to evbuffer.
        */
    cc = bufferevent_set_rate_limit (p_event, cfg);
//    assert (cc == 0);

    /* this is the defauly value EV_SSIZE_MAX, 0x7FFFFFFF */
    ev_ssize_t size = bufferevent_get_read_limit (p_event);
    printf ("bufferevent read limit 0x%x\n", size);   /* 2, average rate */
    size = bufferevent_get_write_limit (p_event);
    printf ("bufferevent write limit 0x%x\n", size);  /* 1, average rate */

    event_base_dispatch(p_base);

    ev_token_bucket_cfg_free (cfg);

    return 0;
}
