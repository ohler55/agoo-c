// Copyright (c) 2019, Peter Ohler, All rights reserved.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "debug.h"
#include "dtime.h"
#include "io.h"
#include "server.h"


static int
con_send(agooErr err, agooIO io, agooCon c) {

    // TBD send

    return AGOO_ERR_OK;
}

// TBD make sure the con max is not exceeded. Create another io object with threads if the max is exceeded.

void*
poll_loop(void *x) {
    agooIO		io = (agooIO)x;
    struct pollfd	pa[MAX_IO_CONS + 2];
    struct pollfd	*pp;
    agooCon		c;
    struct _agooErr	err = AGOO_ERR_INIT;
    bool		dirty;
    int			i;
    //int			con_queue_fd = agoo_queue_listen(&agoo_server.con_queue);
    //int			pub_queue_fd = agoo_queue_listen(&io->pub_queue);

    atomic_fetch_add(&agoo_server.running, 1);
    while (agoo_server.active) {
	dirty = false;
	for (c = io->cons, pp = pa; NULL != c; c = c->next) {
	    if (c->dead || 0 == c->sock) {
		dirty = true;
		continue;
	    }
	    if (AGOO_ERR_OK != con_send(&err, io, c)) {
		// TBD log error? and close
	    }
	    pp->fd = c->sock;
	    c->pp = pp;
	    pp->events = POLLERR | POLLIN;
	    pp->revents = 0;
	    pp++;
	}
	// TBD what about queue notifiers?
	if (0 > (i = poll(pa, pp - pa, 10))) {
	    if (EAGAIN == errno) {
		continue;
	    }
	    // TBD log error, maybe keep a count and exit if too many
	    //printf("*-*-* polling error: %s\n", strerror(errno));
	    continue;
	}
	if (0 == i) {
	    continue;
	}
	for (c = io->cons; NULL != c; c = c->next) {
	    if (c->dead || 0 == c->sock || NULL == c->pp || 0 == c->pp->revents) {
		dirty = true;
		continue;
	    }
	    if (0 != (c->pp->revents & POLLERR)) {
		// TBD log error and mark con as closed
	    }
	    if (0 != (c->pp->revents & POLLIN)) {
		if (!atomic_flag_test_and_set(&c->queued)) {
		    agoo_queue_push(&io->recv_queue, c);
		}
	    }
	}
	if (dirty) {
	    agooCon	prev = NULL;
	    agooCon	next;

	    for (c = io->cons; NULL != c; c = next) {
		if (c->dead || 0 == c->sock) {
		    // TBD make sure it is not on the recv_queue
		    next = c->next;
		    if (NULL == prev) {
			io->cons = next;
		    } else {
			prev->next = next;
		    }
		    agoo_con_destroy(c);
		} else {
		    prev = c;
		}
	    }
	}
    }
    atomic_fetch_sub(&agoo_server.running, 1);

    return NULL;
}

void*
recv_loop(void *x) {
    //agooIO	io = (agooIO)x;

    atomic_fetch_add(&agoo_server.running, 1);
    while (agoo_server.active) {

	// TBD attempt recv

	dsleep(0.2); // TBD remove
    }
    atomic_fetch_sub(&agoo_server.running, 1);

    return NULL;
}

agooIO
agoo_io_create(agooErr err, int id) {
     agooIO	io;

    if (NULL == (io = (agooIO)AGOO_MALLOC(sizeof(struct _agooIO)))) {
	AGOO_ERR_MEM(err, "IO handler");
    } else {
	int	stat;

	io->next = NULL;
	if (AGOO_ERR_OK != agoo_queue_multi_init(err, &io->pub_queue, 256, true, false) ||
	    AGOO_ERR_OK != agoo_queue_multi_init(err, &io->recv_queue, MAX_IO_CONS + 4, false, false)) {
	    AGOO_FREE(io);
	    return NULL;
	}
	io->id = id;
	io->cons = NULL;
	io->res_head = NULL;
	io->res_tail = NULL;
	if (0 != pthread_mutex_init(&io->lock, 0)) {
	    AGOO_FREE(io);
	    agoo_err_no(err, "Failed to initialize io mutex.");
	    return NULL;
	}
	if (0 != (stat = pthread_create(&io->poll_thread, NULL, recv_loop, io))) {
	    agoo_err_set(err, stat, "Failed to create poll thread. %s", strerror(stat));
	    return NULL;
	}
	if (0 != (stat = pthread_create(&io->recv_thread, NULL, poll_loop, io))) {
	    agoo_err_set(err, stat, "Failed to create receiving thread. %s", strerror(stat));
	    return NULL;
	}
    }
    return io;
}

void
agoo_io_destroy(agooIO io) {
    volatile agooRes	res;

    agoo_queue_cleanup(&io->pub_queue);
    while (NULL != (res = io->res_head)) {
	io->res_head = res->next;
	AGOO_FREE(res);
    }
    AGOO_FREE(io);
}
