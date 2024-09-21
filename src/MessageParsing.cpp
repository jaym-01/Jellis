#ifndef MESSAGEPARSING_CPP
#define MESSAGEPARSING_CPP

#include "headers/MessageParsing.hpp"
#include "headers/RedisTypes.hpp"
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
    const std::string delim = "\r\n";
    redis::data parsed_input = parse_resp(input);

    std::string command;

    for (const char c : parsed_input[0].get_val())
      command.push_back(tolower(c));

    if (command == "echo")
    {
      std::string val = parsed_input[1].get_val();
      std::stringstream out;
      out << "$" << val.length() << delim << val << delim;
      return out.str();
    }
    else if (command == "set")
    {
      std::string key = parsed_input[1].get_val();
      redis::data val = parsed_input[2];

      redis::db[key] = val;

      return "+OK\r\n";
    }
    else if (command == "get")
    {
      std::string key = parsed_input[1].get_val();
      std::unordered_map<std::string, redis::data>::iterator val = redis::db.find(key);

      if (val == redis::db.end())
        return "$-1\r\n";
      else
      {
        std::string str_val = val->second.get_val();
        std::stringstream out;
        out << "$" << str_val.length() << delim << str_val << delim;
        return out.str();
      }
    }
    else
    {
      return "+PONG\r\n";
    }
  }
}

#endif