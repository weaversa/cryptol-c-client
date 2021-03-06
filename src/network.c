#include "cryptol-service.h"

/**
 * Create a connection to a remote Cryptol service
 **/

caas_t *caas_connect(char ip_address[16], uint32_t port) {
  caas_t *caas = malloc(sizeof(caas_t));
  struct sockaddr_in caas_addr;
  //Create socket to Cryptol service
  if ((caas->socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "Socket creation to Cryptol service failed.\n");
    return NULL;
  }
  
  caas_addr.sin_family = PF_INET;
  caas_addr.sin_port = htons(port);
  
  //Fill caas_addr.sin_addr with the IP address of the Cryptol service
  if(inet_pton(PF_INET, ip_address, &caas_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Can't parse IP address (%s) of Cryptol service\n", ip_address);
    return NULL;
  }

  //Attempt to connect to the Cryptol service
  if(connect(caas->socket, (struct sockaddr *)&caas_addr, sizeof(caas_addr)) < 0) {
    printf("Error: Connection to Cryptol service failed\n");
    perror("Errno is \n");
    return NULL;
  }

  //Initialize a new session --- empty state ("null") and id of 0.
  caas->state = json_type_null;
  caas->id = json_object_new_int(0);
  
  return caas;
}


/**
 * Disconnect from a remote Cryptol service
 **/

void caas_disconnect(caas_t *caas) {
  close(caas->socket);
  json_object_put(caas->state);
  json_object_put(caas->id);

  free(caas);
}


/**
 * Count the number of digits in a decimal integer
 **/

int numPlaces (int n) {
  if (n == 0) return 1;
  return floor (log10 (abs (n))) + 1;
}


/**
 * Send a JSON formatted message to a remote Cryptol service
 * The message is consumed.
 **/

void caas_send(caas_t *caas, json_object *message) {
  json_object_object_add(message, "jsonrpc", json_object_new_string("2.0"));

  json_object *params;
  if(json_object_object_get_ex(message, "params", &params) == 0) {
    //Missing params, add it to message
    params = json_object_new_object();
    json_object_object_add(message, "params", params);
  }
  
  json_object_object_add(params, "state", json_object_get(caas->state));
  json_object_object_add(message, "id", json_object_get(caas->id));

  const char *message_string = json_object_get_string(message);

  size_t netstring_size = strlen(message_string) + numPlaces(strlen(message_string)) + 3;
  char *netstring = malloc(netstring_size);
  snprintf(netstring, netstring_size, "%ld:%s,", strlen(message_string), message_string);
  send(caas->socket, netstring, strlen(netstring), 0);
  fprintf(stdout, "Message sent: %s\n", message_string);

  free(netstring);
  json_object_put(message); //free message and all referenced objects
}


/**
 * Received a JSON formatted message from a remote Cryptol service
 **/

json_object *caas_read(caas_t *caas) {
  char c = 0;
  char buffer[10];
  int i, r;
  for(i = 0; (i < 10) && (c != ':'); i++) {
    r = read(caas->socket, &c, 1);
    if(r != 1) return NULL;
    buffer[i] = c;
  }
  if(buffer[i-1] != ':') return NULL;
  buffer[i-1] = 0;

  uint32_t nLength = atoi(buffer);

  char *jsonstring = calloc((nLength+1), sizeof(char));
  r = read(caas->socket, jsonstring, nLength);
  if(r != nLength) return NULL;

  r = read(caas->socket, &c, 1); //Read comma
  if(r != 1) return NULL;
  if(c != ',') return NULL;

  json_object *json_from_read = json_tokener_parse(jsonstring);
  free(jsonstring);

  printf("Message received: %s\n", json_object_get_string(json_from_read));
  
  json_object *id;
  if(json_object_object_get_ex(json_from_read, "id", &id) == 0) {
    fprintf(stderr, "Cryptol service id missing\n");
    json_object_put(json_from_read);
    return NULL;
  }
 
  if(json_object_get_int(caas->id) != json_object_get_int(id)) {
    fprintf(stderr, "Cryptol service id mismatch\n");
    json_object_put(json_from_read);
    return NULL;
  }

  if(json_object_object_get_ex(json_from_read, "error", NULL) == 1) {
    //return the error
    return json_from_read;
  }
  
  json_object *result;
  if(json_object_object_get_ex(json_from_read, "result", &result) == 0) {
    fprintf(stderr, "Cryptol service result missing\n");
    return NULL;
  }
  json_object_get(result);

  //Test to see if a new state is returned
  if(json_object_object_get_ex(result, "state", NULL) == 1) {
    //decrement reference count of old state
    json_object_put(caas->state);
    //save new state
    json_object_object_get_ex(result, "state", &caas->state);
  }

  //increment reference count of new state
  json_object_get(caas->state);

  //increment id
  json_object_put(caas->id);
  caas->id = json_object_new_int(json_object_get_int(id) + 1);

  json_object_put(json_from_read);

  return result;
}


/**
 * Get the 'value' tag of a json result
 **/

json_object *caas_get_value(json_object *jresult) {
  json_object *answer, *value;

  //Check for correct answer tag
  if(json_object_object_get_ex(jresult, "answer", &answer) == 0) {
    fprintf(stderr, "Missing 'answer' tag\n");
    return NULL;
  }

    //Check for correct value tag
  if(json_object_object_get_ex(answer, "value", &value) == 0) {
    fprintf(stderr, "Missing 'value' tag\n");
    return NULL;
  }
  
  return value;  
}


/**
 * Create a bitvector_t from a json expression.
 * Sample input ---
 *   { "data": "1" , "width": 8, "expression": "bits", "encoding": "hex" }
 **/

bitvector_t *caas_bitvector_t_from_bits(json_object *jbv) {
  json_object *expression, *encoding, *data, *width;

  //Check for correct expression tag
  if(json_object_object_get_ex(jbv, "expression", &expression) == 0) {
    fprintf(stderr, "Missing 'expression' tag\n");
    return NULL;
  }
  if(strcmp(json_object_get_string(expression), "bits") != 0) {
    fprintf(stderr, "'expression' tag not \"bits\"\n");
    return NULL;
  }

  //Check for correct encoding tag
  if(json_object_object_get_ex(jbv, "encoding", &encoding) == 0) {
    fprintf(stderr, "Missing 'encoding' tag\n");
    return NULL;
  }
  if(strcmp(json_object_get_string(encoding), "hex") != 0) {
    fprintf(stderr, "'encoding' tag not \"hex\"\n");
    return NULL;
  }

  //Get data
  if(json_object_object_get_ex(jbv, "data", &data) == 0) {
    fprintf(stderr, "Missing 'data' tag\n");
    return NULL;
  }
  
  //Get width
  if(json_object_object_get_ex(jbv, "width", &width) == 0) {
    fprintf(stderr, "Missing 'width' tag\n");
    return NULL;
  }

  bitvector_t *ret_bv = bitvector_t_fromHexString((char *)json_object_get_string(data));
  uint32_t nBits = json_object_get_int(width);
  if(nBits > ret_bv->nBits) {
    bitvector_t_widenUpdate(ret_bv, nBits - ret_bv->nBits);
  } else if(nBits < ret_bv->nBits) {
    bitvector_t_dropUpdate(ret_bv, ret_bv->nBits - nBits);
  } //Else, number of bits are equal

  return ret_bv;  
}


/**
 * Create a sequence_t from a json expression.
 * Sample input ---
 *   { "expression": "sequence",
       "data": [ { "data": "1" , "width": 8, "expression": "bits", "encoding": "hex" },
                 { "data": "89", "width": 8, "expression": "bits", "encoding": "hex" },
                 { "data": "2" , "width": 8, "expression": "bits", "encoding": "hex" },
                 { "data": "90", "width": 8, "expression": "bits", "encoding": "hex" }
               ]
     }
 **/

sequence_t *caas_sequence_t_from_sequence(json_object *jseq) {
  json_object *expression, *data;

  //Check for correct expression tag
  if(json_object_object_get_ex(jseq, "expression", &expression) == 0) {
    fprintf(stderr, "Missing 'expression' tag\n");
    return NULL;
  }
  if(strcmp(json_object_get_string(expression), "sequence") != 0) {
    fprintf(stderr, "'expression' tag not \"sequence\"\n");
    return NULL;
  }

  //Get data
  if(json_object_object_get_ex(jseq, "data", &data) == 0) {
    fprintf(stderr, "Missing 'data' tag\n");
    return NULL;
  }
  
  uint32_t nLength = json_object_array_length(data);
  sequence_t *seq = sequence_t_alloc(nLength);
  
  uint32_t i;
  for(i = 0; i < nLength; i++) {
    json_object *datai = json_object_array_get_idx(data, i);
    bitvector_t *bv = caas_bitvector_t_from_bits(datai);
    if(bv == NULL) {
      fprintf(stderr, "Incorrectly formatted json sequence\n");
      sequence_t_free(seq, bitvector_t_free_inner);
      return NULL;
    }
    sequence_t_push(seq, *bv);
  }

  return seq;
}


/**
 * Reset the Cryptol state.
 */

void caas_reset_state(caas_t *caas) {
  json_object_put(caas->state);
  caas->state = json_type_null;
}


/**
 * Request the remote Cryptol service to load a particular
 * module. Akin to:
 * > :m 'module_name'
 **/

void caas_load_module(caas_t *caas, char *module_name) {
  json_object *message = json_object_new_object();

  json_object *params = json_object_new_object();
  json_object_object_add(message, "params", params);
  
  json_object_object_add(message, "method", json_object_new_string("load module"));
  
  json_object_object_add(params, "module name", json_object_new_string(module_name));

  //Reset the state just prior to sending to avoid unnecessary state
  //growth.
  caas_reset_state(caas);

  caas_send(caas, message);

  json_object *jresult = caas_read(caas);
  json_object_put(jresult); //free result
  //Should return success or failure
}


/**
 * Create a JSON boolean expression from a bit
 * Types of the form : Bit
 **/

json_object *caas_from_boolean(uint8_t bit) {
  json_object *jbool = json_object_new_boolean(bit);
  return jbool;
}


/**
 * Create a JSON bit expression from a string of hex characters. Do
 * not prepend '0x'. nBits can be any number --- does not have to be a
 * multiple of 4 (like it is when using the interpreter directly).
 * Types of the form : {a} (fin a) => [a]
 **/

json_object *caas_from_hex(char *hex, uint32_t nBits) {
  if(hex == NULL) return NULL;

  json_object *jhex = json_object_new_object();
  json_object_object_add(jhex, "expression", json_object_new_string("bits"));
  json_object_object_add(jhex, "encoding", json_object_new_string("hex"));
  json_object_object_add(jhex, "data", json_object_new_string(hex));
  json_object_object_add(jhex, "width", json_object_new_int(nBits));

  return jhex;
}


/**
 * Create a JSON expression from a string
 * Essentially: > 'command'
 **/

json_object *caas_command(char *command) {
  json_object *jcommand = json_object_new_string(command);
  return jcommand;
}


/**
 * Create a JSON bit expression from a bitvector_t.
 * Types of the form : {a} (fin a) => [a]
 **/

json_object *caas_from_bitvector_t(bitvector_t *bv) {
  if(bv == NULL) return NULL;

  char *hex=bitvector_t_toHexString(bv);
  json_object *jbv = caas_from_hex(hex, bv->nBits);
  free(hex);

  return jbv;
}


/**
 * Create a JSON sequence expression from a sequence_t.
 * Types of the form : {a, b} (fin a, b) => [a][b]
 **/

json_object *caas_from_sequence_t(sequence_t *seq) {
  if(seq == NULL) return NULL;

  json_object *jseq = json_object_new_object();
  json_object_object_add(jseq, "expression", json_object_new_string("sequence"));

  json_object *data = json_object_new_array();
  json_object_object_add(jseq, "data", data);
  
  uint32_t i;
  for(i = 0; i < seq->nLength; i++) {
    json_object_array_add(data, caas_from_bitvector_t(&seq->pList[i]));
  }
  
  return jseq;  
}


/**
 * Create a Cryptol-formatted string from a bitvector_t.
 * This will produce, for example, (`0x1234:[15])
 **/

char *bitvector_t_toCryptolString(bitvector_t *bv) {
  char *hex_string = bitvector_t_toHexString(bv);
  if(hex_string == NULL) return NULL;
  
  uint32_t length = strlen(hex_string);
  length += numPlaces(bv->nBits);
  length += 9; //9 extra characters --- "(`0x:[])" + '\0' character

  char *ret_string = malloc(length);

  snprintf(ret_string, length, "(`0x%s:[%u])", hex_string, bv->nBits);
  free(hex_string);
  
  return ret_string;
}


/**
 * Create a Cryptol-formatted sequence from a sequence_t.
 * This will produce, for example, ([`0x1234,`0xabcd]:[2][15])
 **/

char *sequence_t_toCryptolString(sequence_t *sequence) {
  uint32_t i, length = 0;
  char **bvs = malloc(sizeof(char *) * sequence->nLength);
  for(i = 0; i < sequence->nLength; i++) {
    bvs[i] = bitvector_t_toHexString(&sequence->pList[i]);
    if(bvs[i] == NULL) { //Some problem, free all previous strings
      uint32_t j;
      for(j = 0;j < i; j++) {
	free(bvs[j]);
      }
      free(bvs);
      return NULL;
    }
    length += strlen(bvs[i]) + 4; //4 extra characters ---  ",`0x"
  }
  //Subtract one comma
  length--;
  length += numPlaces(sequence->pList[0].nBits);
  length += numPlaces(sequence->nLength);
  length += 10; //10 extra characters --- "([]:[][])" + '\0' character
  
  char *ret_string = malloc(length);

  snprintf(ret_string, length, "([");

  for(i = 0; i < sequence->nLength; i++) {
    strcat(ret_string, "`0x");
    strcat(ret_string, bvs[i]);
    if(i + 1 < sequence->nLength) {
      strcat(ret_string, ",");
    }
    free(bvs[i]);
  }
  strcat(ret_string, "]");
  free(bvs);
  
  length = numPlaces(sequence->pList[0].nBits);
  length += numPlaces(sequence->nLength);
  length += 7; //7 extra characters --- ":[][])" + '\0' character
  char *type_string = malloc(length);

  snprintf(type_string, length, ":[%zu][%u])", sequence->nLength, sequence->pList[0].nBits);

  strcat(ret_string, type_string);

  free(type_string);
  
  return ret_string;
}


/**
 * Split a JSON sequence expression into 'n' parts.
 * Consumes the sequence.
 */

json_object *caas_split(json_object *jseq, uint32_t parts) {
  if(jseq == NULL) {
    return NULL;
  }

  json_object *data;
  if(json_object_object_get_ex(jseq, "data", &data) == 0) {
    fprintf(stderr, "Error: Sequence missing data array\n");
    return NULL;
  }
  uint32_t nLength = json_object_array_length(data);

  if((nLength == 0) || (nLength % parts != 0)) {
    fprintf(stderr, "Cannot split a sequence of length %u into %u equal sized parts.\n", nLength, parts);
    return NULL;
  }

  json_object *jret = json_object_new_object();
  json_object_object_add(jret, "expression", json_object_new_string("sequence"));
  json_object *data_ret = json_object_new_array();
  json_object_object_add(jret, "data", data_ret);
  
  uint32_t i, j;
  for(i = 0; i < parts; i++) {
    json_object *jseqi = json_object_new_object();
    json_object_object_add(jseqi, "expression", json_object_new_string("sequence"));
    json_object *datai = json_object_new_array();
    json_object_object_add(jseqi, "data", datai);
  
    for(j = 0; j < nLength / parts; j++) {
      json_object_array_add(datai, json_object_get(json_object_array_get_idx(data, i*(nLength/parts) + j)));
    }
    json_object_array_add(data_ret, jseqi);
  }

  json_object_put(jseq); //free result
  
  return jret;
}

/**
 * Add an element to a JSON tuple expression (or create one if jtuple
 * is NULL.
 * Types of the form : {a} (fin a) => (..., a)
 **/

json_object *caas_add_to_tuple(json_object *jtuple, json_object *value) {
  json_object *data;
  if(jtuple == NULL) {
    jtuple = json_object_new_object();
    json_object_object_add(jtuple, "expression", json_object_new_string("tuple"));

    data = json_object_new_array();
    json_object_object_add(jtuple, "data", data);
  } else {
    if(json_object_object_get_ex(jtuple, "data", &data) == 0) {
      fprintf(stderr, "Error: Tuple missing data array\n");
      return NULL;
    }
  }

  json_object_array_add(data, value);
  
  return jtuple;
}


/**
 * Add an element to a JSON record expression (or create one if jrec
 * is NULL.
 * Types of the form : {a} (fin a) => {..., field=b)
 **/

json_object *caas_add_to_record(json_object *jrec, char *field, json_object *value) {
  json_object *data;
  if(jrec == NULL) {
    jrec = json_object_new_object();
    json_object_object_add(jrec, "expression", json_object_new_string("record"));

    data = json_object_new_object();
    json_object_object_add(jrec, "data", data);
  } else {
    if(json_object_object_get_ex(jrec, "data", &data) == 0) {
      fprintf(stderr, "Error: Record missing data object\n");
      return NULL;
    }
  }

  json_object_object_add(data, field, value);
  
  return jrec;
}


/**
 * Add an element to a JSON function's argument expression (or create
 * one if arguments is NULL.
 * Types of the form : {a} (fin a) => f ... a
 **/

json_object *caas_add_argument(json_object *arguments, json_object *argument) {
  if(arguments == NULL) {
    arguments = json_object_new_array();
  }

  json_object_array_add(arguments, argument);
  
  return arguments;
}


/**
 * Create a JSON expression for calling a function.
 **/

json_object *caas_call(char *name, json_object *arguments) {
  json_object *jfun = json_object_new_object();

  json_object_object_add(jfun, "expression", json_object_new_string("call"));
  json_object_object_add(jfun, "function", json_object_new_string(name));
  
  json_object_object_add(jfun, "arguments", arguments);

  return jfun;
}


/**
 * Request the remote Cryptol service to evaluate an expression.
 * The expression is consumed.
 **/

json_object *caas_evaluate_expression(caas_t *caas, json_object *expression) {
  json_object *message = json_object_new_object();

  json_object *params = json_object_new_object();
  json_object_object_add(message, "params", params);
  
  json_object_object_add(message, "method", json_object_new_string("evaluate expression"));

  json_object_object_add(params, "expression", expression);
  
  caas_send(caas, message);

  json_object *jresult = caas_read(caas);

  return jresult;
}
