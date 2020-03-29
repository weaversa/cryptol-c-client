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

  char str[1024] = "{\"jsonrpc\":\"2.0\",\"method\":\"load module\",\"params\":{\"state\":[],\"module name\":\"Primitive::Symmetric::Cipher::Block::AES\"},\"id\":0}";
  char netstr[1024];

  sprintf(netstr, "%ld:%s,", strlen(str), str);
  send(cryservfd, netstr, strlen(netstr), 0);
  fprintf(stdout, "Message sent\n");
  char buffer[1024] = {0};
  int valread = read(cryservfd, buffer, 1024);
  fprintf(stderr, "%s\n",buffer);

  char straes[1024] = "{\"jsonrpc\":\"2.0\",\"method\":\"evaluate expression\",\"params\":{\"state\":[[\"load module\",{\"module name\":\"Primitive::Symmetric::Cipher::Block::AES\"}]],\"expression\":{\"expression\":\"call\",\"function\":\"msgToState\",\"arguments\":[{\"expression\":\"bits\",\"encoding\":\"hex\",\"data\":\"0123456789abcdef0123456789abcdef\",\"width\":128}]}},\"id\":1}";

  sprintf(netstr, "%ld:%s,", strlen(straes), straes);  
  send(cryservfd, netstr, strlen(netstr), 0);
  fprintf(stdout, "Message sent\n");
  valread = read(cryservfd, buffer, 1024);
  fprintf(stderr, "%s\n",buffer);
  
  return 0;
}
