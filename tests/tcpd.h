/*
 * Test-only stand-in for <tcpd.h>: permits every host.  It lets talkd
 * be built and exercised without libwrap installed.  Never ship it.
 */
#include <stdio.h>
struct host_info { char name[128]; };
struct request_info { struct host_info client[1]; };
enum { RQ_FILE = 1, RQ_DAEMON, RQ_CLIENT_SIN, RQ_SERVER_SIN };
void request_init(struct request_info *, ...);
void sock_methods(struct request_info *);
int hosts_access(struct request_info *);
void refuse(struct request_info *);
char *eval_client(struct request_info *);
char *eval_hostinfo(struct host_info *);
extern int allow_severity, deny_severity;
