
#ifndef DEFAULTPROVIDER_H
#define DEFAULTPROVIDER_H

#include <cstddef>

class DefaultDataProvider {
public:
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
class PlatformDataProvider : public DefaultDataProvider {};

#endif
