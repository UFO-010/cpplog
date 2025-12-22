
#pragma once

#include <cstddef>

namespace Log {

class IDataProvider {
public:
    virtual ~IDataProvider() = default;

    virtual size_t getProcessName([[maybe_unused]] char *buffer,
                                  [[maybe_unused]] size_t bufferSize) const {
        return 0;
    }

    virtual size_t getThreadId([[maybe_unused]] char *buffer,
                               [[maybe_unused]] size_t bufferSize) const {
        return 0;
    }

    virtual size_t getCurrentDate([[maybe_unused]] char *buffer,
                                  [[maybe_unused]] size_t bufferSize) const {
        return 0;
    }

    virtual size_t getCurrentTime([[maybe_unused]] char *buffer,
                                  [[maybe_unused]] size_t bufferSize) const {
        return 0;
    }
};

template <typename PlatformTag>
class TDataProvider : public IDataProvider {};

struct Default {};

template <>
class TDataProvider<Default> : public IDataProvider {};

}  // namespace Log
