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
#include "agoo/req.h"
#include "agoo/res.h"
#include "agoo/server.h"
#include "agoo/graphql.h"

static volatile bool	running = true;

static void
sig_handler(int sig) {
    running = false;
}

int
agoo_init(agooErr err, const char *app_name) {
    agoo_log_init(err, app_name);
    if (AGOO_ERR_OK != agoo_log_start(err, false)) {
	return err->code;
    }
    agoo_server_setup(err);

    return AGOO_ERR_OK;
}

int
agoo_bind_to_url(agooErr err, const char *url) {
    agooBind	b = agoo_bind_url(err, url);

    if (NULL == b) {
	return err->code;
    }
    b->read = agoo_con_http_read;
    b->write = agoo_con_http_write;
    b->events = agoo_con_http_events;
    agoo_server_bind(b);

    return AGOO_ERR_OK;
}

int
agoo_bind_to_port(agooErr err, int port) {
    agooBind	b = agoo_bind_port(err, port);

    if (NULL == b) {
	return err->code;
    }
    b->read = agoo_con_http_read;
    b->write = agoo_con_http_write;
    b->events = agoo_con_http_events;
    agoo_server_bind(b);

    return AGOO_ERR_OK;
}

int
agoo_add_func_hook(agooErr	err,
		   agooMethod	method,
		   const char	*pattern,
		   void		(*func)(agooReq req),
		   bool		quick) {

    return agoo_server_add_func_hook(err, method, pattern, func, &agoo_server.eval_queue, quick);
}

int
agoo_setup_graphql(agooErr err, const char *path) {
    char	schema_path[1024];

    // TBD
    if (AGOO_ERR_OK != gql_init(err)) {
	return err->code;
    }
    snprintf(schema_path, sizeof(schema_path), "%s/schema", path);
    if (AGOO_ERR_OK != agoo_server_add_func_hook(err, AGOO_GET, path, gql_eval_get_hook, &agoo_server.eval_queue, false) ||
	AGOO_ERR_OK != agoo_server_add_func_hook(err, AGOO_POST, path, gql_eval_post_hook, &agoo_server.eval_queue, false) ||
	AGOO_ERR_OK != agoo_server_add_func_hook(err, AGOO_GET, schema_path, gql_dump_hook, &agoo_server.eval_queue, false)) {
	return err->code;
    }
    return AGOO_ERR_OK;
}

static void
bad_request(agooReq req, int status, int line, const char *body) {
    const char *msg = agoo_http_code_message(status);
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
    agoo_res_set_message(req->res, agoo_text_create(buf, cnt));
}

static void*
eval_loop(void *ptr) {
    agooReq	req;

    atomic_fetch_add(&agoo_server.running, 1);
    while (agoo_server.active) {
	if (NULL != (req = (agooReq)agoo_queue_pop(&agoo_server.eval_queue, 0.1))) {
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
	    //agoo_queue_wakeup(&agoo_server.con_queue);
	    agoo_req_destroy(req);
	}
    }
    atomic_fetch_sub(&agoo_server.running, 1);

    return NULL;
}

int
agoo_start(agooErr err, const char *version) {
    pthread_t	*threads;

    if (NULL == (threads = (pthread_t*)malloc(sizeof(pthread_t) * agoo_server.thread_cnt))) {
	agoo_err_set(err, AGOO_ERR_MEMORY, "failed to allocate memory for thread pool.");
	return err->code;
    }
    pthread_t	*tp = threads;

    agoo_server.active = true;
    for (int i = 0; i < agoo_server.thread_cnt; i++, tp++) {
	if (0 != pthread_create(tp, NULL, eval_loop, NULL)) {
	    agoo_err_no(err, "failed to start threads");
	    return err->code;
	}
    }
    // TBD wait for threads to be started?
    // TBD is running reset?

    agoo_server.inited = true;
    if (AGOO_ERR_OK != setup_listen(err) ||
	AGOO_ERR_OK != agoo_server_start(err, agoo_log.app, version)) {
	return err->code;
    }
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    while (running) {
	dsleep(0.5);
    }
    agoo_server_shutdown(agoo_log.app, NULL);
    agoo_log_flush(0.5);

    return AGOO_ERR_OK;
}

void
agoo_shutdown(void (*stop)()) {
    agoo_server_shutdown(agoo_log.app, stop);
}

agooText
agoo_respond(int status, const char *body, int blen, agooKeyVal headers) {
    agooText	t;
    agooKeyVal	h;

    if (0 > blen) {
	if (NULL == body) {
	    blen = 0;
	} else {
	    blen = (int)strlen(body);
	}
    }
    if (NULL == (t = agoo_text_allocate(256 + blen))) {
	// If out of memory for this then might as well exit.
	printf("*-*-* Out of memory *-*-*\n");
	exit(AGOO_ERR_MEMORY);
	return NULL;
    }
    t->len = snprintf(t->text, 256, "HTTP/1.1 %d %s\r\nContent-Length: %u\r\n", status, agoo_http_code_message(status), blen);
    if (NULL != headers) {
	for (h = headers; NULL != h->key; h++) {
	    t = agoo_text_append(t, h->key, -1);
	    t = agoo_text_append(t, ": ", 2);
	    t = agoo_text_append(t, h->value, -1);
	    t = agoo_text_append(t, "\r\n", 2);
	}
    }
    t = agoo_text_append(t, "\r\n", 2);
    if (NULL != body) {
	t = agoo_text_append(t, body, blen);
    }
    return t;
}
