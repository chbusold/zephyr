/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/mutex.h>
#include <zephyr/internal/syscall_handler.h>

/* Use the LSB of the futex value as indicator that there are waiters;
 * TID is the address of the thread object, which should be 4-byte aligned, and
 * therefore leave the lower two bits free */
#define HAS_WAITER BIT(0)

void sys_mutex_init(struct sys_mutex *mutex)
{
	(void)atomic_set(&mutex->futex.val, 0);
}

int sys_mutex_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	int ret = 0;
	atomic_t current_value, new_value, target_value;
	k_tid_t tid = k_current_get();
	atomic_t *target = &mutex->futex.val;

	target_value = (atomic_t)tid;
	do {
		/* Try to lock mutex; if this works, we are the owner */
		if (atomic_cas(target, 0, target_value)) {
			return 0;
		}

		current_value = atomic_get(target);
		/* In the unlikely case that it got unlocked just after our attempt,
		 * try locking again */
		if (unlikely(current_value == 0)) {
			continue;
		}

		/* Indicate that the mutex is contended */
		target_value |= HAS_WAITER;
		new_value = current_value | HAS_WAITER;
		/* If no other thread is waiting already, set indicator before starting
		 * to wait, so that the owner will call k_futex_wake when unlocking */
		if (new_value != current_value) {
			(void)atomic_cas(target, current_value, new_value);
		}

		ret = k_futex_wait(&mutex->futex, new_value, timeout);
	} while (ret == 0 || ret == -EAGAIN);

	return ret;
}

int sys_mutex_unlock(struct sys_mutex *mutex)
{
	int ret = 0;
	atomic_t current_value;
	k_tid_t tid = k_current_get();
	atomic_t *target = &mutex->futex.val;

	/* Try to release without wait indicator; if this succeeds, we can return
	 * directly because no one was waiting */
	current_value = (atomic_t)tid;
	if (atomic_cas(target, current_value, 0)) {
		return 0;
	}

	/* Try to release with wait indicator; if this succeeds, wake up the next
	 * waiting thread */
	current_value |= HAS_WAITER;
	if (likely(atomic_cas(target, current_value, 0))) {
		ret = k_futex_wake(&mutex->futex, false);
		return ret >= 0 ? 0 : ret;
	}

	/* If neither worked, some other thread must be the owner */
	return -EPERM;
}
