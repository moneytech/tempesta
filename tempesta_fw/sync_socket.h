/**
 *		Synchronous Socket API.
 *
 * Server and client socket (connecton) definitions.
 *
 * Copyright (C) 2012-2014 NatSys Lab. (info@natsys-lab.com).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __SS_SOCK_H__
#define __SS_SOCK_H__

#include <net/sock.h>
#include <linux/skbuff.h>

#include "ss_skb.h"

/*
 * ------------------------------------------------------------------------
 * 		Socket buffers management
 * ------------------------------------------------------------------------
 */
typedef struct {
	struct sk_buff	*first;
	struct sk_buff	*last;
} SsSkbList;

/**
 * The functions below are full analogs of standard Linux functions
 * w/o "ss_" prefix.
 */

static inline void
ss_skb_queue_head_init(SsSkbList *list)
{
	list->first = list->last = (struct sk_buff *)list;
}

static inline int
ss_skb_queue_empty(const SsSkbList *list)
{
	return list->first == (struct sk_buff *)list;
}

/**
 * Add new @skb to the @list in FIFO order.
 */
static inline void
ss_skb_queue_tail(SsSkbList *list, struct sk_buff *skb)
{
	SsSkbCb *scb = TFW_SKB_CB(skb);

	/* Don't link the skb twice. */
	if (unlikely(ss_skb_passed(skb)))
		return;

	scb->next = (struct sk_buff *)list;
	scb->prev = list->last;
	if (ss_skb_queue_empty(list))
		list->first = skb;
	else
		TFW_SKB_CB(list->last)->next = skb;
	list->last = skb;
}

static inline void
ss_skb_unlink(SsSkbList *list, struct sk_buff *skb)
{
	SsSkbCb *scb = TFW_SKB_CB(skb);

	if (scb->next == (struct sk_buff *)list) {
		list->last = scb->prev;
	} else {
		TFW_SKB_CB(scb->next)->prev = scb->prev;
	}
	if (scb->prev == (struct sk_buff *)list) {
		list->first = scb->next;
	} else {
		TFW_SKB_CB(scb->prev)->next = scb->next;
	}
	scb->next = scb->prev = NULL;
}

static inline struct sk_buff *
ss_skb_next(const SsSkbList *list, struct sk_buff *skb)
{
	skb = TFW_SKB_CB(skb)->next;

	if (skb == (struct sk_buff *)list)
		return NULL;
	return skb;

}

static inline struct sk_buff *
ss_skb_peek(const SsSkbList *list)
{
	struct sk_buff *skb = list->first;

	if (skb == (struct sk_buff *)list)
		return NULL;
	return skb;
}

static inline struct sk_buff *
ss_skb_peek_tail(const SsSkbList *list)
{
	struct sk_buff *skb = list->last;

	if (skb == (struct sk_buff *)list)
		return NULL;
	return skb;

}

static inline struct sk_buff *
ss_skb_dequeue(SsSkbList *list)
{
	struct sk_buff *skb = ss_skb_peek(list);
	if (skb)
		ss_skb_unlink(list, skb);
	return skb;
}


/*
 * ------------------------------------------------------------------------
 * 		Synchronous Sockets API
 * ------------------------------------------------------------------------
 */
/* Protocol descriptor. */
typedef struct ss_proto_t {
	const struct ss_hooks	*hooks;
	struct sock		*listener;
	int			type;
} SsProto;

/* Table of Synchronous Sockets connection callbacks. */
typedef struct ss_hooks {
	/* New connection accepted. */
	int (*connection_new)(struct sock *sk);

	/* Drop TCP connection associated with the socket. */
	int (*connection_drop)(struct sock *sk);

	/* Error on TCP connection associated with the socket. */
	int (*connection_error)(struct sock *sk);

	/* Process data received on the socket. */
	int (*connection_recv)(void *conn, struct sk_buff *skb,
			       unsigned int off);
} SsHooks;

static inline void ss_callback_write_lock(struct sock *sk)
{
	write_lock(&sk->sk_callback_lock);
}

static inline void ss_callback_write_unlock(struct sock *sk)
{
	write_unlock(&sk->sk_callback_lock);
}

int ss_hooks_register(SsHooks* hooks);
void ss_hooks_unregister(SsHooks* hooks);

void ss_proto_init(SsProto *proto, const SsHooks *hooks, int type);
void ss_proto_inherit(const SsProto *parent, SsProto *child, int child_type);
void ss_set_callbacks(struct sock *sk);
void ss_set_listen(struct sock *sk);
void ss_send(struct sock *sk, const SsSkbList *skb_list);
void ss_close(struct sock *sk);
int ss_sock_create(int family, int type, int protocol, struct sock **res);
void ss_release(struct sock *sk);
int ss_connect(struct sock *sk, struct sockaddr *addr, int addrlen, int flags);
int ss_bind(struct sock *sk, struct sockaddr *addr, int addrlen);
int ss_listen(struct sock *sk, int backlog);
int ss_getpeername(struct sock *sk, struct sockaddr *addr, int *addrlen);

#endif /* __SS_SOCK_H__ */