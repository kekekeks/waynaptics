#include "shim.h"

#define BitIsOn(ptr, bit) (!!(((const BYTE *) (ptr))[(bit)>>3] & (1 << ((bit) & 7))))
#define SetBit(ptr, bit)  (((BYTE *) (ptr))[(bit)>>3] |= (1 << ((bit) & 7)))
#define ClearBit(ptr, bit) (((BYTE *)(ptr))[(bit)>>3] &= ~(1 << ((bit) & 7)))
extern _X_EXPORT int CountBits(const uint8_t * mask, int len);

int max(int a, int b)
{
    return a > b ? a : b;
}

int min(int a, int b)
{
    return a < b ? a : b;
}


/**
 * Alloc a valuator mask large enough for num_valuators.
 */
ValuatorMask *
valuator_mask_new(int num_valuators)
{
    /* alloc a fixed size mask for now and ignore num_valuators. in the
     * flying-car future, when we can dynamically alloc the masks and are
     * not constrained by signals, we can start using num_valuators */
    ValuatorMask *mask = calloc(1, sizeof(ValuatorMask));

    if (mask == NULL)
        return NULL;

    mask->last_bit = -1;
    return mask;
}

void
valuator_mask_free(ValuatorMask **mask)
{
    free(*mask);
    *mask = NULL;
}

/**
 * Sets a range of valuators between first_valuator and num_valuators with
 * the data in the valuators array. All other values are set to 0.
 */
void
valuator_mask_set_range(ValuatorMask *mask, int first_valuator,
                        int num_valuators, const int *valuators)
{
    int i;

    valuator_mask_zero(mask);

    for (i = first_valuator;
         i < min(first_valuator + num_valuators, MAX_VALUATORS); i++)
        valuator_mask_set(mask, i, valuators[i - first_valuator]);
}

/**
 * Reset mask to zero.
 */
void
valuator_mask_zero(ValuatorMask *mask)
{
    memset(mask, 0, sizeof(*mask));
    mask->last_bit = -1;
}

/**
 * Returns the current size of the mask (i.e. the highest number of
 * valuators currently set + 1).
 */
int
valuator_mask_size(const ValuatorMask *mask)
{
    return mask->last_bit + 1;
}

/**
 * Returns the number of valuators set in the given mask.
 */
int
valuator_mask_num_valuators(const ValuatorMask *mask)
{
    return CountBits(mask->mask, min(mask->last_bit + 1, MAX_VALUATORS));
}

/**
 * Return true if the valuator is set in the mask, or false otherwise.
 */
int
valuator_mask_isset(const ValuatorMask *mask, int valuator)
{
    return mask->last_bit >= valuator && BitIsOn(mask->mask, valuator);
}

static inline void
_valuator_mask_set_double(ValuatorMask *mask, int valuator, double data)
{
    mask->last_bit = max(valuator, mask->last_bit);
    SetBit(mask->mask, valuator);
    mask->valuators[valuator] = data;
}

/**
 * Set the valuator to the given floating-point data.
 */
void
valuator_mask_set_double(ValuatorMask *mask, int valuator, double data)
{
    BUG_WARN_MSG(mask->has_unaccelerated,
                 "Do not mix valuator types, zero mask first\n");
    _valuator_mask_set_double(mask, valuator, data);
}

/**
 * Set the valuator to the given integer data.
 */
void
valuator_mask_set(ValuatorMask *mask, int valuator, int data)
{
    valuator_mask_set_double(mask, valuator, data);
}

/**
 * Return the requested valuator value as a double. If the mask bit is not
 * set for the given valuator, the returned value is undefined.
 */
double
valuator_mask_get_double(const ValuatorMask *mask, int valuator)
{
    return mask->valuators[valuator];
}

/**
 * Return the requested valuator value as an integer, rounding towards zero.
 * If the mask bit is not set for the given valuator, the returned value is
 * undefined.
 */
int
valuator_mask_get(const ValuatorMask *mask, int valuator)
{
    return trunc(valuator_mask_get_double(mask, valuator));
}

