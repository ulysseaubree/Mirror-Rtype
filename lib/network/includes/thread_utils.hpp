// /*
// ** EPITECH PROJECT, 2026
// ** r-type
// ** File description:
// ** Thread utilities
// **
// */

// #pragma once

// #include <queue>
// #include <mutex>
// #include <condition_variable>
// #include <utility>

// namespace thread_utils {

// class ThreadRunner {
// public:
//     ThreadRunner() = default;
//     ~ThreadRunner() { stop_and_join(); }

//     ThreadRunner(const ThreadRunner&) = delete;
//     ThreadRunner& operator=(const ThreadRunner&) = delete;

//     ThreadRunner(ThreadRunner&& other) noexcept
//     {
//         *this = std::move(other);
//     }

//     ThreadRunner& operator=(ThreadRunner&& other) noexcept
//     {
//         if (this != &other) {
//             stop_and_join();
//             _running.store(other._running.load());
//             _thread = std::move(other._thread);
//             other._running.store(false);
//         }
//         return *this;
//     }

//     // Thread une fonction : f(running, args...)
//     template <typename F, typename... Args>
//     void start(F&& f, Args&&... args)
//     {
//         stop_and_join();
//         _running.store(true);

//         _thread = std::thread(
//             [this, func = std::forward<F>(f)](auto&&... innerArgs) mutable {
//                 func(_running, std::forward<decltype(innerArgs)>(innerArgs)...);
//             },
//             std::forward<Args>(args)...);
//     }

//     // Thread un objet qui a : void run(std::atomic_bool&)
//     template <typename Obj>
//     void start_object(Obj& obj)
//     {
//         start([&](std::atomic_bool& running) { obj.run(running); });
//     }

//     void request_stop() { _running.store(false); }

//     void join()
//     {
//         if (_thread.joinable())
//             _thread.join();
//     }

//     void stop_and_join()
//     {
//         request_stop();
//         join();
//     }

//     bool is_running() const { return _running.load(); }

// private:
//     std::atomic_bool _running{false};
//     std::thread _thread;
// };

// // ============================================================
// //  MutexValue<T> : protection simple d’un objet partagé
// // ============================================================
// template <typename T>
// class MutexValue {
// public:
//     template <typename... Args>
//     explicit MutexValue(Args&&... args)
//         : _value(std::forward<Args>(args)...)
//     {}

//     // Accès safe via callback
//     template <typename F>
//     auto with_lock(F&& f) -> decltype(f(_value))
//     {
//         std::lock_guard<std::mutex> lock(_mtx);
//         return f(_value);
//     }

// private:
//     std::mutex _mtx;
//     T _value;
// };
// } // namespace thread_utils