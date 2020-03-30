#include "cryptol-service.h"

int main(int argc, char const *argv[]) { 
  if(argc != 3) {
    fprintf(stderr,"usage %s hostname port\n", argv[0]);
    exit(0);
  }
  
  char ip_address[16];  
  strncpy(ip_address, argv[1], 16);
  uint32_t port = atoi(argv[2]);
  
  cryptol_service_t *cryserv = cryptol_service_connect(ip_address, port);
  if(cryserv == NULL) return 0;

  cryptol_service_load_module(cryserv, "Primitive::Symmetric::Cipher::Block::AES");

  json_object *msg = json_object_new_object();
  json_object_object_add(msg, "method", json_object_new_string("evaluate expression"));

  json_object *params = json_object_new_object();
  json_object_object_add(msg, "params", params);

  json_object *expression = json_object_new_object();
  json_object_object_add(params, "expression", expression);

  json_object_object_add(expression, "expression", json_object_new_string("call"));
  json_object_object_add(expression, "function", json_object_new_string("msgToState"));
  
  json_object *arguments = json_object_new_array();
  json_object_object_add(expression, "arguments", arguments);

  json_object *arg0 = json_object_new_object();
  json_object_array_add(arguments, arg0);

  json_object_object_add(arg0, "expression", json_object_new_string("bits"));
  json_object_object_add(arg0, "encoding", json_object_new_string("hex"));
  json_object_object_add(arg0, "data", json_object_new_string("0123456789abcdef0123456789abcdef"));
  json_object_object_add(arg0, "width", json_object_new_int(128));

  cryptol_service_send(cryserv, msg);
  
  json_object *json_result = cryptol_service_read(cryserv);
  json_object_put(json_result); //free result

  cryptol_service_disconnect(cryserv);
  
  return 0;
}
