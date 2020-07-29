#ifndef GAL_GAL_EXCEPTION_H_
#define GAL_GAL_EXCEPTION_H_

#include <exception>
#include <string>

namespace gal {

class Exception : public std::exception {
public:
  Exception(const std::string msg) : msg_(msg) {}
  const char* what() const final { return msg_.c_str(); }

private:
  std::string msg_;
};

} // namespace gal

#endif // GAL_GAL_EXCEPTION_H_