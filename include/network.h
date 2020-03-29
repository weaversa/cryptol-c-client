#ifndef CRYSRV_NETWORK_H
#define CRYSRV_NETWORK_H

#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <json-c/json.h>

int CryptolServiceConnect(char ip_address[16], uint32_t port);
json_object *CryptolServiceRead(int cryservfd);

#endif