/**
 * Set value to the requested valuator. If the mask bit is set for this
 * valuator, value contains the requested valuator value and TRUE is
 * returned.
 * If the mask bit is not set for this valuator, value is unchanged and
 * FALSE is returned.
 */
Bool
valuator_mask_fetch_double(const ValuatorMask *mask, int valuator,
                           double *value)
{
    if (valuator_mask_isset(mask, valuator)) {
        *value = valuator_mask_get_double(mask, valuator);
        return TRUE;
    }
    else
        return FALSE;
}

/**
 * Set value to the requested valuator. If the mask bit is set for this
 * valuator, value contains the requested valuator value and TRUE is
 * returned.
 * If the mask bit is not set for this valuator, value is unchanged and
 * FALSE is returned.
 */
Bool
valuator_mask_fetch(const ValuatorMask *mask, int valuator, int *value)
{
    if (valuator_mask_isset(mask, valuator)) {
        *value = valuator_mask_get(mask, valuator);
        return TRUE;
    }
    else
        return FALSE;
}

/**
 * Remove the valuator from the mask.
 */
void
valuator_mask_unset(ValuatorMask *mask, int valuator)
{
    if (mask->last_bit >= valuator) {
        int i, lastbit = -1;

        ClearBit(mask->mask, valuator);
        mask->valuators[valuator] = 0.0;
        mask->unaccelerated[valuator] = 0.0;

        for (i = 0; i <= mask->last_bit; i++)
            if (valuator_mask_isset(mask, i))
                lastbit = max(lastbit, i);
        mask->last_bit = lastbit;

        if (mask->last_bit == -1)
            mask->has_unaccelerated = FALSE;
    }
}

void
valuator_mask_copy(ValuatorMask *dest, const ValuatorMask *src)
{
    if (src)
        memcpy(dest, src, sizeof(*dest));
    else
        valuator_mask_zero(dest);
}

Bool
valuator_mask_has_unaccelerated(const ValuatorMask *mask)
{
    return mask->has_unaccelerated;
}

void
valuator_mask_drop_unaccelerated(ValuatorMask *mask)
{
    memset(mask->unaccelerated, 0, sizeof(mask->unaccelerated));
    mask->has_unaccelerated = FALSE;
}

void
valuator_mask_set_absolute_unaccelerated(ValuatorMask *mask,
                                         int valuator,
                                         int absolute,
                                         double unaccel)
{
    BUG_WARN_MSG(mask->last_bit != -1 && !mask->has_unaccelerated,
                 "Do not mix valuator types, zero mask first\n");
    _valuator_mask_set_double(mask, valuator, absolute);
    mask->has_unaccelerated = TRUE;
    mask->unaccelerated[valuator] = unaccel;
}

/**
 * Set both accelerated and unaccelerated value for this mask.
 */
void
valuator_mask_set_unaccelerated(ValuatorMask *mask,
                                int valuator,
                                double accel,
                                double unaccel)
{
    BUG_WARN_MSG(mask->last_bit != -1 && !mask->has_unaccelerated,
                 "Do not mix valuator types, zero mask first\n");
    _valuator_mask_set_double(mask, valuator, accel);
    mask->has_unaccelerated = TRUE;
    mask->unaccelerated[valuator] = unaccel;
}

double
valuator_mask_get_accelerated(const ValuatorMask *mask,
                              int valuator)
{
    return valuator_mask_get_double(mask, valuator);
}

double
valuator_mask_get_unaccelerated(const ValuatorMask *mask,
                                int valuator)
{
    return mask->unaccelerated[valuator];
}

Bool
valuator_mask_fetch_unaccelerated(const ValuatorMask *mask,
                                  int valuator,
                                  double *accel,
                                  double *unaccel)
{
    if (valuator_mask_isset(mask, valuator)) {
        if (accel)
            *accel = valuator_mask_get_accelerated(mask, valuator);
        if (unaccel)
            *unaccel = valuator_mask_get_unaccelerated(mask, valuator);
        return TRUE;
    }
    else
        return FALSE;
}

int
CountBits(const uint8_t * mask, int len)
{
    int i;
    int ret = 0;

    for (i = 0; i < len; i++)
        if (BitIsOn(mask, i))
            ret++;

    return ret;
}