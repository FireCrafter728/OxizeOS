#pragma once
#include "File.hpp"
#include "Global.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace cppextra
{
    class Log
    {
    public:
        Log();
        ~Log();

        void print(const std::string &str);
        inline void println(const std::string &str) { print(str + '\n'); };

        void fprint(File *stream, const std::string &str);
        inline void fprintln(File *stream, const std::string &str) { fprint(stream, str + '\n'); };

        template <typename... Args>
        inline void printfm(const std::string &Format, Args &&...args)
        {
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            printfm_dispatch(Format, argsTuple);
        }

        template <typename... Args>
        inline void printfmln(const std::string &Format, Args &&...args)
        {
            printfm(Format + '\n', std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline void fprintfm(File *stream, const std::string &Format, Args &&...args)
        {
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            fprintfm_dispatch(stream, Format, argsTuple);
        }

        template <typename... Args>
        inline void fprintfmln(File *stream, const std::string &Format, Args &&...args)
        {
            fprintfm(stream, Format + '\n', std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline std::string format(const std::string &Format, Args &&...args)
        {
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            return format_dispatch(Format, argsTuple);
        }

        void start();
        void stop();

    private:
        std::string format_init(const std::string &Format, const std::format_args &args);

        template <typename Tuple>
        inline void printfm_dispatch(const std::string &Format, Tuple &&argsTuple)
        {
            constexpr size_t N = std::tuple_size_v<std::decay_t<Tuple>>;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                logQueue.push({printfm_dispatch_init(Format, std::forward<Tuple>(argsTuple), std::make_index_sequence<N>{}), true, nullptr});
            }
            cv.notify_one();
        }

        template <typename Tuple, std::size_t... I>
        inline std::string printfm_dispatch_init(const std::string &Format, Tuple &&argsTuple, std::index_sequence<I...>)
        {
            return format_init(Format, std::make_format_args(std::get<I>(std::forward<Tuple>(argsTuple))...));
        }

        template <typename Tuple>
        inline void fprintfm_dispatch(File *stream, const std::string &Format, Tuple &&argsTuple)
        {
            constexpr size_t N = std::tuple_size_v<std::decay_t<Tuple>>;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                logQueue.push({printfm_dispatch_init(Format, std::forward<Tuple>(argsTuple), std::make_index_sequence<N>{}), false, stream});
            }
            cv.notify_one();
        }

        template <typename Tuple>
        inline std::string format_dispatch(const std::string &Format, Tuple &&argsTuple)
        {
            constexpr size_t N = std::tuple_size_v<std::decay_t<Tuple>>;
            return printfm_dispatch_init(Format, std::forward<Tuple>(argsTuple), std::make_index_sequence<N>{});
        }

        struct logDesc
        {
            std::string msg;
            bool printStdout;
            File *stream;
        };

        std::queue<logDesc> logQueue;
        std::mutex queueMutex;
        std::condition_variable cv;
        std::atomic<bool> running;
        std::thread loggerThread;

        void logger();
    };
}