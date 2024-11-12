#include <sys/time.h>

#include "utils.h"

#define QUOTE(s) #s

int Communicate(size_t id, int sock, const ClientPair* another_pair) {
  int status = 0;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 10000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  int size = 0;

  struct sockaddr_in another_local_addr =
      GetSockaddr(another_pair->local_addr, another_pair->local_port);
  struct sockaddr_in another_global_addr =
      GetSockaddr(another_pair->global_addr, another_pair->global_port);

  struct sockaddr_in another_client_sock_addr;
  socklen_t len = sizeof(struct sockaddr_in);

  size_t num = 42;
  printf("Try to connect... ");
  while (1) {
    len = sizeof(struct sockaddr_in);
    status = sendto(sock, (void*)&num, sizeof(num), 0,
                    (struct sockaddr*)&another_global_addr, len);
    status = sendto(sock, (void*)&num, sizeof(num), 0,
                    (struct sockaddr*)&another_local_addr, len);

    size = recvfrom(sock, (void*)&num, sizeof(num), MSG_WAITALL,
                    (struct sockaddr*)&another_client_sock_addr, &len);
    if (size > 0 && num == 42) {
      printf("OK: GLOBAL connection\n");
      PrintAddress(&another_client_sock_addr.sin_addr.s_addr,
                   another_client_sock_addr.sin_port);
      break;
    }

    size = recvfrom(sock, (void*)&num, sizeof(num), MSG_WAITALL,
                    (struct sockaddr*)&another_client_sock_addr, &len);
    if (size > 0 && num == 42) {
      printf("OK: LOCAL connection\n");
      PrintAddress(&another_client_sock_addr.sin_addr.s_addr,
                   another_client_sock_addr.sin_port);
      break;
    }
  }

  status = sendto(sock, (void*)&num, sizeof(num), 0,
                  (struct sockaddr*)&another_client_sock_addr, len);
}

int main(int argc, char* argv[]) {
  in_addr_t serv_addr;
  in_port_t serv_port;

  int status = 0;

  status = ProcessArgs(argc, argv, &serv_addr, &serv_port);
  if (status < 0) {
    return -1;
  }

  ClientPair pair;
  status = ProcessArgs(argc - 2, argv + 2, &pair.local_addr, &pair.local_port);
  if (status < 0) {
    return -1;
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in serv_sock_addr = {};
  serv_sock_addr.sin_family = AF_INET;
  serv_sock_addr.sin_port = serv_port;
  serv_sock_addr.sin_addr.s_addr = serv_addr;

  socklen_t len = sizeof(struct sockaddr_in);
  printf("Try to register at rendezvous server... ");
  Pkt pkt;
  pkt.type = PKT_TYPE_REG_CLIENT;
  pkt.data.pair = pair;
  status = sendto(sock, (void*)&pkt, sizeof(Pkt), 0,
                  (struct sockaddr*)&serv_sock_addr, len);
  if (status < 0) {
    perror(NULL);
    return -1;
  }
  printf("OK\n");
  printf("Try to get id from server... ");

  size_t id = MAX_CLIENTS + 1;
  int size = recvfrom(sock, (void*)&pkt, sizeof(Pkt), 0,
                      (struct sockaddr*)&serv_sock_addr, &len);
  if (size < 0) {
    perror(NULL);
    return -1;
  }
  assert(pkt.type == PKT_TYPE_RET_ID);
  id = pkt.data.id;

  printf("OK, id: %lu\n", id);

  ClientPair another_pair;

  printf("Try to get another client address... ");
  while (1) {
    pkt.type = PKT_TYPE_GET_ANOTHER_CLIENT;
    status = sendto(sock, (void*)&pkt, sizeof(Pkt), 0,
                    (struct sockaddr*)&serv_sock_addr, len);
    size = recvfrom(sock, (void*)&pkt, sizeof(Pkt), MSG_WAITALL,
                    (struct sockaddr*)&serv_sock_addr, &len);
    if (pkt.type != PKT_TYPE_RET_ANOTHER_CLIENT) {
      continue;
    }
    another_pair = pkt.data.pair;
    break;
  }
  printf("OK, another client:\n");
  PrintClientPair(&another_pair);

  printf("Start data exchange\n");
  status = Communicate(id, sock, &another_pair);
  close(sock);
  return 0;
}