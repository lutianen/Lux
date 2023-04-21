/**
 * @file Poller.cc
 * @brief
 *
 * @author Lux
 */

#include <polaris/Channel.h>
#include <polaris/EPollPoller.h>
#include <polaris/Poller.h>

using namespace Lux;
using namespace Lux::polaris;

Poller::Poller(EventLoop* loop) : ownerLoop_(loop) {}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const {
    assertInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

/// @brief
/// @param loop
/// @return
Poller* Poller::newDefaultPoller(EventLoop* loop) {
    if (::getenv("LUX_USE_POLL")) {
        // BUG pollpoller
        return new EPollPoller(loop);
        // return new PollPoller(loop);
    } else {
        return new EPollPoller(loop);
    }
}