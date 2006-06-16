#include "glapi.h"
#include "glThread.h"

#ifdef WIN32_THREADS
extern "C" _glthread_Mutex OneTimeLock;
extern "C" _glthread_Mutex GenTexturesLock;

extern "C" void FreeAllTSD(void);

class _CriticalSectionInit
{
public:
	static _CriticalSectionInit	m_inst;

	_CriticalSectionInit()
	{
		_glthread_INIT_MUTEX(OneTimeLock);
		_glthread_INIT_MUTEX(GenTexturesLock);
	}

	~_CriticalSectionInit()
	{
		_glthread_DESTROY_MUTEX(OneTimeLock);
		_glthread_DESTROY_MUTEX(GenTexturesLock);
		FreeAllTSD();
	}
};

_CriticalSectionInit _CriticalSectionInit::m_inst;


#endif
