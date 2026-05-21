#pragma once

#include "bcmd/client/domain/inbox_message.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace bcmd::client::application::port {

class IPresenter {
public:
    virtual ~IPresenter() = default;

    virtual void showMessage(domain::InboxMessage message) = 0;
    virtual void showReplayComplete(std::string_view channel_id) = 0;
    virtual void showError(std::string_view error_text) = 0;
    virtual void showInfo(std::string_view info_text) = 0;
    virtual void updateConnectionStatus(bool connected, bool tls, std::string_view username) = 0;
    virtual void showChannelList(std::vector<std::string> channel_names) = 0;
    virtual void clearMessages() = 0;

protected:
    IPresenter() = default;
    IPresenter(const IPresenter&) = default;
    IPresenter& operator=(const IPresenter&) = default;
    IPresenter(IPresenter&&) = default;
    IPresenter& operator=(IPresenter&&) = default;
};

}  // namespace bcmd::client::application::port
