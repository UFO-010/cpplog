Logging library
=============

Simple logging library in C++. Support different logging levels and message formatting.

Usage
-----

Logging library provides several varinats and levels of writing logs. Default log level is `DEBUG`

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`
- `FATAL`

Logging can be used in printf-style like

```c++
Error("error\n");
```

Or with `<<` operator

```c++
sError << "error" << "\n"
```

### Using colors
Library provides colorozed log outputs to the console using ANSI escape colors. Colors are automatically applied if set
```c++
Log::Logger::colorize(true);
```

To prevent data races library use stack message allocation and cout synchronization. Library use mutex only when writing to file operation is performed.


Formatting
----------

Logs are customizable via format string. Options:
- **%{date}** - Current date
- **%{time}** - Time when logging was called
- **%{type}** - Log level (DEBUG, INFO, WARN, ERROR, FATAL)
- **%{file}** - File name from which the log was called
- **%{thread}** - Thread identifier that wrote the log.
- **%{function}** - Function name from which the log was called
- **%{line}** - Line number in the file from which the log was called
- **%{pid}** - Current process name, Linux and Windows only
- **%{%{message}}** - Log message

Always put separators between components, or else component will be treated as message. You can also place messages between components, they will be printed as well.
For example
```c++
Log::Logger::setMessagePattern(
        "%{type}\t %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");
Error("message\n");
```
Will print
`ERROR	 16.05.2025 11:36:37 logger_example file {your path}/example/main.cpp function void func() line 31 message`



TODO
----

- Add buffered message print
