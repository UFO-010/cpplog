#include "gtest/gtest.h"
#include "logger.h"

#include <chrono>

TEST(namespace_test, namespace) {
    using namespace Log;
    ASSERT_TRUE(true);
}

std::string out;
void test_log_callback(const Log::messageType &msgType, const std::string &message) {
    (void)msgType;
    out.clear();
    out.append(message);
}

TEST(format_test, empty_test) {
    Log::Logger::setMessagePattern("");
    Log::Logger::setMessageHandler(&test_log_callback);
    std::string s("");
    Debug("");
    EXPECT_EQ(s, out);
    Info("");
    EXPECT_EQ(s, out);
    Warning("");
    EXPECT_EQ(s, out);
    Error("");
    EXPECT_EQ(s, out);
    Fatal("");
    EXPECT_EQ(s, out);
}

TEST(format_test, empty_message) {
    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{message}");
    Debug("");
    std::string s("");
    EXPECT_EQ(s, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, error_test) {
    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("type");
    Debug("");
    std::string s("");
    EXPECT_EQ(s, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, type_test) {
    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{type}");
    Debug("");
    std::string s("DEBUG");
    EXPECT_EQ(s, out);

    Log::Logger::setMessagePattern("type %{type}");
    Debug("");
    s = "type DEBUG";
    EXPECT_EQ(s, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, file_test) {
    std::string file = __FILE__;
    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{file}");
    Debug("");
    EXPECT_EQ(file, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, func_test) {
#if defined(__GNUC__) || defined(__clang__)
    std::string func = __PRETTY_FUNCTION__;
#elif _MSC_VER && !__INTEL_COMPILER
    std::string func = __PRETTY_FUNCTION__;
#else
    std::string func = __func__;
#endif
    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{function}");
    Debug("");
    EXPECT_EQ(func, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, line_test) {
    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{line}");
    std::string line = std::to_string(__LINE__ + 1);  // must be on the next line
    Debug("");
    EXPECT_EQ(line, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, date_test) {
    time_t timestamp;
    time(&timestamp);
    struct tm datetime;
    datetime = *localtime_r(&timestamp, &datetime);
    char date[16];  // unsafe
    strftime(date, 16, "%d.%m.%Y", &datetime);

    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{date}");
    Debug("");
    EXPECT_EQ(std::string(date), out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, time_test) {
    time_t timestamp;
    time(&timestamp);
    struct tm datetime;
    datetime = *localtime_r(&timestamp, &datetime);
    char time[16];  // unsafe
    strftime(time, 16, "%H:%M:%S", &datetime);

    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{time}");
    Debug("");
    EXPECT_EQ(time, out);

    Debug(" ");
    EXPECT_EQ(time, out);

    Debug("aaa");
    EXPECT_EQ(time, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, thread_test) {
    std::string thread;
#if defined(__linux__)
    thread.append(std::to_string(gettid()));
#elif defined(_WIN32)
    thread.append(std::to_string(GetCurrentThreadId()));
#endif

    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{thread}");
    Debug("");
    EXPECT_EQ(thread, out);

    Debug(" ");
    EXPECT_EQ(thread, out);

    Debug("aaa");
    EXPECT_EQ(thread, out);
    Log::Logger::setMessagePattern("");
}

TEST(format_test, pid_test) {
    std::string pid;
#if defined(__linux__)
    pid.append(std::to_string(getpid()));
#elif defined(_WIN32)
    pid.append(std::to_string(GetCurrentProcessId()));
#endif

    Log::Logger::setMessageHandler(&test_log_callback);
    Log::Logger::setMessagePattern("%{pid}");
    Debug("");
    EXPECT_EQ(pid, out);

    Debug(" ");
    EXPECT_EQ(pid, out);

    Debug("aaa");
    EXPECT_EQ(pid, out);
    Log::Logger::setMessagePattern("");
}
