#include "cryptol-service.h"

int CryptolServiceConnect(char ip_address[16], uint32_t port) {
  int cryservfd = 0;
  struct sockaddr_in cryserv_addr;
  //Create socket to Cryptol service
  if ((cryservfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "Socket creation to Cryptol service failed.\n");
    return -1;
  }
  
  cryserv_addr.sin_family = PF_INET;
  cryserv_addr.sin_port = htons(port);
  
  //Fill cryserv_addr.sin_addr with the IP address of the Cryptol service
  if(inet_pton(PF_INET, ip_address, &cryserv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Can't parse IP address (%s) of Cryptol service\n", ip_address);
    return -1;
  }

  //Attempt to connect to the Cryptol service
  if(connect(cryservfd, (struct sockaddr *)&cryserv_addr, sizeof(cryserv_addr)) < 0) {
    printf("Error: Connection to Cryptol service failed\n");
    return -1;
  }
  
  return cryservfd;
}
