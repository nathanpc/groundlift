/**
 * threads.c
 * Bare bones cross-platform multithreading support implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "threads.h"

#include <stdlib.h>

/**
 * Allocates and initializes a brand new thread object.
 *
 * @return Newly allocated an initialized thread object.
 *
 * @see thread_create
 */
thread_t thread_new(void) {
	thread_t thread;

#ifdef _WIN32
#error Not yet implemented.
#else
	thread = (thread_t)malloc(sizeof(thread_t));
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
#error Not yet implemented.
#else
	return (thread_err_t)pthread_create(thread, NULL, proc, arg);
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
#error Not yet implemented.
#else
	/* Join the thread. */
	err = (thread_err_t)pthread_join(**thread, value_ptr);

	/* Free the thread handle pointer. */
	free(*thread);
	*thread = NULL;
#endif /* _WIN32 */

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
#error Not yet implemented.
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
#error Not yet implemented.
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
#error Not yet implemented.
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
	/* Do we even have anything to do? */
	if ((mutex == NULL) || (*mutex == NULL))
		return THREAD_OK;

#ifdef _WIN32
#error Not yet implemented.
#else
	/* Free our mutex. */
	free(*mutex);
	*mutex = NULL;

	return THREAD_OK;
#endif /* _WIN32 */
}
