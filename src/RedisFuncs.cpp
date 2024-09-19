#ifndef REDIS_FUNCS_CPP
#define REDIS_FUNCS_CPP

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include "headers/MessageParsing.hpp"

namespace redis
{
  std::string process_input(std::string input)
  {
    try
    {
      std::vector<std::string> inputs = msg_parsing::parse_resp(input);

      if (inputs.size() < 1)
        throw std::runtime_error("invalid number of arguements from the user");

      std::string command;

      for (const char c : inputs[0])
        command.push_back(tolower(c));

      if (command == "echo")
      {
        std::string val = inputs[1];
        std::string delim = "\r\n";
        std::stringstream out;
        out << "$" << val.length() << delim << val << delim;
        return out.str();
      }
      else
      {
        return "+PONG\r\n";
      }
    }
    catch (...)
    {
      return "";
    }
  }
}

#endif