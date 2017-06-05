#include "TaskGraph.h"
#include <thread>


//#define TASK_GRAPH_DEBUGGING

#ifdef TASK_GRAPH_DEBUGGING
#include <iostream>
#include <sstream>
std::mutex _cerr_mutex;
#define tg_debug(args) {\
    std::unique_lock<std::mutex> _lk{_cerr_mutex}; \
    std::cerr << std::this_thread::get_id() << ": " << args << std::endl; \
}
#else
#define tg_debug(args)
#endif


mt::TaskGraph::TaskGraph() {}

void mt::TaskGraph::addTask(mt::Task *task, const std::vector<Task *> &dependencies) {
    if (task->getId() >= 0) throw std::runtime_error("Task is already registered");
    for(auto dep : dependencies) {
        if (dep->getId() < 0) throw std::runtime_error("Dependency is not registered");
        int id = dep->getId();
        if (id >= _tasks.size() || dep != _tasks[id].task)
            throw std::runtime_error("Dependency does not belong to this graph");
    }
    std::vector<int> ids;
    for (auto dep : dependencies) ids.push_back(dep->getId());
    task->setId(_tasks.size());
    _tasks.push_back({task, ids});
}

void mt::TaskGraph::runAll(unsigned nThreads) {
#ifdef TASK_GRAPH_DEBUGGING
    tg_debug("runAll");
    for (auto &state : _tasks) {
        std::stringstream ss;
        ss << state.task->getId() << " << ";
        for (int dep : state.dependencies) {
            ss << " " << dep;
        }
        tg_debug(ss.str());
    }
#endif

    // initialize all states:
    for(auto &state : _tasks) state.reset();
    for(const auto &state : _tasks) {
        for(int depId : state.dependencies) {
            _tasks[depId].nUsersNotFinished++;
        }
    }

    std::vector<std::thread> workers;
    for(int i = 1; i < nThreads; i++) {
        workers.push_back(std::thread{&TaskGraph::_workerThread, this});
    }
    _workerThread();

    for(auto &worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

void mt::TaskGraph::_workerThread() {
    while(true) {
        int taskId = _startNextPortion();
        if (taskId < 0) return;
        bool canGo = _callStartIfNeeded(taskId);
        bool done = false;
        if (canGo) {
            Task *task = _tasks[taskId].task;
            tg_debug("before runPortion(): " << taskId);
            done = task->runPortion();
            tg_debug("after runPortion(): " << taskId);
        }
        _portionDone(taskId, done);
    }
}

int mt::TaskGraph::_startNextPortion() {
    std::unique_lock<std::mutex> _lock{_mtxTasks};
    int taskId = -1;

    while (taskId == -1) {
        // at first, try to find non-waiting in-progress task
        for (int i = 0; i < _tasks.size(); i++) {
            const auto &state = _tasks[i];
            if (state.started==YES && !state.finished && !state.runsNow
                    && !state.task->isWaiting()) {
                taskId = i;
                tg_debug("resuming task: " << taskId);
                break;
            }
        }
        if (taskId >= 0) {
            // if task found, simply turn it into running state
            // its start/stop state is not changing, so no need to notify deps/users
            _tasks[taskId].runsNow = true;
            return taskId;
        }

        // maybe we can start a new task?
        for (int i = 0; i < _tasks.size(); i++) {
            const auto &state = _tasks[i];
            if (state.started==NO && state.nDependenciesNotStarted == 0) {
                taskId = i;
                tg_debug("will start task: " << taskId );
                break;
            }
        }
        if (taskId >= 0) {
            // we are about to start a new task:
            auto &state = _tasks[taskId];
            // remember that this task is started and runs:
            state.started = WILL_NOW;
            state.runsNow = true;
            // notify users that this task (theirs dependency) was started
            for(int i = 0; i < _tasks.size(); i++) {
                for(int depId : _tasks[i].dependencies) {
                    if (depId == taskId) {
                        _tasks[i].nDependenciesNotStarted--;
                        if (_tasks[i].nDependenciesNotStarted == 0) {
                            _hasReadyTasks.notify_all();
                        }
                        break;
                    }
                }
            }
            // finally, return the taskId for its first runPortion():
            return taskId;
        }

        // maybe there are no tasks to do?
        if (_areAllFinished()) {
            tg_debug("finishing thread");
            return -1;
        }

        // not all tasks are done, but we cannot start a new task
        // so we have no way than to wait until some task will not be able to start
        _hasReadyTasks.wait(_lock, [this] {
            return this->_areAllFinished() || (this->_getNReadyTasks()>0);
        });
    }
    return -1;
}

void mt::TaskGraph::_portionDone(int taskId, bool done) {
    std::unique_lock<std::mutex> _lock{_mtxTasks};
    auto &state = _tasks[taskId];
    state.runsNow = false;
    if (done) {
        state.finished = true;

        // notify dependencies that one more client is gone
        // and check for dependencies that has no more clients
        // and deallocate their resources (their lifecycle ended)
        for(int depId : state.dependencies) {
            auto &dep = _tasks[depId];
            dep.nUsersNotFinished--;
            if (dep.finished && dep.nUsersNotFinished == 0) {
                dep.task->deallocateResources();
                dep.deallocated = true;
            }
        }

        // for the case if this task does not have users
        // also check itself and maybe deallocate
        if (state.nUsersNotFinished == 0) {
            state.task->deallocateResources();
            state.deallocated = true;
        }

        // notify waiting threads, maybe all tasks are finished
        _hasReadyTasks.notify_all();
    }
}

bool mt::TaskGraph::_callStartIfNeeded(int taskId) {
    bool needToCall;
    TaskState *state;
    std::vector<Task*> deps;
    bool canGoNow = true; 
    {
        std::unique_lock<std::mutex> _lock{_mtxTasks};
        state = &_tasks[taskId];
        needToCall = state->started == WILL_NOW;
        if (needToCall) {
            for(int depId : _tasks[taskId].dependencies) {
                deps.push_back(_tasks[depId].task);
            }
        }
        tg_debug(taskId << " will init now");
    }
    if (needToCall) {
        canGoNow = state->task->start(deps);
        {
            std::unique_lock<std::mutex> _lock{_mtxTasks};
            state->started = YES;
            tg_debug(taskId << " init ok");
        }
    }
    return canGoNow;
}

int mt::TaskGraph::_getNReadyTasks() const {
    int nReady = 0;
    for (const auto &state : _tasks) {
        if (state.started==NO && state.nDependenciesNotStarted==0) {
            nReady++;
        }
    }
    return nReady;
}

bool mt::TaskGraph::_areAllFinished() const {
    bool allFinished = true;
    for (const auto &state : _tasks) {
        if (!state.finished) {
            allFinished = false;
        }
    }
    return allFinished;
}
