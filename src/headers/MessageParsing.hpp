#ifndef MESSAGEPARSING_HPP
#define MESSAGEPARSING_HPP

#include <string>
#include <vector>
#include "RedisFuncs.hpp"

namespace msg_parsing
{
  redis::data parse_resp(std::string msg);

  std::string process_input(std::string input);
}

#endif