// Copyright (c) 2019, Peter Ohler, All rights reserved.

#ifndef AGOO_IO_H
#define AGOO_IO_H

#include <pthread.h>

#include "con.h"
#include "err.h"
#include "queue.h"
#include "res.h"

#define MAX_IO_CONS	4096

struct _agooRes;

typedef struct _agooIO {
    struct _agooIO	*next;
    struct _agooQueue	pub_queue;
    struct _agooQueue	recv_queue;
    int			id;
    volatile agooCon	cons;

    // Response FIFO list.
    volatile agooRes	res_head;
    volatile agooRes	res_tail;
    pthread_mutex_t	lock; // for res list

    pthread_t		poll_thread;
    pthread_t		recv_thread;
} *agooIO;

extern agooIO	agoo_io_create(agooErr err, int id);
extern void	agoo_io_destroy(agooIO io);

#endif // AGOO_IO_H
