// Copyright (c) 2019, Peter Ohler, All rights reserved.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "debug.h"
#include "dtime.h"
#include "io.h"
#include "log.h"
#include "pub.h"
#include "server.h"
#include "subject.h"
#include "upgraded.h"


static int
con_send(agooErr err, agooIO io, agooCon c) {
    if (NULL != c->res_head && NULL != c->bind->write) {
	if (c->bind->write(c)) {
	    // TBD error, mark as closed?
	}
    }
    return AGOO_ERR_OK;
}

// TBD make sure the con max is not exceeded. Create another io object with threads if the max is exceeded.


static void
publish_pub(agooPub pub, agooIO io) {
    agooUpgraded	up;
    const char		*sub = pub->subject->pattern;
    int			cnt = 0;

    for (up = agoo_server.up_list; NULL != up; up = up->next) {
	if (NULL != up->con && up->con->io == io && agoo_upgraded_match(up, sub)) {
	    agooRes	res = agoo_res_create(up->con);

	    if (NULL != res) {
		if (NULL == up->con->res_tail) {
		    up->con->res_head = res;
		} else {
		    up->con->res_tail->next = res;
		}
		up->con->res_tail = res;
		res->con_kind = AGOO_CON_ANY;
		agoo_res_set_message(res, agoo_text_dup(pub->msg));
		cnt++;
	    }
	}
    }
}
static void
unsubscribe_pub(agooPub pub) {
    if (NULL == pub->up) {
	agooUpgraded	up;

	for (up = agoo_server.up_list; NULL != up; up = up->next) {
	    agoo_upgraded_del_subject(up, pub->subject);
	}
    } else {
	agoo_upgraded_del_subject(pub->up, pub->subject);
    }
}

static void
process_pub_con(agooPub pub, agooIO io) {
    agooUpgraded	up = pub->up;

    if (NULL != up && NULL != up->con && up->con->io == io) {
	int	pending;

	// TBD Change pending to be based on length of con queue
	if (1 == (pending = atomic_fetch_sub(&up->pending, 1))) {
	    if (NULL != up && agoo_server.ctx_nil_value != up->ctx && up->on_empty) {
		agooReq	req = agoo_req_create(0);

		req->up = up;
		req->method = AGOO_ON_EMPTY;
		req->hook = agoo_hook_create(AGOO_NONE, NULL, up->ctx, PUSH_HOOK, &agoo_server.eval_queue);
		agoo_upgraded_ref(up);
		agoo_queue_push(&agoo_server.eval_queue, (void*)req);
	    }
	}
    }
    switch (pub->kind) {
    case AGOO_PUB_CLOSE:
	// A close after already closed is used to decrement the reference
	// count on the upgraded so it can be destroyed in the io loop
	// threads.
	if (NULL != up->con && up->con->io == io) {
	    agooRes	res = agoo_res_create(up->con);

	    if (NULL != res) {
		if (NULL == up->con->res_tail) {
		    up->con->res_head = res;
		} else {
		    up->con->res_tail->next = res;
		}
		up->con->res_tail = res;
		res->con_kind = up->con->bind->kind;
		res->close = true;
	    }
	}
	break;
    case AGOO_PUB_WRITE: {
	if (NULL == up->con) {
	    agoo_log_cat(&agoo_warn_cat, "Connection already closed. WebSocket write failed.");
	} else if (up->con->io == io) {
	    agooRes	res = agoo_res_create(up->con);

	    if (NULL != res) {
		if (NULL == up->con->res_tail) {
		    up->con->res_head = res;
		} else {
		    up->con->res_tail->next = res;
		}
		up->con->res_tail = res;
		res->con_kind = AGOO_CON_ANY;
		agoo_res_set_message(res, pub->msg);
	    }
	}
	break;
    case AGOO_PUB_SUB:
	if (NULL != up && up->con->io == io) {
	    agoo_upgraded_add_subject(pub->up, pub->subject);
	    pub->subject = NULL;
	}
	break;
    case AGOO_PUB_UN:
	if (NULL != up && up->con->io == io) {
	    unsubscribe_pub(pub);
	}
	break;
    case AGOO_PUB_MSG:
	publish_pub(pub, io);
	break;
    }
    default:
	break;
    }
    agoo_pub_destroy(pub);
}


