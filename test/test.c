#include "cryptol-service.h"

int main(int argc, char const *argv[]) { 
  if(argc != 3) {
    fprintf(stderr,"usage %s hostname port\n", argv[0]);
    exit(0);
  }
  
  char ip_address[16];  
  strncpy(ip_address, argv[1], 16);
  uint32_t port = atoi(argv[2]);
  
  int cryservfd = CryptolServiceConnect(ip_address, port);
  if(cryservfd == -1) return 0;

  //Setup new session
  json_object *msg = json_object_new_object();
  json_object_object_add(msg, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(msg, "id", json_object_new_string("0"));
  
  json_object *params = json_object_new_object();
  json_object_object_add(msg, "params", params);
  json_object_object_add(params, "state", json_object_new_array());
  
  printf("msg=%s\n", json_object_get_string(msg));

  json_object_put(msg); //free msg and all referenced objects

  //Finish later
  
  char str[1024] = "{\"jsonrpc\":\"2.0\",\"method\":\"load module\",\"params\":{\"state\":[],\"module name\":\"Primitive::Symmetric::Cipher::Block::AES\"},\"id\":0}";
  char netstr[1024];

  sprintf(netstr, "%ld:%s,", strlen(str), str);
  send(cryservfd, netstr, strlen(netstr), 0);
  fprintf(stdout, "Message sent\n");

  json_object *json_result = CryptolServiceRead(cryservfd);
  printf("result=%s\n", json_object_get_string(json_result));
  json_object_put(json_result); //free result

  char straes[1024] = "{\"jsonrpc\":\"2.0\",\"method\":\"evaluate expression\",\"params\":{\"state\":[[\"load module\",{\"module name\":\"Primitive::Symmetric::Cipher::Block::AES\"}]],\"expression\":{\"expression\":\"call\",\"function\":\"msgToState\",\"arguments\":[{\"expression\":\"bits\",\"encoding\":\"hex\",\"data\":\"0123456789abcdef0123456789abcdef\",\"width\":128}]}},\"id\":1}";

  sprintf(netstr, "%ld:%s,", strlen(straes), straes);  
  send(cryservfd, netstr, strlen(netstr), 0);
  fprintf(stdout, "Message sent\n");

  json_result = CryptolServiceRead(cryservfd);
  printf("result=%s\n", json_object_get_string(json_result));
  json_object_put(json_result); //free result
  
  return 0;
}
