#ifndef _statserve_startserve_h
#define _statserve_startserve_h

#include <lcthw/bstrlib.h>
#include <lcthw/ringbuffer.h>


int setup_data_store();

int parse_line(bstring data, RingBuffer *send_rb);

int server_echo(const char* host, const char* port);

#endif