void*
poll_loop(void *x) {
    agooIO		io = (agooIO)x;
    struct pollfd	pa[MAX_IO_CONS + 1];
    struct pollfd	*pp;
    agooCon		c;
    agooPub		pub;
    struct _agooErr	err = AGOO_ERR_INIT;
    bool		dirty;
    int			i;
    int			pub_queue_fd = agoo_queue_listen(&io->pub_queue);
    int			err_cnt = 0;

    atomic_fetch_add(&agoo_server.running, 1);
    while (agoo_server.active) {
	pthread_mutex_lock(&io->con_lock);
	while (NULL != (c = io->new_cons)) {
	    io->new_cons = c->next;
	    c->io = io;
	    c->next = io->cons;
	    io->cons = c;
	    atomic_fetch_add(&io->con_cnt, 1);
	}
	pthread_mutex_unlock(&io->con_lock);
	dirty = false;
	pp = pa;
	pp->fd = pub_queue_fd;
	pp->revents = 0;
	pp++;
	for (c = io->cons; NULL != c; c = c->next) {
	    if (c->dead || 0 == c->sock) {
		printf("*** dead\n");
		dirty = true;
		continue;
	    }
	    if (AGOO_ERR_OK != con_send(&err, io, c)) {
		agoo_log_cat(&agoo_error_cat, "Send on %llu failed.", (unsigned long long)c->id);
		c->dead = true;
		dirty = true;
		continue;
	    }
	    pp->fd = c->sock;
	    c->pp = pp;
	    pp->events = POLLERR | POLLIN;
	    pp->revents = 0;
	    pp++;
	}
	if (0 > (i = poll(pa, pp - pa, 10))) {
	    if (EAGAIN == errno) {
		continue;
	    }
	    agoo_log_cat(&agoo_warn_cat, "poll() failed.");
	    err_cnt++;
	    if (3 < err_cnt) {
		agoo_log_cat(&agoo_error_cat, "too many polling errors.");
		agoo_server_shutdown("", NULL);
	    }
	    continue;
	}
	err_cnt = 0;
	if (0 == i) {
	    continue;
	}
	while (NULL != (pub = (agooPub)agoo_queue_pop(&io->pub_queue, 0.0))) {
	    process_pub_con(pub, io);
	}
	for (c = io->cons; NULL != c; c = c->next) {
	    if (c->dead || 0 == c->sock || NULL == c->pp || 0 == c->pp->revents) {
		dirty = true;
		continue;
	    }
	    if (0 != (c->pp->revents & POLLERR)) {
		agoo_log_cat(&agoo_warn_cat, "Error on socket %llu.", (unsigned long long)c->id);
		c->dead = true;
		dirty = true;
		continue;
	    }
	    if (0 != (c->pp->revents & POLLIN)) {
		if (!atomic_flag_test_and_set(&c->queued)) {
		    //printf("*** push\n");
		    agoo_queue_push(&io->recv_queue, c);
		}
	    }
	}
	if (dirty) {
	    agooCon	prev = NULL;
	    agooCon	next;

	    for (c = io->cons; NULL != c; c = next) {
		next = c->next;
		if (c->dead || 0 == c->sock) {
		    // TBD make sure it is not on the recv_queue
		    next = c->next;
		    if (NULL == prev) {
			io->cons = next;
		    } else {
			prev->next = next;
		    }
		    atomic_fetch_sub(&io->con_cnt, 1);
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
    agooIO	io = (agooIO)x;
    agooCon	c;

    atomic_fetch_add(&agoo_server.running, 1);
    while (agoo_server.active) {
	if (NULL == (c = agoo_queue_pop(&io->recv_queue, 0.1))) {
	    continue;
	}
	if (NULL != c->bind->read) {
	    if (c->bind->read(c)) {
		// TBD error, mark as closed?
	    }
	}
	//printf("*** pop\n");
	atomic_flag_clear(&c->queued);
    }
    atomic_fetch_sub(&agoo_server.running, 1);

    return NULL;
}

agooIO
agoo_io_create(agooErr err) {
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
	io->cons = NULL;
	io->new_cons = NULL;
	io->res_head = NULL;
	io->res_tail = NULL;
	io->con_cnt = AGOO_ATOMIC_INT_INIT(0);
	if (0 != pthread_mutex_init(&io->con_lock, 0) ||
	    0 != pthread_mutex_init(&io->res_lock, 0)) {
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

int
agoo_io_con_count(agooIO io) {
    return atomic_load(&io->con_cnt);
}

void
agoo_io_add_con(agooIO io, struct _agooCon *con) {
    con->io = io;
    pthread_mutex_lock(&io->con_lock);
    con->next = io->new_cons;
    io->new_cons = con;
    pthread_mutex_unlock(&io->con_lock);
}
