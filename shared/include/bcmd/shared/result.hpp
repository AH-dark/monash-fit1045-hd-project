#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace bcmd {

enum class Error : std::uint8_t {
    ChannelNotFound,
    ChannelAlreadyExists,
    ClientNotFound,
    ClientAlreadyExists,
    AlreadyMember,
    NotAMember,
    MessageEmpty,
    MessageInvalidPrefix,
    MessageTooLong,
    InvalidUsername,
    InvalidChannelName,
    StorageError,
    NetworkError,
    NotImplemented,
};

template <typename T>
using Result = std::expected<T, Error>;

using VoidResult = Result<void>;

std::string_view error_message(Error error) noexcept;

}  // namespace bcmd
