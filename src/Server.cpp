#ifndef SERVER_CPP
#define SERVER_CPP

#include <iostream>
#include <string>
#include <asio.hpp>
#include "headers/MessageParsing.hpp"
#include "headers/Debugging.hpp"
#include "headers/RedisTypes.hpp"

#define PORT 6379

using asio::ip::tcp;

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
  typedef std::shared_ptr<tcp_connection> pointer;

  // creates a shared pointer to a new connection object
  static pointer create(asio::io_context &io_context, int connection_id)
  {
    return pointer(new tcp_connection(io_context, connection_id));
  }

  tcp::socket &socket()
  {
    return socket_;
  }

  // start of the main loop
  // clear the message buffer and wait for a message
  void start()
  {
    message_.assign(buffer_size_, '\0');
    // shared_from_this() is used to share ownership of the object with the callback
    // object is removed from the heap with reference counting
    // handle_read callback is called when a message is received
    socket_.async_read_some(asio::buffer(message_), std::bind(&tcp_connection::handle_read, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
  }

private:
  tcp_connection(asio::io_context &io_context, int connection_id) : socket_(io_context), connection_id_(connection_id)
  {
    message_.resize(buffer_size_, '\0');
  }

  void handle_read(const std::error_code &error, size_t bytes_transferred)
  {
    // Check if connection has been closed
    if (error == asio::error::eof)
      return;
    else if (error)
    {
      // An error has occurred
      throw asio::system_error(error);
      return;
    }

    std::cout << "Connection " << connection_id_ << std::endl;
    std::cout << "Received: " << debugging::convert_to_raw_string(message_) << ", " << bytes_transferred << std::endl;
    if (bytes_transferred > 0 && message_ != "")
    {
      std::string response = msg_parsing::run_command(message_);
      asio::async_write(socket_, asio::buffer(response), std::bind(&tcp_connection::handle_write, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
      std::cout << "Sent: " << debugging::convert_to_raw_string(response) << "\n\n"
                << std::endl;
    }
    else
    {
      start();
    }
  }

  void handle_write(const std::error_code &error, size_t bytes_transferred)
  {
    // Check if connection has been closed
    if (error == asio::error::eof)
      return;
    else if (error)
    {
      // An error has occurred
      throw asio::system_error(error);
      return;
    }

    start();
  }

  tcp::socket socket_;
  std::string message_;
  int connection_id_;
  int buffer_size_ = 1024;
};

class tcp_server
{
public:
  tcp_server(asio::io_context &io_context) : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), PORT)), connection_id(0)
  {
    // starts here
    start_accept();
    std::cout << "Listening..." << std::endl;
  }

private:
  // creates a new connection object and waits for accept
  void start_accept()
  {
    tcp_connection::pointer new_connection = tcp_connection::create(io_context_, connection_id++);

    acceptor_.async_accept(new_connection->socket(), std::bind(&tcp_server::handle_accept, this, new_connection, asio::placeholders::error));
  }

  // called when a new connection is accepted
  // starts the main loop of the connection
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

inline int parse_args(int argc, char **argv)
{
  for (int i = 1; i < argc; i++)
  {
    std::string key(argv[i]);
    if (key.size() > 2 && key[0] == '-' && key[1] == '-')
    {
      std::string value(argv[++i]);

      redis::config[key.substr(2, key.size() - 2)] = redis::data(redis::BULK_STRING, value);
    }
    else
    {
      throw std::runtime_error("Invalid arguements.");
    }
  }

  return 0;
}

int main(int argc, char **argv)
{
  try
  {
    parse_args(argc, argv);

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

#endif