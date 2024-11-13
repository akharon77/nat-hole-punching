#include <sys/time.h>
#include <time.h>

#include "utils.h"

#define QUOTE(s) #s

char IsPrime(uint32_t num) {
  for (uint32_t div = 2; div * div <= num; ++div) {
    if (num % div == 0) {
      return 0;
    }
  }
  return 1;
}

uint32_t NextPrime(uint32_t num) {
  do {
    ++num;
  } while (!IsPrime(num));
  return num;
}

int EstablishConnection(size_t id, int sock, in_addr_t addr, const ClientPair* another_pair, struct sockaddr_in* another_client_addr) {
  int status = 0;
  int size = 0;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 10000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  struct sockaddr_in another_local_addr =
      GetSockaddr(another_pair->local_addr, another_pair->local_port);
  struct sockaddr_in another_global_addr =
      GetSockaddr(another_pair->global_addr, another_pair->global_port);

  socklen_t len = sizeof(struct sockaddr_in);

  size_t num = 0;
  printf("Try to connect... ");
  char* connection_type = NULL;
  while (1) {
    len = sizeof(struct sockaddr_in);
    num = MAGIC_GLOBAL;
    status = sendto(sock, (void*)&num, sizeof(num), 0,
                    (struct sockaddr*)&another_global_addr, len);
    size = recvfrom(sock, (void*)&num, sizeof(num), MSG_WAITALL,
                    (struct sockaddr*)another_client_addr, &len);
    if (size > 0 && num == MAGIC_GLOBAL) {
      connection_type = "GLOBAL";
      break;
    }

    if (another_local_addr.sin_addr.s_addr == addr) {
      continue;
    }
    num = MAGIC_LOCAL;
    status = sendto(sock, (void*)&num, sizeof(num), 0,
                    (struct sockaddr*)&another_local_addr, len);

    size = recvfrom(sock, (void*)&num, sizeof(num), MSG_WAITALL,
                    (struct sockaddr*)another_client_addr, &len);
    if (size > 0 && num == MAGIC_LOCAL) {
      connection_type = "LOCAL";
      break;
    }
  }
  status = sendto(sock, (void*)&num, sizeof(num), 0,
                  (struct sockaddr*)another_client_addr, len);
  printf("OK\n%s connection, the address of another client: ",
          connection_type);
  PrintAddress(&another_client_addr->sin_addr.s_addr,
                another_client_addr->sin_port);
  printf("\n");
}

int Communicate(size_t id, int sock, struct sockaddr_in* another_client_addr){
  int status = 0;
  int size = 0;

  // struct timeval tv;
  // tv.tv_sec = 0;
  // tv.tv_usec = 0;
  // setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  struct sockaddr_in buf_sock;
  socklen_t len = sizeof(struct sockaddr_in);

  uint32_t num = 2;
  while (num > 0) {
    if (id % 2 == 0) {
      printf("Send: %u\n", num);
      num = MAGIC_BASE | (num & MAGIC_MASK);
      status = sendto(sock, (void*)&num, sizeof(num), 0,
                      (struct sockaddr*)another_client_addr, len);

      size = recvfrom(sock, (void*)&num, sizeof(num), 0,
                      (struct sockaddr*)&buf_sock, &len);

      num = num & MAGIC_MASK;
      printf("Recv: %u\n", num);
      num = NextPrime(num);
    } else {
      size = recvfrom(sock, (void*)&num, sizeof(num), 0,
                      (struct sockaddr*)&buf_sock, &len);
      num = num & MAGIC_MASK;
      printf("Recv: %u\n", num);

      num = NextPrime(num);
      printf("Send: %u\n", num);
      num = MAGIC_BASE | (num & MAGIC_MASK);
      status = sendto(sock, (void*)&num, sizeof(num), 0,
                      (struct sockaddr*)another_client_addr, len);
    }
  }
}

int main(int argc, char* argv[]) {
  srand(time(NULL));

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
  struct sockaddr_in local_addr = GetSockaddr(pair.local_addr, pair.local_port);
  status = bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));

  struct sockaddr_in serv_sock_addr = GetSockaddr(serv_addr, serv_port);

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
  printf("OK\nTry to get id from server... ");

  size_t id = MAX_CLIENTS + 1;
  int size = recvfrom(sock, (void*)&pkt, sizeof(Pkt), 0,
                      (struct sockaddr*)&serv_sock_addr, &len);
  if (size < 0) {
    perror(NULL);
    return -1;
  }
  if (pkt.type != PKT_TYPE_RET_ID) {
    return -1;
  }
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

  struct sockaddr_in another_client_addr;
  printf("Try to connect to another client...\n");
  status = EstablishConnection(id, sock, pair.local_addr, &another_pair, &another_client_addr);
  status = Communicate(id, sock, &another_client_addr);

  close(sock);
  return 0;
}