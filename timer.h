#ifndef _TIMING_WHEEL_TIMER_H_
#define _TIMING_WHEEL_TIMER_H_

#include <stdint.h>
#include "list.h"

#define TIMING_WHEEL_HIERARCHY 4
#define TIMING_WHEEL_TICK      256

#define TIMEOUT_MASK 0x0FFULL

typedef uint64_t (*clock_func_t)();

typedef void* (*alloc_func_t)(size_t);

typedef void (*free_func_t)(void*);

typedef void (*callback_func_t)(void*);

typedef struct timer_node 
{
    list_node_t     dblink_node;
    uint64_t        expire_time;
    callback_func_t callback_fun;
    void*           callback_arg;
}timer_node_t;

typedef struct timer_mgr
{
    uint64_t     timer_grain;
    uint64_t     max_timeout;
    uint64_t     cur_time;
    uint32_t     timer_cursor[TIMING_WHEEL_HIERARCHY];
    list_node_t  timer_wheel[TIMING_WHEEL_HIERARCHY][TIMING_WHEEL_TICK];
    clock_func_t timer_clock;
    alloc_func_t timer_alloc;
    free_func_t  timer_free;
}timer_mgr_t;

timer_mgr_t* timer_init(uint64_t timer_grain, clock_func_t clock_func, alloc_func_t alloc_func, free_func_t free_func);

void timer_fini(timer_mgr_t* manager);

timer_node_t* timer_add(timer_mgr_t* manager, uint64_t timeout, callback_func_t callbackfun, void* callbackarg);

void timer_del(timer_mgr_t* manager, timer_node_t* timer);

void timer_tick(timer_mgr_t* manager);

#endif
