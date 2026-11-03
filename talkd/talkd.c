/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

char copyright[] =
  "@(#) Copyright (c) 1983 Regents of the University of California.\n"
  "All rights reserved.\n";

/*
 * From: @(#)talkd.c	5.8 (Berkeley) 2/26/91
 */
char talkd_rcsid[] = 
  "$Id: talkd.c,v 1.12 1999/09/28 22:04:15 netbug Exp $";

#include "../version.h"

/*
 * talkd - internet talk daemon
 * loops waiting for and processing requests until idle for a while,
 * then exits.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <paths.h>
#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif
#include "prot_talkd.h"
#include "proto.h"

#define TIMEOUT 30
#define MAXIDLE 120
#define MINUDPSRCPORT 1024

#if !defined(MAXHOSTNAMELEN)
#define	MAXHOSTNAMELEN	64
#endif
char ourhostname[MAXHOSTNAMELEN];

static time_t lastmsgtime;
#if defined(HAVE_LIBWRAP) && !defined(__USE_GNU)
static char *program_invocation_short_name;
#endif
static int quiet;

static void
timeout(int ignore)
{
	(void)ignore;

	if (time(NULL) - lastmsgtime >= MAXIDLE)
		_exit(0);
	signal(SIGALRM, timeout);
	alarm(TIMEOUT);
}

/*
 * Returns true if the address belongs to the local host. If it's
 * not a loopback address, try binding to it.
 */
static int
is_local_address(u_int32_t addr)
{
	struct sockaddr_in sn;
	int sock, ret;
	if (addr == htonl(INADDR_LOOPBACK)) {
		return 1;
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0) {
		syslog(LOG_WARNING, "socket: %s", strerror(errno));
		return 0;
	}
	memset(&sn, 0, sizeof(sn));
	sn.sin_family = AF_INET;
	sn.sin_port = htons(0);
	sn.sin_addr.s_addr = addr;
	ret = bind(sock, (struct sockaddr *)&sn, sizeof(sn));
	close(sock);
	return ret==0;
}

#ifndef HAVE_LIBWRAP
static int
same_ip(const struct sockaddr *a, const struct sockaddr_in *b)
{
	const struct sockaddr_in *in = (const struct sockaddr_in *)a;

	return in->sin_addr.s_addr == b->sin_addr.s_addr;
}

/*
 * Name of the sending host, for the announcement banner.  A name is only
 * believed if it resolves back to the sender's address; otherwise the
 * numeric address is used.
 */
static void
peer_name(const struct sockaddr_in *sn, char *buf, size_t size)
{
	struct addrinfo hints, *res, *ai;
	char name[NI_MAXHOST];
	int ok = 0;

	if (getnameinfo((const struct sockaddr *)sn, sizeof(*sn),
			name, sizeof(name), NULL, 0, NI_NAMEREQD) == 0) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		if (getaddrinfo(name, NULL, &hints, &res) == 0) {
			for (ai = res; ai != NULL && !ok; ai = ai->ai_next)
				ok = same_ip(ai->ai_addr, sn);
			freeaddrinfo(res);
		}
	}
	if (ok && strlen(name) < size)
		strcpy(buf, name);
	else
		snprintf(buf, size, "%s", inet_ntoa(sn->sin_addr));
}
#endif

static void
send_packet(CTL_RESPONSE *response, struct sockaddr_in *sn, int quirk)
{
	char buf[2*sizeof(CTL_RESPONSE)];
	size_t sz = sizeof(CTL_RESPONSE);
	int cc, err=0;

	memcpy(buf, response, sz);
	if (quirk) {
		sz = irrationalize_reply(buf, sizeof(buf), quirk);
	}
	while (sz > 0) {
		cc = sendto(1, buf, sz, 0, (struct sockaddr *)sn, sizeof(*sn));
		if (cc<0) {
			syslog(LOG_WARNING, "sendto: %s", strerror(errno));
			if (err) return;
			err = 1;
		}
		else sz -= cc;
	}
}

