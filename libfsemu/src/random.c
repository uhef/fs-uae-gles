#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Originally developed and coded by Makoto Matsumoto and Takuji
 * Nishimura.  Please mail <matumoto@math.keio.ac.jp>, if you're using
 * code from this file in your own programs or libraries.
 * Further information on the Mersenne Twister can be found at
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 * This code was adapted to glib by Sebastian Wilhelmi.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include <fs/base.h>
#include <fs/init.h>
#include <fs/thread.h>
#include <fs/random.h>

//#include "config.h"

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 #include "grand.h"

 #include "genviron.h"
 #include "gmain.h"
 #include "gmem.h"
 #include "gtestutils.h"
 #include "gthread.h"
 */

#ifdef WINDOWS
#include <process.h>        /* For getpid() */
#endif

/**
 * SECTION:random_numbers
 * @title: Random Numbers
 * @short_description: pseudo-random number generator
 *
 * The following functions allow you to use a portable, fast and good
 * pseudo-random number generator (PRNG). It uses the Mersenne Twister
 * PRNG, which was originally developed by Makoto Matsumoto and Takuji
 * Nishimura. Further information can be found at
 * <ulink url="http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html">
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html</ulink>.
 *
 * If you just need a random number, you simply call the
 * <function>g_random_*</function> functions, which will create a
 * globally used #fs_rand_context and use the according
 * <function>g_rand_*</function> functions internally. Whenever you
 * need a stream of reproducible random numbers, you better create a
 * #fs_rand_context yourself and use the <function>g_rand_*</function> functions
 * directly, which will also be slightly faster. Initializing a #fs_rand_context
 * with a certain seed will produce exactly the same series of random
 * numbers on all platforms. This can thus be used as a seed for e.g.
 * games.
 *
 * The <function>g_rand*_range</function> functions will return high
 * quality equally distributed random numbers, whereas for example the
 * <literal>(g_random_int()&percnt;max)</literal> approach often
 * doesn't yield equally distributed numbers.
 *
 * GLib changed the seeding algorithm for the pseudo-random number
 * generator Mersenne Twister, as used by
 * <structname>fs_rand_context</structname> and <structname>fs_rand_contextom</structname>.
 * This was necessary, because some seeds would yield very bad
 * pseudo-random streams.  Also the pseudo-random integers generated by
 * <function>g_rand*_int_range()</function> will have a slightly better
 * equal distribution with the new version of GLib.
 *
 * The original seeding and generation algorithms, as found in GLib
 * 2.0.x, can be used instead of the new ones by setting the
 * environment variable <envar>G_RANDOM_VERSION</envar> to the value of
 * '2.0'. Use the GLib-2.0 algorithms only if you have sequences of
 * numbers generated with Glib-2.0 that you need to reproduce exactly.
 **/

/**
 * fs_rand_context:
 *
 * The #fs_rand_context struct is an opaque data structure. It should only be
 * accessed through the <function>g_rand_*</function> functions.
 **/

//G_LOCK_DEFINE_STATIC( global_random);
static fs_rand_context *g_global_random = NULL;
static fs_mutex *g_global_random_mutex = NULL;

FS_INIT_FUNCTION(global_random) {
    printf("\n\n\n\n\n\n\n\n random init\n\n\n\n\n");
    g_global_random_mutex = fs_mutex_create();
}

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned int get_random_version(void) {
    static unsigned int random_version = 22;
    return random_version;
}

struct fs_rand_context {
    uint32_t mt[N]; /* the array for the state vector  */
    unsigned int mti;
};

/**
 * fs_rand_new_with_seed:
 * @seed: a value to initialize the random number generator.
 *
 * Creates a new random number generator initialized with @seed.
 *
 * Return value: the new #fs_rand_context.
 **/
fs_rand_context *fs_rand_new_with_seed(uint32_t seed) {
    fs_rand_context *rand = fs_new0(fs_rand_context, 1);
    fs_rand_set_seed(rand, seed);
    return rand;
}

/**
 * fs_rand_new_with_seed_array:
 * @seed: an array of seeds to initialize the random number generator.
 * @seed_length: an array of seeds to initialize the random number generator.
 *
 * Creates a new random number generator initialized with @seed.
 *
 * Return value: the new #fs_rand_context.
 *
 * Since: 2.4
 **/
