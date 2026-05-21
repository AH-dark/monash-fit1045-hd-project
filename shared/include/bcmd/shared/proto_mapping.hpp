#pragma once

// Forward declarations of domain <-> proto translators.
//
// Kept declaration-only so the shared library does not pull protobuf-generated
// headers into its public interface. Concrete signatures and the matching .cpp
// will land alongside the gRPC adapter implementation.

namespace bcmd::v1 {
class ConnectRequest;
class ChannelSummary;
}  // namespace bcmd::v1

namespace bcmd {
template <typename Tag>
class StrongId;
struct ChannelIdTag;
struct ClientIdTag;
struct MessageIdTag;
}  // namespace bcmd

namespace bcmd::mapping {

// Future translators will be declared here, e.g.:
//   ChannelId   channel_id_from_proto(const std::string& proto_id);
//   std::string channel_id_to_proto(const ChannelId& id);

}  // namespace bcmd::mapping
