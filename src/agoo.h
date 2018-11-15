// Copyright (c) 2018, Peter Ohler, All rights reserved.

#ifndef AGOO_H
#define AGOO_H

#include "agoo/err.h"
#include "agoo/method.h"
#include "agoo/req.h"

#define AGOO_VERSION	"0.1.0"

typedef struct _agooHeader {
    const char	*key;
    const char	*value;
} *agooHeader;

extern void	agoo_init(const char *app_name);

extern int	agoo_bind_url(Err err, const char *url);
extern int	agoo_bind_port(Err err, int port);

extern int	agoo_add_func_hook(Err		err,
				   Method	method,
				   const char	*pattern,
				   void		(*func)(Req req),
				   bool		quick);

extern int	agoo_start(Err err, const char *version);
extern void	agoo_shutdown(void (*stop)());

extern void	agoo_respond(Req req, int status, const char *body, int blen, agooHeader headers);

#endif // AGOO_H
