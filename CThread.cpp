/*
NIISim - Nios II Simulator, A simulator that is capable of simulating various systems containing Nios II cpus.
Copyright (C) 2012 Emil Lenngren

This file is part of NIISim.

NIISim is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

NIISim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with NIISim.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CThread.h"

CThread::CThread(void *(*func)(void*))
{
	init(func);
}

void CThread::init(void *(*func)(void*))
{
	inited = true;
#ifndef WINNT
	pthread_create(&thread, NULL, func, NULL);
#else
	thread_handle = CreateThread(
					NULL,                   // default security attributes
					0,                      // use default stack size  
					(LPTHREAD_START_ROUTINE)func,       // thread function name
					NULL,          // argument to thread function 
					0,                      // use default creation flags 
					&threadID);   // returns the thread identifier 
#endif
}

void CThread::join()
{
	if(inited){
#ifndef WINNT
		pthread_join(thread, NULL);
#else
		CloseHandle(thread_handle);
#endif
		inited = false;
	}
}

CThread::~CThread()
{
	join();
}
