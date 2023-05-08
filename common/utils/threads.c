/**
 * threads.c
 * Bare bones cross-platform multithreading support implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "threads.h"

#include <stdlib.h>

#ifdef _WIN32
/* Thread function wrapper for Windows. */
void thread_func_wrapper(void *arg);
#endif /* _WIN32 */

/**
 * Allocates and initializes a brand new thread object.
 *
 * @return Newly allocated an initialized thread object.
 *
 * @see thread_create
 */
thread_t thread_new(void) {
	thread_t thread;

	/* Allocate some memory for our handle object */
	thread = (thread_t)malloc(sizeof(thread_t));

	/* Ensure that we start with a known invalid state. */
#ifdef _WIN32
	thread->hnd = (HANDLE)-1;
	thread->running = false;
	thread->proc = NULL;
	thread->proc_arg = NULL;
	thread->retval = NULL;
#endif /* _WIN32 */

	return thread;
}

/**
 * Creates and starts a new thread.
 *
 * @param thread Thread handle.
 * @param proc   Procedure to be executed in the separate thread.
 * @param arg    Argument passed to the thread procedure.
 *
 * @return THREAD_OK if the operation was successful.
 *
 * @see thread_join
 */
thread_err_t thread_create(thread_t thread, thread_proc_t proc, void *arg) {
#ifdef _WIN32
	/* Setup our thread object. */
	thread->proc = proc;
	thread->proc_arg = arg;

	/* Create the new thread. */
	thread->hnd = (HANDLE)_beginthreadex(thread_func_wrapper, 0,
		(void *)thread, NULL, 0, NULL);
	if (thread->hnd == (HANDLE)-1)
		return THREAD_ERR_UNKNOWN;

	return THREAD_OK;
#else
	return (thread_err_t)pthread_create(&thread->hnd, NULL, proc, arg);
#endif /* _WIN32 */
}

/**
 * Waits for a thread to finish its execution and frees up any resources
 * allocated by the thread handle.
 *
 * @param thread    Thread handle.
 * @param value_ptr Pointer to a value returned by the thread procedure.
 *
 * @return THREAD_OK if the operation was successful.
 *
 * @see thread_create
 */
thread_err_t thread_join(thread_t *thread, void **value_ptr) {
	thread_err_t err;

	/* Do we even have anything to do? */
	if ((thread == NULL) || (*thread == NULL)) {
		if (value_ptr)
			*value_ptr = NULL;

		return THREAD_OK;
	}

#ifdef _WIN32
	/* Wait for the thread to procedure to return. */
	WaitForSingleObject((*thread)->hnd, INFINITE);
	CloseHandle((*thread)->hnd);

	/* Get our return value or free it. */
	if (value_ptr) {
		*value_ptr = (*thread)->retval;
	} else {
		free((*thread)->retval);
	}

	err = THREAD_OK;
#else
	/* Join the thread. */
	err = (thread_err_t)pthread_join((*thread)->hnd, value_ptr);
#endif /* _WIN32 */

	/* Free the thread handle pointer. */
	free(*thread);
	*thread = NULL;

	return err;
}

/**
 * Allocates and initializes a brand new mutex handle.
 *
 * @return Newly allocated an initialized mutex handle.
 *
 * @see thread_mutex_free
 */
thread_mutex_t thread_mutex_new(void) {
	thread_mutex_t mutex;

#ifdef _WIN32
	mutex = (thread_mutex_t)malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(mutex);
#else
	mutex = (thread_mutex_t)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
#endif /* _WIN32 */

	return mutex;
}

/**
 * Locks the referenced mutex.
 *
 * @param mutex Mutex object to be locked.
 *
 * @return THREAD_OK if the operation was successful.
 *
 * @see thread_mutex_unlock
 */
thread_err_t thread_mutex_lock(thread_mutex_t mutex) {
#ifdef _WIN32
	EnterCriticalSection(mutex);
	return THREAD_OK;
#else
	return (thread_err_t)pthread_mutex_lock(mutex);
#endif /* _WIN32 */
}

/**
 * Unlocks the referenced mutex.
 *
 * @param mutex Mutex object to be unlocked.
 *
 * @return THREAD_OK if the operation was successful.
 *
 * @see thread_mutex_lock
 */
thread_err_t thread_mutex_unlock(thread_mutex_t mutex) {
#ifdef _WIN32
	LeaveCriticalSection(mutex);
	return THREAD_OK;
#else
	return (thread_err_t)pthread_mutex_unlock(mutex);
#endif /* _WIN32 */
}

/**
 * Frees up any resources allocated by a mutex.
 *
 * @param mutex Pointer to mutex object to be free'd.
 *
 * @return THREAD_OK if the operation was successful.
 *
 * @see thread_mutex_new
 */
thread_err_t thread_mutex_free(thread_mutex_t *mutex) {
	thread_err_t err;

	/* Do we even have anything to do? */
	if ((mutex == NULL) || (*mutex == NULL))
		return THREAD_OK;

	/* Destroy the mutex. */
#ifdef _WIN32
	DeleteCriticalSection(*mutex);
	err = THREAD_OK;
#else
	err = (thread_err_t)pthread_mutex_destroy(*mutex);
#endif /* _WIN32 */

	/* Free our resources. */
	free(*mutex);
	*mutex = NULL;

	return err;
}

#ifdef _WIN32
/**
 * Wrapper function to allow Windows threads to return a value from the thread
 * procedure.
 *
 * @param arg Thread handle object.
 */
void thread_func_wrapper(void *arg) {
	thread_t thread;

	/* Get the thread handle object from the passed argument. */
	thread = (thread_t)arg;

	/* Call the intended thread procedure. */
	thread->running = true;
	thread->retval = thread->proc(thread->proc_arg);
	thread->running = false;

	/* Ensure that the thread resources are free'd. */
	_endthreadex(0);
}
#endif /* _WIN32 */
