#ifndef MESSAGE_PARSING_HPP
#define MESSAGE_PARSING_HPP

#include <string>
#include <vector>
#include "RedisTypes.hpp"

namespace msg_parsing
{
  redis::data parse_resp(std::string msg);

  std::string process_input(std::string input);
}

#endif