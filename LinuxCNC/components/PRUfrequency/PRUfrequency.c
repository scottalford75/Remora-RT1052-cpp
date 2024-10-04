/********************************************************************
* Description:  PRUfrequency.c
*               This file, "PRUfrequency.c", is a HAL component for REMORA
* 				firmware,  that converts the frequency to what you need, 
* 				for example the RPM
*
* Author: Vasily Leontev <leo.perm@gmail.com>
* Based heavily on the "encoder" hal module by John Kasunich and
* "counter" module by Chris Radek
* License: GPL Version 2
*    
* Copyright (c) 2024 All rights reserved.
*
********************************************************************/
/** This module receives the frequency in hertz from the QDC module, 
 * REMORA firmware, calculated by hardware on the NVEM board, 
 * and translates it into some understandable units
 * 
 * at servo flow frequency of 40 kHz, frequency error from 1-500 = 0-5 Hz
 * from 500-1000 = 0-25
 * from 1000-2000 = ~50
 * if you increase the servo flow frequency, the error will be less
*/

#include "rtapi.h"              /* RTAPI realtime OS API */
#include "rtapi_app.h"          /* RTAPI realtime module decls */
#include "rtapi_errno.h"        /* EINVAL etc */
#include "hal.h"                /* HAL public API decls */

#define MODNAME "PRUfrequency"

/* module information */
MODULE_AUTHOR("Vasily Leontev");
MODULE_DESCRIPTION("frequency converter for EMC HAL");
MODULE_LICENSE("GPL");
static int num_chan = 1;        /* number of channels - default = 1 */
RTAPI_MP_INT(num_chan, "number of channels");

/***********************************************************************
*                STRUCTURES AND GLOBAL VARIABLES                       *
************************************************************************/

/* this structure contains the runtime data for a single counter */

typedef struct {
    hal_float_t *raw_freq;	/* pin: freq value connect to pv[] remora */
    hal_float_t *min_out;	/* min value (floating point) */
    hal_float_t *max_out;	/* max value (floating point) */
    hal_float_t *out;		/* scaled value (floating point) */
    hal_float_t *scale;		/* scale */
} freq_t;

/* pointer to array of counter_t structs in shmem, 1 per counter */
static freq_t *freq_array;

/* other globals */
static int comp_id;		/* component ID */

/***********************************************************************
*                  LOCAL FUNCTION DECLARATIONS                         *
************************************************************************/

static int export_freq(int num, freq_t * addr);
static void update(void *arg, long period);

/***********************************************************************
*                       INIT AND EXIT CODE                             *
************************************************************************/

#define MAX_CHAN 8

int rtapi_app_main(void)
{
    int n, retval;

    /* test for number of channels */
    if ((num_chan <= 0) || (num_chan > MAX_CHAN)) {
	rtapi_print_msg(RTAPI_MSG_ERR,
	    "PRUfrequency: ERROR: invalid num_chan: %d\n", num_chan);
	return -EINVAL;
    }
    /* have good config info, connect to the HAL */
    comp_id = hal_init("PRUfrequency");
    if (comp_id < 0) {
	rtapi_print_msg(RTAPI_MSG_ERR, "PRUfrequency: ERROR: hal_init() failed\n");
	return -EINVAL;
    }
    /* allocate shared memory for frequency data */
    freq_array = hal_malloc(num_chan * sizeof(freq_t));
    if (!freq_array) {
	rtapi_print_msg(RTAPI_MSG_ERR,
	    "PRUfrequency: ERROR: hal_malloc() failed\n");
	hal_exit(comp_id);
	return -ENOMEM;
    }
    /* export all the variables for each frequency */
    for (n = 0; n < num_chan; n++) {
	/* export all vars */
	retval = export_freq(n, &(freq_array[n]));
	if (retval != 0) {
	    rtapi_print_msg(RTAPI_MSG_ERR,
		"PRUfrequency: ERROR: PRUfrequency %d var export failed\n", n);
	    hal_exit(comp_id);
	    return -EIO;
	}
	/* init counter */
	*(freq_array[n].raw_freq) = 0;
	*(freq_array[n].scale) = 1.0;
	*(freq_array[n].min_out) = 0;
	*(freq_array[n].max_out) = 0;
    }
    /* export functions */
    retval = hal_export_funct("PRUfrequency.update-counters", update,
	freq_array, 0, 0, comp_id);
    if (retval != 0) {
	rtapi_print_msg(RTAPI_MSG_ERR,
	    "PRUfrequency: ERROR: count funct export failed\n");
	hal_exit(comp_id);
	return -EIO;
    }
    rtapi_print_msg(RTAPI_MSG_INFO,
	"PRUfrequency: installed %d freq counters\n", num_chan);
    hal_ready(comp_id);
    return 0;
}

void rtapi_app_exit(void)
{
    hal_exit(comp_id);
}

/***********************************************************************
*            REALTIME COUNTER COUNTING AND UPDATE FUNCTIONS            *
************************************************************************/

static void update(void *arg, long period)
{
		freq_t *freq;
		int n;

		for (freq = arg, n = 0; n < num_chan; freq++, n++) 
		{
			double raw_freq;
			double out;
			
			out = *(freq->raw_freq) * *(freq->scale);
			if(out > *(freq->max_out) && *(freq->max_out) != 0)
			{
				*(freq->out) = *(freq->max_out);
			} 
			else if (out < *(freq->min_out) && *(freq->min_out) != 0)
			{
				*(freq->out) = 0;
			}
			else
			{
				*(freq->out) = out;
			}
		}
}

/***********************************************************************
*                   LOCAL FUNCTION DEFINITIONS                         *
************************************************************************/

static int export_freq(int num, freq_t * addr)
{
    int retval, msg;

    msg = rtapi_get_msg_level();
    rtapi_set_msg_level(RTAPI_MSG_WARN);

    /* export parameter for raw counts */
    retval = hal_pin_float_newf(HAL_IN, &(addr->raw_freq), comp_id,"PRUfrequency.%d.raw_freq", num);
    if (retval != 0) {
	return retval;
    }
    /* export parameter for min out */
    retval = hal_pin_float_newf(HAL_IN, &(addr->min_out), comp_id,"PRUfrequency.%d.min_out", num);
    if (retval != 0) {
	return retval;
    }
    /* export parameter for max out */
    retval = hal_pin_float_newf(HAL_IN, &(addr->max_out), comp_id,"PRUfrequency.%d.max_out", num);
    if (retval != 0) {
	return retval;
    }
    /* export pin for scaled velocity captured by capture() */
    retval = hal_pin_float_newf(HAL_OUT, &(addr->out), comp_id, "PRUfrequency.%d.out", num);
    if (retval != 0) {
	return retval;
    }
    /* export parameter for scaling */
    retval = hal_pin_float_newf(HAL_IO, &(addr->scale), comp_id, "PRUfrequency.%d.scale", num);
    if (retval != 0) {
	return retval;
    }    
    /* restore saved message level */
    rtapi_set_msg_level(msg);
    return 0;
}
