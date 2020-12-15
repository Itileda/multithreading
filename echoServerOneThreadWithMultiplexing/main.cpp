#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef USE_POLL
#include <poll.h>
#endif

#define POLL_SIZE 2048

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
  sa.sin_port = htons(12346);
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

#ifdef USE_POLL
  struct pollfd pollfd_arr[POLL_SIZE];
  pollfd_arr[0].fd = sd;
  pollfd_arr[0].events = POLLIN;
#endif
  while (true) {
#ifdef USE_POLL
    unsigned int index = 1;
    for (auto sl_sock : SlaveSockets) {
      pollfd_arr[index].fd = sl_sock;
      pollfd_arr[index].events = POLLIN;
      index++;
    }
    unsigned int SetSize = 1 + SlaveSockets.size();
    poll(pollfd_arr, SetSize, -1);
    for (uint i = 0; i < SetSize; i++) {
      if (pollfd_arr[i].revents & POLLIN) {
        if (i) {
          static char buffer[1024];
          int recv_size = recv(pollfd_arr[i].fd, buffer, 1024, MSG_NOSIGNAL);
          if (recv_size == 0 && errno != EAGAIN) {
            shutdown(pollfd_arr[i].fd, SHUT_RDWR);
            close(pollfd_arr[i].fd);
            SlaveSockets.erase(pollfd_arr[i].fd);
          } else if (recv_size > 0) {
            send(pollfd_arr[i].fd, buffer, recv_size, MSG_NOSIGNAL);
          }
        } else {
          int slave_sock = accept(sd, 0, 0);
          set_nonblock(slave_sock);
          SlaveSockets.insert(slave_sock);
        }
      }
    }
#endif
    int fail = 0;
    while (!fail) {
#ifdef USE_SELECT
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
#endif
    }
    std::cout << "Client disconnected" << std::endl;
  }
  close(sd);
  return 0;
}
