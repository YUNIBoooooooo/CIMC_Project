
#ifndef RINGBUFFER_H__
#define RINGBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif
    
#include "stdint.h"
#include "stdio.h"
#include "assert.h"
    
typedef uint8_t     rt_uint8_t;
typedef uint16_t    rt_uint16_t;
typedef int16_t     rt_int16_t;
typedef size_t      rt_size_t;

#define RT_ASSERT   assert
#define RT_NULL     NULL
#define rt_inline   inline
#define rt_memcpy   memcpy

#define RT_ALIGN_DOWN(size,align)	((size)&~((align)-1)) 

struct rt_ringbuffer
{
    rt_uint8_t *buffer_ptr;
  
    rt_uint16_t read_mirror : 1;
    rt_uint16_t read_index : 15;
    rt_uint16_t write_mirror : 1;
    rt_uint16_t write_index : 15;
   
    rt_int16_t buffer_size;
};

enum rt_ringbuffer_state
{
    RT_RINGBUFFER_EMPTY,
    RT_RINGBUFFER_FULL,
    RT_RINGBUFFER_HALFFULL,
};


void rt_ringbuffer_init(struct rt_ringbuffer *rb, rt_uint8_t *pool, rt_int16_t size);
void rt_ringbuffer_reset(struct rt_ringbuffer *rb);
rt_size_t rt_ringbuffer_put(struct rt_ringbuffer *rb, const rt_uint8_t *ptr, rt_uint16_t length);
rt_size_t rt_ringbuffer_put_force(struct rt_ringbuffer *rb, const rt_uint8_t *ptr, rt_uint16_t length);
rt_size_t rt_ringbuffer_putchar(struct rt_ringbuffer *rb, const rt_uint8_t ch);
rt_size_t rt_ringbuffer_putchar_force(struct rt_ringbuffer *rb, const rt_uint8_t ch);
rt_size_t rt_ringbuffer_get(struct rt_ringbuffer *rb, rt_uint8_t *ptr, rt_uint16_t length);
rt_size_t rt_ringbuffer_peek(struct rt_ringbuffer *rb, rt_uint8_t **ptr);
rt_size_t rt_ringbuffer_getchar(struct rt_ringbuffer *rb, rt_uint8_t *ch);
rt_size_t rt_ringbuffer_data_len(struct rt_ringbuffer *rb);

#ifdef RT_USING_HEAP
struct rt_ringbuffer* rt_ringbuffer_create(rt_uint16_t length);
void rt_ringbuffer_destroy(struct rt_ringbuffer *rb);
#endif


rt_inline rt_uint16_t rt_ringbuffer_get_size(struct rt_ringbuffer *rb)
{
    RT_ASSERT(rb != RT_NULL);
    return rb->buffer_size;
}

#define rt_ringbuffer_space_len(rb) ((rb)->buffer_size - rt_ringbuffer_data_len(rb))


#ifdef __cplusplus
}
#endif

#endif
