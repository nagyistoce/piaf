#include <stdio.h>
#include <assert.h>

// OpenCV
#include "swopencv.h"


// include component header
#include "SwPluginCore.h"
#include "swimage_utils.h"

extern "C"
{
#include "jpeglib.h"
#include <setjmp.h>
}

#define DEBUG 0
#define OUTPUT_IMAGES 1


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
#define SUBCATEGORY	"DCT"




void LogDCT();
uchar LogDCT_radius = 5;
float LogDCT_coef = 0.5f;
uchar LogDCT_add = 100;

swFuncParams LogDCT_params[] = {
	{"LogDCT_mode", swU8, (void *)&LogDCT_radius},
	{"LogDCT_coef", swFloat, (void *)&LogDCT_coef},
	{"LogDCT_add", swU8, (void *)&LogDCT_add}

};

/* swFunctionDescriptor :
	char * : function name
	int : number of parameters
	swFuncParams : function parameters
	swType : input type
	swType : output type
	void * : procedure
	*/
swFunctionDescriptor functions[] = {
	{"LogDCT", 		3,	LogDCT_params,  	swImage, swImage, &LogDCT, NULL}
};
int nb_functions = 1;


IplImage * cvIm1 = NULL;
IplImage * cvIm2 = NULL;
IplImage * planes1[4];
IplImage * planes2[4];

IplImage * cvImHSV = NULL;
IplImage * cvImRGB = NULL;

IplImage * cvImGray = NULL;





void allocateImages()
{
	swImageStruct * imIn = ((swImageStruct *)plugin.data_in);
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	unsigned char * imageIn  = (unsigned char *)imIn->buffer;
	unsigned char * imageOut = (unsigned char *)imOut->buffer;

	CvSize size;
	size.width = imIn->width;
	size.height = imIn->height;

	if(!cvIm1) {
		cvIm1 = cvCreateImageHeader(size,  IPL_DEPTH_8U, imIn->depth);

		cvImHSV = cvCreateImage(size, IPL_DEPTH_8U, 3);
		cvImRGB = cvCreateImage(size, IPL_DEPTH_8U, 3);

		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++) {
				planes1[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			}
			for(int i=imIn->depth; i<4;i++) {
				planes1[i] = NULL;
			}
			for(int i=0; i<4;i++) {
				planes2[i] = NULL;
			}

			cvImGray = cvCreateImage(size,  IPL_DEPTH_8U, 1);
		}
		else {
			for(int i=0; i<4;i++) {	planes1[i] = NULL; }

			cvImGray = cvCreateImageHeader(size,  IPL_DEPTH_8U, 1);
		}
	}
	if(!cvIm2) {
		cvIm2 = cvCreateImageHeader(size,  IPL_DEPTH_8U, imIn->depth);
		if(imIn->depth > 1) {
			for(int i=0;i<imIn->depth; i++)
				planes2[i] = cvCreateImage(size,  IPL_DEPTH_8U, 1);
			for(int i=imIn->depth; i<4;i++)
				planes2[i] = NULL;
		}
		else {
			for(int i=0; i<4;i++) {	planes2[i] = NULL; }
		}
	}

	cvIm1->imageData = (char *)imageIn;
	cvIm2->imageData = (char *)imageOut;

	if(cvIm1->nChannels == 4)
	{
		cvCvtColor(cvIm1, cvImRGB, CV_RGBA2RGB);
	}
	else if(cvIm1->nChannels == 3)
	{
		cvCopy(cvIm1, cvImRGB);
	}

	cvCvtColor(cvImRGB, cvImGray, CV_RGB2GRAY);
}



void finishImages()
{
	swImageStruct * imOut = ((swImageStruct *)plugin.data_out);
	mapIplImageToSwImage(cvImGray, imOut);
	return;

	if(cvIm2->nChannels>1) {
		if(cvIm2->nChannels == 3)
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGB);
		else
			cvCvtColor(cvImGray, cvIm2, CV_GRAY2RGBA);

		// Try to change outpue nchannels
		fprintf(stderr, "[%s] %s:%d : try to change output nChannels\n",
				__FILE__, __func__, __LINE__);
		mapIplImageToSwImage(cvImGray, imOut);
	}
	else
	{
		cvCopy(cvIm1, cvIm2);
	}
}





/*
 * Extract the DC terms from the specified component.
 */
IplImage *
extract_dc(j_decompress_ptr cinfo, jvirt_barray_ptr *coeffs, int ci)
{
    jpeg_component_info *ci_ptr = &cinfo->comp_info[ci];
    CvSize size = cvSize(ci_ptr->width_in_blocks, ci_ptr->height_in_blocks);
    IplImage *dc = cvCreateImage(size, IPL_DEPTH_8U, 1);
    assert(dc != NULL);

    JQUANT_TBL *tbl = ci_ptr->quant_table;
    UINT16 dc_quant = tbl->quantval[0];

#if DEBUG
    printf("DCT method: %x\n", cinfo->dct_method);
    printf
    (
        "component: %d (%d x %d blocks) sampling: (%d x %d)\n", 
        ci, 
        ci_ptr->width_in_blocks, 
        ci_ptr->height_in_blocks,
        ci_ptr->h_samp_factor, 
        ci_ptr->v_samp_factor
    );

    printf("quantization table: %d\n", ci);
    for (int i = 0; i < DCTSIZE2; ++i)
    {
        printf("% 4d ", (int)(tbl->quantval[i]));
        if ((i + 1) % 8 == 0)
            printf("\n");
    }

    printf("raw DC coefficients:\n");
#endif

    JBLOCKARRAY buf =
    (cinfo->mem->access_virt_barray)
    (
        (j_common_ptr)cinfo,
        coeffs[ci],
        0,
        ci_ptr->v_samp_factor,
        FALSE
    );
    for (int sf = 0; (JDIMENSION)sf < ci_ptr->height_in_blocks; ++sf)
    {
        for (JDIMENSION b = 0; b < ci_ptr->width_in_blocks; ++b)
        {
            int intensity = 0;

            intensity = buf[sf][b][0]*dc_quant/DCTSIZE + 128;
            intensity = MAX(0,   intensity);
            intensity = MIN(255, intensity);

            cvSet2D(dc, sf, (int)b, cvScalar(intensity));

#if DEBUG
            printf("% 2d ", buf[sf][b][0]);                        
#endif
        }
#if DEBUG
        printf("\n");
#endif
    }

    return dc;

}

