#ifndef CRYSRV_NETWORK_H
#define CRYSRV_NETWORK_H

#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <math.h>

typedef struct cryptol_service_t {
  int socket;
  json_object *state;
  json_object *id;
} cryptol_service_t;

cryptol_service_t *cryptol_service_connect(char ip_address[16], uint32_t port);
void cryptol_service_disconnect(cryptol_service_t *cryserv);
void cryptol_service_send(cryptol_service_t *cryserv, json_object *msg);
json_object *cryptol_service_read(cryptol_service_t *cryserv);

void cryptol_service_load_module(cryptol_service_t *cryserv, char *module_name);

#endif
