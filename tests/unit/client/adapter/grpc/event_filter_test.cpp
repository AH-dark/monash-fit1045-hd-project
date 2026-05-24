#include "bcmd/client/adapter/grpc/detail/event_filter.hpp"

#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>

namespace {

bcmd::v1::ChannelEvent makeMessageEvent(const char* channel_id) {
    bcmd::v1::ChannelEvent event;
    auto* message = event.mutable_message();
    message->set_channel_id(channel_id);
    message->set_message_id("msg-1");
    message->set_sender_id("sender-1");
    message->set_content("hello");
    return event;
}

bcmd::v1::ChannelEvent makeMemberLeftEvent(const char* channel_id) {
    bcmd::v1::ChannelEvent event;
    auto* left = event.mutable_member_left();
    left->set_channel_id(channel_id);
    left->set_client_id("client-1");
    left->set_username("alice");
    return event;
}

bcmd::v1::ChannelEvent makeMemberJoinedEvent(const char* channel_id) {
    bcmd::v1::ChannelEvent event;
    auto* joined = event.mutable_member_joined();
    joined->set_channel_id(channel_id);
    joined->set_client_id("client-1");
    joined->set_username("alice");
    return event;
}

bcmd::v1::ChannelEvent makeReplayCompleteEvent(const char* channel_id) {
    bcmd::v1::ChannelEvent event;
    auto* complete = event.mutable_replay_complete();
    complete->set_channel_id(channel_id);
    complete->set_replayed_count(0);
    return event;
}

}  // namespace

TEST_CASE("shouldEmitEventForChannel allows MemberLeft only for the subscribed channel",
          "[adapter][grpc][event-filter][member-left]") {
    namespace detail = bcmd::client::adapter::grpc::detail;

    CHECK(detail::shouldEmitEventForChannel(makeMemberLeftEvent("chan-a"), "chan-a"));
    CHECK_FALSE(detail::shouldEmitEventForChannel(makeMemberLeftEvent("chan-b"), "chan-a"));
    CHECK_FALSE(detail::shouldEmitEventForChannel(makeMemberLeftEvent(""), "chan-a"));
}

TEST_CASE("shouldEmitEventForChannel allows MemberJoined only for the subscribed channel",
          "[adapter][grpc][event-filter][member-joined]") {
    namespace detail = bcmd::client::adapter::grpc::detail;

    CHECK(detail::shouldEmitEventForChannel(makeMemberJoinedEvent("chan-a"), "chan-a"));
    CHECK_FALSE(detail::shouldEmitEventForChannel(makeMemberJoinedEvent("chan-b"), "chan-a"));
}

TEST_CASE("shouldEmitEventForChannel always allows non-membership events through",
          "[adapter][grpc][event-filter][passthrough]") {
    namespace detail = bcmd::client::adapter::grpc::detail;

    CHECK(detail::shouldEmitEventForChannel(makeMessageEvent("chan-a"), "chan-a"));
    CHECK(detail::shouldEmitEventForChannel(makeMessageEvent("chan-b"), "chan-a"));

    CHECK(detail::shouldEmitEventForChannel(makeReplayCompleteEvent("chan-a"), "chan-a"));
    CHECK(detail::shouldEmitEventForChannel(makeReplayCompleteEvent("chan-b"), "chan-a"));
}
