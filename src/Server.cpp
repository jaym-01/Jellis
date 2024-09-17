#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <asio.hpp>

using asio::ip::tcp;

int main(int argc, char **argv)
{
  try
  {
    asio::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 6379));

    tcp::socket socket(io_context);
    acceptor.accept(socket);

    char res[] = "+PONG\r\n";
    char req[1024];
    asio::error_code error;

    while (true)
    {
      memset(req, 0, sizeof(req));
      size_t length = socket.read_some(asio::buffer(req), error);

      if (error == asio::error::eof)
      {
        // Connection closed cleanly by the peer
        return -1;
      }
      else if (error)
      {
        // Some other error occurred
        throw asio::system_error(error);
        return -1;
      }

      // Echo the received data back to the client
      asio::write(socket, asio::buffer(res, sizeof(res) - 1));
    }
  }
  catch (std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
