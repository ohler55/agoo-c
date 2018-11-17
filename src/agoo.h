// Copyright (c) 2018, Peter Ohler, All rights reserved.

#ifndef AGOO_H
#define AGOO_H

#include "agoo/err.h"
#include "agoo/method.h"
#include "agoo/req.h"

#define AGOO_VERSION	"0.2.0"

typedef struct _agooKeyVal {
    const char	*key;
    const char	*value;
} *agooKeyVal;

extern void	agoo_init(const char *app_name);

extern int	agoo_bind_url(agooErr err, const char *url);
extern int	agoo_bind_port(agooErr err, int port);

extern int	agoo_add_func_hook(agooErr	err,
				   agooMethod	method,
				   const char	*pattern,
				   void		(*func)(agooReq req),
				   bool		quick);

extern int	agoo_start(agooErr err, const char *version);
extern void	agoo_shutdown(void (*stop)());

extern void	agoo_respond(agooReq req, int status, const char *body, int blen, agooKeyVal headers);

#endif // AGOO_H
