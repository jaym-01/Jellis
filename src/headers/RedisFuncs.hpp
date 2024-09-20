#ifndef REDIS_FUNCS_HPP
#define REDIS_FUNCS_HPP

#include <string>
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

    std::string get_val()
    {
      if (type_ == ARRAY_ELEMENT)
        throw std::runtime_error("Invalid Operation: tried to fetch a single value from an array.");

      return value_;
    }

    // TODO: make this recursively the the values of arguements
    std::string get_val(int index)
    {
      if (type_ != ARRAY_ELEMENT)
        throw std::runtime_error("Invalid Operation: tried to fetch value at index on a type that is not an array.");
      else if (index < -1 && index >= array_values_.size())
      {
        throw std::runtime_error("Invalid Arguement: index provided for the array is out of bounds");
      }
      return array_values_[index].get_val();
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
    std::vector<data> array_values_;
    std::string value_;
    data_type type_;
  };

  // std::unordered_map<std::string, data> db;
}

#endif