/* Sample UDP server */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char**argv) {
  int sockfd, n;
  struct sockaddr_in servaddr, cliaddr = { 0 };
  socklen_t len;
  char msg[2048] = { 0 };

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(32000);
  bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  for (;;) {
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, msg, sizeof(msg), 0, (struct sockaddr *) &cliaddr,
                 &len);
    printf("server recv msg: %s(%d)\n", msg, n);

    sleep(1);  // intended

    n = sendto(sockfd, msg, n, 0, (struct sockaddr *) &cliaddr, len);
    printf("response to client: ret = %d\n", n);
  }
}
