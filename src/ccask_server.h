#ifndef _CCASK_SERVER_H
#define _CCASK_SERVER_H

#include "ccask_db.h"

typedef struct ccask_server ccask_server;

// init / destroy
ccask_server* ccask_server_init(ccask_server* srv, char* port, ccask_db* db);
ccask_server* ccask_server_new(char* port, ccask_db* db);

void ccask_server_destroy(ccask_server* srv);
void ccask_server_delete(ccask_server* srv);

void ccask_server_print(ccask_server* srv);

int ccask_server_run(ccask_server* srv);

#endif
