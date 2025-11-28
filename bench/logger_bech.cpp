
#include <benchmark/benchmark.h>
#include "logger.h"

using MyConfig = Log::Config::Traits<Log::Config::Default>;

class NullSink : public Log::ILogSink<NullSink> {
public:
    void send(const Log::level &, const char *, size_t) const {
        // do nothing
    }
};

struct Empty {};

template <>
class PlatformDataProvider<Empty> : public DefaultDataProvider {
public:
    PlatformDataProvider() = default;

    size_t getProcessName(char *, size_t) const final { return 0; }

    size_t getThreadId(char *, size_t) const final { return 0; }

    size_t getCurrentDate(char *, size_t) const final { return 0; }

    size_t getCurrentTime(char *, size_t) const final { return 0; }
};

static void BM_CreateMessage(benchmark::State &state) {
    const PlatformDataProvider<Empty> emptyProvider;
    const NullSink nullSink;
    Log::Logger my_logger(emptyProvider, nullSink);
    my_logger.setLogPattern("%{level} file %{file} function %{function} line %{line} %{message}");

    std::array<char, MyConfig::LOGGER_MAX_STR_SIZE> buf_ar = {};
    char *buf = buf_ar.data();
    size_t buf_size = buf_ar.size();

    const char *msg = "test";
    size_t msg_len = 4;

    constexpr std::string_view file_sv = __FILE__;
    constexpr std::string_view func_sv = LOG_CURRENT_FUNC;
    constexpr Log::LogRecord l = {Log::level::DebugMsg, file_sv, func_sv, 18};

    for (auto _ : state) {
        (void)_;
        size_t len = my_logger.createMessage(buf, buf_size, l, msg, msg_len);

        benchmark::DoNotOptimize(len);
        benchmark::DoNotOptimize(buf);
    }
}

BENCHMARK(BM_CreateMessage);

static void BM_Stdprint(benchmark::State &state) {
    std::array<char, MyConfig::LOGGER_MAX_STR_SIZE> buf_ar = {};
    char *buf = buf_ar.data();
    size_t buf_size = buf_ar.size();

    const char *file = __FILE__;
    const char *func = LOG_CURRENT_FUNC;
    int line = __LINE__;
    const char *msg = "test";

    for (auto _ : state) {
        (void)_;
        int len = std::snprintf(buf, buf_size, "%d file %s function %s line %d %s",
                                Log::level::DebugMsg, file, func, line, msg);

        benchmark::DoNotOptimize(len);
        benchmark::DoNotOptimize(buf);
    }
}

BENCHMARK(BM_Stdprint);

static void BM_logging(benchmark::State &state) {
    const PlatformDataProvider<Empty> emptyProvider;
    const NullSink nullSink;
    Log::Logger my_logger(emptyProvider, nullSink);
    my_logger.setLogLevel(Log::level::DebugMsg);
    my_logger.setLogPattern("%{level} file %{file} function %{function} line %{line} %{message}");

    for (auto _ : state) {
        (void)_;
        const char *msg = "test";
        size_t size = sizeof("test");
        Debug(my_logger, msg, size);
    }
}

BENCHMARK(BM_logging);

static void BM_SingleMessage(benchmark::State &state) {
    const PlatformDataProvider<Empty> emptyProvider;
    const NullSink nullSink;
    Log::Logger my_logger(emptyProvider, nullSink);
    my_logger.setLogLevel(Log::level::DebugMsg);
    my_logger.setLogPattern("%{message}");

    for (auto _ : state) {
        (void)_;
        const char *msg = "test";
        size_t size = sizeof("test");
        Debug(my_logger, msg, size);
    }
}

BENCHMARK(BM_SingleMessage);

int main(int argc, char *argv[]) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    return 0;
}
