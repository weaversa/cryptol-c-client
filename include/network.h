#ifndef CAAS_NETWORK_H
#define CAAS_NETWORK_H

#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <math.h>

typedef struct caas_t {
  int socket;
  json_object *state;
  json_object *id;
} caas_t;

caas_t *caas_connect(char ip_address[16], uint32_t port);
void caas_disconnect(caas_t *cryserv);
void caas_send(caas_t *cryserv, json_object *msg);
json_object *caas_read(caas_t *cryserv);

void caas_load_module(caas_t *cryserv, char *module_name);



#endif
