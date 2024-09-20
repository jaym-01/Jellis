#ifndef MESSAGEPARSING_CPP
#define MESSAGEPARSING_CPP

#include "headers/MessageParsing.hpp"
#include "headers/RedisFuncs.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <vector>

namespace msg_parsing
{
  // returns the iterator to the character after the CRLF
  // CLRF = CR(\r) & LF(\n)
  std::string::iterator find_end_of_next_crlf(std::string::iterator start, const std::string::iterator end)
  {
    auto current = start;
    while (start < end - 1)
    {
      if (*current == '\r' && *(current + 1) == '\n')
      {
        return current + 2;
      }
      current++;
    }

    throw std::runtime_error("Invalid RESP message passed into find_end_of_next_crlf");
  }

  redis::data parse_resp_helper(std::string::iterator &current, const std::string::iterator end)
  {
    switch (*current)
    {
    case '+':
    {
      std::string::iterator cur_end = find_end_of_next_crlf(current, end);
      auto out = std::string(current + 1, cur_end - 2);
      current = cur_end;
      return redis::data(redis::SIMPLE_STRING, out);
    }
    case '-':
    {
      std::string::iterator cur_end = find_end_of_next_crlf(current, end);
      throw std::runtime_error("Recieved Error from RESP message: " + std::string(current + 1, cur_end - 2));
    }
    case '$':
    {
      std::string::iterator cur_end = find_end_of_next_crlf(current, end);
      int length = std::stoi(std::string(current + 1, cur_end - 2));
      if (length == -1)
      {
        current = cur_end;
        return redis::data();
      }
      else
      {
        current = cur_end + length + 2;
        return redis::data(redis::BULK_STRING, std::string(cur_end, cur_end + length));
      }
    }
    case '*':
    {
      std::string::iterator tmp = find_end_of_next_crlf(current, end);
      int length = std::stoi(std::string(current + 1, tmp - 2));
      current = tmp;
      if (length == -1)
        return redis::data();

      std::vector<redis::data> list;

      for (int i = 0; i < length && current < end; i++)
      {
        list.push_back(parse_resp_helper(current, end));
      }

      return redis::data(redis::ARRAY_ELEMENT, list);
    }
    default:
      throw std::runtime_error("Invalid starting chacter for RESP message: " + *current);
    }
  }

  redis::data parse_resp(std::string msg)
  {
    std::string::iterator cur = msg.begin();
    return parse_resp_helper(cur, msg.end());
  }

  std::string process_input(std::string input)
  {
    redis::data parsed_input = parse_resp(input);

    std::string command;

    for (const char c : parsed_input.get_val(0))
      command.push_back(tolower(c));

    if (command == "echo")
    {
      std::string val = parsed_input.get_val(1);
      std::string delim = "\r\n";
      std::stringstream out;
      out << "$" << val.length() << delim << val << delim;
      return out.str();
    }
    else if (command == "set")
    {
      return "";
    }
    else
    {
      return "+PONG\r\n";
    }
  }
}

#endif