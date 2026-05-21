#pragma once

#include <stdexcept>
#include <string>

namespace bcmd::server::domain {

// Domain layer prefers `bcmd::Result<T>` for expected errors. These exceptions
// are reserved for programmer-error or truly exceptional situations.
class DomainException : public std::runtime_error {
public:
    explicit DomainException(const std::string& message) : std::runtime_error(message) {}
};

class ValidationException : public DomainException {
public:
    explicit ValidationException(const std::string& message) : DomainException(message) {}
};

}  // namespace bcmd::server::domain
