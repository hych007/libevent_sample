#include <stdio.h>
#include <event.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

int pp[2];
struct event *evw;
struct event *evr;

void
read_cb(evutil_socket_t fd, short evtype, void *arg) {
        printf("read_cb\n");
        char buf[1024];
        int ret = read(fd, buf, 1024);
        buf[ret] = '\0';
        printf("read == %s\n", buf);

        /* test event_active and event_pending function. */
        if(event_pending(evw, EV_WRITE, NULL) == 0) {
                printf("evw being pending, make it active now.\n");
                event_active(evw, EV_WRITE, 1); /* no matter the event is pending nor non-pending. */
        }
}

void
write_cb(evutil_socket_t fd, short evtype, void *arg) {
        printf("write_cb\n");

        /* when callback is executed, event are active. */

        /* just write once */
        static bool bfirst = true;
        if(bfirst == true) {
                write(pp[1], "hello", 5);
                bfirst = false;
        }


}

int
main() {
        pipe(pp);
        struct event_base *base = event_base_new();
        evr = event_new(base, pp[0], EV_READ, read_cb, NULL);
        evw = event_new(base, pp[1], EV_WRITE  | EV_TIMEOUT, write_cb, NULL);

        event_add(evw, NULL);
        event_add(evr, NULL);

        struct event_base *outbase;
        int fdout;
        event_get_assignment(evw, NULL, &fdout, NULL, NULL, NULL);		/* Note fdout. */
        outbase = event_get_base(evw);		/* outbase == base */
        event_callback_fn cb = event_get_callback(evw);
        assert(write_cb == cb);
        assert(NULL == event_get_callback_arg(evw));
        assert(5 == event_get_events(evw));
        assert(pp[1] == event_get_fd(evw));
        strcmp("2.0.21-stable",event_get_version());
        assert(0x2001500 == event_get_version_number());
        assert(event_initialized(evw) == 1);
        event_base_dispatch(base);

        return 0;
}


