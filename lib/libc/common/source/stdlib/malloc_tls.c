/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <errno.h>
#include <sys_malloc.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

Z_THREAD_LOCAL struct sys_heap_local *thread_heap;

#ifdef CONFIG_MULTITHREADING
static inline void malloc_lock()
{
	int lock_ret;

	lock_ret = sys_mutex_lock(&thread_heap->lock, K_FOREVER);
	__ASSERT_NO_MSG(lock_ret == 0);
}

static inline void malloc_unlock()
{
	(void)sys_mutex_unlock(&thread_heap->lock);
}
#else
#define malloc_lock()
#define malloc_unlock()
#endif /* CONFIG_MULTITHREADING */

#ifdef CONFIG_USERSPACE
static inline bool is_kernel_thread()
{
	if (arch_is_user_context()) {
		return false;
	}
	return (k_current_get()->base.user_options & K_USER) == 0;
}
#else
static inline bool is_kernel_thread()
{
	return true;
}
#endif /* CONFIG_USERSPACE */

static inline bool fallback_to_system_heap()
{
#ifdef CONFIG_COMMON_LIBC_MALLOC_TLS_FALLBACK
	return is_kernel_thread();
#else
	return false;
#endif /* CONFIG_COMMON_LIBC_MALLOC_TLS_FALLBACK */
}

void *aligned_alloc(size_t align, size_t size)
{
	void *ret = NULL;

	if (thread_heap != NULL) {
		malloc_lock();
		ret = sys_heap_aligned_alloc(&thread_heap->heap, align, size);
		malloc_unlock();
	} else if (fallback_to_system_heap()) {
		ret = k_aligned_alloc(align, size);
	}

	if (ret == NULL && size != 0) {
		errno = ENOMEM;
	}

	return ret;
}

void *malloc(size_t size)
{
	return aligned_alloc(__alignof__(z_max_align_t), size);
}

void *realloc(void *ptr, size_t requested_size)
{
	void *ret = NULL;

	if (thread_heap != NULL) {
		malloc_lock();
		ret = sys_heap_aligned_realloc(&thread_heap->heap, ptr, __alignof__(z_max_align_t),
					       requested_size);
		malloc_unlock();
	} else if (fallback_to_system_heap()) {
		ret = k_realloc(ptr, requested_size);
	}

	if (ret == NULL && requested_size != 0) {
		errno = ENOMEM;
	}

	return ret;
}

void free(void *ptr)
{
	if (thread_heap != NULL) {
		malloc_lock();
		sys_heap_free(&thread_heap->heap, ptr);
		malloc_unlock();
	} else if (fallback_to_system_heap()) {
		k_free(ptr);
	}
}

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
int malloc_runtime_stats_get(struct sys_memory_stats *stats)
{
	int ret = -ENOTSUP;

	if (thread_heap != NULL) {
		malloc_lock();
		ret = sys_heap_runtime_stats_get(&thread_heap->heap, stats);
		malloc_unlock();
	}

	return ret;
}
#endif /* CONFIG_SYS_HEAP_RUNTIME_STATS */