fs_rand_context *fs_rand_new_with_seed_array(const uint32_t *seed,
        unsigned int seed_length) {
    fs_rand_context *rand = fs_new0(fs_rand_context, 1);
    fs_rand_set_seed_array(rand, seed, seed_length);
    return rand;
}

/**
 * fs_rand_new:
 *
 * Creates a new random number generator initialized with a seed taken
 * either from <filename>/dev/urandom</filename> (if existing) or from
 * the current time (as a fallback).
 *
 * Return value: the new #fs_rand_context.
 **/
fs_rand_context *fs_rand_new(void) {
    uint32_t seed[4];
    fs_time_val now;
#ifdef G_OS_UNIX
    static int dev_urandom_exists = TRUE;

    if (dev_urandom_exists)
    {
        FILE* dev_urandom;

        do
        {
            errno = 0;
            dev_urandom = fopen("/dev/urandom", "rb");
        }
        while G_UNLIKELY (errno == EINTR);

        if (dev_urandom)
        {
            int r;

            setvbuf (dev_urandom, NULL, _IONBF, 0);
            do
            {
                errno = 0;
                r = fread (seed, sizeof (seed), 1, dev_urandom);
            }
            while G_UNLIKELY (errno == EINTR);

            if (r != 1)
            dev_urandom_exists = FALSE;

            fclose (dev_urandom);
        }
        else
        dev_urandom_exists = FALSE;
    }
#else
    static int dev_urandom_exists = FALSE;
#endif

    if (!dev_urandom_exists) {
        fs_get_current_time(&now);
        seed[0] = now.tv_sec;
        seed[1] = now.tv_usec;
        seed[2] = getpid();
#ifdef G_OS_UNIX
        seed[3] = getppid ();
#else
        seed[3] = 0;
#endif
    }

    return fs_rand_new_with_seed_array(seed, 4);
}

/**
 * g_rand_free:
 * @rand_: a #fs_rand_context.
 *
 * Frees the memory allocated for the #fs_rand_context.
 **/
void fs_rand_free(fs_rand_context *rand) {
    if (rand == NULL) {
        return;
    }
    free(rand);
}

/**
 * g_rand_copy:
 * @rand_: a #fs_rand_context.
 *
 * Copies a #fs_rand_context into a new one with the same exact state as before.
 * This way you can take a snapshot of the random number generator for
 * replaying later.
 *
 * Return value: the new #fs_rand_context.
 *
 * Since: 2.4
 **/
fs_rand_context *fs_rand_copy(fs_rand_context *rand) {
    fs_rand_context *new_rand;

    if (rand == NULL) {
        return NULL;
    }

    new_rand = fs_new0(fs_rand_context, 1);
    memcpy(new_rand, rand, sizeof(fs_rand_context));

    return new_rand;
}

/**
 * g_rand_set_seed:
 * @rand_: a #fs_rand_context.
 * @seed: a value to reinitialize the random number generator.
 *
 * Sets the seed for the random number generator #fs_rand_context to @seed.
 **/
void fs_rand_set_seed(fs_rand_context *rand, uint32_t seed) {
    if (rand == NULL) {
        return;
    }

    switch (get_random_version()) {
    case 20:
        /* setting initial seeds to mt[N] using         */
        /* the generator Line 25 of Table 1 in          */
        /* [KNUTH 1981, The Art of Computer Programming */
        /*    Vol. 2 (2nd Ed.), pp102]                  */

        if (seed == 0) /* This would make the PRNG produce only zeros */
        seed = 0x6b842128; /* Just set it to another number */

        rand->mt[0] = seed;
        for (rand->mti = 1; rand->mti < N; rand->mti++)
            rand->mt[rand->mti] = (69069 * rand->mt[rand->mti - 1]);

        break;
    case 22:
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous version (see above), MSBs of the    */
        /* seed affect only MSBs of the array mt[].            */

        rand->mt[0] = seed;
        for (rand->mti = 1; rand->mti < N; rand->mti++)
            rand->mt[rand->mti] =
                    1812433253UL
                            * (rand->mt[rand->mti - 1]
                                    ^ (rand->mt[rand->mti - 1] >> 30))
                            + rand->mti;
        break;
    //default:
    //    g_assert_not_reached();
    }
}

