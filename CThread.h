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

/*

This file implements a platform-independent wrapper around threads, mutexes and sleep.

*/
#include <pthread.h>
#include <glib.h>
#include <time.h>
#include <sys/time.h>

#ifndef _CTHREAD_H_
#define _CTHREAD_H_

class CThread
{
private:
#ifdef WINNT
	HANDLE thread_handle;
	DWORD threadID;
#else
	pthread_t thread;
#endif
	bool inited;
public:
	CThread() : inited(false) {}
	CThread(void *(*func)(void*));
	void init(void *(*func)(void*));
	~CThread();
	
	static void Sleep(unsigned long ms){
#ifdef WINNT
		::Sleep(ms);
#else
		struct timespec waiting_time = {ms / 1000U, (ms % 1000U)*1000000}; // 100 ms
		nanosleep(&waiting_time, NULL);
#endif
	}
	
	void join();
};

class CMutex
{
private:
	GMutex *mutex;
	bool inited;
public:
	CMutex() {
		mutex = NULL;
		inited = false;
	}
	~CMutex() {
		if (inited)
			g_mutex_free(mutex);
	}
	
	void lock(){
		if(!inited){
			mutex = g_mutex_new();
			inited = true;
		}
		g_mutex_lock(mutex);
	}
	void unlock(){ g_mutex_unlock(mutex); }
};

#endif
