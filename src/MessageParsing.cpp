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

  inline std::string to_lowercase(std::string s)
  {
    std::string out;
    for (const char c : s)
      out.push_back(tolower(c));
    return out;
  }

  std::string process_input(std::string input)
  {
    // parse input
    const std::string delim = "\r\n";
    const std::string null_msg = "$-1\r\n";
    redis::data parsed_input = parse_resp(input);
    std::string command = to_lowercase(parsed_input[0].get_val());

    // execute commands
    if (command == "echo")
    {
      if (parsed_input.size() != 2)
        throw std::runtime_error("Invalid ECHO command: invalid number of arguments");

      std::string val = parsed_input[1].get_val();
      std::stringstream out;
      out << "$" << val.length() << delim << val << delim;
      return out.str();
    }
    else if (command == "set")
    {
      std::unordered_map<std::string, std::string> args = {
          {"px", "-1"}};

      if (parsed_input.size() < 3)
        throw std::runtime_error("Invalid SET command: invalid number of arguments.");
      std::string key = parsed_input[1].get_val();
      redis::data val = parsed_input[2];

      for (int i = 3; i < parsed_input.size(); i++)
      {
        if (to_lowercase(parsed_input[i].get_val()) == "px")
        {
          args["px"] = parsed_input[++i].get_val();
          std::cout << "px: " << parsed_input[++i].get_val() << std::endl;
        }
      }

      redis::db[key] = redis::data_store(val, std::stoi(args["px"]));

      return "+OK\r\n";
    }
    else if (command == "get")
    {
      if (parsed_input.size() != 2)
        throw std::runtime_error("Invalid GET command: invalid number of arguments");

      std::string key = parsed_input[1].get_val();
      std::unordered_map<std::string, redis::data_store>::iterator val = redis::db.find(key);

      if (val == redis::db.end())
        return null_msg;
      else if (val->second.is_item_expired())
      {
        redis::db.erase(val);
        return null_msg;
      }
      else
      {
        std::string str_val = val->second.get_value().get_val();
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