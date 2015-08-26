#include <stdlib.h>
#include <time.h>
#include "timer.h"

uint64_t _fast_power(uint64_t base, uint64_t time)
{
    uint64_t result = 1;
    while ( time != 0 )
    {
        if ( time & 0x01 )
        {
            result *= base;
        }
        base *= base;
        time >>= 1;
    }
    return result;
}

uint64_t _monotonic_clock()
{
    struct timespec curr_time; 
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    return (uint64_t)(curr_time.tv_sec * 1000 + curr_time.tv_nsec / 1000000);
}

timer_mgr_t* timer_init(uint64_t timer_grain, clock_func_t clock_func, alloc_func_t alloc_func, free_func_t free_func)
{
    if ( timer_grain == 0 )
    {
        return NULL;
    }
    if ( (alloc_func && !free_func) || (!alloc_func && free_func) )
    {
        return NULL;
    }
    timer_mgr_t* manager = (timer_mgr_t*)malloc(sizeof(timer_mgr_t));
    if ( manager == NULL )
    {
        return NULL;
    }
    manager->timer_clock = clock_func ? clock_func : _monotonic_clock;
    manager->timer_alloc = alloc_func ? alloc_func : malloc;
    manager->timer_free  = free_func  ? free_func  : free;
    manager->timer_grain = timer_grain;
    manager->max_timeout = timer_grain * _fast_power(TIMING_WHEEL_TICK, TIMING_WHEEL_HIERARCHY); 
    manager->cur_time    = manager->timer_clock();
    int i, j;
    for ( i = 0; i < TIMING_WHEEL_HIERARCHY; ++i )
    {
        manager->timer_cursor[i] = 0;
        for ( j = 0; j < TIMING_WHEEL_TICK; ++j )
        {
            list_init(&manager->timer_wheel[i][j]);
        }
    }
    return manager;
}

void timer_fini(timer_mgr_t* manager)
{
    int i, j;
    for ( i = 0; i < TIMING_WHEEL_HIERARCHY; ++i )
    {
        for ( j = 0; j < TIMING_WHEEL_TICK; ++j )
        {
            list_node_t* head = &manager->timer_wheel[i][j];
            while ( !list_empty(head) )
            {
                timer_node_t* timer = container_of(head->next, timer_node_t, dblink_node);
                list_del(head->next);
                manager->timer_free(timer);
            }
        }
    }
    free(manager);
}

list_node_t* _timer_list(timer_mgr_t* manager, uint64_t timeout)
{
    timeout = (timeout + manager->timer_grain - 1) / manager->timer_grain;
    if ( timeout < TIMING_WHEEL_TICK )
    {
        return &manager->timer_wheel[0][(manager->timer_cursor[0] + timeout) & TIMEOUT_MASK];
    }
    uint32_t hier_idx = 0, tick_idx;
    timeout >>= 8;
    while ( timeout )
    {
        hier_idx += 1;
        tick_idx = timeout & TIMEOUT_MASK;
        timeout >>= 8;
    }
    tick_idx = (manager->timer_cursor[hier_idx] + tick_idx) & TIMEOUT_MASK;
    return &manager->timer_wheel[hier_idx][tick_idx];
}

timer_node_t* timer_add(timer_mgr_t* manager, uint64_t timeout, callback_func_t callbackfun, void* callbackarg)
{
    if ( timeout == 0 || manager->max_timeout <= timeout )
    {
        return NULL;
    }
    timer_node_t* timer = (timer_node_t*)manager->timer_alloc(sizeof(timer_node_t));
    if ( timer == NULL )
    {
        return NULL;
    }  
    timer->expire_time  = manager->cur_time + timeout;
    timer->callback_fun = callbackfun;
    timer->callback_arg = callbackarg;
    list_add(_timer_list(manager, timeout), &timer->dblink_node);
    return timer;
}

void timer_del(timer_mgr_t* manager, timer_node_t* timer)
{
    list_del(&timer->dblink_node);
    manager->timer_free(timer);
}

void _timer_fall(timer_mgr_t* manager, uint32_t hier)
{
    if ( hier < 1 || TIMING_WHEEL_HIERARCHY <= hier )
    {
        return;
    }
    manager->timer_cursor[hier] = (manager->timer_cursor[hier] + 1) & TIMEOUT_MASK;
    if ( manager->timer_cursor[hier] == 0 )
    { 
        _timer_fall(manager, hier + 1);
    }
    list_node_t* head = &manager->timer_wheel[hier][manager->timer_cursor[hier]];
    while ( !list_empty(head) )
    {
        list_node_t* node = head->next;
        timer_node_t* timer = container_of(node, timer_node_t, dblink_node);
        list_del(node);
        if ( manager->cur_time < timer->expire_time )
        {
            list_add(_timer_list(manager, timer->expire_time - manager->cur_time), node);
        }
        else
        {
            timer->callback_fun(timer->callback_arg);
            manager->timer_free(timer);
        }      
    }

}

void timer_tick(timer_mgr_t* manager)
{
    if ( manager == NULL )
    {
        return;
    }
    uint64_t pre_tick = manager->cur_time / manager->timer_grain;
    manager->cur_time = manager->timer_clock();
    uint64_t cur_tick = manager->cur_time / manager->timer_grain;
    while ( pre_tick < cur_tick )
    {
        manager->timer_cursor[0] = (manager->timer_cursor[0] + 1) %  TIMING_WHEEL_TICK;
        if ( manager->timer_cursor[0] == 0 )
        {
            _timer_fall(manager, 1); 
        }
        list_node_t* head = &manager->timer_wheel[0][manager->timer_cursor[0]];
        while ( !list_empty(head) )
        {
            timer_node_t* timer = container_of(head->next, timer_node_t, dblink_node);
            list_del(head->next);
            timer->callback_fun(timer->callback_arg); 
            manager->timer_free(timer);
        }
        ++pre_tick; 
    }
}
