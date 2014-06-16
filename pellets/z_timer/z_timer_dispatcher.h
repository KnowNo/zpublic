#pragma once
#include "z_timer_task.h"
#include "z_timer_task_queue.h"
#include "z_timer_watcher.h"
#include "../z_mutex.h"
#include <windows.h>
#include <process.h>


class TimerDispatcher
{
public:
    TimerDispatcher()
    {
        pWatcher_       = NULL;
        event_          = NULL;
        thread_         = NULL;
        stop_           = false;
    }
    ~TimerDispatcher() {}

    int Init()
    {
        event_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!event_)
        {
            return -1;
        }
        return 0;
    }
    int Uninit(DWORD dwMilliseconds = 2000)
    {
        Stop();
        if (thread_)
        {
            DWORD dwRet = ::WaitForSingleObject(thread_, dwMilliseconds);
            if (dwRet == WAIT_TIMEOUT)
            {
                ::TerminateThread(thread_, 0);
            }
            ::CloseHandle(thread_);
            thread_ = NULL;
        }
        return 0;
    }
    int Start()
    {
        stop_ = false;
        ///> event��Ч������
        if (!thread_ && event_)
        {
            thread_ = (HANDLE)_beginthreadex(NULL, 0, _ThreadRoute, this, 0, NULL);
        }
        if (!thread_)
        {
            return -1;
        }
        return 0;
    }
    int Stop()
    {
        stop_ = true;
        if (event_)
        {
            ::SetEvent(event_);
        }
        return 0;
    }

    int AddTimerTask(TimerTaskBase* pTask)
    {
        _InsertTask(pTask);
        if (event_)
        {
            ::SetEvent(event_);
        }
        return 0;
    }

private:
    static unsigned int __stdcall _ThreadRoute( LPVOID pRaram )
    {
        TimerDispatcher* pThis = (TimerDispatcher*)pRaram;
        pThis->_WorkThread();
        _endthreadex(0);
        return 0;
    }

    void _WorkThread()
    {
        DWORD dwWait = INFINITE;
        int waittime = -1;
        do 
        {
            _RunTask();

            waittime = -1;
            if (_CalcWaitTime(waittime))
            {
                if (waittime <= 0)
                {
                    waittime = 0;
                }
            }
            dwWait = INFINITE;
            if (waittime >= 0)
            {
                dwWait = waittime;
            }
            DWORD dwRet = ::WaitForSingleObject(event_, dwWait);
            if (dwRet != WAIT_TIMEOUT && dwRet != WAIT_OBJECT_0)
            {
                break;
            }
            if (dwRet == WAIT_OBJECT_0 && stop_)
            {
                break;
            }
        } while (true);
    }

    void _RunTask()
    {
        bool bRepeat = false;
        TimerTaskBase* pTask;
        ///> ִ�������ѵ�ʱ�������
        while (_GetTask(&pTask))
        {
            if (pWatcher_)
            {
                pWatcher_->RunTaskBegin(pTask);
            }

            pTask->DoWork();
            ///> ����Ҫ�ظ�ִ�е�����
            bRepeat = false;
            pTask->ExecutedAdd();
            if (pTask->Repeat()
                || pTask->Times() > pTask->Executed() )
            {
                _InsertTask(pTask);
                bRepeat = true;
            }

            if (pWatcher_)
            {
                pWatcher_->RunTaskEnd(pTask);
            }
            if (!bRepeat && pTask->Release())
            {
                delete pTask;
            }
        }
    }
    void _InsertTask(TimerTaskBase* pTask)
    {
        if (pWatcher_)
        {
            pWatcher_->InsertTask(pTask);
        }
        z_mutex_guard x(queue_mutex_);
        queue_.Insert(pTask);
    }
    bool _CalcWaitTime(int& waittime)
    {
        z_mutex_guard x(queue_mutex_);
        while (queue_.Size() > 0)
        {
            if (queue_.Top() == NULL)
            {
                continue;
            }
            if (queue_.Top()->Invalid())
            {
                TimerTaskBase* pTask = queue_.Pop();
                if (pWatcher_)
                {
                    pWatcher_->RemoveTask(pTask, false);
                }
                if (pTask->Release())
                {
                    delete pTask;
                }
                continue;
            }
            waittime = queue_.Top()->WaitTime();
            return true;
        }
        return false;
    }
    bool _GetTask(TimerTaskBase** ppTask)
    {
        z_mutex_guard x(queue_mutex_);
        while (queue_.Size() > 0)
        {
            if (queue_.Top() == NULL)
            {
                continue;
            }
            if (queue_.Top()->Invalid())
            {
                TimerTaskBase* pTask = queue_.Pop();
                if (pWatcher_)
                {
                    pWatcher_->RemoveTask(pTask, false);
                }
                if (pTask->Release())
                {
                    delete pTask;
                }
                continue;
            }
            int waittime = queue_.Top()->WaitTime();
            if (waittime <= 0)
            {
                *ppTask = queue_.Pop();
                if (pWatcher_)
                {
                    pWatcher_->RemoveTask(*ppTask, true);
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        return false;
    }

private:
    ITimerWatcher*      pWatcher_;          ///> �۲�����ִ��״��
    TimerTaskQueue      queue_;             ///> ��ʱ���������
    z_mutex             queue_mutex_;       ///> ��ʱ��������еķ�����
    HANDLE              event_;             ///> ֪ͨ�˳�����������
    HANDLE              thread_;            ///> �����߳�
    bool                stop_;              ///> �˳���־
};