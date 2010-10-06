/***************************************************************************
                          flouphoto.c
                             -------------------
    copyright            : (C) 2002 by Christophe Seyve
    email                : christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <math.h>

// include componenet header
#include "SwPluginCore.h"

// specific code
#include <srfftw.h>

/* compile with :
g++ -Wall -c -I/usr/local/sisell/include -I/usr/X11R6/include \
	-I../../main/inc -I../../main/tools/inc -o outoffocus.o outoffocus.cpp
g++ outoffocus.o SwPluginCore.o -lm -L/usr/local/lib -lfftw -lrfftw /usr/local/sisell/lib/libswvisionworkshop.a -o outoffocus 
# needs SwPluginCore.o
*/



/********************** GLOBAL SECTION ************************
DO NOT MODIFY THIS SECTION
plugin.data_in  : input data. You should cast this pointer to read the
			data with the correct type
plugin.data_out : output data. You should cast this pointer to write the
			data with the correct type
***************************************************************/
SwPluginCore plugin;

/******************* END OF GLOBAL SECTION ********************/

/********************** USER SECTION ************************
YOU SHOULD MODIFY THIS SECTION
Declare your variables, function parameters and function list
	

***************************************************************/
#define CATEGORY	"Vision"
#define SUBCATEGORY	"Simulation"

void out_of_focus();
void motionblur();
void noisify();

// function parameters
short radius = 5;

swFuncParams focus_params[] = {
	{"radius", swS16, (void *)&radius}
};

float angle = 45.f;

short length = 15;
swFuncParams motionblur_params[] = {
	{"length", swS16, (void *)&length},
	{"angle (degree)", swFloat, (void *)&angle}
};


unsigned char noise_level = 40;
swFuncParams noisify_params[] = {
	{"noise", swU8, (void *)&noise_level},
};


short offset_x = 15;
short offset_y = 5;
swFuncParams offset_params[] = {
	{"offset_x", swS16, (void *)&offset_x},
	{"offset_x", swS16, (void *)&offset_y}
};
void offset();


int nb_steps=50;
char mod_var=1;
int slope = 100;
void Sin();

swFuncParams Sin_params[] = {
	{"nb steps", swS32, (void *)&nb_steps},
	{"mode", swS8, (void *)&mod_var},
	{"reduction (mode 1)", swS32, (void *)&slope}
	};

	
void underexposure();
void overexposure();
	
	
/* swFunctionDescriptor : 
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"Out of focus blur", 	1, 	focus_params,	swImage, swImage, &out_of_focus, NULL},
	{"Motion blur", 	2, 	motionblur_params,	swImage, swImage, &motionblur, NULL},
	{"Noisify", 	1, 	noisify_params,	swImage, swImage, &noisify, NULL},
	{"Offset", 		2, 	offset_params,	swImage, swImage, &offset, NULL},
	{"Over-exposure", 		0, 	NULL,	swImage, swImage, &overexposure, NULL},
	{"Under-exposure", 		0, 	NULL,	swImage, swImage, &underexposure, NULL}
};
int nb_functions = 6;

/******************* END OF USER SECTION ********************/



void offset() {
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	
	int width = imIn->width;
	int height = imIn->height;
	int depth = imIn->depth;
	
	int imSize = (int)imIn->buffer_size;
	int off = (offset_x + offset_y*width)*depth;
	for(int pix = abs(off); pix<imSize-abs(off); pix++) 
		imageOut[pix+off] = imageIn[pix];
}

unsigned char underexposure_init = 0;
unsigned char GammaCorrection[257];

void underexposure() {
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	
	
	if(!underexposure_init) {
		underexposure_init = 1;
		for(int i=0; i<=256; i++) {
			GammaCorrection[i] = (unsigned char)roundf(((float)i/255.f)*140.f);
		}
	}
	int width = imIn->width;
	int height = imIn->height;
	int depth = imIn->depth;
	
	int imSize = (int)imIn->buffer_size;
	for(int pix = 0; pix<imSize; pix++) 
		imageOut[pix] = GammaCorrection[imageIn[pix]];
}

void overexposure() {
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;
	
	
	if(!underexposure_init) {
		underexposure_init = 1;
		for(int i=0; i<=256; i++) {
			int val = roundf( sqrt(((float)i/240.f)) * 255.f );
			if(val>255) val = 255;
			GammaCorrection[i] = (unsigned char)val;
		}
	}
	int width = imIn->width;
	int height = imIn->height;
	int depth = imIn->depth;
	
	int imSize = (int)imIn->buffer_size;
	for(int pix = 0; pix<imSize; pix++) 
		imageOut[pix] = GammaCorrection[imageIn[pix]];
}