/**
 * g_rand_set_seed_array:
 * @rand_: a #fs_rand_context.
 * @seed: array to initialize with
 * @seed_length: length of array
 *
 * Initializes the random number generator by an array of
 * longs.  Array can be of arbitrary size, though only the
 * first 624 values are taken.  This function is useful
 * if you have many low entropy seeds, or if you require more then
 * 32bits of actual entropy for your application.
 *
 * Since: 2.4
 **/
void fs_rand_set_seed_array(fs_rand_context *rand, const uint32_t *seed,
        unsigned int seed_length) {
    int i, j, k;

    if (rand == NULL) {
        return;
    }
    if (seed_length < 1) {
        return;
    }

    fs_rand_set_seed(rand, 19650218UL);

    i = 1;
    j = 0;
    k = (N > seed_length ? N : seed_length);
    for (; k; k--) {
        rand->mt[i] = (rand->mt[i]
                ^ ((rand->mt[i - 1] ^ (rand->mt[i - 1] >> 30)) * 1664525UL))
                + seed[j] + j; /* non linear */
        rand->mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        j++;
        if (i >= N) {
            rand->mt[0] = rand->mt[N - 1];
            i = 1;
        }
        if (j >= seed_length) j = 0;
    }
    for (k = N - 1; k; k--) {
        rand->mt[i] = (rand->mt[i]
                ^ ((rand->mt[i - 1] ^ (rand->mt[i - 1] >> 30)) * 1566083941UL))
                - i; /* non linear */
        rand->mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i >= N) {
            rand->mt[0] = rand->mt[N - 1];
            i = 1;
        }
    }

    rand->mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
}

/**
 * g_rand_int:
 * @rand_: a #fs_rand_context.
 *
 * Returns the next random #uint32_t from @rand_ equally distributed over
 * the range [0..2^32-1].
 *
 * Return value: A random number.
 **/
uint32_t fs_rand_int(fs_rand_context *rand) {
    uint32_t y;
    static const uint32_t mag01[2] = { 0x0, MATRIX_A };
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (rand == NULL) {
        return 0;
    }

    if (rand->mti >= N) { /* generate N words at one time */
        int kk;

        for (kk = 0; kk < N - M; kk++) {
            y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk + 1] & LOWER_MASK);
            rand->mt[kk] = rand->mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        for (; kk < N - 1; kk++) {
            y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk + 1] & LOWER_MASK);
            rand->mt[kk] = rand->mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (rand->mt[N - 1] & UPPER_MASK) | (rand->mt[0] & LOWER_MASK);
        rand->mt[N - 1] = rand->mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];

        rand->mti = 0;
    }

    y = rand->mt[rand->mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    return y;
}

/* transform [0..2^32] -> [0..1] */
#define FS_RAND_DOUBLE_TRANSFORM 2.3283064365386962890625e-10

/**
 * g_rand_int_range:
 * @rand_: a #fs_rand_context.
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns the next random #int32_t from @rand_ equally distributed over
 * the range [@begin..@end-1].
 *
 * Return value: A random number.
 **/
int32_t fs_rand_int_range(fs_rand_context *rand, int32_t begin, int32_t end) {
    uint32_t dist = end - begin;
    uint32_t random;

    if (rand == NULL) {
        return begin;
    }
    if (end <= begin) {
        return begin;
    }

    switch (get_random_version()) {
    case 20:
        if (dist <= 0x10000L) /* 2^16 */
        {
            /* This method, which only calls g_rand_int once is only good
             * for (end - begin) <= 2^16, because we only have 32 bits set
             * from the one call to g_rand_int (). */

            /* we are using (trans + trans * trans), because g_rand_int only
             * covers [0..2^32-1] and thus g_rand_int * trans only covers
             * [0..1-2^-32], but the biggest double < 1 is 1-2^-52.
             */

            double double_rand = fs_rand_int(rand)
                    * (FS_RAND_DOUBLE_TRANSFORM
                            + FS_RAND_DOUBLE_TRANSFORM
                                    * FS_RAND_DOUBLE_TRANSFORM);

            random = (int32_t) (double_rand * dist);
        }
        else {
            /* Now we use g_rand_double_range (), which will set 52 bits for
             us, so that it is safe to round and still get a decent
             distribution */
            random = (int32_t) fs_rand_double_range(rand, 0, dist);
        }
        break;
    case 22:
        if (dist == 0) random = 0;
        else {
            /* maxvalue is set to the predecessor of the greatest
             * multiple of dist less or equal 2^32. */
            uint32_t maxvalue;
            if (dist <= 0x80000000u) /* 2^31 */
            {
                /* maxvalue = 2^32 - 1 - (2^32 % dist) */
                uint32_t leftover = (0x80000000u % dist) * 2;
                if (leftover >= dist) leftover -= dist;
                maxvalue = 0xffffffffu - leftover;
            }
            else maxvalue = dist - 1;

            do
                random = fs_rand_int(rand);
            while (random > maxvalue);

            random %= dist;
        }
        break;
    default:
        random = 0; /* Quiet GCC */
        //g_assert_not_reached();
    }

    return begin + random;
}

