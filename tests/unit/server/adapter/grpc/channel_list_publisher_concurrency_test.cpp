#include "bcmd/server/adapter/grpc/channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/impl/call_op_set.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/sync_stream.h>

#include "../../../../fakes/fake_channel_repository.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace {

class RecordingChannelListWriter final
    : public ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent> {
public:
    void SendInitialMetadata() override {}

    bool Write(const bcmd::v1::ChannelListEvent& msg, ::grpc::WriteOptions /*options*/) override {
        events_.push_back(msg);
        return true;
    }

    [[nodiscard]] const std::vector<bcmd::v1::ChannelListEvent>& events() const { return events_; }

private:
    std::vector<bcmd::v1::ChannelListEvent> events_;
};

bcmd::server::domain::ChannelName makeChannelName(const char* name) {
    auto channel_name = bcmd::server::domain::ChannelName::create(name);
    REQUIRE(channel_name.has_value());
    return *channel_name;
}

}  // namespace

TEST_CASE(
    "channel_list_publisher_concurrency_test registerSubscriberWithSnapshot sends snapshot before "
    "returning",
    "[adapter][grpc][channel-list-publisher]") {
    namespace adapter = bcmd::server::adapter::grpc;

    auto channels = std::make_shared<bcmd::tests::FakeChannelRepository>();
    std::shared_ptr<bcmd::server::application::port::IChannelRepository> repository = channels;
    REQUIRE(channels->create(makeChannelName("general")).has_value());
    REQUIRE(channels->create(makeChannelName("random")).has_value());

    adapter::GrpcChannelListPublisher publisher{repository};
    RecordingChannelListWriter writer;

    publisher.registerSubscriberWithSnapshot(bcmd::ClientId::generate(), &writer);

    REQUIRE(writer.events().size() == 1);
    const auto& event = writer.events().front();
    REQUIRE(event.has_snapshot());
    CHECK(event.snapshot().channels_size() == 2);
}

TEST_CASE(
    "channel_list_publisher_concurrency_test publishChannelCreated is delivered to all subscribers",
    "[adapter][grpc][channel-list-publisher]") {
    namespace adapter = bcmd::server::adapter::grpc;

    auto channels = std::make_shared<bcmd::tests::FakeChannelRepository>();
    std::shared_ptr<bcmd::server::application::port::IChannelRepository> repository = channels;
    adapter::GrpcChannelListPublisher publisher{repository};
    RecordingChannelListWriter first_writer;
    RecordingChannelListWriter second_writer;
    publisher.registerSubscriberWithSnapshot(bcmd::ClientId::generate(), &first_writer);
    publisher.registerSubscriberWithSnapshot(bcmd::ClientId::generate(), &second_writer);

    auto channel = channels->create(makeChannelName("general"));
    REQUIRE(channel.has_value());
    publisher.publishChannelCreated(*channel);

    REQUIRE(first_writer.events().size() == 2);
    REQUIRE(second_writer.events().size() == 2);
    CHECK(first_writer.events().back().has_created());
    CHECK(second_writer.events().back().has_created());
    CHECK(first_writer.events().back().created().channel().id() == channel->id().to_string());
    CHECK(second_writer.events().back().created().channel().name() == "general");
}

TEST_CASE("channel_list_publisher_concurrency_test publishMemberCountChanged is delivered",
          "[adapter][grpc][channel-list-publisher]") {
    namespace adapter = bcmd::server::adapter::grpc;

    auto channels = std::make_shared<bcmd::tests::FakeChannelRepository>();
    std::shared_ptr<bcmd::server::application::port::IChannelRepository> repository = channels;
    adapter::GrpcChannelListPublisher publisher{repository};
    RecordingChannelListWriter writer;
    publisher.registerSubscriberWithSnapshot(bcmd::ClientId::generate(), &writer);

    const auto channel_id = bcmd::ChannelId::generate();
    publisher.publishMemberCountChanged(channel_id, 3);

    REQUIRE(writer.events().size() == 2);
    const auto& event = writer.events().back();
    REQUIRE(event.has_member_count_changed());
    CHECK(event.member_count_changed().channel_id() == channel_id.to_string());
    CHECK(event.member_count_changed().member_count() == 3);
}

TEST_CASE("channel_list_publisher_concurrency_test unregisterSubscriber stops delivery",
          "[adapter][grpc][channel-list-publisher]") {
    namespace adapter = bcmd::server::adapter::grpc;

    auto channels = std::make_shared<bcmd::tests::FakeChannelRepository>();
    std::shared_ptr<bcmd::server::application::port::IChannelRepository> repository = channels;
    adapter::GrpcChannelListPublisher publisher{repository};
    RecordingChannelListWriter writer;
    const auto subscriber_id = bcmd::ClientId::generate();
    publisher.registerSubscriberWithSnapshot(subscriber_id, &writer);
    REQUIRE(writer.events().size() == 1);

    publisher.unregisterSubscriber(subscriber_id);
    auto channel = channels->create(makeChannelName("general"));
    REQUIRE(channel.has_value());
    publisher.publishChannelCreated(*channel);

    CHECK(writer.events().size() == 1);
}
