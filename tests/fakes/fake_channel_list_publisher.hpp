#pragma once

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/shared/ids.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bcmd::tests {

class FakeChannelListPublisher final
    : public bcmd::server::application::port::IChannelListPublisher {
public:
    struct CreatedRecord {
        bcmd::server::domain::Channel channel;
    };

    struct MemberCountRecord {
        bcmd::ChannelId channel_id;
        std::int32_t member_count;
    };

    struct RegisterRecord {
        bcmd::ClientId subscriber_id;
        ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer;
    };

    void registerSubscriberWithSnapshot(
        const bcmd::ClientId& subscriber_id,
        ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer) override {
        register_calls_.push_back(RegisterRecord{.subscriber_id = subscriber_id, .writer = writer});
    }

    void unregisterSubscriber(const bcmd::ClientId& subscriber_id) override {
        unregister_calls_.push_back(subscriber_id);
    }

    void publishChannelCreated(const bcmd::server::domain::Channel& channel) override {
        created_calls_.push_back(CreatedRecord{.channel = channel});
    }

    void publishMemberCountChanged(const bcmd::ChannelId& channel_id,
                                   std::int32_t member_count) override {
        member_count_calls_.push_back(
            MemberCountRecord{.channel_id = channel_id, .member_count = member_count});
    }

    [[nodiscard]] std::size_t publishChannelCreatedCallCount() const {
        return created_calls_.size();
    }

    [[nodiscard]] std::size_t publishMemberCountChangedCallCount() const {
        return member_count_calls_.size();
    }

    [[nodiscard]] std::size_t registerSubscriberWithSnapshotCallCount() const {
        return register_calls_.size();
    }

    [[nodiscard]] std::size_t unregisterSubscriberCallCount() const {
        return unregister_calls_.size();
    }

    [[nodiscard]] const std::vector<CreatedRecord>& createdCalls() const { return created_calls_; }

    [[nodiscard]] const std::vector<MemberCountRecord>& memberCountCalls() const {
        return member_count_calls_;
    }

    [[nodiscard]] const std::vector<RegisterRecord>& registerCalls() const {
        return register_calls_;
    }

    [[nodiscard]] const std::vector<bcmd::ClientId>& unregisterCalls() const {
        return unregister_calls_;
    }

private:
    std::vector<CreatedRecord> created_calls_;
    std::vector<MemberCountRecord> member_count_calls_;
    std::vector<RegisterRecord> register_calls_;
    std::vector<bcmd::ClientId> unregister_calls_;
};

}  // namespace bcmd::tests
