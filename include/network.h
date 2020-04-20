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
json_object *caas_get_value(json_object *jresult);

bitvector_t *caas_bitvector_t_from_bits(json_object *jbv);
sequence_t *caas_sequence_t_from_sequence(json_object *jseq);

void caas_reset_state(caas_t *caas);
void caas_load_module(caas_t *cryserv, char *module_name);
json_object *caas_from_boolean(uint8_t bit);
json_object *caas_from_hex(char *hex, uint32_t nBits);
json_object *caas_command(char *command);
json_object *caas_from_bitvector_t(bitvector_t *bv);
json_object *caas_from_sequence_t(sequence_t *seq);
char *bitvector_t_toCryptolString(bitvector_t *bv);
char *sequence_t_toCryptolString(sequence_t *sequence);
json_object *caas_split(json_object *jseq, uint32_t parts);
json_object *caas_add_to_tuple(json_object *jtuple, json_object *argument);
json_object *caas_add_to_record(json_object *jrec, char *field, json_object *value);
json_object *caas_add_argument(json_object *arguments, json_object *argument);
json_object *caas_call(char *name, json_object *arguments);
json_object *caas_evaluate_expression(caas_t *caas, json_object *expression);


#endif
