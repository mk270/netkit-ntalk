/* Test-only permissive replacement for libwrap; see tcpd.h. */
#include "tcpd.h"

int allow_severity, deny_severity;

void request_init(struct request_info *r, ...) { r->client[0].name[0] = 0; }
void sock_methods(struct request_info *r) { (void)r; }
int hosts_access(struct request_info *r) { (void)r; return 1; }
void refuse(struct request_info *r) { (void)r; }
char *eval_client(struct request_info *r) { (void)r; return "test"; }
char *eval_hostinfo(struct host_info *h) { return h->name; }
