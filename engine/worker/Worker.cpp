#include "Worker.h"
#include "../MrManager.h"
#include "../dev/print.h"

#if DEBUG
char const * Worker::statusString(Status status) const {
    switch (status) {
    case STATUS_INACTIVE:   return "INACTIVE";
    case STATUS_WORKING:    return "WORKING";
    case STATUS_COMPLETE:   return "COMPLETE";
    default:                return "(invalid)";
    }
}
#endif // DEBUG

void Worker::init(Fn const & task, void * group) {
    _task = task;
    _group = group;
    _thread = (std::thread *)mm.memMan.request({.size=sizeof(std::thread)});
    new (_thread) std::thread{[this]{
        setStatus(STATUS_WORKING);
        if (_task) {
            _task();
        }
        setStatus(STATUS_COMPLETE);
    }};
}

void Worker::shutdown() {
    mm.memMan.request({.ptr=_thread, .size=0});
}

bool Worker::isComplete() const {
    guard_t guard{_mutex};
    return (_status == STATUS_COMPLETE);
}

void * Worker::group() const {
    return _group;
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
