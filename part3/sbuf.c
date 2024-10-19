#include <stdlib.h>
#include <string.h>
#include "sbuf.h"

sbuf *sbuf_new(int cap)
{
	sbuf *buf = malloc(sizeof(sbuf));
	buf->sz = 0;
	buf->cap = cap;
	buf->mem = malloc(sizeof(char*) * cap);
	return buf;
}

void sbuf_free(sbuf *buf)
{
	sbuf_clear(buf);
	free(buf);
}

void sbuf_clear(sbuf *buf)
{
	for (int i = 0; i < buf->sz; i++)
		free(buf->mem[i]);
	buf->sz = 0;
}

int sbuf_full(sbuf *buf)
{
	return buf->sz == buf->cap;
}

void sbuf_append(sbuf *buf, const char *s)
{
	buf->mem[buf->sz++] = strdup(s);
}

int sbuf_queue_empty(sbuf_queue *queue)
{
	return queue->front == NULL;
}

void sbuf_queue_push(sbuf_queue *queue, sbuf *buf)
{
	sbuf_node *node = malloc(sizeof(sbuf_node));
	node->buf = buf;
	node->next = NULL;

	if (queue->front)
		queue->back->next = node;
	else
		queue->front = node;

	queue->back = node;
}

sbuf *sbuf_queue_pop(sbuf_queue *queue)
{
	sbuf_node *node = queue->front;
	sbuf *buf = node->buf;

	queue->front = queue->front->next;
	if (!queue->front)
		queue->back = NULL;

	free(node);
	return buf;
}
