
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
    size_t getProcessName(char *buffer, size_t bufferSize) const {
        return static_cast<const Derived *>(this)->getProcessNameImpl(buffer, bufferSize);
    }

    size_t getThreadId(char *buffer, size_t bufferSize) const {
        return static_cast<const Derived *>(this)->getThreadIdImpl(buffer, bufferSize);
    }

    size_t getCurrentDate(char *buffer, size_t bufferSize) const {
        return static_cast<const Derived *>(this)->getCurrentDateImpl(buffer, bufferSize);
    }

    size_t formatTime(char *buffer, size_t bufferSize, long timestamp) const {
        return static_cast<const Derived *>(this)->formatTimeImpl(buffer, bufferSize, timestamp);
    }

    long long getTimestamp() const {
        return static_cast<const Derived *>(this)->getTimestampImpl();
    }
};

}  // namespace Log