IplImage *upscale_chroma(IplImage *quarter, CvSize full_size)
{
    IplImage *full = cvCreateImage(full_size, IPL_DEPTH_8U, 1);
    cvResize(quarter, full, CV_INTER_NN);
    return full;
}

GLOBAL(int)
read_JPEG_file (char * filename, IplImage **dc)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;

  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE * infile;        /* source file */

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  cinfo.err = jpeg_std_error(&jerr);

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.txt for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  jvirt_barray_ptr *coeffs = jpeg_read_coefficients(&cinfo);

  IplImage *y    = extract_dc(&cinfo, coeffs, 0);
  IplImage *cb_q = extract_dc(&cinfo, coeffs, 1);
  IplImage *cr_q = extract_dc(&cinfo, coeffs, 2);

  IplImage *cb = upscale_chroma(cb_q, cvGetSize(y));
  IplImage *cr = upscale_chroma(cr_q, cvGetSize(y));

  cvReleaseImage(&cb_q);
  cvReleaseImage(&cr_q);

#if OUTPUT_IMAGES
  cvSaveImage("y.png",   y);
  cvSaveImage("cb.png", cb);
  cvSaveImage("cr.png", cr);
#endif

  *dc = cvCreateImage(cvGetSize(y), IPL_DEPTH_8U, 3);
  assert(dc != NULL);

  cvMerge(y, cr, cb, NULL, *dc);

  cvReleaseImage(&y);
  cvReleaseImage(&cb);
  cvReleaseImage(&cr);

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  fclose(infile);

  return 1;
}

IplImage *  cvIm32Fin = NULL, *  cvIm32F = NULL, * cvImDCT = NULL;

void LogDCT()
{
	allocateImages();

	if(!cvIm32F) {
		cvIm32Fin = cvCreateImage(cvGetSize(cvImGray), IPL_DEPTH_32F, 1);
		cvIm32F = cvCreateImage(cvGetSize(cvImGray), IPL_DEPTH_32F, 1);
		cvImDCT = cvCreateImage(cvGetSize(cvImGray), IPL_DEPTH_32F, 1);
	}


	cvConvertScale(cvImGray, cvIm32Fin);
//	cvConvertScale(cvImGray, cvIm32F);
	cvLog(cvIm32Fin, cvIm32F);

	cvDCT(cvIm32F, cvImDCT, CV_DXT_FORWARD);

	// Reduce low DCT
	float rad2 = (int)LogDCT_radius * (int)LogDCT_radius / 10000.;
	for(int r  = 0; r<cvImDCT->height; r++)
	{
		float fr = (float)r / (float)cvImDCT->height;

		float * line = (float *)(cvImDCT->imageData + r*cvImDCT->widthStep);
		for(int c  = 0; c<cvImDCT->width; c++)
		{
			float fc = (float)c / (float)cvImDCT->width;
			float dc = fc*fc;
			float dr = fr*fr;
			if(dc > rad2 || dr > rad2)
			{
				line[c] = 0;
			}
		}
	}
//	cvCircle(cvImDCT, cvPoint(0,0),
//			 LogDCT_radius, cvScalarAll(0), -1);
	cvConvertScale(cvImDCT, cvImGray, 100.);

	cvDCT(cvImDCT, cvIm32F, CV_DXT_INVERSE);

	cvExp(cvIm32F, cvImDCT);
	cvConvertScale(cvImDCT, cvImGray, 1.);

	//cvConvertScale(cvIm32F, cvImGray, 1.);
	cvConvertScale(cvImDCT, cvImDCT, LogDCT_coef);
	// Substract low pass image from input image
	cvSub(cvIm32Fin, cvImDCT, cvIm32F);
	cvConvertScale(cvIm32F, cvImGray, 1.);
	//cvConvertScale(cvImDCT, cvImGray, 1.);

	cvAddS(cvIm32F, cvScalarAll(LogDCT_add), cvImDCT);
	cvConvertScale(cvImDCT, cvImGray, 1.);

	finishImages();
}


// DO NOT MODIFY BELOW THIS LINE ------------------------------------------------------------------------------


void signalhandler(int sig)
{
	fprintf(stderr, "================== RECEIVED SIGNAL %d = '%s' From process %d ==============\n", sig, sys_siglist[sig], getpid());
	signal(sig, signalhandler);
	if(sig != SIGUSR1) {
		exit(0);
	}
}
int main(int argc, char *argv[])
{
	// SwPluginCore load
	for(int i=0; i<NSIG; i++) {
		signal(i, signalhandler);
	}

	fprintf(stderr, "registerCategory '%s'/'%s'...\n", CATEGORY, SUBCATEGORY);
	plugin.registerCategory(CATEGORY, SUBCATEGORY);

	// register functions
	fprintf(stderr, "register %d functions...\n", nb_functions);
	plugin.registerFunctions(functions, nb_functions );

	// process loop
	fprintf(stderr, "loop...\n");
	plugin.loop();

	return EXIT_SUCCESS;
}


