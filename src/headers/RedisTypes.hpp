#ifndef REDIS_TYPES_HPP
#define REDIS_TYPES_HPP

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <chrono>

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

    data(data_type type_data, const std::vector<data> &data) : type_(type_data), array_values_(data) {}

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
      if (type_ != ARRAY_ELEMENT)
        throw std::runtime_error("Invalid operation: used the '[]' operator on a non array type.");
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

    std::string resp_encode() const
    {
      std::string delim = "\r\n";
      std::stringstream out;
      switch (type_)
      {
      case (SIMPLE_STRING):
        out << "+" << value_ << delim;
        return out.str();
      case (SIMPLE_ERROR):
        out << "-" << value_ << delim;
        return out.str();
      case (INT):
        out << ":" << (std::stoi(value_) > 0 ? "+" : (std::stoi(value_) < 0 ? "-" : "")) << value_ << delim;
        return out.str();
      case (BULK_STRING):
        out << "$" << value_.size() << delim << value_ << delim;
        return out.str();
      case (NULL_TYPE):
        return "$-1\r\n";
      case (ARRAY_ELEMENT):
      {
        out << "*" << array_values_.size() << delim;
        for (const data element : array_values_)
        {
          out << element.resp_encode();
        }

        return out.str();
      }
      default:
        throw std::runtime_error("Invalid type to encode: " + type_);
      }
    }

  private:
    std::vector<redis::data> array_values_;
    std::string value_;
    data_type type_;
  };

  class data_store
  {
  public:
    data_store() {}

    data_store(data &value, int exp_in) : value_(value)
    {
      if (exp_in != -1)
        expiry_time_ = get_current_t_ms() + exp_in;
    }

    bool is_item_expired()
    {
      if (expiry_time_ == -1)
        return false;
      else
        return get_current_t_ms() > expiry_time_;
    }

    data get_data_stored()
    {
      if (is_item_expired())
        throw std::runtime_error("Did not check if the item has expired.");
      else
      {
        return value_;
      }
    }

  private:
    long long get_current_t_ms()
    {
      std::chrono::_V2::system_clock::time_point now = std::chrono::system_clock::now();
      return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }

    data value_;
    long long expiry_time_ = -1;
  };

  extern std::unordered_map<std::string, data_store>
      db;

  extern std::vector<data> config;
}

#endif