#ifndef MTP_LAB1_JOB_H
#define MTP_LAB1_JOB_H

#include <vector>
#include <functional>
#include <mutex>

namespace mt {

class Task {

public:

    bool setId(int id) {
        if (!idWasSet) {
            this->id = id;
            idWasSet = true;
            return true;
        }
        return false;
    }
    int getId() const { return id; }

    bool start(const std::vector<Task*>& dependencies) {
        _workMtx.lock();
        _done = false;
        return doStart(dependencies);
    }
    bool runPortion() {
        bool done;
        {
            std::unique_lock<std::mutex> _lock{_portionMtx};
            done = doWorkPortion();
        }
        {
            std::unique_lock<std::mutex> _lock{_doneMtx};
            _done = done;
        }
        return done;
    }
    void deallocateResources() {
        doFinalize();
        _workMtx.unlock();
    }
    virtual bool isWaiting() = 0;

    bool isDone() const {
        std::unique_lock<std::mutex> _lock{_doneMtx};
        return _done;
    }

protected:
    virtual bool doStart(const std::vector<Task*>& dependencies) { return true; }
    virtual bool doWorkPortion() = 0;
    virtual void doFinalize() {}

private:
    bool idWasSet = false;
    int id = -1;
    bool _done = false;
    mutable std::mutex _workMtx, _portionMtx, _doneMtx;

};

}

#endif //MTP_LAB1_JOB_H
