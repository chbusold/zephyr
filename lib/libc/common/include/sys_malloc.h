/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr common libc malloc extensions
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_INCLUDE_SYS_MALLOC_H_
#define ZEPHYR_LIB_LIBC_COMMON_INCLUDE_SYS_MALLOC_H_

#include <zephyr/sys/sys_heap.h>

#ifdef CONFIG_MULTITHREADING
#include <zephyr/sys/mutex.h>
#endif /* CONFIG_MULTITHREADING */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the runtime statistics of the malloc heap
 *
 * @kconfig_dep{CONFIG_SYS_HEAP_RUNTIME_STATS}
 *
 * @param stats Pointer to struct to copy statistics into
 * @return -EINVAL if null pointers, otherwise 0
 */
int malloc_runtime_stats_get(struct sys_memory_stats *stats);

#ifdef CONFIG_COMMON_LIBC_MALLOC_TLS
struct sys_heap_local {
    struct sys_heap heap;
#ifdef CONFIG_MULTITHREADING
    struct sys_mutex lock;
#endif
};

/**
 * @brief Initialize a local malloc heap
 *
 * @param heap Pointer to heap struct
 * @param mem Pointer to memory for heap
 * @param size Size of memory for heap
 */
static inline void sys_heap_local_init(struct sys_heap_local *heap, void *mem, size_t size)
{
#ifdef CONFIG_MULTITHREADING
    sys_mutex_init(&heap->lock);
#endif /* CONFIG_MULTITHREADING */
    sys_heap_init(&heap->heap, mem, size);
}
#endif /* CONFIG_COMMON_LIBC_MALLOC_TLS */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_COMMON_INCLUDE_SYS_MALLOC_H_ */
