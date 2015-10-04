/*	$OpenBSD$ */

/*
 * Copyright (c) 2015 Renato Westphal <renato@openbsd.org>
 * Copyright (c) 2004 Esben Norby <norby@openbsd.org>
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _EIGRPD_H_
#define _EIGRPD_H_

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/tree.h>
#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>
#include <event.h>

#include <imsg.h>
#include "eigrp.h"

#define CONF_FILE		"/etc/eigrpd.conf"
#define	EIGRPD_SOCKET		"/var/run/eigrpd.sock"
#define EIGRPD_USER		"_eigrpd"

#define NBR_IDSELF		1
#define NBR_CNTSTART		(NBR_IDSELF + 1)

#define	READ_BUF_SIZE		65535
#define	PKG_DEF_SIZE		512	/* compromise */
#define	RT_BUF_SIZE		16384
#define	MAX_RTSOCK_BUF		128 * 1024

#define	F_EIGRPD_INSERTED	0x0001
#define	F_KERNEL		0x0002
#define	F_CONNECTED		0x0004
#define	F_STATIC		0x0008
#define	F_DYNAMIC		0x0010
#define F_DOWN                  0x0020
#define	F_REJECT		0x0040
#define	F_BLACKHOLE		0x0080
#define	F_REDISTRIBUTED		0x0100
#define	F_CTL_EXTERNAL		0x0200	/* only used by eigrpctl */
#define	F_CTL_ACTIVE		0x0400
#define	F_CTL_ALLLINKS		0x0800

struct imsgev {
	struct imsgbuf		 ibuf;
	void			(*handler)(int, short, void *);
	struct event		 ev;
	void			*data;
	short			 events;
};

enum imsg_type {
	IMSG_CTL_RELOAD,
	IMSG_CTL_SHOW_INTERFACE,
	IMSG_CTL_SHOW_NBR,
	IMSG_CTL_SHOW_TOPOLOGY,
	IMSG_CTL_FIB_COUPLE,
	IMSG_CTL_FIB_DECOUPLE,
	IMSG_CTL_IFACE,
	IMSG_CTL_KROUTE,
	IMSG_CTL_IFINFO,
	IMSG_CTL_END,
	IMSG_CTL_LOG_VERBOSE,
	IMSG_KROUTE_CHANGE,
	IMSG_KROUTE_DELETE,
	IMSG_NONE,
	IMSG_IFINFO,
	IMSG_IFDOWN,
	IMSG_NEWADDR,
	IMSG_DELADDR,
	IMSG_NETWORK_ADD,
	IMSG_NETWORK_DEL,
	IMSG_NEIGHBOR_UP,
	IMSG_NEIGHBOR_DOWN,
	IMSG_RECV_UPDATE_INIT,
	IMSG_RECV_UPDATE,
	IMSG_RECV_QUERY,
	IMSG_RECV_REPLY,
	IMSG_RECV_SIAQUERY,
	IMSG_RECV_SIAREPLY,
	IMSG_SEND_UPDATE,
	IMSG_SEND_QUERY,
	IMSG_SEND_REPLY,
	IMSG_SEND_MUPDATE,
	IMSG_SEND_MQUERY,
	IMSG_SEND_UPDATE_END,
	IMSG_SEND_REPLY_END,
	IMSG_SEND_SIAQUERY_END,
	IMSG_SEND_SIAREPLY_END,
	IMSG_SEND_MUPDATE_END,
	IMSG_SEND_MQUERY_END,
	IMSG_RECONF_CONF,
	IMSG_RECONF_IFACE,
	IMSG_RECONF_INSTANCE,
	IMSG_RECONF_EIGRP_IFACE,
	IMSG_RECONF_END
};

union eigrpd_addr {
	struct in_addr	v4;
	struct in6_addr	v6;
};

/* interface types */
enum iface_type {
	IF_TYPE_POINTOPOINT,
	IF_TYPE_BROADCAST,
};

struct if_addr {
	TAILQ_ENTRY(if_addr)	 entry;
	int			 af;
	union eigrpd_addr	 addr;
	uint8_t			 prefixlen;
	union eigrpd_addr	 dstbrd;
};
TAILQ_HEAD(if_addr_head, if_addr);

struct iface {
	TAILQ_ENTRY(iface)	 entry;
	TAILQ_HEAD(, eigrp_iface) ei_list;
	unsigned int		 ifindex;
	char			 name[IF_NAMESIZE];
	struct if_addr_head      addr_list;
	struct in6_addr		 linklocal;
	int			 mtu;
	enum iface_type		 type;
	uint8_t			 if_type;
	uint64_t		 baudrate;
	uint16_t		 flags;
	uint8_t			 linkstate;
	uint8_t			 group_count_v4;
	uint8_t			 group_count_v6;
};

enum route_type {
	EIGRP_ROUTE_INTERNAL,
	EIGRP_ROUTE_EXTERNAL
};