/**
 * g_rand_double:
 * @rand_: a #fs_rand_context.
 *
 * Returns the next random #double from @rand_ equally distributed over
 * the range [0..1).
 *
 * Return value: A random number.
 **/
double fs_rand_double(fs_rand_context *rand) {
    /* We set all 52 bits after the point for this, not only the first
     32. Thats why we need two calls to g_rand_int */
    double retval = fs_rand_int(rand) * FS_RAND_DOUBLE_TRANSFORM;
    retval = (retval + fs_rand_int(rand)) * FS_RAND_DOUBLE_TRANSFORM;

    /* The following might happen due to very bad rounding luck, but
     * actually this should be more than rare, we just try again then */
    if (retval >= 1.0) return fs_rand_double(rand);

    return retval;
}

/**
 * g_rand_double_range:
 * @rand_: a #fs_rand_context.
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns the next random #double from @rand_ equally distributed over
 * the range [@begin..@end).
 *
 * Return value: A random number.
 **/
double fs_rand_double_range(fs_rand_context *rand, double begin, double end) {
    double r;

    r = fs_rand_double(rand);

    return r * end - (r - 1) * begin;
}

/**
 * g_random_int:
 *
 * Return a random #uint32_t equally distributed over the range
 * [0..2^32-1].
 *
 * Return value: A random number.
 **/
uint32_t fs_random_int(void) {
    FS_INIT(global_random);
    uint32_t result;
    fs_mutex_lock(g_global_random_mutex);
    if (!g_global_random) g_global_random = fs_rand_new();

    result = fs_rand_int(g_global_random);
    fs_mutex_unlock(g_global_random_mutex);
    return result;
}

/**
 * g_random_int_range:
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns a random #int32_t equally distributed over the range
 * [@begin..@end-1].
 *
 * Return value: A random number.
 **/
int32_t fs_random_int_range(int32_t begin, int32_t end) {
    FS_INIT(global_random);
    int32_t result;
    fs_mutex_lock(g_global_random_mutex);
    if (!g_global_random) g_global_random = fs_rand_new();

    result = fs_rand_int_range(g_global_random, begin, end);
    fs_mutex_unlock(g_global_random_mutex);
    return result;
}

/**
 * g_random_double:
 *
 * Returns a random #double equally distributed over the range [0..1).
 *
 * Return value: A random number.
 **/
double fs_random_double(void) {
    FS_INIT(global_random);
    double result;
    fs_mutex_lock(g_global_random_mutex);
    if (!g_global_random) g_global_random = fs_rand_new();

    result = fs_rand_double(g_global_random);
    fs_mutex_unlock(g_global_random_mutex);
    return result;
}

/**
 * g_random_double_range:
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns a random #double equally distributed over the range [@begin..@end).
 *
 * Return value: A random number.
 **/
double fs_random_double_range(double begin, double end) {
    FS_INIT(global_random);
    double result;
    fs_mutex_lock(g_global_random_mutex);
    if (!g_global_random) g_global_random = fs_rand_new();

    result = fs_rand_double_range(g_global_random, begin, end);
    fs_mutex_unlock(g_global_random_mutex);
    return result;
}

/**
 * g_random_set_seed:
 * @seed: a value to reinitialize the global random number generator.
 *
 * Sets the seed for the global random number generator, which is used
 * by the <function>g_random_*</function> functions, to @seed.
 **/
void fs_random_set_seed(uint32_t seed) {
    FS_INIT(global_random);
    fs_mutex_lock(g_global_random_mutex);
    if (!g_global_random) g_global_random = fs_rand_new_with_seed(seed);
    else fs_rand_set_seed(g_global_random, seed);
    fs_mutex_unlock(g_global_random_mutex);
}
