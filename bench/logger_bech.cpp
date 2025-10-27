#include <benchmark/benchmark.h>
#include "logger.h"

class NullSink : public Log::ILogSink<NullSink> {
public:
    void send(const Log::level &, const char *, size_t) const {
        // do nothing
    }
};

class EmptyProvider {
public:
    size_t getProcessName(char *, size_t) const { return 0; }

    size_t getThreadId(char *, size_t) const { return 0; }

    size_t getCurrentDate(char *, size_t) const { return 0; }

    size_t getCurrentTime(char *, size_t) const { return 0; }
};

static void BM_CreateMessage(benchmark::State &state) {
    const EmptyProvider emptyProvider;
    const NullSink nullSink;
    Log::Logger<EmptyProvider, NullSink> my_logger(emptyProvider, nullSink);
    my_logger.setLogPattern("%{level} file %{file} function %{function} line %{line} %{message}");

    std::string buf;
    buf.resize(LOGGER_MAX_STR_SIZE);
    constexpr Log::LogRecord l = {Log::level::DebugMsg, "main.cpp", sizeof("main.cpp"), "main",
                                  sizeof("main"),       18};

    for (auto _ : state) {
        my_logger.createMessage(buf.data(), buf.size(), l, "test");
    }
}

BENCHMARK(BM_CreateMessage);

static void BM_Stdprint(benchmark::State &state) {
    std::string buf;
    buf.resize(LOGGER_MAX_STR_SIZE);
    for (auto _ : state) {
        std::snprintf(buf.data(), buf.size(), "%d file %s function %s line %d %s",
                      Log::level::DebugMsg, "main.cpp", "main", 18, "test");
    }
}

BENCHMARK(BM_Stdprint);

static void BM_logging(benchmark::State &state) {
    const EmptyProvider emptyProvider;
    const NullSink nullSink;
    Log::Logger<EmptyProvider, NullSink> my_logger(emptyProvider, nullSink);
    my_logger.setLogLevel(Log::level::DebugMsg);
    my_logger.setLogPattern("%{level} file %{file} function %{function} line %{line} %{message}");

    for (auto _ : state) {
        Debug(my_logger, "test");
    }
}

BENCHMARK(BM_logging);

static void BM_SingleMessage(benchmark::State &state) {
    const EmptyProvider emptyProvider;
    const NullSink nullSink;
    Log::Logger<EmptyProvider, NullSink> my_logger(emptyProvider, nullSink);
    my_logger.setLogLevel(Log::level::DebugMsg);
    my_logger.setLogPattern("%{message}");

    for (auto _ : state) {
        Debug(my_logger, "test");
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
