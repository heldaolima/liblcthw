#ifndef _lcthw_stack_h
#define _lcthw_stack_h

#include <lcthw/list.h>

typedef List Stack;

#define Stack_create List_create
#define Stack_destroy List_destroy
#define Stack_clear List_clear
#define Stack_clear_destroy List_clear_destroy

#define Stack_push List_push // List push ja bota o elemento no topo
#define Stack_pop List_pop // List Pop ja retorna o elemento do topo
#define Stack_peek List_last

#define Stack_count List_count

#define STACK_FOREACH(S, V) LIST_FOREACH(S, last, prev, V) // a pilha vai de cima para baixo

#endif