// Copyright (c) 2018, Peter Ohler, All rights reserved.

#ifndef AGOO_H
#define AGOO_H

#include "agoo/err.h"
#include "agoo/method.h"
#include "agoo/req.h"
#include "agoo/text.h"

#define AGOO_VERSION	"0.2.0"

struct _agooCon;

typedef struct _agooKeyVal {
    const char	*key;
    const char	*value;
} *agooKeyVal;

extern void	agoo_init(const char *app_name);

extern int	agoo_bind_to_url(agooErr err, const char *url);
extern int	agoo_bind_to_port(agooErr err, int port);

extern int	agoo_add_func_hook(agooErr	err,
				   agooMethod	method,
				   const char	*pattern,
				   void		(*func)(agooReq req),
				   bool		quick);

extern int	agoo_start(agooErr err, const char *version);
extern void	agoo_shutdown(void (*stop)());

extern agooText	agoo_respond(int status, const char *body, int blen, agooKeyVal headers);

#endif // AGOO_H
