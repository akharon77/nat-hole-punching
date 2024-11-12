#include "utils.h"

int AcceptClient(int sock, ClientPair* pairs, size_t* cnt_clients) {
  ClientPair* pair = pairs + *cnt_clients;
  int res = 0;

  struct sockaddr_in client_sock_addr;
  socklen_t len = sizeof(struct sockaddr_in);

  Pkt pkt;
  int size = recvfrom(sock, (void*)&pkt, sizeof(Pkt), MSG_WAITALL,
                      (struct sockaddr*)&client_sock_addr, &len);
  if (size > 0 && pkt.type != PKT_TYPE_REG_CLIENT) {
    pkt.type = PKT_TYPE_NONE;
    res = sendto(sock, (void*)&pkt, sizeof(Pkt), 0,
                 (struct sockaddr*)&client_sock_addr, len);
    return -1;
  }

  printf("Accept new client: %lu\n", *cnt_clients);

  *pair = pkt.data.pair;

  pair->global_addr = client_sock_addr.sin_addr.s_addr;
  pair->global_port = client_sock_addr.sin_port;

  PrintClientPair(pair);
  printf("\n");

  pkt.type = PKT_TYPE_RET_ID;
  pkt.data.id = *cnt_clients;
  ++(*cnt_clients);
  res = sendto(sock, (void*)&pkt, sizeof(Pkt), 0,
               (struct sockaddr*)&client_sock_addr, len);

  return 0;
}

int Communicate(int sock, ClientPair* clients) {
  int res = 0;
  struct sockaddr_in client_sock_addr;
  ClientPair pair;
  Pkt pkt;
  socklen_t len = sizeof(struct sockaddr_in);

  while (1) {
    size_t id = MAX_CLIENTS + 1;
    int size = recvfrom(sock, (void*)&pkt, sizeof(Pkt), MSG_WAITALL,
                        (struct sockaddr*)&client_sock_addr, &len);
    if (size > 0 && pkt.type != PKT_TYPE_GET_ANOTHER_CLIENT) {
      pkt.type = PKT_TYPE_NONE;
      res = sendto(sock, (void*)&pkt, sizeof(Pkt), 0,
                   (struct sockaddr*)&client_sock_addr, len);
      continue;
    }

    id = pkt.data.id;
    id = (id + 1) % MAX_CLIENTS;

    pkt.type = PKT_TYPE_RET_ANOTHER_CLIENT;
    pkt.data.pair = clients[id];
    res = sendto(sock, (void*)&pkt, sizeof(Pkt), 0,
                 (struct sockaddr*)&client_sock_addr, len);
  }

  return 0;
}

int main(int argc, char* argv[]) {
  in_addr_t serv_addr;
  in_port_t serv_port;

  int status = 0;

  status = ProcessArgs(argc, argv, &serv_addr, &serv_port);
  if (status < 0) {
    return -1;
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in serv_sock_addr = {};
  serv_sock_addr.sin_family = AF_INET;
  serv_sock_addr.sin_port = serv_port;
  serv_sock_addr.sin_addr.s_addr = serv_addr;

  status =
      bind(sock, (struct sockaddr*)&serv_sock_addr, sizeof(serv_sock_addr));

  if (status < 0) {
    perror(NULL);
    return -1;
  }

  size_t cnt_clients = 0;
  ClientPair clients[MAX_CLIENTS];

  struct sockaddr_in client_sock_addr = {.sin_addr = 2};
  socklen_t len = 0;

  while (cnt_clients < MAX_CLIENTS) {
    status = AcceptClient(sock, clients, &cnt_clients);
  }

  status = Communicate(sock, clients);

  close(sock);

  return 0;
}
