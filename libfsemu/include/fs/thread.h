#ifndef FS_THREAD_H_
#define FS_THREAD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//void fs_thread_init();

struct fs_thread;
typedef struct fs_thread fs_thread;

typedef void *(*fs_thread_function)(void *);
fs_thread *fs_thread_create(fs_thread_function fn, void *data);
void *fs_thread_wait(fs_thread *thread);
void fs_thread_destroy(fs_thread *thread);

struct fs_mutex;
typedef struct fs_mutex fs_mutex;

fs_mutex *fs_mutex_create(void);
void fs_mutex_destroy(fs_mutex *mutex);
int fs_mutex_lock(fs_mutex *mutex);
int fs_mutex_unlock(fs_mutex *mutex);

struct fs_condition;
typedef struct fs_condition fs_condition;

fs_condition *fs_condition_create(void);
void fs_condition_destroy(fs_condition *condition);
int fs_condition_signal(fs_condition *condition);
int fs_condition_wait(fs_condition *condition, fs_mutex *mutex);
int fs_condition_timed_wait(fs_condition *condition, fs_mutex *mutex,
        int64_t real_time);

struct fs_semaphore;
typedef struct fs_semaphore fs_semaphore;

fs_semaphore *fs_semaphore_create(int value);
void fs_semaphore_destroy(fs_semaphore *semaphore);
int fs_semaphore_post(fs_semaphore *semaphore);
int fs_semaphore_wait(fs_semaphore *semaphore);
int fs_semaphore_try_wait(fs_semaphore *semaphore);

#ifdef __cplusplus
}
#endif

#endif // FS_THREAD_H_