/* routing information advertised by update/query/reply messages */
struct rinfo {
	int				 af;
	enum route_type			 type;
	union eigrpd_addr		 prefix;
	uint8_t				 prefixlen;
	union eigrpd_addr		 nexthop;
	struct classic_metric		 metric;
	struct classic_emetric		 emetric;
};

struct rinfo_entry {
	TAILQ_ENTRY(rinfo_entry)	 entry;
	struct rinfo			 rinfo;
};
TAILQ_HEAD(rinfo_head, rinfo_entry);

/* interface states */
#define	IF_STA_DOWN		0x01
#define	IF_STA_ACTIVE		0x02

struct eigrp_iface {
	RB_ENTRY(eigrp_iface)	 id_tree;
	TAILQ_ENTRY(eigrp_iface) e_entry;
	TAILQ_ENTRY(eigrp_iface) i_entry;
	struct eigrp		*eigrp;
	struct iface		*iface;
	int			 state;
	uint32_t		 ifaceid;
	struct event		 hello_timer;
	uint32_t		 delay;
	uint32_t		 bandwidth;
	uint16_t		 hello_holdtime;
	uint16_t		 hello_interval;
	uint8_t			 splithorizon;
	uint8_t			 passive;
	time_t			 uptime;
	TAILQ_HEAD(, nbr)	 nbr_list;
	struct nbr		*self;
	struct rinfo_head	 update_list;	/* multicast updates */
	struct rinfo_head	 query_list;	/* multicast queries */
};

#define INADDRSZ	4
#define IN6ADDRSZ	16

struct seq_addr_entry {
	TAILQ_ENTRY(seq_addr_entry)	 entry;
	int				 af;
	union eigrpd_addr		 addr;
};
TAILQ_HEAD(seq_addr_head, seq_addr_entry);

struct nbr;
RB_HEAD(nbr_addr_head, nbr);
RB_HEAD(nbr_pid_head, nbr);
struct rt_node;
RB_HEAD(rt_tree, rt_node);

#define	REDIST_STATIC		0x01
#define	REDIST_RIP		0x02
#define	REDIST_OSPF		0x04
#define	REDIST_CONNECTED	0x08
#define	REDIST_DEFAULT		0x10
#define	REDIST_ADDR		0x20
#define	REDIST_NO		0x40

struct redist_metric {
	uint32_t		bandwidth;
	uint32_t		delay;
	uint8_t			reliability;
	uint8_t			load;
	uint16_t		mtu;
};

struct redistribute {
	SIMPLEQ_ENTRY(redistribute)	 entry;
	uint8_t				 type;
	int				 af;
	union eigrpd_addr		 addr;
	uint8_t				 prefixlen;
	struct redist_metric		*metric;
	struct {
		uint32_t		 as;
		uint32_t		 metric;
		uint32_t		 tag;
	} emetric;
};
SIMPLEQ_HEAD(redist_list, redistribute);

/* eigrp instance */
struct eigrp {
	TAILQ_ENTRY(eigrp)	 entry;
	int			 af;
	uint16_t		 as;
	uint8_t			 kvalues[6];
	uint16_t		 active_timeout;
	uint8_t			 maximum_hops;
	uint8_t			 maximum_paths;
	uint8_t			 variance;
	struct redist_metric	*dflt_metric;
	struct redist_list	 redist_list;
	TAILQ_HEAD(, eigrp_iface) ei_list;
	struct nbr_addr_head	 nbrs;
	struct rde_nbr		*rnbr_redist;
	struct rde_nbr		*rnbr_summary;
	struct rt_tree		 topology;
	uint32_t		 seq_num;
};

/* eigrp_conf */
enum {
	PROC_MAIN,
	PROC_EIGRP_ENGINE,
	PROC_RDE_ENGINE
} eigrpd_process;

struct eigrpd_conf {
	struct in_addr		 rtr_id;

	uint32_t		 opts;
#define EIGRPD_OPT_VERBOSE	0x00000001
#define EIGRPD_OPT_VERBOSE2	0x00000002
#define EIGRPD_OPT_NOACTION	0x00000004

	int			 flags;
#define	EIGRPD_FLAG_NO_FIB_UPDATE 0x0001

	time_t			 uptime;
	int			 eigrp_socket_v4;
	int			 eigrp_socket_v6;
	unsigned int		 rdomain;
	uint8_t			 fib_priority_internal;
	uint8_t			 fib_priority_external;
	TAILQ_HEAD(, iface)	 iface_list;
	TAILQ_HEAD(, eigrp)	 instances;
	char			*csock;
};

/* kroute */
struct kroute {
	int			 af;
	union eigrpd_addr	 prefix;
	uint8_t			 prefixlen;
	union eigrpd_addr	 nexthop;
	unsigned short		 ifindex;
	uint8_t			 priority;
	uint16_t		 flags;
};