/*
 * Issue an error packet. Should not assume anything other than the
 * header part (the u_int8_t's) of mp is valid, and assume as little
 * as possible about that, since it might have been a packet we 
 * couldn't dequirk.
 */
static void 
send_reject_packet(CTL_MSG *mp, struct sockaddr_in *sn, int code, int quirk)
{
	CTL_RESPONSE rsp;
	memset(&rsp, 0, sizeof(rsp));
	rsp.vers = TALK_VERSION;
	rsp.type = mp->type;
	rsp.answer = code;
	send_packet(&rsp, sn, quirk);
}

static void
do_one_packet(void)
{
	char inbuf[2*sizeof(CTL_MSG)];
	int quirk = 0;
	CTL_RESPONSE response;
	CTL_MSG *mp;
	char theirhost[MAXHOSTNAMELEN];
	const char *theirip;

	struct sockaddr_in sn;
#ifdef HAVE_LIBWRAP
	struct sockaddr_in server_sin;
#endif
	int cc;
	socklen_t addrlen;
 	int theirport;

#ifdef HAVE_LIBWRAP
	struct request_info request;
#endif

	signal(SIGALRM, timeout);
	alarm(TIMEOUT);
	addrlen = sizeof(sn);
	cc = recvfrom(0, inbuf, sizeof(inbuf), 0,
		      (struct sockaddr *)&sn, &addrlen);
	if (cc<0) {
		if (errno==EINTR || errno==EAGAIN) {
			return;
		}
		syslog(LOG_WARNING, "recvfrom: %s", strerror(errno));
		return;
	}

	/* 
	 * This should be set on any input, even trash, because even
	 * trash input will cause us to be restarted if we exit.
	 */
	lastmsgtime = time(NULL);

	if (addrlen!=sizeof(sn)) {
		syslog(LOG_WARNING, "recvfrom: bogus address length");
		return;
	}
	if (sn.sin_family!=AF_INET) {
		syslog(LOG_WARNING, "recvfrom: bogus address family");
		return;
	}

 	theirport = ntohs(sn.sin_port);
 	if (theirport < MINUDPSRCPORT) {
		syslog(LOG_WARNING, "%d: bad port", theirport);
		return;
	}

	/* 
	 * If we get here we have an address we can reply to, although
	 * it may not be good for much. If possible, reply to it, because
	 * if we just drop the packet the remote talk client will keep
	 * throwing junk at us.
	 */
	theirip = inet_ntoa(sn.sin_addr);
	mp = (CTL_MSG *)inbuf;

#ifdef HAVE_LIBWRAP
	addrlen = sizeof(server_sin);
	if (getsockname(0, (struct sockaddr *)&server_sin, &addrlen)<0) {
		syslog(LOG_WARNING, "getsockname: %s", strerror(errno));
		return;
	}
	request_init(&request,
		     RQ_FILE, 0,
		     RQ_DAEMON, program_invocation_short_name,
		     RQ_CLIENT_SIN, &sn,
		     RQ_SERVER_SIN, &server_sin,
		     0);
	sock_methods(&request);
	if (!hosts_access(&request)) {
		refuse(&request);
		send_reject_packet(mp, &sn, MACHINE_UNKNOWN, 0);
		return;
	}
	if (!quiet) {
		syslog(LOG_INFO, "connect from %s", eval_client(&request));
	}

	signal(SIGALRM, timeout);
	alarm(TIMEOUT);
	strncpy(theirhost, eval_hostinfo(request.client), sizeof(theirhost));
	theirhost[sizeof(theirhost)-1] = 0;
#else
	peer_name(&sn, theirhost, sizeof(theirhost));
	signal(SIGALRM, timeout);
	alarm(TIMEOUT);
	if (!quiet) {
		syslog(LOG_INFO, "connect from %s (%s)", theirhost, theirip);
	}
#endif

	/*
	 * Try to straighten out bad packets.
	 */
	quirk = rationalize_packet(inbuf, cc, sizeof(inbuf), &sn);
	if (quirk<0) {
		print_broken_packet(inbuf, cc, &sn);
		syslog(LOG_WARNING, "%s (%s): unintelligible packet", 
		       theirhost, theirip);
		send_reject_packet(mp, &sn, UNKNOWN_REQUEST, 0);
		return;
	}

	/*
	 * Make sure we know what we're getting into.
	 */
	if (mp->vers!=TALK_VERSION) {
		syslog(LOG_WARNING, "%s (%s): bad protocol version %d", 
		       theirhost, theirip, mp->vers);
		send_reject_packet(mp, &sn, BADVERSION, 0);
		return;
	}

	/*
	 * LEAVE_INVITE messages should only come from localhost.
	 * Of course, old talk clients send from our hostname's IP
	 * rather than localhost, complicating the issue...
	 */
	if (mp->type==LEAVE_INVITE && !is_local_address(sn.sin_addr.s_addr)) {
		syslog(LOG_WARNING, "%s (%s) sent invite packet",
		       theirhost, theirip);
		send_reject_packet(mp, &sn, MACHINE_UNKNOWN, quirk);
		return;
	}

	/*
	 * Junk the reply address they reported for themselves. Write 
	 * the real one over it because announce.c gets it from there.
	 */
	mp->ctl_addr.ta_family = AF_INET;
	mp->ctl_addr.ta_port = sn.sin_port;
	mp->ctl_addr.ta_addr = sn.sin_addr.s_addr;

	/*
	 * Since invite messages only come from localhost, and nothing
	 * but invite messages use the TCP address, force it to be our
	 * address.
	 * 
	 * Actually, if it's a local address, leave it alone. talk has
	 * to play games to figure out the right interface address to
	 * use, and we don't want to get into that - they can work it
	 * out, but we can't since we don't know who they're trying to
	 * talk to.
	 *
	 * If it's not a local address, someone's trying to play games
	 * with us. Rather than trying to pick a local address to use,
	 * reject the packet.
	 */
	if (mp->type==LEAVE_INVITE) {
		mp->addr.ta_family = AF_INET;
		if (!is_local_address(mp->addr.ta_addr)) {
			syslog(LOG_WARNING, 
			       "invite packet had bad return address");
			send_reject_packet(mp, &sn, BADADDR, quirk);
			return;
		}
	}
	else {
		/* non-invite packets don't use this field */
		memset(&mp->addr, 0, sizeof(mp->addr));
	}

	process_request(mp, &response, theirhost);

	/* can block here, is this what I want? */
	send_packet(&response, &sn, quirk);
}

