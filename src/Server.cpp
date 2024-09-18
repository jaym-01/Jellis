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

#define PORT 6379

using asio::ip::tcp;

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
  typedef std::shared_ptr<tcp_connection> pointer;

  static pointer create(asio::io_context &io_context, int connection_id)
  {
    return pointer(new tcp_connection(io_context, connection_id));
  }

  tcp::socket &socket()
  {
    return socket_;
  }

  void start()
  {
    memset(message_, 0, sizeof(message_));
    socket_.async_read_some(asio::buffer(message_), std::bind(&tcp_connection::handle_read, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
  }

private:
  tcp_connection(asio::io_context &io_context, int connection_id) : socket_(io_context), connection_id_(connection_id)
  {
    std::cout << "Connection started with " << connection_id << std::endl;
  }

  void handle_read(const std::error_code &error, size_t bytes_transferred)
  {
    std::cout << "Connection " << connection_id_ << std::endl;
    std::cout << "Received: " << message_ << ", " << bytes_transferred << std::endl;
    std::string response = "+PONG\r\n";

    if (error == asio::error::eof)
    {
      // Connection closed cleanly by the peer
      return;
    }
    else if (error)
    {
      // Some other error occurred
      throw asio::system_error(error);
      return;
    }

    asio::async_write(socket_, asio::buffer(response), std::bind(&tcp_connection::handle_write, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
    std::cout << "Sent: " << response << std::endl;
  }

  void handle_write(const std::error_code &error, size_t bytes_transferred)
  {
    if (error)
    {
      // Some other error occurred
      throw asio::system_error(error);
      return;
    }

    start();
  }

  tcp::socket socket_;
  char message_[1024];
  int connection_id_;
};

class tcp_server
{
public:
  tcp_server(asio::io_context &io_context) : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), PORT)), connection_id(0)
  {
    start_accept();
    std::cout << "Listening..." << std::endl;
  }

private:
  void start_accept()
  {
    tcp_connection::pointer new_connection = tcp_connection::create(io_context_, connection_id++);

    acceptor_.async_accept(new_connection->socket(), std::bind(&tcp_server::handle_accept, this, new_connection, asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection, const std::error_code &error)
  {
    if (!error)
    {
      new_connection->start();
    }

    start_accept();
  }

  asio::io_context &io_context_;
  tcp::acceptor acceptor_;
  int connection_id;
};

int main(int argc, char **argv)
{
  try
  {
    asio::io_context io_context;
    tcp_server server(io_context);
    io_context.run();
  }
  catch (std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
