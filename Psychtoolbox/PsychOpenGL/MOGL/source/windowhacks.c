/* These are hacks to get the Windows version running - to be replaced by a
   proper solution(TM) soon...
*/

#include "mogltypes.h"

double gluCheckExtension(const GLubyte* a, const GLubyte* b) {
	printf("MOGL-WARNING: gluCheckExtension() called - Unsupported on Windows for now!!!\n");
   return(0);
}

double gluBuild1DMipmapLevels(double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, void* a9)
{
	printf( "MOGL-WARNING: gluBuild1DMipmapLevels() called - Unsupported on Windows for now!!!\n");
   return(0);
}
double gluBuild2DMipmapLevels(double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, double a9, void* a10)
{
	printf( "MOGL-WARNING: gluBuild2DMipmapLevels() called - Unsupported on Windows for now!!!\n");
   return(0);
}
double gluBuild3DMipmapLevels(double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, double a9, double a10, void* a11)
{
	printf( "MOGL-WARNING: gluBuild3DMipmapLevels() called - Unsupported on Windows for now!!!\n");
   return(0);
}
double gluBuild3DMipmaps(double a1, double a2, double a3, double a4, double a5, double a6, double a7, void* a8)
{
	printf( "MOGL-WARNING: gluBuild3DMipmaps() called - Unsupported on Windows for now!!!\n");
   return(0);
}

#ifndef PTBOCTAVE3MEX

double gluUnProject4(double a1, double a2, double a3, double a4, double* a5, double* a6, int* a7, double a8, double a9, double* a10, double* a11, double* a12, double* a13)
{
	printf( "MOGL-WARNING: gluUnproject4() called - Unsupported on Windows for now!!!\n");
   return(0);
}

mxArray* mxCreateNumericMatrix(int m, int n, int class, int complex)
{
/* On Matlab R11 builds, we use int for the dims array.
 * On R2007a and later, we use the defined mwSize type, which
 * will properly adapt to 32-bit on 32-bit builds and 64-bit on
 * 64-bit builds of PTB. The TARGET_OS_WIN32 is only defined on >= R2007a.
 */
#ifndef TARGET_OS_WIN32
 /* R11 build: Legacy int definition: */
 int dims[2];
 dims[0]=m;
 dims[1]=n;
#else
 /* R2007a and later build: Proper 64-bit clean definition: */
 mwSize dims[2];
 dims[0] = (mwSize) m;
 dims[1] = (mwSize) n;
#endif

 return(mxCreateNumericArray(2, dims, class, complex));
}

#else

GLint APIENTRY gluUnProject4 (GLdouble winX, GLdouble winY, GLdouble winZ, GLdouble clipW, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble nearVal, GLdouble farVal, GLdouble* objX, GLdouble* objY, GLdouble* objZ, GLdouble* objW)
{
	printf( "MOGL-WARNING: gluUnproject4() called - Unsupported on Windows for now!!!\n");
   return(0);
}

#endif