struct kaddr {
	unsigned short		 ifindex;
	int			 af;
	union eigrpd_addr	 addr;
	uint8_t			 prefixlen;
	union eigrpd_addr	 dstbrd;
};

struct kif {
	char			 ifname[IF_NAMESIZE];
	unsigned short		 ifindex;
	int			 flags;
	uint8_t			 link_state;
	int			 mtu;
	uint8_t			 if_type;
	uint64_t		 baudrate;
	uint8_t			 nh_reachable;	/* for nexthop verification */
};

/* control data structures */
struct ctl_iface {
	int			 af;
	uint16_t		 as;
	char			 name[IF_NAMESIZE];
	unsigned int		 ifindex;
	union eigrpd_addr	 addr;
	uint8_t			 prefixlen;
	uint16_t		 flags;
	uint8_t			 linkstate;
	int			 mtu;
	enum iface_type		 type;
	uint8_t			 if_type;
	uint64_t		 baudrate;
	uint32_t		 delay;
	uint32_t		 bandwidth;
	uint16_t		 hello_holdtime;
	uint16_t		 hello_interval;
	uint8_t			 splithorizon;
	uint8_t			 passive;
	time_t			 uptime;
	int			 nbr_cnt;
};

struct ctl_nbr {
	int			 af;
	uint16_t		 as;
	char			 ifname[IF_NAMESIZE];
	union eigrpd_addr	 addr;
	uint16_t		 hello_holdtime;
	time_t			 uptime;
};

struct ctl_rt {
	int			 af;
	uint16_t		 as;
	union eigrpd_addr	 prefix;
	uint8_t			 prefixlen;
	enum route_type		 type;
	union eigrpd_addr	 nexthop;
	char			 ifname[IF_NAMESIZE];
	uint32_t		 distance;
	uint32_t		 rdistance;
	uint32_t		 fdistance;
	int			 state;
	uint8_t			 flags;
	struct {
		uint32_t	 delay;
		uint32_t	 bandwidth;
		uint32_t	 mtu;
		uint8_t		 hop_count;
		uint8_t		 reliability;
		uint8_t		 load;
	} metric;
	struct classic_emetric	 emetric;
};
#define	F_CTL_RT_FIRST		0x01
#define	F_CTL_RT_SUCCESSOR	0x02
#define	F_CTL_RT_FSUCCESSOR	0x04

struct ctl_show_topology_req {
	int			 af;
	union eigrpd_addr	 prefix;
	uint8_t			 prefixlen;
	uint16_t		 flags;
};

/* parse.y */
struct eigrpd_conf	*parse_config(char *, int);
int			 cmdline_symset(char *);

/* in_cksum.c */
uint16_t	 in_cksum(void *, size_t);

/* kroute.c */
int		 kif_init(void);
void		 kif_redistribute(void);
int		 kr_init(int, unsigned int);
int		 kr_change(struct kroute *);
int		 kr_delete(struct kroute *);
void		 kif_clear(void);
void		 kr_shutdown(void);
void		 kr_fib_couple(void);
void		 kr_fib_decouple(void);
void		 kr_fib_reload(void);
void		 kr_dispatch_msg(int, short, void *);
void		 kr_show_route(struct imsg *);
void		 kr_ifinfo(char *, pid_t);
struct kif	*kif_findname(char *);
void		 kr_reload(void);

/* util.c */
uint8_t		 mask2prefixlen(in_addr_t);
uint8_t		 mask2prefixlen6(struct sockaddr_in6 *);
in_addr_t	 prefixlen2mask(uint8_t);
struct in6_addr	*prefixlen2mask6(uint8_t);
void		 eigrp_applymask(int, union eigrpd_addr *,
    const union eigrpd_addr *, int);
int		 eigrp_addrcmp(int, union eigrpd_addr *, union eigrpd_addr *);
int		 eigrp_addrisset(int, union eigrpd_addr *);
void		 embedscope(struct sockaddr_in6 *);
void		 recoverscope(struct sockaddr_in6 *);
void		 addscope(struct sockaddr_in6 *, uint32_t);
void		 clearscope(struct in6_addr *);

/* eigrpd.c */
void		 main_imsg_compose_eigrpe(int, pid_t, void *, uint16_t);
void		 main_imsg_compose_rde(int, pid_t, void *, uint16_t);
void		 merge_config(struct eigrpd_conf *, struct eigrpd_conf *);
void		 config_clear(struct eigrpd_conf *);
void		 imsg_event_add(struct imsgev *);
int		 imsg_compose_event(struct imsgev *, uint16_t, uint32_t,
    pid_t, int, void *, uint16_t);
uint32_t	 eigrp_router_id(struct eigrpd_conf *);
struct eigrp	*eigrp_find(struct eigrpd_conf *, int, uint16_t);

/* printconf.c */
void	print_config(struct eigrpd_conf *);

#endif	/* _EIGRPD_H_ */
