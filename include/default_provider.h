
#pragma once

#include <cstddef>

#include "message.h"

namespace Log {

/**
 * @brief The IMessageQueue class
 *
 * Class that store data captured in hot path and should be processed in background
 */
template <typename Derived, typename TConfig = Config::Traits<Config::Default>>
class IMessageQueue {
    using ConfigType = TConfig;
    using MessageType = LogMessage<TConfig>;

public:
    ~IMessageQueue() = default;

    bool enqueue(const MessageType &msg) { return static_cast<Derived *>(this)->enqueueImpl(msg); }

    bool dequeue(MessageType &msg) { return static_cast<Derived *>(this)->dequeueImpl(msg); }

    bool dequeueBlocking(MessageType &msg, unsigned long timeout_ms = 0) {
        return static_cast<Derived *>(this)->dequeueBlockingImpl(msg, timeout_ms);
    }
};

template <typename Derived>
class IContextProvider {
public:
    size_t getProcessName([[maybe_unused]] char *buffer, [[maybe_unused]] size_t bufferSize) const {
        return static_cast<const Derived *>(this)->getProcessNameImpl(buffer, bufferSize);
    }

    size_t getThreadId([[maybe_unused]] char *buffer, [[maybe_unused]] size_t bufferSize) const {
        return static_cast<const Derived *>(this)->getThreadIdImpl(buffer, bufferSize);
    }

    size_t getCurrentDate([[maybe_unused]] char *buffer, [[maybe_unused]] size_t bufferSize) const {
        return static_cast<const Derived *>(this)->getCurrentDateImpl(buffer, bufferSize);
    }

    size_t getCurrentTime([[maybe_unused]] char *buffer, [[maybe_unused]] size_t bufferSize) const {
        return static_cast<const Derived *>(this)->getCurrentTimeImpl(buffer, bufferSize);
    }

    long long getTimestamp() const {
        return static_cast<const Derived *>(this)->getTimeStampImpl();
    }
};

}  // namespace Log
