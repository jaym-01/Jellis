#ifndef REDIS_TYPES_CPP
#define REDIS_TYPES_CPP

#include "headers/RedisTypes.hpp"

namespace redis
{
  std::unordered_map<std::string, redis::data_store> db;
  std::unordered_map<std::string, redis::data> config;
}

#endif
