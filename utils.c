#include "utils.h"

int ProcessArgs(int argc, char* argv[], in_addr_t* serv_addr,
                in_port_t* serv_port) {
  int status = 0;

  if (argc < 3) {
    printf("Invalid arguments\n");
    return -1;
  }

  status = inet_pton(AF_INET, argv[1], serv_addr);
  if (status <= 0) {
    perror("Invalid server address");
    return -1;
  }

  *serv_port = atoi(argv[2]);
  *serv_port = htons(*serv_port);
  if (serv_port == 0) {
    printf("Invalid port\n");
    return -1;
  }

  return 0;
}

struct sockaddr_in GetSockaddr(in_addr_t addr, in_port_t port) {
  struct sockaddr_in sock_addr = {};
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = port;
  sock_addr.sin_addr.s_addr = addr;
  return sock_addr;
}

void PrintAddress(const in_addr_t* addr, in_port_t port) {
  char str[INET_ADDRSTRLEN] = "";
  inet_ntop(AF_INET, addr, str, sizeof(str));
  port = ntohs(port);
  printf("%s:%hu", str, port);
}

void PrintClientPair(const ClientPair* pair) {
  printf("Local: ");
  PrintAddress(&pair->local_addr, pair->local_port);
  printf("\nGlobal: ");
  PrintAddress(&pair->global_addr, pair->global_port);
  printf("\n");
}
