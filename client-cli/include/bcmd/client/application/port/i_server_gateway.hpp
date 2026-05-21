#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "bcmd/client/domain/inbox_message.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::client::application::port {

struct ChannelInfo {
    std::string id;
    std::string name;
    std::int32_t member_count{0};
};

class IServerGateway {
public:
    using MessageCallback = std::function<void(domain::InboxMessage)>;

    virtual ~IServerGateway() = default;

    virtual bcmd::Result<std::string> connect(std::string_view username) = 0;
    virtual bcmd::VoidResult disconnect(std::string_view client_id) = 0;
    virtual bcmd::Result<std::vector<ChannelInfo>> listChannels() = 0;
    virtual bcmd::VoidResult joinChannel(std::string_view client_id,
                                         std::string_view channel_id) = 0;
    virtual bcmd::Result<std::string> joinChannelByName(std::string_view client_id,
                                                        std::string_view channel_name) = 0;
    virtual bcmd::VoidResult leaveChannel(std::string_view client_id,
                                          std::string_view channel_id) = 0;
    virtual bcmd::Result<std::string> sendMessage(std::string_view client_id,
                                                  std::string_view channel_id,
                                                  std::string_view content) = 0;
    virtual bcmd::VoidResult subscribeToChannel(std::string_view client_id,
                                                std::string_view channel_id,
                                                std::uint32_t replay_count,
                                                MessageCallback callback) = 0;

protected:
    IServerGateway() = default;
    IServerGateway(const IServerGateway&) = default;
    IServerGateway& operator=(const IServerGateway&) = default;
    IServerGateway(IServerGateway&&) = default;
    IServerGateway& operator=(IServerGateway&&) = default;
};

}  // namespace bcmd::client::application::port
