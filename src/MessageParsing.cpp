#ifndef MESSAGEPARSING_CPP
#define MESSAGEPARSING_CPP

#include "headers/MessageParsing.hpp"
#include <stdexcept>
#include <iostream>
#include <vector>

namespace msg_parsing
{
  std::string::iterator extract_next_msg(std::string::iterator start, std::string::iterator end)
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

    throw std::runtime_error("Invalid RESP message passed into extract_next_msg");
  }

  std::vector<std::string> parse_resp_helper(std::string::iterator &current, std::string::iterator end)
  {
    switch (*current)
    {
    case '+':
    {
      std::string::iterator tmp = extract_next_msg(current, end);
      auto out = std::string(current + 1, tmp - 2);
      current = tmp;
      return {out};
    }
    case '-':
    {
      std::string::iterator tmp = extract_next_msg(current, end);
      auto out = std::string(current + 1, current - 2);
      throw std::runtime_error("Recieved Error from RESP message: " + out);
    }
    case '$':
    {
      std::string::iterator tmp = extract_next_msg(current, end);
      int length = std::stoi(std::string(current + 1, tmp - 2));
      current = tmp + length + 2;
      return {std::string(tmp, tmp + length)};
    }
    case '*':
    {
      std::string::iterator tmp = extract_next_msg(current, end);
      int length = std::stoi(std::string(current + 1, tmp - 2));
      current = tmp;

      std::vector<std::string> out;

      for (int i = 0; i < length && current < end; i++)
      {
        std::string next_val = parse_resp_helper(current, end)[0];
        out.push_back(next_val);
      }

      return out;
    }
    default:
      throw std::runtime_error("Invalid starting chacter for RESP message: " + *current);
    }
  }

  std::vector<std::string> parse_resp(std::string msg)
  {
    std::string::iterator tmp = msg.begin();
    return parse_resp_helper(tmp, msg.end());
  }
}

#endif