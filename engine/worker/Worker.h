#pragma once
#include <thread>
#include <mutex>

class Worker {
// TYPES
public:
    using Fn = std::function<void(void)>;
    using guard_t = std::lock_guard<std::mutex>;

// INTERFACE
public:
    bool isComplete() const;
    void join();

// PRIVATE TYPES
private:
    enum Status {
        STATUS_INACTIVE,
        STATUS_WORKING,
        STATUS_COMPLETE, // waiting to join
    };

// INIT
private:
    friend class MemMan;
    Worker(Fn const & task);

// STORAGE
private:
    Fn _task = nullptr;
    std::thread * _thread;
    Status _status = STATUS_INACTIVE;
    mutable std::mutex _mutex;

// INTERNALS
private:
    void setStatus(Status status);
};
