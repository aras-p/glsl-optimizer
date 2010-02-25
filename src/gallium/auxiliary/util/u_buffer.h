#ifndef U_BUFFER_H
#define U_BUFFER_H

struct util_buffer
{
	void* data;
	unsigned size;
	unsigned capacity;
};

static inline void
util_buffer_init(struct util_buffer* buf)
{
	memset(buf, 0, sizeof(*buf));
}

static inline  void
util_buffer_fini(struct util_buffer* buf)
{
	if(buf->data) {
		free(buf->data);
		util_buffer_init(buf);
	}
}

static inline void*
util_buffer_grow(struct util_buffer* buf, int size)
{
	unsigned newsize = buf->size + size;
	char* p;
	if(newsize > buf->capacity) {
		buf->capacity <<= 1;
		if(newsize > buf->capacity)
			buf->capacity = newsize;
		buf->data = realloc(buf->data, buf->capacity);
	}

	p = (char*)buf->data + buf->size;
	buf->size = newsize;
	return p;
}

static inline void
util_buffer_trim(struct util_buffer* buf)
{
	buf->data = realloc(buf->data, buf->size);
	buf->capacity = buf->size;
}

#define util_buffer_append(buf, type, v) do {type __v = (v); memcpy(util_buffer_grow((buf), sizeof(type)), &__v, sizeof(type));} while(0)
#define util_buffer_top_ptr(buf, type) (type*)((buf)->data + (buf)->size - sizeof(type))
#define util_buffer_top(buf, type) *util_buffer_top_ptr(buf, type)
#define util_buffer_pop_ptr(buf, type) (type*)((buf)->data + ((buf)->size -= sizeof(type)))
#define util_buffer_pop(buf, type) *util_buffer_pop_ptr(buf, type)
#define util_buffer_contains(buf, type) ((buf)->size >= sizeof(type))

#endif /* U_BUFFER_H */
