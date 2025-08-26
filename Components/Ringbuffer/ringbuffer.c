#include "ringbuffer.h"
#include <string.h>

static inline enum rt_ringbuffer_state rt_ringbuffer_status(struct rt_ringbuffer *rb)
{
    if (rb->read_index == rb->write_index)
    {
        if (rb->read_mirror == rb->write_mirror)
            return RT_RINGBUFFER_EMPTY;
        else
            return RT_RINGBUFFER_FULL;
    }
    return RT_RINGBUFFER_HALFFULL;
}

void rt_ringbuffer_init(struct rt_ringbuffer *rb,
                        rt_uint8_t           *pool,
                        rt_int16_t            size)
{
    RT_ASSERT(rb != NULL);
    RT_ASSERT(size > 0);

    /* initialize read and write index */
    rb->read_mirror = rb->read_index = 0;
    rb->write_mirror = rb->write_index = 0;

    /* set buffer pool and size */
    rb->buffer_ptr = pool;
    rb->buffer_size = RT_ALIGN_DOWN(size, 4);
}
rt_size_t rt_ringbuffer_put(struct rt_ringbuffer *rb,
                            const rt_uint8_t     *ptr,
                            rt_uint16_t           length)
{
    rt_uint16_t size;

    RT_ASSERT(rb != RT_NULL);

   
    size = rt_ringbuffer_space_len(rb);

   
    if (size == 0)
        return 0;

   
    if (size < length)
        length = size;

    if (rb->buffer_size - rb->write_index > length)
    {
       
        rt_memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);
        
        rb->write_index += length;
        return length;
    }

    rt_memcpy(&rb->buffer_ptr[rb->write_index],
              &ptr[0],
              rb->buffer_size - rb->write_index);
    rt_memcpy(&rb->buffer_ptr[0],
              &ptr[rb->buffer_size - rb->write_index],
              length - (rb->buffer_size - rb->write_index));

   
    rb->write_mirror = ~rb->write_mirror;
    rb->write_index = length - (rb->buffer_size - rb->write_index);

    return length;
}



rt_size_t rt_ringbuffer_put_force(struct rt_ringbuffer *rb,
                                  const rt_uint8_t     *ptr,
                                  rt_uint16_t           length)
{
    rt_uint16_t space_length;

    RT_ASSERT(rb != RT_NULL);

    space_length = rt_ringbuffer_space_len(rb);

    if (length > rb->buffer_size)
    {
        ptr = &ptr[length - rb->buffer_size];
        length = rb->buffer_size;
    }

    if (rb->buffer_size - rb->write_index > length)
    {
      
        rt_memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);
       
        rb->write_index += length;

        if (length > space_length)
            rb->read_index = rb->write_index;

        return length;
    }

    rt_memcpy(&rb->buffer_ptr[rb->write_index],
              &ptr[0],
              rb->buffer_size - rb->write_index);
    rt_memcpy(&rb->buffer_ptr[0],
              &ptr[rb->buffer_size - rb->write_index],
              length - (rb->buffer_size - rb->write_index));

  
    rb->write_mirror = ~rb->write_mirror;
    rb->write_index = length - (rb->buffer_size - rb->write_index);

    if (length > space_length)
    {
        if (rb->write_index <= rb->read_index)
            rb->read_mirror = ~rb->read_mirror;
        rb->read_index = rb->write_index;
    }

    return length;
}



rt_size_t rt_ringbuffer_get(struct rt_ringbuffer *rb,
                            rt_uint8_t           *ptr,
                            rt_uint16_t           length)
{
    rt_size_t size;

    RT_ASSERT(rb != RT_NULL);

    size = rt_ringbuffer_data_len(rb);

    if (size == 0)
        return 0;

    if (size < length)
        length = size;

    if (rb->buffer_size - rb->read_index > length)
    {
     
        rt_memcpy(ptr, &rb->buffer_ptr[rb->read_index], length);
       
        rb->read_index += length;
        return length;
    }

    rt_memcpy(&ptr[0],
              &rb->buffer_ptr[rb->read_index],
              rb->buffer_size - rb->read_index);
    rt_memcpy(&ptr[rb->buffer_size - rb->read_index],
              &rb->buffer_ptr[0],
              length - (rb->buffer_size - rb->read_index));

  
    rb->read_mirror = ~rb->read_mirror;
    rb->read_index = length - (rb->buffer_size - rb->read_index);

    return length;
}



