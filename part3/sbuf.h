#ifndef __SBUF_H
#define __SBUF_H

struct sbuf
{
	int sz;
	int cap;
	char **mem;
};

typedef struct sbuf sbuf;

struct sbuf_node
{
	sbuf *buf;
	struct sbuf_node *next;
};

typedef struct sbuf_node sbuf_node;

struct sbuf_queue
{
	sbuf_node *front;
	sbuf_node *back;
};

typedef struct sbuf_queue sbuf_queue;

sbuf *sbuf_new(int);
void sbuf_free(sbuf*);
void sbuf_clear(sbuf*);
int sbuf_full(sbuf*);
void sbuf_append(sbuf*, const char*);

int sbuf_queue_empty(sbuf_queue*);
void sbuf_queue_push(sbuf_queue*, sbuf*);
sbuf *sbuf_queue_pop(sbuf_queue*);

#endif
