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
  
  send(cryservfd, "4:test,", strlen("4:test,"), 0);
  fprintf(stdout, "Message sent\n");
  
  char buffer[1024] = {0};
  int valread = read(cryservfd, buffer, 1024);
  fprintf(stderr, "%s\n",buffer);
  return 0;
}
