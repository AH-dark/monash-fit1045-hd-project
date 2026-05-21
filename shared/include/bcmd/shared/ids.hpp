#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <uuid.h>

namespace bcmd {

// Strong-typed UUID wrapper. `Tag` is a phantom type that prevents implicit
// mixing of different ID kinds (e.g. passing a ClientId where a ChannelId is
// expected). The underlying value is a canonical RFC 4122 UUID string.
template <typename Tag>
class StrongId {
public:
    // Generates a new random (v4) UUID. Uses a thread-local Mersenne Twister
    // seeded once from std::random_device so callers do not pay seeding cost
    // per ID.
    static StrongId generate() {
        static thread_local std::mt19937 engine = [] {
            std::random_device random_dev;
            std::array<int, std::mt19937::state_size> seed_data{};
            std::generate(seed_data.begin(), seed_data.end(), std::ref(random_dev));
            std::seed_seq seq(seed_data.begin(), seed_data.end());
            return std::mt19937(seq);
        }();
        uuids::uuid_random_generator generator{engine};
        return StrongId{uuids::to_string(generator())};
    }

    // Parses a canonical UUID string. Returns std::nullopt if the input is not
    // a syntactically valid RFC 4122 UUID, or is the nil UUID.
    static std::optional<StrongId> parse(std::string_view str) {
        auto parsed_uuid = uuids::uuid::from_string(str);
        if (!parsed_uuid.has_value() || parsed_uuid->is_nil()) {
            return std::nullopt;
        }
        return StrongId{uuids::to_string(*parsed_uuid)};
    }

    const std::string& value() const noexcept { return value_; }
    std::string to_string() const { return value_; }

    bool operator==(const StrongId&) const = default;
    bool operator<(const StrongId& other) const noexcept { return value_ < other.value_; }

private:
    explicit StrongId(std::string raw) : value_(std::move(raw)) {}

    std::string value_{};
};

// Phantom tag types — empty by design; their sole purpose is to make
// StrongId<ChannelIdTag> and StrongId<ClientIdTag> distinct C++ types.
struct ChannelIdTag {};
struct ClientIdTag {};
struct MessageIdTag {};

using ChannelId = StrongId<ChannelIdTag>;
using ClientId = StrongId<ClientIdTag>;
using MessageId = StrongId<MessageIdTag>;

}  // namespace bcmd

// std::hash specialisation so StrongId can be used in unordered_map / set.
template <typename Tag>
struct std::hash<bcmd::StrongId<Tag>> {
    std::size_t operator()(const bcmd::StrongId<Tag>& identifier) const noexcept {
        return std::hash<std::string>{}(identifier.value());
    }
};
