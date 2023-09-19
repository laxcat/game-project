#include "Worker.h"
#include "../MrManager.h"

Worker::Worker(Fn const & task) :
    _task(task)
{
    _thread = mm.memMan.create<std::thread>([this]{
        setStatus(STATUS_WORKING);
        if (_task) {
            _task();
        }
        setStatus(STATUS_COMPLETE);
    });
}

bool Worker::isComplete() const {
    guard_t guard{_mutex};
    return (_status == STATUS_COMPLETE);
}

void Worker::join() {
    if (_status == STATUS_INACTIVE || _thread == nullptr) {
        return;
    }
    _thread->join();
    mm.memMan.request({.ptr=_thread, .size=0});
    _thread = nullptr;
    _status = STATUS_INACTIVE;
}

void Worker::setStatus(Status status) {
    _mutex.lock();
    _status = status;
    _mutex.unlock();
}
