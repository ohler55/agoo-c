// Copyright (c) 2018, Peter Ohler, All rights reserved.

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "agoo.h"
#include "agoo/bind.h"
#include "agoo/con.h"
#include "agoo/dtime.h"
#include "agoo/http.h"
#include "agoo/log.h"
#include "agoo/res.h"
#include "agoo/server.h"

static volatile bool	running = true;

static void
sig_handler(int sig) {
    running = false;
}

void
agoo_init(const char *app_name) {
    log_init(app_name);
    log_start(false);
    server_setup();
}

int
agoo_bind_url(Err err, const char *url) {
    Bind	b = bind_url(err, url);

    if (NULL == b) {
	return err->code;
    }
    b->read = con_http_read;
    b->write = con_http_write;
    b->events = con_http_events;
    server_bind(b);

    return ERR_OK;
}

int
agoo_bind_port(Err err, int port) {
    Bind	b = bind_port(err, port);

    if (NULL == b) {
	return err->code;
    }
    b->read = con_http_read;
    b->write = con_http_write;
    b->events = con_http_events;
    server_bind(b);

    return ERR_OK;
}

int
agoo_add_func_hook(Err		err,
		   Method	method,
		   const char	*pattern,
		   void		(*func)(Req req),
		   bool		quick) {

    return server_add_func_hook(err, method, pattern, func, &the_server.eval_queue, quick);
}

static void
bad_request(Req req, int status, int line, const char *body) {
    const char *msg = http_code_message(status);
    char	buf[1024];
    int		mlen = 0;
    int		cnt;
    
    if (NULL != body) {
	mlen = strlen(body);
	cnt = snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\nConnection: Close\r\nContent-Length: %d\r\n\r\n%s", status, msg, mlen, body);
    } else {
	cnt = snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n", status, msg);
    }
    req->res->close = true;
    res_set_message(req->res, text_create(buf, cnt));
}

static void*
eval_loop(void *ptr) {
    Req	req;

    atomic_fetch_add(&the_server.running, 1);
    while (the_server.active) {
	if (NULL != (req = (Req)queue_pop(&the_server.eval_queue, 0.1))) {
	    if (NULL == req->hook) {
		bad_request(req, 404, __LINE__, NULL);
		continue;
	    }
	    switch (req->hook->type) {
	    case FUNC_HOOK:
		req->hook->func(req);
		break;
	    default:
		bad_request(req, 404, __LINE__, NULL);
		break;
	    }
	    queue_wakeup(&the_server.con_queue);
	    req_destroy(req);
	}
    }
    atomic_fetch_sub(&the_server.running, 1);

    return NULL;
}

int
agoo_start(Err err, const char *version) {
    pthread_t	*threads;

    if (NULL == (threads = (pthread_t*)malloc(sizeof(pthread_t) * the_server.thread_cnt))) {
	err_set(err, ERR_MEMORY, "failed to allocate memory for thread pool.");
	return err->code;
    }
    pthread_t	*tp = threads;

    for (int i = 0; i < the_server.thread_cnt; i++, tp++) {
	if (0 != pthread_create(tp, NULL, eval_loop, NULL)) {
	    err_no(err, "failed to start threads");
	    return err->code;
	}
    }
    the_server.inited = true;
    if (ERR_OK != setup_listen(err) ||
	ERR_OK != server_start(err, the_log.app, version)) {
	return err->code;
    }
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    while (running) {
	dsleep(0.5);
    }
    server_shutdown(the_log.app, NULL);
    log_flush(0.5);

    return ERR_OK;
}

void
agoo_shutdown(void (*stop)()) {
    server_shutdown(the_log.app, stop);
}

void
agoo_respond(Req req, int status, const char *body, int blen, agooHeader headers) {
    Text	t;
    agooHeader	h;
    
    if (NULL == (t = text_allocate(1024))) {
	// If out of memory for this then might as well exit.
	printf("*-*-* Out of memory *-*-*\n");
	exit(ERR_MEMORY);
	return;
    }
    if (0 > blen) {
	if (NULL == body) {
	    blen = 0;
	} else {
	    blen = (int)strlen(body);
	}
    }
    t->len = snprintf(t->text, 1024, "HTTP/1.1 %d %s\r\nContent-Length: %u\r\n", status, http_code_message(status), blen);
    if (NULL != headers) {
	for (h = headers; NULL != h->key; h++) {
	    t = text_append(t, h->key, -1);
	    t = text_append(t, ": ", 2);
	    t = text_append(t, h->value, -1);
	    t = text_append(t, "\r\n", 2);
	}
    }
    t = text_append(t, "\r\n", 2);
    if (NULL != body) {
	t = text_append(t, body, blen);
    }
    res_set_message(req->res, t);
}

