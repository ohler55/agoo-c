// Copyright (c) 2018, Peter Ohler, All rights reserved.

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "agoo.h"
#include "agoo/bind.h"
#include "agoo/con.h"
#include "agoo/debug.h"
#include "agoo/dtime.h"
#include "agoo/gqlintro.h"
#include "agoo/gqlvalue.h"
#include "agoo/graphql.h"
#include "agoo/http.h"
#include "agoo/log.h"
#include "agoo/req.h"
#include "agoo/res.h"
#include "agoo/sdl.h"
#include "agoo/server.h"

static volatile bool	running = true;

double	agoo_poll_wait = 0.01;

static void
sig_handler(int sig) {
    running = false;
}

int
agoo_init(agooErr err, const char *app_name) {
    agoo_log_init(err, app_name);

    if (AGOO_ERR_OK != agoo_log_start(err, false) ||
	AGOO_ERR_OK != agoo_server_setup(err)) {
	return err->code;
    }
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

gqlRef	agoo_query_object = NULL;
gqlRef	agoo_mutation_object = NULL;

static int
schema_query(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    struct _gqlCobj	child = { .clas = ((gqlCobj)agoo_query_object)->clas, .ptr = NULL };

    return gql_eval_sels(err, doc, (gqlRef)&child, field, sel->sels, result, depth + 1);
}

static int
schema_mutation(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    struct _gqlCobj	child = { .clas = ((gqlCobj)agoo_mutation_object)->clas, .ptr = NULL };

    return gql_eval_sels(err, doc, (gqlRef)&child, field, sel->sels, result, depth + 1);
}

static struct _gqlCmethod	schema_methods[] = {
    { .key = "query",    .func = schema_query },
    { .key = "mutation", .func = schema_mutation },
    { .key = NULL,       .func = NULL },
};

static struct _gqlCclass	schema_class = {
    .name = "schema",
    .methods = schema_methods,
};

static struct _gqlCobj	root_obj = {
    .clas = &schema_class,
    .ptr = NULL,
};

static gqlRef
root_op(const char *op) {
    gqlRef	ref = NULL;

    if (0 == strcmp("query", op)) {
	ref = agoo_query_object;
    } else if (0 == strcmp("mutation", op)) {
	ref = agoo_mutation_object;
    }
    return ref;
}

static int
resolve(agooErr err, gqlDoc doc, gqlRef target, gqlField field, gqlSel sel, gqlValue result, int depth) {
    gqlCobj	obj = (gqlCobj)target;

    if ('_' == *sel->name && '_' == sel->name[1]) {
	if (0 == strcmp("__typename", sel->name)) {
	    const char	*key = sel->name;

	    if (NULL != sel->alias) {
		key = sel->alias;
	    }
	    if (AGOO_ERR_OK != gql_set_typename(err, gql_cobj_ref_type(target), key, result)) {
		return err->code;
	    }
	    return AGOO_ERR_OK;
	}
	switch (doc->op->kind) {
	case GQL_QUERY:
	    return gql_intro_eval(err, doc, sel, result, depth);
	case GQL_MUTATION:
	    return agoo_err_set(err, AGOO_ERR_EVAL, "%s can not be called on a mutation.", sel->name);
	case GQL_SUBSCRIPTION:
	    return agoo_err_set(err, AGOO_ERR_EVAL, "%s can not be called on a subscription.", sel->name);
	default:
	    return agoo_err_set(err, AGOO_ERR_EVAL, "Not a valid operation on the root object.");
	}
    }
    gqlCmethod	method;

    for (method = obj->clas->methods; NULL != method->key; method++) {
	if (0 == strcmp(method->key, sel->name)) {
	    return method->func(err, doc, obj, field, sel, result, depth);
	}
    }
    return agoo_err_set(err, AGOO_ERR_EVAL, "%s is not a field on %s.", sel->name, obj->clas->name);
}

int
agoo_setup_graphql(agooErr err, const char *path, ...) {
    va_list	ap;
    const char	*sdl;
    char	schema_path[1024];

    va_start(ap, path);
    if (AGOO_ERR_OK != gql_init(err)) {
	return err->code;
    }
    snprintf(schema_path, sizeof(schema_path), "%s/schema", path);
    if (AGOO_ERR_OK != agoo_server_add_func_hook(err, AGOO_GET, path, gql_eval_get_hook, &agoo_server.eval_queue, false) ||
	AGOO_ERR_OK != agoo_server_add_func_hook(err, AGOO_POST, path, gql_eval_post_hook, &agoo_server.eval_queue, false) ||
	AGOO_ERR_OK != agoo_server_add_func_hook(err, AGOO_GET, schema_path, gql_dump_hook, &agoo_server.eval_queue, false)) {
	return err->code;
    }
    gql_root = (gqlRef)&root_obj;

    gql_doc_eval_func = NULL;
    gql_resolve_func = resolve;
    gql_type_func = gql_cobj_ref_type;
    gql_root_op = root_op;

    while (NULL != (sdl = va_arg(ap, const char*))) {
	if (AGOO_ERR_OK != sdl_parse(err, sdl, -1)) {
	    va_end(ap);
	    return err->code;
	}
    }
    va_end(ap);

    gqlType	schema;
    gqlType	query = NULL;
    gqlType	mutation = NULL;

    if (NULL == (schema = gql_type_get("schema"))) {
	if (NULL == (schema = gql_schema_create(err, NULL, 0))) {
	    return err->code;
	}
	if (NULL == (query = gql_type_get("Query"))) {
	    return agoo_err_set(err, AGOO_ERR_NOT_FOUND, "Query type not defined.");
	}
	if (NULL == gql_type_field(err, schema, "query", query, NULL, NULL, 0, false)) {
	    return err->code;
	}
	if (NULL != (mutation = gql_type_get("Mutation"))) {
	    if (NULL == gql_type_field(err, schema, "mutation", mutation, NULL, NULL, 0, false)) {
		return err->code;
	    }
	}
    } else {
	gqlField	f = gql_type_get_field(schema, "query");

	if (NULL == f || NULL == f->type) {
	    return agoo_err_set(err, AGOO_ERR_NOT_FOUND, "schema query field not found.");
	}
	query = f->type;

	if (NULL != (f = gql_type_get_field(schema, "mutation")) && NULL != f->type) {
	    mutation = f->type;
	}
    }
    return AGOO_ERR_OK;
}

int
agoo_load_graphql(agooErr err, const char *path, const char *filename) {
    FILE	*f = fopen(filename, "r");
    long	size;
    char	*sdl;

    if (NULL == f) {
	return agoo_err_no(err, "Failed to open %s.", filename);
    }
    if (0 != fseek(f, 0, SEEK_END)) {
	return agoo_err_no(err, "Failed to seek %s.", filename);
    }
    if (0 > (size = ftell(f))) {
	return agoo_err_no(err, "Failed to ftell %s.", filename);
    }
    rewind(f);
    if (0 < size) {
	if (NULL == (sdl = AGOO_MALLOC(size))) {
	    return AGOO_ERR_MEM(err, "SDL");
	}
	if (size != (long)fread(sdl, 1, size, f)) {
	    return agoo_err_set(err, AGOO_ERR_READ, "Failed to read %s.", filename);
	}
    } else {
	return agoo_err_set(err, AGOO_ERR_READ, "Empty file %s.", filename);
    }
    fclose(f);

    return agoo_setup_graphql(err, path, sdl, NULL);
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
    agoo_res_message_push(req->res, agoo_text_create(buf, cnt));
}

static void*
eval_loop(void *ptr) {
    agooReq	req;

    atomic_fetch_add(&agoo_server.running, 1);
    while (agoo_server.active) {
	if (NULL != (req = (agooReq)agoo_queue_pop(&agoo_server.eval_queue, agoo_poll_wait))) {
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
	    agoo_queue_wakeup(&agoo_server.con_queue);
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
