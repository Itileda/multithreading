#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

int set_nonblock(int fd) {
  int flags;
#if defined(O_NONBLOCK)
  if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
    flags = 0;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
  flags = 1;
  return ioctl(fd, FIOBIO, &flags);
#endif
};

int main() {
  int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  std::set<int> SlaveSockets;

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(12345);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (-1 == bind(sd, (sockaddr*)&sa, sizeof(sa))) {
    return 1;
  }

  set_nonblock(sd);

  if (-1 == listen(sd, SOMAXCONN)) {
    return 2;
  }
  int BUF_SIZE = 1024;
  char buf[BUF_SIZE];
  while (true) {
    //    int client_sd = accept(sd, 0, 0);
    //    if (client_sd < 0) {
    //      return 3;
    //    }
    int fail = 0;
    while (!fail) {
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(sd, &rfds);
      auto it = SlaveSockets.begin();
      for (; it != SlaveSockets.end(); it++) {
        FD_SET(*it, &rfds);
      }
      int max_desc = std::max(
          sd, *std::max_element(SlaveSockets.begin(), SlaveSockets.end()));
      select(max_desc + 1, &rfds, nullptr, nullptr, nullptr);
      for (auto sl_sock : SlaveSockets) {
        if (FD_ISSET(sl_sock, &rfds)) {
          char buffer_sl[BUF_SIZE];
          int recv_size = recv(sl_sock, buffer_sl, BUF_SIZE, MSG_NOSIGNAL);
          if (recv_size == 0 && errno != EAGAIN) {
            shutdown(sl_sock, SHUT_RDWR);
            close(sl_sock);
            SlaveSockets.erase(sl_sock);
          } else if (recv_size > 0) {
            buffer_sl[recv_size] = '\0';
            std::cout << "Client " << std::to_string(sl_sock) << buf
                      << std::endl;
            int total = recv_size, written = 0;
            while (written < total + 1) {
              std::cout << "Sending to client" << std::endl;
              int n =
                  send(sl_sock, buffer_sl + written, total + 1 - written, 0);
              if (n < 0) {
                std::cout << "Send error" << std::endl;
                fail = 1;
                break;
              }
              if (n == 0) {
                std::cout << "EOF received" << std::endl;
              }
              std::cout << "Sent " << n << " bytes" << std::endl;
              written += n;
            }
          }
        }
      }
      if (FD_ISSET(sd, &rfds)) {
        int sl_sock = accept(sd, 0, 0);
        set_nonblock(sl_sock);
        SlaveSockets.insert(sl_sock);
      }
    }
    std::cout << "Client disconnected" << std::endl;
  }
  close(sd);
  return 0;
}
