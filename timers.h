/****************************************************************
 * zherebjatjew@gmail.com
 ****************************************************************/
#ifndef TIMERS_H
#define TIMERS_H


typedef void (*DeferredFunction)(void* data, int* callsRemained);
typedef struct {} * handle;

#define REPEAT_INFINITELY (-1)
#define TIMER_RESOLUTION_MS 10


/**
 * The function must be called before any function of the library
 */
void timers_init();

/**
 * The function releases data allocated by other functions of the library and cancels all schedules
 */
void timers_close();

/**
 * Schedule <foo> to be called every <period_ms> milliseconds <repeat> times.
 * <p>
 * You can change remained number of calls by assignment desired value to <calls_remained> from within the foo.
 * </p>
 * @param foo Function to execute. The function will take <data> and pointer to remained call counter as arguments
 * @param period_ms Interval between calls in milliseconds. Foo will be called first time in <period_ms>.
 * @param data Pointer to any custom data what will be passed to <foo> every time it's executed.
 * @param repeat How many times to execute <foo>. Can be changed from within <foo>.
 * @return Handle to schedule record of NULL if invalid arguments were supplied. Can be used by {@link timers_cancel} to cancel particular schedule.
 */
handle timers_schedule(DeferredFunction foo, int period_ms, void* data = NULL, int repeat = REPEAT_INFINITELY);

/**
 * Cancel single schedule
 * @param schedule Handle to a schedule returned by previous call to {@link timers_schedule}
 * @return True if given schedule was found and successfully canceled.
 */
bool timers_cancel(handle schedule);


#endif