fftw_real *a = NULL, *b = NULL, *bstat = NULL, *c = NULL, maxc;
fftw_complex *A, *B, * C = NULL; // C[M][N/2+1];
short oldradius = 0;
int surf=0;


void out_of_focus()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	int rmin = 225;
	int M, N, depth, offset, N2;
	
    rfftwnd_plan p, pinv;
    fftw_real scale;
	int i, j, pix, r, imSize;
	 
	imSize = (int)imIn->buffer_size;
	
	M = imIn->width;	
	N = imIn->height;

	// allocate buffers
	N2 = 2*(N/2+1);
	if( ! a) {
		a = (fftw_real *)calloc(M * N2, sizeof(fftw_real));
		b = (fftw_real *)calloc(M * N2, sizeof(fftw_real));
		bstat = (fftw_real *)calloc(M * N2, sizeof(fftw_real));
		c = (fftw_real *)calloc(M * N, sizeof(fftw_real));
		C = (fftw_complex *)calloc(M * (N/2+1), sizeof(fftw_complex));
	
		if(!a || !b || !c || !C ) {
			fprintf(stderr, "Error : not enough memory\n");
			exit(1);
		}
	}
	
    p    = rfftw2d_create_plan(M, N, FFTW_REAL_TO_COMPLEX,
                                FFTW_ESTIMATE | FFTW_IN_PLACE);
    pinv = rfftw2d_create_plan(M, N, FFTW_COMPLEX_TO_REAL,
                                FFTW_ESTIMATE);

	// read image in
	rmin = radius;
	rmin = rmin * rmin;
	scale = 1.0 / (M * N);
	
    /* aliases for accessing complex transform outputs: */
    A = (fftw_complex*) a;
    B = (fftw_complex*) b;
    depth = imIn->depth;
	
	for(offset = 0; offset<(depth<4 ? depth : 3); offset++) {
//		fprintf(stderr, "PLANE %d / %d\n\tCreate planes\n", offset+1, depth);
	 	pix = offset;
		if(oldradius != radius) {
			surf = 0;
    		
			fprintf(stderr, "recalculate convolution mask r=%d\n", radius);
			for (j = 0; j < N; j++) {
				for (i = 0; i < M; i++) {
			  	int pixa = (j + i * N2);
			  	r = ((int)i - M /2)  * ((int)i - M /2)
					  + ((int)j - N /2)  * ((int)j - N /2);
				if(r > rmin) {
    	           	bstat[pixa] = (fftw_real)0.0;
				}
				else {
	               	bstat[pixa] = (fftw_real)1.0;
					surf++;
				}
				pix+=depth;
	         }
			}
			oldradius = radius;
	 	}
		
	 	pix = offset;
		for (j = 0; j < N; j++) {
    		for (i = 0; i < M; i++) {
			  	int pixa = (j + i * N2);
    	    	a[pixa] = (fftw_real)imageIn[pix] ;
				pix+=depth;
			}
		}		
		// copy bstat in b
		memcpy(b, bstat, M*N2*sizeof(fftw_real));
		
//		fprintf(stderr, "\tConvert planes %dx%d\n", M, N);
	    rfftwnd_one_real_to_complex(p, a, NULL);
    	rfftwnd_one_real_to_complex(p, b, NULL);

//		fprintf(stderr, "\tConvole planes\n");
	    for (i = 0; i < M; ++i)
    	    for (j = 0; j < N/2+1; ++j) {
               	int ij = i*(N/2+1) + j;
               	C[ij].re = (A[ij].re * B[ij].re
                             - A[ij].im * B[ij].im) * scale;
               	C[ij].im = (A[ij].re * B[ij].im
                             + A[ij].im * B[ij].re) * scale;
          }

     	/* inverse transform to get c, the convolution of a and b;
	        this has the side effect of overwriting C */
//		fprintf(stderr, "\tFFT inv\n");

		rfftwnd_one_complex_to_real(pinv, C, c);

//		fprintf(stderr, "\tmaxc\n");
		maxc = 0.0;
/* old version, some problem with color images 	*/
		if(depth == 4) {
	    	for (i = 0; i < M; i++) {
				for (j = 0; j < N; j++)  {
					
	 				if(c[i*N+j]>maxc) 
						maxc = c[i*N+j];
				}
			}
		}
		else if(surf>0.f)
				maxc = (M * N) / ((float)surf / 255.0) ; 
		
		if(maxc<1.0E-4)
				maxc = 1.f;
		
//		fprintf(stderr, "\tcalculate imageOut maxc=%g\n", maxc); fflush(stderr);
		// image out is separated in 4 quarter in disorder, we just set then in order...
		// top-Left -> Bottom-Right
	    for (i = 0; i < M/2; i++)
          	for (j = 0; j < N/2+1; j++) {
		  		int pix2 = ((j+N/2)*M + i + M/2);
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}
					
		// top-right -> Bottom-left
    	for (i = M/2; i < M; i++)
        	for (j = 0; j < N/2+1; j++) {
		  		int pix2 = (j+N/2)*M + i - M/2;
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}

		// bottom-Left -> top-Right
    	for (i = 0; i < M/2; i++)
        	for (j = N/2; j < N; j++) {
		  		int pix2 = (j-N/2)*M + i + M/2;
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}
		// bottom-right -> top-left
    	for (i = M/2; i < M; i++)
        	for (j = N/2; j < N; j++) {
		  		int pix2 = (j-N/2)*M + i - M/2;
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}
	} // for d
	