rt_size_t rt_ringbuffer_peek(struct rt_ringbuffer *rb, rt_uint8_t **ptr)
{
    rt_size_t size;

    RT_ASSERT(rb != RT_NULL);

    *ptr = RT_NULL;

   
    size = rt_ringbuffer_data_len(rb);

  
    if (size == 0)
        return 0;

    *ptr = &rb->buffer_ptr[rb->read_index];

    if ((rt_size_t)(rb->buffer_size - rb->read_index) > size)
    {
        rb->read_index += size;
        return size;
    }

    size = rb->buffer_size - rb->read_index;

  
    rb->read_mirror = ~rb->read_mirror;
    rb->read_index = 0;

    return size;
}



rt_size_t rt_ringbuffer_putchar(struct rt_ringbuffer *rb, const rt_uint8_t ch)
{
    RT_ASSERT(rb != RT_NULL);

   
    if (!rt_ringbuffer_space_len(rb))
        return 0;

    rb->buffer_ptr[rb->write_index] = ch;

   
    if (rb->write_index == rb->buffer_size - 1)
    {
        rb->write_mirror = ~rb->write_mirror;
        rb->write_index = 0;
    }
    else
    {
        rb->write_index++;
    }

    return 1;
}



rt_size_t rt_ringbuffer_putchar_force(struct rt_ringbuffer *rb, const rt_uint8_t ch)
{
    enum rt_ringbuffer_state old_state;

    RT_ASSERT(rb != RT_NULL);

    old_state = rt_ringbuffer_status(rb);

    rb->buffer_ptr[rb->write_index] = ch;

  
    if (rb->write_index == rb->buffer_size - 1)
    {
        rb->write_mirror = ~rb->write_mirror;
        rb->write_index = 0;
        if (old_state == RT_RINGBUFFER_FULL)
        {
            rb->read_mirror = ~rb->read_mirror;
            rb->read_index = rb->write_index;
        }
    }
    else
    {
        rb->write_index++;
        if (old_state == RT_RINGBUFFER_FULL)
            rb->read_index = rb->write_index;
    }

    return 1;
}



rt_size_t rt_ringbuffer_getchar(struct rt_ringbuffer *rb, rt_uint8_t *ch)
{
    RT_ASSERT(rb != RT_NULL);

  
    if (!rt_ringbuffer_data_len(rb))
        return 0;

   
    *ch = rb->buffer_ptr[rb->read_index];

    if (rb->read_index == rb->buffer_size - 1)
    {
        rb->read_mirror = ~rb->read_mirror;
        rb->read_index = 0;
    }
    else
    {
        rb->read_index++;
    }

    return 1;
}



rt_size_t rt_ringbuffer_data_len(struct rt_ringbuffer *rb)
{
    switch (rt_ringbuffer_status(rb))
    {
    case RT_RINGBUFFER_EMPTY:
        return 0;
    case RT_RINGBUFFER_FULL:
        return rb->buffer_size;
    case RT_RINGBUFFER_HALFFULL:
    default:
    {
        rt_size_t wi = rb->write_index, ri = rb->read_index;

        if (wi > ri)
            return wi - ri;
        else
            return rb->buffer_size - (ri - wi);
    }
    }
}



void rt_ringbuffer_reset(struct rt_ringbuffer *rb)
{
    RT_ASSERT(rb != RT_NULL);

    rb->read_mirror = 0;
    rb->read_index = 0;
    rb->write_mirror = 0;
    rb->write_index = 0;
}


#ifdef RT_USING_HEAP

/
struct rt_ringbuffer *rt_ringbuffer_create(rt_uint16_t size)
{
    struct rt_ringbuffer *rb;
    rt_uint8_t *pool;

    RT_ASSERT(size > 0);

    size = RT_ALIGN_DOWN(size, RT_ALIGN_SIZE);

    rb = (struct rt_ringbuffer *)rt_malloc(sizeof(struct rt_ringbuffer));
    if (rb == RT_NULL)
        goto exit;

    pool = (rt_uint8_t *)rt_malloc(size);
    if (pool == RT_NULL)
    {
        rt_free(rb);
        rb = RT_NULL;
        goto exit;
    }
    rt_ringbuffer_init(rb, pool, size);

exit:
    return rb;
}



void rt_ringbuffer_destroy(struct rt_ringbuffer *rb)
{
    RT_ASSERT(rb != RT_NULL);

    rt_free(rb->buffer_ptr);
    rt_free(rb);
}

#endif
