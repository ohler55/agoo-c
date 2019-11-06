// Copyright 2018 by Peter Ohler, All Rights Reserved

#include <stdio.h>
#include <stdlib.h>

#include <agoo.h>
#include <agoo/gqlintro.h>
#include <agoo/gqlvalue.h>
#include <agoo/graphql.h>
#include <agoo/log.h>
#include <agoo/page.h>
#include <agoo/res.h>
#include <agoo/sdl.h>
#include <agoo/server.h>

// Start the app and then use curl or a browser to access this URL.
//
// http://localhost:6464/graphql?query={hello}

static agooText		emptyResp = NULL;

static const char	*sdl = "\n\
type Query {\n\
  hello(name: String!): String\n\
}\n\
type Mutation {\n\
  \"Double the number provided.\"\n\
  double(number: Int!): Int\n\
}\n";

static void
empty_handler(agooReq req) {
    if (NULL == emptyResp) {
	emptyResp = agoo_respond(200, NULL, 0, NULL);
	agoo_text_ref(emptyResp);
    }
    agoo_res_message_push(req->res, emptyResp);
}

///// Query type setup

static int
query_hello(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    const char	*key = sel->name;
    char	hello[1024];
    int		len;
    gqlValue	val;
    gqlValue	nv = gql_extract_arg(err, field, sel, "name");
    const char	*name = NULL;

    if (NULL != nv) {
	name = gql_string_get(nv);
    }
    if (NULL == name) {
	name = "";
    }
    len = snprintf(hello, sizeof(hello), "Hello %s", name);
    val = gql_string_create(err, hello, len);

    if (NULL != sel->alias) {
	key = sel->alias;
    }
    return gql_object_set(err, result, key, val);
}

static struct _gqlCmethod	query_methods[] = {
    { .key = "hello", .func = query_hello },
    { .key = NULL,    .func = NULL },
};

static struct _gqlCclass	query_class = {
    .name = "query",
    .methods = query_methods,
};

static struct _gqlCobj	query_obj = {
    .clas = &query_class,
    .ptr = NULL,
};

///// Mutation type setup

static int
mutation_double(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    const char	*key = sel->name;
    gqlValue	nv = gql_extract_arg(err, field, sel, "number");
    gqlValue	val;

    if (NULL == nv || &gql_int_type != nv->type) {
	return agoo_err_set(err, AGOO_ERR_EVAL, "number must be an Int.");
    }
    val = gql_int_create(err, nv->i * 2);
    if (NULL != sel->alias) {
	key = sel->alias;
    }
    return gql_object_set(err, result, key, val);
}

static struct _gqlCmethod	mutation_methods[] = {
    { .key = "double", .func = mutation_double },
    { .key = NULL,     .func = NULL },
};

static struct _gqlCclass	mutation_class = {
    .name = "mutation",
    .methods = mutation_methods,
};

static struct _gqlCobj	mutation_obj = {
    .clas = &mutation_class,
    .ptr = NULL,
};

int
main(int argc, char **argv) {
    struct _agooErr	err = AGOO_ERR_INIT;
    int			port = 6464;

    if (AGOO_ERR_OK != agoo_init(&err, "hello")) {
	printf("Failed to initialize Agoo. %s\n", err.msg);
	return err.code;
    }
    // Set the number of eval threads.
    agoo_server.thread_cnt = 1;

    if (AGOO_ERR_OK != agoo_pages_set_root(&err, ".")) {
	printf("Failed to set root. %s\n", err.msg);
	return err.code;
    }
    if (AGOO_ERR_OK != agoo_bind_to_port(&err, port)) {
	printf("Failed to bind to port %d. %s\n", port, err.msg);
	return err.code;
    }
    if (AGOO_ERR_OK != agoo_setup_graphql(&err, "/graphql", sdl, NULL)) {
	return err.code;
    }
    agoo_query_object = &query_obj;
    agoo_mutation_object = &mutation_obj;

    // set up hooks or routes
    if (AGOO_ERR_OK != agoo_add_func_hook(&err, AGOO_GET, "/", empty_handler, true)) {
	return err.code;
    }
    // start the server and wait for it to be shutdown
    if (AGOO_ERR_OK != agoo_start(&err, AGOO_VERSION)) {
	printf("%s\n", err.msg);
	return err.code;
    }
    return 0;
}