int
main(int argc, char *argv[])
{
	struct sockaddr_in sn;
	socklen_t sz = sizeof(sn);
	int do_debug=0, do_badpackets=0, ch;

#if defined(HAVE_LIBWRAP) && !defined(__USE_GNU)
	program_invocation_short_name = argv[0];
	if (argv[0][0] == '/') {
		program_invocation_short_name = strrchr(argv[0], '/') + 1;
	}
#endif
	/* make sure we're a daemon */
	if (getsockname(0, (struct sockaddr *)&sn, &sz)) {
		printf ("must be run from inetd.\n");
		exit (1);
	}
	openlog("talkd", LOG_PID, LOG_DAEMON);
	if (gethostname(ourhostname, sizeof(ourhostname) - 1) < 0) {
		syslog(LOG_ERR, "gethostname: %s", strerror(errno));
		exit(1);
	}
	if (chdir(_PATH_DEV) < 0) {
		syslog(LOG_ERR, "chdir: %s: %s", _PATH_DEV, strerror(errno));
		exit(1);
	}
	while ((ch = getopt(argc, argv, "dpq"))!=-1) {
		switch (ch) {
		    case 'd': do_debug=1; break;
		    case 'p': do_badpackets=1; break;
		    case 'q': quiet=1; break;
		}
	}
	set_debug(do_debug, do_badpackets);

	for (;;) {
		do_one_packet();
	}
/*	return 0;  <--- unreachable because of the above loop */
}
