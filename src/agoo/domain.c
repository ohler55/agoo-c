// Copyright 2019 by Peter Ohler, All Rights Reserved

#include <regex.h>

#include "debug.h"
#include "domain.h"

typedef struct _domain {
    struct _domain	*next;
    char		*host;
    char		*path;
    regex_t		*rhost;
    char		*rpath;
} *Domain;

static Domain	domains = NULL;

bool
agoo_domain_use() {
    return NULL != domains;
}

int
agoo_domain_add(agooErr err, const char *host, const char *path) {
    Domain	d = (Domain)AGOO_CALLOC(1, sizeof(struct _domain));

    if (NULL == d) {
	return AGOO_ERR_MEM(err, "Domain");
    }
    if (NULL == (d->host = AGOO_STRDUP(host))) {
	return AGOO_ERR_MEM(err, "Domain host");
    }
    if (NULL == (d->path = AGOO_STRDUP(path))) {
	return AGOO_ERR_MEM(err, "Domain path");
    }
    if (NULL == domains) {
	domains = d;
    } else {
	Domain	last = domains;

	for (; NULL != last->next; last = last->next) {
	}
	last->next = d;
    }
    return AGOO_ERR_OK;
}

int
agoo_domain_add_regex(agooErr err, const char *host, const char *path) {

    // TBD

    return AGOO_ERR_OK;
}

const char*
agoo_domain_resolve(const char *host, char *buf, size_t blen) {
    Domain	d;

    for (d = domains; NULL != d; d = d->next) {
	if (NULL != d->host) { // simple string compare
	    if (0 == strcmp(host, d->host)) {
		return d->path;
	    }
	} else {
	    // TBD regex
	}
    }
    return NULL;
}
