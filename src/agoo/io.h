// Copyright (c) 2019, Peter Ohler, All rights reserved.

#ifndef AGOO_IO_H
#define AGOO_IO_H

#include <pthread.h>

#include "atomic.h"
#include "con.h"
#include "err.h"
#include "queue.h"
#include "res.h"

#define MAX_IO_CONS	4096

struct _agooCon;
struct _agooRes;

typedef struct _agooIO {
    struct _agooIO	*next;
    struct _agooQueue	pub_queue;
    struct _agooQueue	recv_queue;
    volatile agooCon	cons;
    volatile agooCon	new_cons;
    atomic_uint_fast8_t	con_cnt;
    pthread_mutex_t	con_lock; // just for new_cons

    // Response FIFO list.
    volatile agooRes	res_head;
    volatile agooRes	res_tail;
    pthread_mutex_t	res_lock;

    pthread_t		poll_thread;
    pthread_t		recv_thread;
} *agooIO;

extern agooIO	agoo_io_create(agooErr err);
extern void	agoo_io_destroy(agooIO io);
extern int	agoo_io_con_count(agooIO io);
extern void	agoo_io_add_con(agooIO io, struct _agooCon *con);

#endif // AGOO_IO_H
