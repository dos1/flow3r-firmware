#pragma once
#include <stdint.h>

/* GENERAL INFORMATION
 *
 * petal index:     0 is the top petal above the USB-C jack, increases ccw so that
 *                  bottom petals are uneven and top petals even.
 *                  TODO: LEDs are indexed differently, this should be harmonized
 *                  in the impending API refactor.
 *
 * captouch data:   full uint16_t range per stage, higher values indicate touch.
 * pad index:       defined in captouch.c
 *
 *
 * IOU: the internals are still subject to major restructuring and are not
 * documented as of yet. will do once the data structures actually make sense
 * and are not duct tape upon duct tape.
 */

/* polls data from both captouch chips and processes it, either by updating
 * the captouch data exposed to the user or running a calibration cycle.
 *
 * the captouch chips has updated their registers every 9.2ms, so the fn
 * should be called every 10ms or so.
 *
 * this will be replaced someday by an interrupt event triggered system,
 * but this would ideally already implement configuration switching to
 * optimize latency by grouping pads and to expose the unused pad which
 * is a nontrivial task for another day.
 */
void captouch_read_cycle(void);

/* the captouch chip can generate an "afe" offset in the analog domain before
 * the ADC to optimize the readout range. according to the datasheet this should
 * be in the middle of the 16bit delta sigma ADC range (without much reasoning
 * supplied), however we found that having a higher range is beneficial.
 * 
 * the calibration cycle is optimizing the afe coefficients so that the output of
 * the "untouched" pads is close to the target set with this with this function/
 *
 * default target: 6000, manufacturer recommendation: 32676
 */
void captouch_set_calibration_afe_target(uint16_t target);

/* starts a a calibration cycle which is intended to run when the captouch
 * pads are not being touched. the cycle consists of two parts:
 * 
 * 1) optimize the afe coefficients (see captouch_set_calibration_afe_target)
 *
 * 2) with the new afe coefficients applied, average the readings in the untouched
 *    state into a software calibration list  which is normally subtracted from the
 *    captouch outputs. this makes up for the limited resolution of the of the afe
 *    coefficient calibration.
 */
void captouch_force_calibration();

/* indicates if a calibration cycle is currently active. the readings for
 * captouch_read_cycle and captouch_get_petal_* during a calibration cycle.
 *
 * 1: calibration cycle active
 * 0: calibration cycle inactive
 */
uint8_t captouch_calibration_active();


/* returns uint16_t which encodes each petal "touched" state as the bit
 * corresponding to the petal index. "touched" is determined by checking if
 * any of the pads belonging to the petal read a value higher than their
 * calibration value plus their threshold (see captouch_set_petal_pad_threshold)
 */
uint16_t read_captouch(void);

/* sets threshold for a petal pad above which read_captouch considers a pad as
 * touched.
 *
 * petal: petal index
 * pad:   pad index
 * thres: threshold value
 */
void captouch_set_petal_pad_threshold(uint8_t petal, uint8_t pad, uint16_t thres);

/* returns last read captouch value from a petal pad without subtracting its
 * calibration reference. typically only needed for debugging.
 *
 * petal: petal index
 * pad:   pad index
 */
uint16_t captouch_get_petal_pad_raw(uint8_t petal, uint8_t pad);

/* returns calibration reference for a petal pad.
 *
 * petal: petal index
 * pad:   pad index
 */
uint16_t captouch_get_petal_pad_calib_ref(uint8_t petal, uint8_t pad);

/* returns calibrated value from petal. clamps below 0.
 *
 * petal: petal index
 * pad:   pad index
 */
uint16_t captouch_get_petal_pad(uint8_t petal, uint8_t pad);

/* estimates the azimuthal finger position on a petal in arbitrary units.
 * returns 0 if hardware doesn't support this.
 * 
 * petal: petal index
 * pad:   pad index
 */
int32_t captouch_get_petal_phi(uint8_t petal);

/* estimates the radial finger position on a petal in arbitrary units.
 * returns 0 if hardware doesn't support this.
 * 
 * petal: petal index
 * pad:   pad index
 */
int32_t captouch_get_petal_rad(uint8_t petal);


/* configures the captouch chips, prefills internal structs etc.
 */
void captouch_init(void);

/* TODO: didn't look into what it does, never used it, just copied it from
 * the hardware verification firmware along with the rest. didn't break it
 * _intentionally_ but never tested it either.
 */
void captouch_print_debug_info(void);
