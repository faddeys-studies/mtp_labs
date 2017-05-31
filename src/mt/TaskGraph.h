#ifndef MTP_LAB1_TASKGRAPH_H
#define MTP_LAB1_TASKGRAPH_H

#include <vector>
#include <condition_variable>
#include "Task.h"


namespace mt {

class TaskGraph {

public:

    TaskGraph();

    void addTask(Task* task, const std::vector<Task*> &dependencies);
    void runAll(unsigned nThreads);

private:

    class TaskState {
    public:
        // turns into true before calling start() and first runPortion()
        bool started = false;

        // turns into true before each runPortion()
        // turns into false after each runPortion()
        bool runsNow = false;

        // turns into true when runPortion() says that job is done
        bool finished = false;

        // turns into true when deallocateResources() called
        bool deallocated = false;

        // initially set to number of users
        // decrement when user finishes
        // used to determine when we can finalize task:
        //      no users - no need to preserve this task
        int nUsersNotFinished = 0;

        // initially set to number of dependencies
        // decrement when starting a dependency
        // used to determine when we can start the task
        //      all dependencies are running - it makes sense to start this task also
        unsigned long nDependenciesNotStarted;

        // task itself and its dependencies
        Task *const task;
        const std::vector<int> dependencies;

        TaskState(Task* task, const std::vector<int>& dependencies)
                : task(task)
                , dependencies(dependencies)
                , nDependenciesNotStarted(dependencies.size()) {}
        void reset() {
            started = false;
            finished = false;
            deallocated = false;
            runsNow = false;
            nUsersNotFinished = 0;
            nDependenciesNotStarted = dependencies.size();
        }
    };

    void _workerThread();
    int _startNextPortion();
    void _portionDone(int taskId, bool done);

    std::vector<TaskState> _tasks;
    std::mutex _mtxTasks;
    std::condition_variable _hasReadyTasks;

    int _getNReadyTasks() const;
    bool _areAllFinished() const;

};

}

#endif //MTP_LAB1_TASKGRAPH_H
