#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

class thread_pool 
{
    public:
    thread_pool(size_t thread_count = std::thread::hardware_concurrency());
    ~thread_pool();
    thread_pool(const thread_pool &) = delete;
    thread_pool &operator=(const thread_pool &) = delete;

    template <typename F, typename... Args, std::enable_if_t<std::is_invocable_v<F &&, Args &&...>, int> = 0>
    auto execute(F &&, Args &&...);

    private:
    
    class _task_container_base 
    {
        public:
        virtual ~_task_container_base(){};

        virtual void operator()() = 0;
    };
    
    using _task_ptr = std::unique_ptr<_task_container_base>;
    template <typename F, std::enable_if_t<std::is_invocable_v<F &&>, int> = 0>
    class _task_container : public _task_container_base 
    {
        public:
        // here, std::forward is needed because we need the construction of _f not
        //  to bind an lvalue reference - it is not a guarantee that an object of
        //  type F is CopyConstructible, only that it is MoveConstructible.
        _task_container(F &&func) : _f(std::forward<F>(func)) {}

        void operator()() override { _f(); }

        private:
        F _f;
  };

    template <typename F> _task_container(F) -> _task_container<std::decay<F>>;

    std::vector<std::thread> _threads;
    std::queue<_task_ptr> _tasks;
    std::mutex _task_mutex;
    std::condition_variable _task_cv;
    bool _stop_threads = false;
};
    template <typename F, typename... Args, std::enable_if_t<std::is_invocable_v<F &&, Args &&...>, int>>
    auto thread_pool::execute(F &&function, Args &&...args) {
  
    std::unique_lock<std::mutex> queue_lock(_task_mutex, std::defer_lock);
    std::packaged_task<std::invoke_result_t<F, Args...>()> task_pkg(
      // in C++20, this could be:
      // [..., _fargs = std::forward<Args>(args)...]
      [_f = std::move(function),
       _fargs = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        return std::apply(std::move(_f), std::move(_fargs));
      });
    std::future<std::invoke_result_t<F, Args...>> future = task_pkg.get_future();

    queue_lock.lock();
    // this lambda move-captures the packaged_task declared above. Since the
    //  packaged_task type is not CopyConstructible, the function is not
    //  CopyConstructible either - hence the need for a _task_container to wrap
    //  around it.
    _tasks.emplace(_task_ptr(new _task_container([task(std::move(task_pkg))]() mutable { task(); })));

    queue_lock.unlock();

    _task_cv.notify_one();

    return std::move(future);
}