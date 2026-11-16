// Minimal double -> char[] conversion for constrained MCUs.
// - No std::string, no std::to_chars, no dynamic allocation.
// - Rounds to `precision` fractional digits.
// - Writes a null-terminated string to `buf` (size buf_size).
// - Returns true on success, false if buffer too small or precision too large.
// - Limits: integer part must fit into unsigned long long when cast
//           (very large doubles may overflow the cast).
// - No <cmath> usage; simple NaN/Inf heuristics included.

//#include <stddef.h> // for size_t

#include <Arduino.h>
#include "KB_Aux.h"

bool KB_dtoa(double value, char *buf, size_t buf_size, int precision)
{
    if (!buf || buf_size == 0) return false;
    if (precision < 0) precision = 6;
    //const int KB_DTOA_MAX_PREC = 18;
    if (precision > KB_DTOA_MAX_PREC) precision = KB_DTOA_MAX_PREC; // cap precision to keep stack small

    char *p = buf;
    size_t rem = buf_size;

    // quick NaN check
    if (value != value) {
        const char *s = "nan";
        while (*s) {
            if (rem <= 1) return false;
            *p++ = *s++;
            --rem;
        }
        *p = '\0';
        return true;
    }

    // crude infinity detection (works for usual cases)
    if (value > 1e308) {
        const char *s = "inf";
        while (*s) {
            if (rem <= 1) return false;
            *p++ = *s++;
            --rem;
        }
        *p = '\0';
        return true;
    }
    if (value < -1e308) {
        const char *s = "-inf";
        while (*s) {
            if (rem <= 1) return false;
            *p++ = *s++;
            --rem;
        }
        *p = '\0';
        return true;
    }

    // handle sign (also handles -0.0)
    bool negative = (value < 0.0) || (value == 0.0 && 1.0/value < 0.0);
    if (negative) {
        value = -value;
        if (rem <= 1) return false;
        *p++ = '-'; --rem;
    }

    // split integer / fractional parts
    unsigned long long int_part = (unsigned long long)(value);
    double frac = value - (double)int_part;
    if (frac < 0) frac = 0.0; // guard against tiny FP negative

    // integer digits -> store reversed into intbuf
    char intbuf[32];
    int int_len = 0;
    if (int_part == 0ULL) {
        intbuf[int_len++] = '0';
    } else {
        while (int_part > 0ULL && int_len < (int)sizeof(intbuf)) {
            unsigned digit = (unsigned)(int_part % 10ULL);
            intbuf[int_len++] = (char)('0' + digit);
            int_part /= 10ULL;
        }
        if (int_part != 0ULL) {
            // overflowed intbuf (number too big)
            return false;
        }
    }

    // fractional digits (not yet rounded)
    char fbuf[32];
    int fprec = precision;
    for (int i = 0; i < fprec; ++i) {
        frac *= 10.0;
        int d = (int)frac;
        if (d < 0) d = 0;
        if (d > 9) d = 9;
        fbuf[i] = (char)('0' + d);
        frac -= (double)d;
    }

    // rounding: look at next digit
    frac *= 10.0;
    int next = (int)frac;
    if (next >= 5) {
        int carry = 1;
        for (int i = fprec - 1; i >= 0 && carry; --i) {
            int d = (fbuf[i] - '0') + carry;
            if (d >= 10) {
                fbuf[i] = '0';
                carry = 1;
            } else {
                fbuf[i] = (char)('0' + d);
                carry = 0;
            }
        }
        // if carry remains, add to integer part (intbuf is reversed)
        for (int i = 0; carry && i < int_len; ++i) {
            int d = (intbuf[i] - '0') + carry;
            if (d >= 10) {
                intbuf[i] = '0';
                carry = 1;
            } else {
                intbuf[i] = (char)('0' + d);
                carry = 0;
            }
        }
        if (carry) {
            // append '1' as new most significant digit (reversed storage)
            if (int_len >= (int)sizeof(intbuf)) return false;
            intbuf[int_len++] = '1';
        }
    }

    // compute required length: sign already written; need int_len + (precision?1+precision:0) + '\0'
    size_t need = (size_t)int_len + (fprec > 0 ? (1 + (size_t)fprec) : 0) + 1;
    if (need > rem) return false;

    // write integer part (reverse intbuf)
    for (int i = int_len - 1; i >= 0; --i) {
        *p++ = intbuf[i];
    }

    // write fractional part
    if (fprec > 0) {
        *p++ = '.';
        for (int i = 0; i < fprec; ++i) {
            *p++ = fbuf[i];
        }
    }

    *p = '\0';
    return true;
}

// Example usage (not for MCU, just illustration):
/*
#include <stdio.h>
int main() {
    char buf[64];
    dtoa(-1234.56789, buf, sizeof(buf), 4);
    puts(buf); // "-1234.5679"
    dtoa(0.000123456, buf, sizeof(buf), 8);
    puts(buf); // "0.00012346"
    return 0;
}
*/
