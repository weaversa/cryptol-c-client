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
json_object *caas_hex(char *hex, uint32_t nBits);
json_object *caas_bitvector(bitvector_t *bv);
json_object *caas_sequence(sequence_t *seq);
json_object *caas_split(json_object *jseq, uint32_t parts);
json_object *caas_add_to_tuple(json_object *jtuple, json_object *argument);
json_object *caas_add_argument(json_object *arguments, json_object *argument);
json_object *caas_call(char *name, json_object *arguments);
json_object *caas_evaluate_expression(caas_t *caas, json_object *expression);


#endif
