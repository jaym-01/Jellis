#ifndef REDIS_TYPES_HPP
#define REDIS_TYPES_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace redis
{
  enum data_type
  {
    SIMPLE_STRING,
    SIMPLE_ERROR,
    INT,
    BULK_STRING,
    ARRAY_ELEMENT,
    NULL_TYPE,
  };

  class data
  {
  public:
    data(data_type type_data, std::string val) : value_({val}), type_(type_data) {}

    data(data_type type_data, std::vector<data> data) : type_(type_data), array_values_(data) {}

    data() : type_(NULL_TYPE) {};

    // used to get the value for a non array type
    const std::string get_val()
    {
      if (type_ == ARRAY_ELEMENT)
        throw std::runtime_error("Invalid Operation: tried to fetch a single value from an array.");

      return value_;
    }

    data &operator[](int i)
    {
      return array_values_[i];
    }

    data_type get_type() { return type_; }

    int size()
    {
      if (type_ == ARRAY_ELEMENT)
        return array_values_.size();
      else
        return 1;
    }

  private:
    std::vector<redis::data> array_values_;
    std::string value_;
    data_type type_;
  };

  extern std::unordered_map<std::string, data> db;
}

#endif