//	fprintf(stderr, "\tdone.\n");
	// free buffers
	rfftwnd_destroy_plan(p);
	rfftwnd_destroy_plan(pinv);
}

float oldangle = 0.f;

void motionblur()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	int rmin = 225;
	int M, N, depth, offset, N2;
	
    rfftwnd_plan p, pinv;
    fftw_real scale;
	int i, j, pix, r, imSize;
	 
	imSize = (int)imIn->buffer_size;
	
	M = imIn->width;	
	N = imIn->height;

	// allocate buffers
	N2 = 2*(N/2+1);
	if( ! a) {
		a = (fftw_real *)calloc(M * N2, sizeof(fftw_real));
		b = (fftw_real *)calloc(M * N2, sizeof(fftw_real));
		bstat = (fftw_real *)calloc(M * N2, sizeof(fftw_real));
		c = (fftw_real *)calloc(M * N, sizeof(fftw_real));
		C = (fftw_complex *)calloc(M * (N/2+1), sizeof(fftw_complex));
	
		if(!a || !b || !c || !C ) {
			fprintf(stderr, "Error : not enough memory\n");
			exit(1);
		}
	}
	
    p    = rfftw2d_create_plan(M, N, FFTW_REAL_TO_COMPLEX,
                                FFTW_ESTIMATE | FFTW_IN_PLACE);
    pinv = rfftw2d_create_plan(M, N, FFTW_COMPLEX_TO_REAL,
                                FFTW_ESTIMATE);

	// read image in
	rmin = radius;
	rmin = rmin * rmin;
	scale = 1.0 / (M * N);
	
    /* aliases for accessing complex transform outputs: */
    A = (fftw_complex*) a;
    B = (fftw_complex*) b;
    depth = imIn->depth;
	
	for(offset = 0; offset<(depth<4 ? depth : 3); offset++) {
//		fprintf(stderr, "PLANE %d / %d\n\tCreate planes\n", offset+1, depth);
	 	pix = offset;
		if(oldradius != length || oldangle != angle) {
			surf = 0;
    		float rad_angle = (angle+90) * 3.1415927 / 180.f;
			
			fprintf(stderr, "recalculate convolution mask r=%d alpha=%g\n", length, rad_angle);
			for (j = 0; j < N; j++) 
				for (i = 0; i < M; i++) {
			  		int pixa = (j + i * N2);
   	           		bstat[pixa] = (fftw_real)0.0;
				}
			// blur area
			if( fabs(sin(rad_angle)) > sqrt(2.f)/2.f) {
				fprintf(stderr, "Cas 1 : tracing / i\n");
				float pente = 1.f/tan(rad_angle);
				
				int iradius = (int) ( fabs((double)length/2.0 * sin( (double)rad_angle )));
				int imin = M/2 - iradius;
				if(imin <0) imin = 0;
				int imax = M/2 + iradius;
				if(imax >=M) imax = M-1;
				
				for( i = imin; i<=imax; i++)
				{
					j = (int) (pente * (float)(i -M/2)) + N/2;
			  		int pixa = (j + i * N2);
					if(pixa>=0 && pixa < M*N2)
	   	           		bstat[pixa] = (fftw_real)1.0;
	         	}
				
			
			}
			else {
				fprintf(stderr, "Cas 2 : tracing / j\n");
				float pente = tan(rad_angle);
				
				int jradius = (int)( fabs((double)length/2.f*cos( rad_angle )));
				int jmin = N2/2 - jradius;
				if(jmin <0) jmin = 0;
				int jmax = N2/2 + jradius;
				if(jmax >=N2) jmax = N2-1;
				
				for(j = jmin; j<=jmax; j++)
				{
					i = (int) round(pente * (float)(j -N/2)) + M/2;
			  		int pixa = (j + i * N2);
					if(pixa>=0 && pixa < M*N2)
	   	           		bstat[pixa] = (fftw_real)1.0;
	         	}
			}			
			
			
			oldangle  = angle;
			oldradius = length;
	 	}
		
	 	pix = offset;
		for (j = 0; j < N; j++) {
    		for (i = 0; i < M; i++) {
			  	int pixa = (j + i * N2);
    	    	a[pixa] = (fftw_real)imageIn[pix] ;
				pix+=depth;
			}
		}		
		// copy bstat in b
		memcpy(b, bstat, M*N2*sizeof(fftw_real));
		
//		fprintf(stderr, "\tConvert planes %dx%d\n", M, N);
	    rfftwnd_one_real_to_complex(p, a, NULL);
    	rfftwnd_one_real_to_complex(p, b, NULL);

//		fprintf(stderr, "\tConvole planes\n");
	    for (i = 0; i < M; ++i)
    	    for (j = 0; j < N/2+1; ++j) {
               	int ij = i*(N/2+1) + j;
               	C[ij].re = (A[ij].re * B[ij].re
                             - A[ij].im * B[ij].im) * scale;
               	C[ij].im = (A[ij].re * B[ij].im
                             + A[ij].im * B[ij].re) * scale;
          }

     	/* inverse transform to get c, the convolution of a and b;
	        this has the side effect of overwriting C */
//		fprintf(stderr, "\tFFT inv\n");

		rfftwnd_one_complex_to_real(pinv, C, c);

//		fprintf(stderr, "\tmaxc\n");
		maxc = 0.0;
/* old version, some problem with color images 	*/
		if(depth == 4) {
	    	for (i = 0; i < M; i++) {
				for (j = 0; j < N; j++)  {
					
	 				if(c[i*N+j]>maxc) 
						maxc = c[i*N+j];
				}
			}
		}
		else if(surf>0.f)
				maxc = (M * N) / ((float)surf / 255.0) ; 
		
		if(maxc<1.0E-4)
				maxc = 1.f;
		
//		fprintf(stderr, "\tcalculate imageOut maxc=%g\n", maxc); fflush(stderr);
		// image out is separated in 4 quarter in disorder, we just set then in order...
		// top-Left -> Bottom-Right
	    for (i = 0; i < M/2; i++)
          	for (j = 0; j < N/2+1; j++) {
		  		int pix2 = ((j+N/2)*M + i + M/2);
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}
					
		// top-right -> Bottom-left
    	for (i = M/2; i < M; i++)
        	for (j = 0; j < N/2+1; j++) {
		  		int pix2 = (j+N/2)*M + i - M/2;
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}

		// bottom-Left -> top-Right
    	for (i = 0; i < M/2; i++)
        	for (j = N/2; j < N; j++) {
		  		int pix2 = (j-N/2)*M + i + M/2;
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}
		// bottom-right -> top-left
    	for (i = M/2; i < M; i++)
        	for (j = N/2; j < N; j++) {
		  		int pix2 = (j-N/2)*M + i - M/2;
				pix2 = pix2*depth + offset;
		  		imageOut[pix2] = (unsigned char)(c[i*N+j]  * 255.0 / maxc);
			}
	} // for d
	
