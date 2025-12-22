
#pragma once

#include "logger.h"
#include <iostream>
#include <vector>

enum class ansi_cols : int {
    DEBUG_COLOR = 0,
    INFO_COLOR = 1,
    WARNING_COLOR = 2,
    ERROR_COLOR = 3,
    FATAL_COLOR = 4,
    RESET_COLOR = 5
};

class ConsoleSink : public Log::ILogSink<ConsoleSink> {
public:
    ConsoleSink() = default;

    void send(const Log::level &msgType, const char *data, size_t size) const {
        if (ansi_cols_support && colors_enabled) {
            std::string colorized_msg;
            auto reset_pos = static_cast<size_t>(ansi_cols::RESET_COLOR);

            colorized_msg.reserve((msg_colors[reset_pos].size() * 2) + size);
            colorized_msg.append(msg_colors[static_cast<size_t>(msgType)]);
            colorized_msg.append(data);
            colorized_msg.append(msg_colors[reset_pos]);

            std::cout << colorized_msg;
            return;
        }

        std::cout << data;
    }

    void colorize(bool col) const {
#if defined(_WIN32)
        if (!setWinConsoleAnsiCols(std::cout.rdbuf())) {
            ansi_cols_support = false;
        }
#endif
        colors_enabled = col;
    }

private:
#if defined(_WIN32)
    /**
     * @brief Logger::getWinConsoleHandle
     * @param osbuf
     * @return Windows handler for stdout or stderr
     *
     */
    inline HANDLE getWinConsoleHandle(const std::streambuf *osbuf) {
        if (osbuf == std::cout.rdbuf()) {
            static const HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
            return std_out_handle;
        }

        if (osbuf == std::cerr.rdbuf()) {
            static const HANDLE std_err_handle = GetStdHandle(STD_ERROR_HANDLE);
            return std_err_handle;
        }
        return INVALID_HANDLE_VALUE;
    }

    /**
     * @brief Logger::setWinConsoleAnsiCols
     * @param osbuf
     * @return true if ANSI colors enabled
     */
    bool setWinConsoleAnsiCols(const std::streambuf *osbuf) {
        HANDLE win_handle = getWinConsoleHandle(osbuf);
        if (win_handle == INVALID_HANDLE_VALUE) {
            return false;
        }
        DWORD dw_mode = 0;
        if (!GetConsoleMode(win_handle, &dw_mode)) {
            return false;
        }
        dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(win_handle, dw_mode)) {
            return false;
        }
        return true;
    }
#endif
    /// terminal colors for logging message
    inline static std::vector<std::string> msg_colors = {
        "\033[35m",  // magenta FATAL_COLOR
        "\033[31m",  // red ERROR_COLOR
        "\033[33m",  // yellow WARNING_COLOR
        "\033[32m",  // green INFO_COLOR
        "\033[97m",  // white DEBUG_COLOR
        "\033[0m",   // reset RESET_COLOR
    };
    mutable bool colors_enabled = true;
    mutable bool ansi_cols_support = true;
};
