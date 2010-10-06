#ifndef VIDEO_ASM_H
#define VIDEO_ASM_H

/* interface to C */

#ifdef __cplusplus
extern "C" {
#endif

/* Fast assambly routines for common operations on video: adding
   buffers, substracting, taking diffs, etc.
   All operations are performed bytewise, the void * are just used
   to keep the compiler happy.
   
   The functions ending in '_mmx' use MMX instructions, acting on 4 or even
   8 bytes at a time. It follows that 'n' should be a multiple of 8. The
   '_smx' function use the Saturated version of the MMX instructions.
   
   Definitions:
   'diff': c = (a - b) / 2
   'intg': c += (a * 2);
   
   'add': a += b
   'plus': c = a + b
   'sub': a -= b
   'minus': c = a - b

   The '128' version of the functions assume the data in the buffers has a
   virtual null point at 128, and compensate for this.
 */

void calc_diff128    (int n, void *dst, void *src_new, void *src_old);
void calc_diff128_mmx(int n, void *dst, void *src_new, void *src_old);

void calc_intg128    (int n, void *dst, void *delta);
void calc_intg128_smx(int n, void *dst, void *delta);

/* Basic operators */
/* Add 'src' to 'dst' */
void calc_add    (int n, void *dst, void *src);
void calc_add_mmx(int n, void *dst, void *src);
void calc_add_smx(int n, void *dst, void *src);

/* Add 'src' to 'dst', with offset 128 */
void calc_add128_mmx(int n, void *dst, void *src);

/* Add 'src1' to 'src2', storing the result in 'dst' */
void calc_plus    (int n, void *dst, void *src1, void *src2);
void calc_plus_mmx(int n, void *dst, void *src1, void *src2);
void calc_plus_smx(int n, void *dst, void *src1, void *src2);

#ifdef __cplusplus
}
#endif

#endif
