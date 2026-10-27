/*
 * This routines handle conversions needed to make standard talk/talkd
 * compatible with Sun's. SunOS/Solaris use a different protocol than
 * anyone else.
 *
 * What we do here is take SunOS' messages and convert them to standard
 * ones, so that our talk/talkd can manage them without many changes.
 *
 * Juan-Mariano de Goyeneche. jmseyas@dit.upm.es
 * Mon Aug 10 1998
 */

#ifdef SUN_HACK
#include <string.h>
#include "talk.h"

char personality;

void msg2msg_S(const CTL_MSG* msg, CTL_MSG_S* msg_S)
{
	msg_S->type = msg->type;
	strncpy (msg_S->l_name, msg->l_name, NAME_SIZE_S);
	msg_S->l_name[NAME_SIZE_S - 1] = '\0';
	strncpy (msg_S->r_name, msg->r_name, NAME_SIZE_S);
	msg_S->r_name[NAME_SIZE_S - 1] = '\0';
	msg_S->pad = 0;
	msg_S->id_num = msg->id_num;
	msg_S->pid = msg->pid;
	strncpy (msg_S->r_tty, msg->r_tty, TTY_SIZE);
	msg_S->r_tty[TTY_SIZE - 1] = '\0';
	memcpy(&(msg_S->addr), &(msg->addr), sizeof(msg_S->addr));
/*	if (*(&(msg_S->addr.sa_family)+1) == (short)0)
		msg_S->addr.sa_family = 0;*/
	memcpy(&(msg_S->ctl_addr), &(msg->ctl_addr), sizeof(msg_S->ctl_addr));
}

void resp_S2resp(const CTL_RESPONSE_S* resp_S, CTL_RESPONSE* resp)
{
	resp->vers = TALK_VERSION;
	resp->type = resp_S->type;
	resp->answer = resp_S->answer;
	resp->pad = 0;
	resp->id_num = resp_S->id_num;
	memcpy(&(resp->addr), &(resp_S->addr), sizeof(resp->addr));
}
#endif
