/**
 * threads.h
 * Bare bones cross-platform multithreading support implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_UTILS_THREADS_H
#define _GL_UTILS_THREADS_H

#include <stdbool.h>
#include <stdint.h>
#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Cross-platform multithreading function return codes.
 */
typedef enum {
	THREAD_OK = 0,
	THREAD_ERR_UNKNOWN
} thread_err_t;

/**
 * Cross-platform thread handle.
 */
#ifdef _WIN32
#else
typedef pthread_t* thread_t;
#endif /* _WIN32 */

/**
 * Cross-platform thread mutex handle.
 */
#ifdef _WIN32
#else
typedef pthread_mutex_t* thread_mutex_t;
#endif /* _WIN32 */

/**
 * Cross-platform thread procedure signature declaration.
 */
typedef void* (*thread_proc_t)(void *arg);

/* Threads */
thread_t thread_new(void);
thread_err_t thread_create(thread_t thread, thread_proc_t proc, void *arg);
thread_err_t thread_join(thread_t *thread, void **value_ptr);

/* Mutexes */
thread_mutex_t thread_mutex_new(void);
thread_err_t thread_mutex_lock(thread_mutex_t mutex);
thread_err_t thread_mutex_unlock(thread_mutex_t mutex);
thread_err_t thread_mutex_free(thread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTILS_THREADS_H */
