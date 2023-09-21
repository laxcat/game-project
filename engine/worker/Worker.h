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
    void * group() const;
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
    Worker(Fn const & task, void * group = nullptr);

// STORAGE
private:
    Fn _task = nullptr;
    void * _group = nullptr;
    std::thread * _thread;
    Status _status = STATUS_INACTIVE;
    mutable std::mutex _mutex;

// INTERNALS
private:
    void setStatus(Status status);
};

class WorkerGroup {
public:
    void * id = nullptr;
    int count = 0;
    Worker::Fn onComplete = nullptr;
};
