#pragma once

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/client/domain/inbox_message.hpp"
#include "bcmd/shared/result.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bcmd::tests {

// NOLINTBEGIN(misc-non-private-member-variables-in-classes,
// readability-redundant-member-init, bugprone-easily-swappable-parameters)

class FakeServerGateway final : public bcmd::client::application::port::IServerGateway {
public:
    using ChannelInfo = bcmd::client::application::port::ChannelInfo;
    using MessageCallback = bcmd::client::application::port::IServerGateway::MessageCallback;

    bcmd::Result<std::string> connect_result{std::string{"client-1"}};
    bcmd::VoidResult disconnect_result{};
    bcmd::Result<std::vector<ChannelInfo>> list_channels_result{std::vector<ChannelInfo>{}};
    bcmd::Result<std::string> create_channel_result{std::string{"channel-1"}};
    bcmd::VoidResult join_channel_result{};
    bcmd::Result<std::string> join_channel_by_name_result{std::string{"channel-1"}};
    bcmd::VoidResult leave_channel_result{};
    bcmd::Result<std::string> send_message_result{std::string{"message-1"}};
    bcmd::VoidResult subscribe_result{};

    std::vector<bcmd::client::domain::InboxMessage> subscription_messages;

    int connect_calls{0};
    int disconnect_calls{0};
    int list_channels_calls{0};
    int create_channel_calls{0};
    int join_channel_calls{0};
    int join_channel_by_name_calls{0};
    int leave_channel_calls{0};
    int send_message_calls{0};
    int subscribe_calls{0};

    std::string last_username;
    std::string last_client_id;
    std::string last_channel_id;
    std::string last_channel_name;
    std::string last_sent_content;
    std::uint32_t last_replay_count{0};

    bcmd::Result<std::string> connect(std::string_view username) override {
        ++connect_calls;
        last_username = std::string{username};
        return connect_result;
    }

    bcmd::VoidResult disconnect(std::string_view client_id) override {
        ++disconnect_calls;
        last_client_id = std::string{client_id};
        return disconnect_result;
    }

    bcmd::Result<std::vector<ChannelInfo>> listChannels() override {
        ++list_channels_calls;
        return list_channels_result;
    }

    bcmd::Result<std::string> createChannel(std::string_view client_id,
                                            std::string_view channel_name) override {
        ++create_channel_calls;
        last_client_id = std::string{client_id};
        last_channel_name = std::string{channel_name};
        return create_channel_result;
    }

    bcmd::VoidResult joinChannel(std::string_view client_id, std::string_view channel_id) override {
        ++join_channel_calls;
        last_client_id = std::string{client_id};
        last_channel_id = std::string{channel_id};
        return join_channel_result;
    }

    bcmd::Result<std::string> joinChannelByName(std::string_view client_id,
                                                std::string_view channel_name) override {
        ++join_channel_by_name_calls;
        last_client_id = std::string{client_id};
        last_channel_name = std::string{channel_name};
        return join_channel_by_name_result;
    }

    bcmd::VoidResult leaveChannel(std::string_view client_id,
                                  std::string_view channel_id) override {
        ++leave_channel_calls;
        last_client_id = std::string{client_id};
        last_channel_id = std::string{channel_id};
        return leave_channel_result;
    }

    bcmd::Result<std::string> sendMessage(std::string_view client_id, std::string_view channel_id,
                                          std::string_view content) override {
        ++send_message_calls;
        last_client_id = std::string{client_id};
        last_channel_id = std::string{channel_id};
        last_sent_content = std::string{content};
        return send_message_result;
    }

    bcmd::VoidResult subscribeToChannel(std::string_view client_id, std::string_view channel_id,
                                        std::uint32_t replay_count,
                                        MessageCallback callback) override {
        ++subscribe_calls;
        last_client_id = std::string{client_id};
        last_channel_id = std::string{channel_id};
        last_replay_count = replay_count;
        for (const auto& message : subscription_messages) {
            callback(message);
        }
        return subscribe_result;
    }
};

// NOLINTEND(misc-non-private-member-variables-in-classes,
// readability-redundant-member-init, bugprone-easily-swappable-parameters)

}  // namespace bcmd::tests
