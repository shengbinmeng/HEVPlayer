#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

class Thread
{
public:
	Thread();
	~Thread();

	void						start();
    void						startAsync();
    int							join();

    void 						waitOnNotify();
    void						notify();
    virtual void				stop();

protected:
    bool						mRunning;

    virtual void                handleRun(void* ptr);
	
private:
    pthread_t                   mThread;
    pthread_mutex_t     		mLock;
    pthread_cond_t				mCondition;

	static void*				startThread(void* ptr);
};

#endif //__THREAD_H__
