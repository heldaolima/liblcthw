#ifndef _lcthw_queue_h
#define _lcthw_queue_h

#include <lcthw/list.h>

typedef List Queue;

#define Queue_create List_create
#define Queue_destroy List_destroy
#define Queue_clear List_clear
#define Queue_clear_destroy List_clear_destroy

#define Queue_count List_count
#define Queue_peak List_first
#define Queue_send List_push
#define Queue_recv List_shift

#define QUEUE_FOREACH(Q, V) LIST_FOREACH(Q, first, next, V)

#endif