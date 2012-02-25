/***************************************************************************
 *  imageinfo - Information about picture with image processing
 *
 *  Jul 2 21:10:56 2009
 *  Copyright  2009  Christophe Seyve
 *  Email cseyve@free.fr
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include <sstream>
#include "imgutils.h"

#ifndef MAX_PATH_LEN
/// Maximum length of path = UTF-8 * 512 char
#define MAX_PATH_LEN	1024
#endif

/// Max length of an EXIF field
#define MAX_EXIF_LEN	64

// Debug modes
#define EMALOG_TRACE	SWLOG_TRACE
#define EMALOG_DEBUG	SWLOG_DEBUG
#define EMALOG_INFO		SWLOG_INFO
#define EMALOG_WARNING	SWLOG_WARNING
#define EMALOG_ERROR	SWLOG_ERROR

#include <QImage>
#include <QString>
#include <QStringList>

#include "imgutils.h"
#include "FileVideoAcquisition.h"

/** @breif Convert an OpenCV IplIMage to a QImage */
QImage iplImageToQImage(IplImage * iplImage, bool swap_RB = true);

typedef struct {
	char * fullpath;				/*!< full path of original image */
	IplImage * iplImage;			/*!< raw image for faster display */
	u8 * compressed;				/*!< compressed image data as JPEG */
	int compressed_size;			/*!< Size of compressed image data */
} t_cached_image;

/** @brief Compress raw IplImage to JPEG */
void compressCachedImage(t_cached_image *);

/** @brief Uncompress JPEG buffer to raw IplImage */
void uncompressCachedImage(t_cached_image *);

/** @brief EXIF metadata for pictures */
typedef struct {

	QString maker;	/*! Company which produces this camera */
	QString model;	/*! Model of this camera */

	QString datetime;	/*! Date/time of the shot */

	/************************ IMAGE *********************/

	char orientation;			/*! Image orientation : 0 for horizontal (landscape), 1 for vertical (portrait) */
	float focal_mm;				/*! Real focal in mm */
	float focal_eq135_mm;		/*! 135mm equivalent focal in mm (if available) */
	float aperture;				/*! F Number */
	float speed_s;				/*! Speed = shutter opening time in seconds */
	int ISO;					/*! ISO Sensitivity */
} t_exif_data;

/** @brief IPTC data */
typedef struct {

// Ref: /usr/share/doc/libexiv2-doc/html/tags-iptc.html
//0x005a 	90 	Iptc.Application2.City 	String 	No 	No 	0 	32 	Identifies city of object data origin according to guidelines established by the provider.
	QString city;		/*! IPTC City name, field Iptc.Application2.City */
//0x005c 	92 	Iptc.Application2.SubLocation 	String 	No 	No 	0 	32 	Identifies the location within a city from which the object data originates
	QString sublocation;		/*! IPTC Province/State name, field Iptc.Application2.Provincestate */
//0x005f 	95 	Iptc.Application2.ProvinceState 	String 	No 	No 	0 	32 	Identifies Province/State of origin according to guidelines established by the provider.
	QString provincestate;		/*! IPTC Province/State name, field Iptc.Application2.Provincestate */
//0x0064 	100 	Iptc.Application2.CountryCode 	String 	No 	No 	3 	3 	Indicates the code of the country/primary location where the intellectual property of the object data was created
	QString countrycode;		/*! IPTC City name, field Iptc.Application2.City */
//0x0065 	101 	Iptc.Application2.CountryName 	String 	No 	No 	0 	64 	Provides full
	QString countryname;		/*! IPTC City name, field Iptc.Application2.City */

} t_iptc_data;

