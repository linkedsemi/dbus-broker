/*
 * Sampler Helper
 *
 * The sampler object is used to compute the min/max/avg/std deviation of
 * samples of CPU time, in fixed size and without memory allocations.
 *
 * The values of min/max/avg are meant to be read out of the struct directly,
 * whereas the standard deviation can only be accessed using a helper function
 * (as it is not actually stored directly, but computed on-demand).
 *
 * Only one sample may be active at any point in time, and every sample that is
 * started, must be stopped.
 *
 * See `Note on a Method for Calculating Corrected Sums of Squares and Products'
 * by W. P. Welford, 1962.
 */

#include <c-stdaux.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "util/sampler.h"

void sampler_init(Sampler *sampler, clockid_t id) {
        *sampler = (Sampler)SAMPLER_INIT(id);
}

void sampler_deinit(Sampler *sampler) {
        c_assert(sampler->timestamp == SAMPLER_TIMESTAMP_INVALID);
        sampler_init(sampler, sampler->id);
}

/**
 * sampler_get_time() - get the current thread CPU time
 *
 * Read the current thread CPU time to be used to record samples.
 *
 * Return: the timestamp in nano seconds.
 */
uint64_t sampler_get_time(Sampler *sampler) {
        struct timespec ts;
        int r;
        clockid_t id_copy;

        // printf("DEBUG sampler_get_time: sampler=%p, sampler alignment=%ld\n",
        //        (void *)sampler, (long)((uintptr_t)sampler % 8));
        /* Use memcpy to avoid alignment issues with clockid_t */
        memcpy(&id_copy, &sampler->id, sizeof(clockid_t));
        // printf("DEBUG sampler_get_time: sampler->id=%d\n", id_copy);
#ifdef __ZEPHYR__
        /* On Zephyr, thread CPU time is not supported. Fall back to CLOCK_MONOTONIC. */
        r = clock_gettime(CLOCK_MONOTONIC, &ts);
#else
        r = clock_gettime(id_copy, &ts);
#endif
        // printf("DEBUG sampler_get_time: clock_gettime returned %d\n", r);
        c_assert(r >= 0);

        // printf("DEBUG sampler_get_time: ts.tv_sec=%ld, ts.tv_nsec=%ld\n", ts.tv_sec, ts.tv_nsec);
        return ts.tv_sec * UINT64_C(1000000000) + ts.tv_nsec;
}

/**
 * sampler_sample_add() - add one sample
 * @sampler:            object to operate on
 * @timestamp:          time the sample was started
 *
 * Update the internal state with a new sample, started at @timestamp
 * and ending at the time the function is called.
 */
void sampler_sample_add(Sampler *sampler, uint64_t timestamp) {
        uint64_t sample, average_old;

        sample = sampler_get_time(sampler) - timestamp;

        sampler->count ++;
        sampler->sum += sample;

        average_old = sampler->average;
        sampler->average = sampler->sum / sampler->count;
        sampler->sum_of_squares += (sample - average_old) * (sample - sampler->average);

        if (sampler->minimum > sample)
                sampler->minimum = sample;

        if (sampler->maximum < sample)
                sampler->maximum = sample;
}

/**
 * sampler_sample_start() - start a new sample
 * @sampler:            object to operate on
 *
 * Start a new sample by recording the current timestamp, verifying that
 * a sample is not currently running.
 */
void sampler_sample_start(Sampler *sampler) {
        uint64_t time_value;

        // printf("DEBUG sampler_sample_start: ENTRY sampler=%p\n", (void *)sampler);
        // printf("DEBUG sampler_sample_start: sampler alignment=%ld\n", (long)((uintptr_t)sampler % 8));
        // printf("DEBUG sampler_sample_start: timestamp=%llu\n", sampler->timestamp);
        c_assert(sampler->timestamp == SAMPLER_TIMESTAMP_INVALID);
        // printf("DEBUG sampler_sample_start: About to call sampler_get_time\n");
        time_value = sampler_get_time(sampler);
        // printf("DEBUG sampler_sample_start: time_value=%llu\n", time_value);
        // printf("DEBUG sampler_sample_start: About to write timestamp\n");
        sampler->timestamp = time_value;
        // printf("DEBUG sampler_sample_start: timestamp=%llu after write\n", sampler->timestamp);
        // printf("DEBUG sampler_sample_start: ABOUT TO RETURN\n");
}

/**
 * sampler_sample_end() - end a running sample
 * @sampler:            object to operate on
 *
 * End a currently running sample, and update the internal state.
 */
void sampler_sample_end(Sampler *sampler) {
        c_assert(sampler->timestamp != SAMPLER_TIMESTAMP_INVALID);

        sampler_sample_add(sampler, sampler->timestamp);

        sampler->timestamp = SAMPLER_TIMESTAMP_INVALID;
}

/**
 * sampler_read_standard_deviation() - read out the current standard deviation
 * @sampler:            objcet to operate on
 *
 * This computes and returns the standard deviation of the samples recorded
 * so far. The standard devitaion is not stored internally, but computed
 * on-demand.
 *
 * If the standard deviation is not defined (no samples were taken), then
 * zero is returned.
 *
 * Return: the standard deviation, or 0 if not defined.
 */
double sampler_read_standard_deviation(Sampler *sampler) {
        if (!sampler->count)
                return 0;

        return sqrt(sampler->sum_of_squares / sampler->count);
}
