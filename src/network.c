#include "cryptol-service.h"

cryptol_service_t *cryptol_service_connect(char ip_address[16], uint32_t port) {
  cryptol_service_t *cryserv = malloc(sizeof(cryptol_service_t));
  struct sockaddr_in cryserv_addr;
  //Create socket to Cryptol service
  if ((cryserv->socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "Socket creation to Cryptol service failed.\n");
    return NULL;
  }
  
  cryserv_addr.sin_family = PF_INET;
  cryserv_addr.sin_port = htons(port);
  
  //Fill cryserv_addr.sin_addr with the IP address of the Cryptol service
  if(inet_pton(PF_INET, ip_address, &cryserv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Can't parse IP address (%s) of Cryptol service\n", ip_address);
    return NULL;
  }

  //Attempt to connect to the Cryptol service
  if(connect(cryserv->socket, (struct sockaddr *)&cryserv_addr, sizeof(cryserv_addr)) < 0) {
    printf("Error: Connection to Cryptol service failed\n");
    return NULL;
  }

  //Initialize a new session -- empty state and id of 0.
  cryserv->state = json_object_new_array();
  cryserv->id = json_object_new_int(0);
  
  return cryserv;
}

void cryptol_service_disconnect(cryptol_service_t *cryserv) {
  close(cryserv->socket);
  json_object_put(cryserv->state);
  json_object_put(cryserv->id);

  free(cryserv);
}

int numPlaces (int n) {
  if (n == 0) return 1;
  return floor (log10 (abs (n))) + 1;
}

void cryptol_service_send(cryptol_service_t *cryserv, json_object *message) {
  json_object_object_add(message, "jsonrpc", json_object_new_string("2.0"));

  json_object *params;
  if(json_object_object_get_ex(message, "params", &params) == FALSE) {
    //Missing params, add it to message
    params = json_object_new_object();
    json_object_object_add(message, "params", params);
  }
  
  json_object_object_add(params, "state", json_object_get(cryserv->state));
  json_object_object_add(message, "id", json_object_get(cryserv->id));

  const char *message_string = json_object_get_string(message);

  uint32_t netstring_size = strlen(message_string) + numPlaces(strlen(message_string)) + 3;
  char *netstring = malloc(netstring_size);
  snprintf(netstring, netstring_size, "%ld:%s,", strlen(message_string), message_string);
  send(cryserv->socket, netstring, strlen(netstring), 0);
  fprintf(stdout, "Message sent: %s\n", message_string);

  free(netstring);
  json_object_put(message); //free message and all referenced objects
}

json_object *cryptol_service_read(cryptol_service_t *cryserv) {
  char c = 0;
  char buffer[10];
  int i, r;
  for(i = 0; (i < 10) && (c != ':'); i++) {
    r = read(cryserv->socket, &c, 1);
    if(r != 1) return NULL;
    buffer[i] = c;
  }
  if(buffer[i-1] != ':') return NULL;
  buffer[i-1] = 0;

  uint32_t nLength = atoi(buffer);

  char *jsonstring = malloc(nLength+1);
  r = read(cryserv->socket, jsonstring, nLength);
  if(r != nLength) return NULL;

  r = read(cryserv->socket, &c, 1); //Read comma
  if(r != 1) return NULL;
  if(c != ',') return NULL;
 
  json_object *json_from_read = json_tokener_parse(jsonstring);
  free(jsonstring);

  printf("Message received: %s\n", json_object_get_string(json_from_read));
  
  json_object *id;
  if(json_object_object_get_ex(json_from_read, "id", &id) == FALSE) {
    fprintf(stderr, "Cryptol service id missing\n");
    json_object_put(json_from_read);
    return NULL;
  }
 
  if(json_object_get_int(cryserv->id) != json_object_get_int(id)) {
    fprintf(stderr, "Cryptol service id mismatch\n");
    json_object_put(json_from_read);
    return NULL;
  }

  if(json_object_object_get_ex(json_from_read, "error", NULL) == TRUE) {
    //return the error
    return json_from_read;
  }
  
  json_object *result;
  if(json_object_object_get_ex(json_from_read, "result", &result) == FALSE) {
    fprintf(stderr, "Cryptol service result missing\n");
    return NULL;
  }
  json_object_get(result);
  json_object_put(json_from_read);

  //Test to see if a new state is returned
  if(json_object_object_get_ex(result, "state", NULL) == TRUE) {
    //decrement reference count of old state
    json_object_put(cryserv->state);
    //save new state
    json_object_object_get_ex(result, "state", &cryserv->state);
  }

  //increment reference count of new state
  json_object_get(cryserv->state);

  //increment id
  json_object_put(cryserv->id);
  cryserv->id = json_object_new_int(json_object_get_int(id) + 1);

  return result;
}

void cryptol_service_load_module(cryptol_service_t *cryserv, char *module_name) {
  json_object *message = json_object_new_object();

  json_object *params = json_object_new_object();
  json_object_object_add(message, "params", params);
  
  json_object_object_add(message, "method", json_object_new_string("load module"));
  
  json_object_object_add(params, "module name", json_object_new_string(module_name));

  cryptol_service_send(cryserv, message);

  json_object *json_result = cryptol_service_read(cryserv);
  json_object_put(json_result); //free result
}

/*
{ "answer":  
  { "value": 
    { "expression": "sequence",
      "data":
      [ { "expression": "sequence",
          "data":
            [ { "data": "1" , "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "89", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "1" , "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "89", "width": 8, "expression": "bits", "encoding": "hex" } ],
        },  
	{ "expression": "sequence",
          "data":
            [ { "data": "23", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "ab", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "23", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "ab", "width": 8, "expression": "bits", "encoding": "hex" } ], 
	},  
        { "expression": "sequence",
          "data":
            [ { "data": "45", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "cd", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "45", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "cd", "width": 8, "expression": "bits", "encoding": "hex" } ], 
        },  
        { "expression": "sequence", 
          "data":
            [ { "data": "67", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "ef", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "67", "width": 8, "expression": "bits", "encoding": "hex" },
              { "data": "ef", "width": 8, "expression": "bits", "encoding": "hex" } ], 
        } ],
    },  
    "type string": "Primitive::Symmetric::Cipher::Block::AES::State", 
    "type": { "propositions": [ ],
              "forall": [ ],
              "type": { "type": "sequence",
                        "length": { "value": 4, "type": "number" },
                        "contents": { "type": "sequence",
                                      "length": { "value": 4, "type": "number" }, 
                                      "contents": { "width": { "value": 8, "type": "number" },
                                                    "type": "bitvector"
                                                  },
                                    },
                      }
            }
  }
}
*/

json_object *caas_bitvector(bitvector_t *bv) {
  char *hex;

  if(bv == NULL) return NULL;

  json_object *jbv = json_object_new_object();
  json_object_object_add(jbv, "expression", json_object_new_string("bits"));
  json_object_object_add(jbv, "encoding", json_object_new_string("hex"));
  json_object_object_add(jbv, "data", json_object_new_string(hex=bitvector_t_toHexString(bv))); free(hex);
  json_object_object_add(jbv, "width", json_object_new_int(bv->nBits));

  return jbv;
}

json_object *caas_sequence(sequence_t *seq) {
  if(seq == NULL) return NULL;

  json_object *jseq = json_object_new_object();
  json_object_object_add(jseq, "expression", json_object_new_string("sequence"));

  json_object *data = json_object_new_array();
  uint32_t i;
  for(i = 0; i < seq->nLength; i++) {
    json_object_array_add(data, caas_bitvector(&seq->pList[i]));
  }
  json_object_object_add(jseq, "data", data);
  
  return jseq;  
}
