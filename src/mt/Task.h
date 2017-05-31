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

    void start(const std::vector<Task*>& dependencies) {
        _workMtx.lock();
        doStart(dependencies);
    }
    bool runPortion() {
        _portionMtx.lock();
        bool done = doWorkPortion();
        _portionMtx.unlock();
        return done;
    }
    void deallocateResources() {
        doFinalize();
        _workMtx.unlock();
    }
    virtual bool isWaiting() = 0;

protected:
    virtual void doStart(const std::vector<Task*>& dependencies) {}
    virtual bool doWorkPortion() = 0;
    virtual void doFinalize() {}

private:
    bool idWasSet = false;
    int id = -1;
    std::mutex _workMtx;
    std::mutex _portionMtx;

};

}

#endif //MTP_LAB1_JOB_H