//	fprintf(stderr, "\tdone.\n");
	// free buffers
	rfftwnd_destroy_plan(p);
	rfftwnd_destroy_plan(pinv);
}




void noisify()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	int width = imIn->width;	
	int height = imIn->height;
	int depth = imIn->depth;
	int pix, pixmax = width*height*depth;
	unsigned char n2 = noise_level / 2;
	for(pix = 0; pix < pixmax; pix++) {
		unsigned char noise = (unsigned char) (random() % (long int)noise_level);
		
		if( noise > n2 ) {
			unsigned char dn = noise - n2;
			if(imageIn[pix] < 255 - dn)
				imageOut[pix] = imageIn[pix] + dn;
			else 
				imageOut[pix] = 255;
		} else {
			unsigned char dn = n2 - noise;
			
			if(imageIn[pix] > dn)
				imageOut[pix] = imageIn[pix] - dn;
			else 
				imageOut[pix] = 0;
		}

	}
}





// DO NOT MODIFY MAIN PROC
void signalhandler(int sig)
{
	fprintf(stderr, "================== RECEIVED SIGNAL %d = '%s' From process %d ==============\n", sig, sys_siglist[sig], getpid());
	signal(sig, signalhandler);
	
	if(sig != SIGUSR1)
		exit(0);
}
int main(int argc, char *argv[])
{
	// SwPluginCore load
	for(int i=0; i<NSIG; i++)
		signal(i, signalhandler);
		
	fprintf(stderr, "registerCategory...\n");
	plugin.registerCategory(CATEGORY, SUBCATEGORY);
	
	// register functions 
	fprintf(stderr, "registerFunctions...\n");
	plugin.registerFunctions(functions, nb_functions );

	// process loop
	fprintf(stderr, "loop...\n");
	plugin.loop();

	fprintf(stderr, "exit(EXIT_SUCCESS). Bye.\n");
  	return EXIT_SUCCESS;
}
