#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define QUOTE(s) #s

#define MAX_CLIENTS 2
#define BUF_SIZE 128

typedef enum _PktType {
  PKT_TYPE_NONE,
  PKT_TYPE_RET_ID,
  PKT_TYPE_RET_ANOTHER_CLIENT,
  PKT_TYPE_REG_CLIENT,
  PKT_TYPE_GET_ANOTHER_CLIENT
} PktType;

typedef struct _ClientPair {
  in_addr_t local_addr;
  in_port_t local_port;
  in_addr_t global_addr;
  in_port_t global_port;
} ClientPair;

typedef struct _Pkt {
  PktType type;
  union {
    ClientPair pair;
    size_t id;
  } data;
} Pkt;

int ProcessArgs(int argc, char* argv[], in_addr_t* servAddr,
                in_port_t* servPort);

struct sockaddr_in GetSockaddr(in_addr_t addr, in_port_t port);

void PrintAddress(const in_addr_t* addr, in_port_t port);
void PrintClientPair(const ClientPair* pair);