/** @brief Useful information about a picture: EXIF, kerywords...

*/
typedef struct {
	QString filepath;			/*!< Full path of image file */
	QString cache_file;			/*!< Path of XML cache file */
	unsigned char valid;		/*!< Valid info flag */
	int width, height;
	int nChannels;	///< Depth of images
	bool isMovie;	///< Flag for movie


	// EXIF TAGS
	t_exif_data exif;
	// IPTC TAGS
	t_iptc_data iptc;


	/****************************** MOVIE ***********************************/
	char FourCC[5];
	unsigned long long filesize;
	float fps;	///< Framerate in frame per sec

	/****************************** DATA ***********************************/
	// ---- Custom tags ----
	QStringList keywords;			/*! User's custom keywords */

	// Image processing data
	bool grayscaled;
	float sharpness_score;			/*! Sharpness factor in [0..100] */
	float histo_score;

	t_cached_image thumbImage;		/*! Thumb image for faster display */
	IplImage * sharpnessImage;		/*! Sharpness image for faster display */
	IplImage * hsvImage;			/*! HSV histogram image for faster display */

	float * log_histogram[3];	/*! Log histogram */

	// Image judgement
	float score;			/*! Final score factor in [0..100] */
	QList<t_movie_pos> bookmarksList;	/*! List of bookmarks */

} t_image_info_struct;

/** @brief Empty image struct cleanly (it contains objects) */
void clearImageInfoStruct(t_image_info_struct * pinfo);


void printImageInfoStruct(t_image_info_struct * pinfo);

/** @brief Load a XML file containing data about the known file

  @return 0 if success, <0 if error
*/
int loadImageInfoStruct(t_image_info_struct * pinfo, QString path);

/** @brief save a XML file containing data about the known file
*/
void saveImageInfoStruct(t_image_info_struct * pinfo, QString path = "");


/** @brief Image processing analyse class
  */
class ImageInfo {
public:
	ImageInfo();
	~ImageInfo();
	/** @brief Load input file as an image */
	int loadFile(QString filename);

	/** @brief Load input file as a movie  */
	int loadMovieFile(QString filename);

	/** @brief Return colored histogram */
	IplImage * getHistogram() { return m_HistoImage; };
	IplImage * getColorHistogram() { return m_ColorHistoImage; };
	IplImage * getSharpnessImage() { return m_sharpnessImage; };

	float getSharpness() { return m_image_info_struct.sharpness_score; };

	/** @brief Get structure containing every image information needed for sorting */
	t_image_info_struct getImageInfo() { return m_image_info_struct; };

	/** @brief purge temporary thumbs */
	void purgeThumbs();

private:
	/** @brief Initialization function */
	void init();
	/** @brief Desallocation function */
	void purge();

	/** @brief Read EXIF and IPTC metadata in image file */
	int readMetadata(QString filename);

	/** @brief Structure containing every image information needed for sorting */
	t_image_info_struct m_image_info_struct;

	/// File video acquisition used to read movie properties
	FileVideoAcquisition mFileVA;


	/** @brief Original image */
	IplImage * m_originalImage;

	/** @brief Histogram */
	IplImage * m_HistoImage;

	/** @brief Scaled version of original image */
	IplImage * m_scaledImage;
	/*! Thumb image for faster display */
	IplImage * m_thumbImage;
	/** @brief Scaled & grayscaled version of original image */
	IplImage * m_grayImage;
	/** @brief Scaled Sobel image */
	IplImage * m_sobelImage;

	/** @brief Scaled sharp image */
	IplImage * m_sharp32fImage;

	/** @brief Scaled & HSV version of original image */
	IplImage * hsvImage;
	/** @brief Scaled & HSV version of original image, H plane */
	IplImage * h_plane;
	/** @brief Scaled & HSV version of original image, S plane */
	IplImage * s_plane;

	/** @brief Scaled & RGB version of original image, RGBA planes array */
	IplImage * m_rgb_plane[4];

	/** @brief Sharpness image */
	IplImage * m_sharpnessImage;

	/** @brief HSV output image */
	IplImage * hsvOutImage;

	/** @brief purge scaled images */
	void purgeScaled();

	/** @brief Process HSV analysis */
	int processHSV();

	/** @brief Process RGB histogram analysis */
	int processRGB();

	/** @brief Process sharpness analysis */
	int processSharpness();



#define H_MAX	180
#define S_MAX	255
	IplImage * m_HSHistoImage;
	IplImage * m_ColorHistoImage;

//	unsigned long m_HSHistoImage[H_MAX][S_MAX];
};

#endif // IMAGEINFO_H

