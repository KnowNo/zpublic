#pragma once
#include "z_timer_def.h"

class TimerTaskBase
{
public:
    TimerTaskBase(
        unsigned int taskid = 0,
        unsigned int tasktype = 0,
        unsigned int timeout = 1,
        unsigned int times = 1)
    {
        taskid_         = taskid;
        tasktype_       = tasktype;
        timeout_        = timeout;
        times_          = times;
        executed_       = 0;
        starttime_      = TimerGetTime();
        invalid_        = false;
        release_        = true;
    }
    virtual ~TimerTaskBase() {}

public:
    virtual void DoWork() = 0;

    ///> 执行该任务需要等待的时间
    int WaitTime()
    {
        return (timeout_ * (executed_ + 1)) - (TimerGetTime() - starttime_);
    }

    unsigned int Taskid() const { return taskid_; }
    void Taskid(unsigned int val) { taskid_ = val; }

    unsigned int Tasktype() const { return tasktype_; }
    void Tasktype(unsigned int val) { tasktype_ = val; }

    unsigned int Timeout() const { return timeout_; }
    void Timeout(unsigned int val) { timeout_ = val; }

    unsigned int Times() const { return times_; }
    void Times(unsigned int val) { times_ = val; }
    bool Repeat() const { return (times_ == TaskRepeatDef); }
    void Repeat(bool val) { times_ = val ? TaskRepeatDef : 0; }

    unsigned int Executed() const { return executed_; }
    void ExecutedAdd() { executed_++; }

    bool Invalid() const { return invalid_; }
    void SetInvalid() { invalid_ = true; }

    bool Release() const { return release_; }
    void Release(bool val) { release_ = val; }

protected:
    unsigned int taskid_;            ///> 任务id，自己设置
    unsigned int tasktype_;          ///> 任务分类或者任务分组，自己设置
    unsigned int timeout_;           ///> 定时时间
    unsigned int times_;             ///> 执行次数(99999为repeat无限次数)
    unsigned int executed_;          ///> 已执行次数
    unsigned int starttime_;         ///> 任务开始时间
    bool         invalid_;           ///> 是否无效，一经标记，永久呵呵
    bool         release_;           ///> 是否由调度器来delete，默认true
};

struct TimerTaskCmp
{
    bool operator()(TimerTaskBase* k1, TimerTaskBase* k2) const
    {
        return k1->WaitTime() > k2->WaitTime();
    }
};
