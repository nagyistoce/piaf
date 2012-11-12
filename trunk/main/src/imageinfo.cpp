/***************************************************************************
 *  imageinfo - Information about picture with image processing
 *
 *  Jul 2 21:10:56 2009
 *  Copyright  2007  Christophe Seyve
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

#include <QString>
#include <QStringList>
#include <QBuffer>
#include <QFileInfo>
#include <sstream>

#include <QDomDocument>
#include <QTextStream>

#include "imageinfo.h"

#include "imgutils.h"
#include "piaf-common.h"

// Read metadata
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/iptc.hpp>

#include <iostream>
#include <iomanip>
#include <cassert>

#include "ffmpeg_file_acquisition.h"

u8 g_debug_ImageInfo = EMALOG_DEBUG;

/******************************************************************************/
ImageInfo::ImageInfo() {
	init();
}
ImageInfo::~ImageInfo() {

	fprintf(stderr, "ImageInfo::%s:%d : delete this=%p\n",
			__func__, __LINE__, this);
	for(int h=0; h<3; h++) {
		if(m_image_info_struct.log_histogram[h])
		{
			delete [] m_image_info_struct.log_histogram[h];
			m_image_info_struct.log_histogram[h] = NULL;
		}
	}

	purgeThumbs();
	purge(); // delete images (should be already done)

}

void ImageInfo::purgeThumbs() {
	// just set the pointer to null, the image themselves will be deleted in purgeScaled
	m_image_info_struct.thumbImage.iplImage =
			m_image_info_struct.sharpnessImage =
			m_image_info_struct.hsvImage = NULL;

	tmReleaseImage(&m_thumbImage);
	tmReleaseImage(&m_sharpnessImage);
	tmReleaseImage(&m_ColorHistoImage);

}

void ImageInfo::purge() {
	if(mpFileVA)
	{
		delete mpFileVA;
		mpFileVA = NULL;
	}

	tmReleaseImage(&m_originalImage);
	tmReleaseImage(&m_HistoImage); // Delete only at the end, because it's always the same size
	tmReleaseImage(&hsvOutImage);
	purgeScaled();
}


void clearImageInfoStruct(t_image_info_struct * pinfo)
{
	pinfo->valid = 0;
	pinfo->Date = pinfo->Tick = 0;
//			// EXIF TAGS
//			QString  maker;	/*! Company which produces this camera */
//			QString model;	/*! Model of this camera */

//			QString datetime;	/*! Date/time of the shot */

	pinfo->exif.orientation = 0;			/*! Image orientation : 0 for horizontal (landscape), 1 for vertical (portrait) */
	pinfo->exif.focal_mm = 				/*! Real focal in mm */
			pinfo->exif.focal_eq135_mm =		/*! 135mm equivalent focal in mm (if available) */
			pinfo->exif.aperture =				/*! F Number */
			pinfo->exif.speed_s = 0.f;				/*! Speed = shutter opening time in seconds */
	pinfo->exif.ISO = 0;					/*! ISO Sensitivity */

//			// IPTC TAGS
//		// Ref: /usr/share/doc/libexiv2-doc/html/tags-iptc.html
//		//0x005a 	90 	Iptc.Application2.City 	String 	No 	No 	0 	32 	Identifies city of object data origin according to guidelines established by the provider.
//			QString iptc_city[MAX_EXIF_LEN];		/*! IPTC City name, field Iptc.Application2.City */
//		//0x005c 	92 	Iptc.Application2.SubLocation 	String 	No 	No 	0 	32 	Identifies the location within a city from which the object data originates
//			QString iptc_sublocation[MAX_EXIF_LEN];		/*! IPTC Province/State name, field Iptc.Application2.Provincestate */
//		//0x005f 	95 	Iptc.Application2.ProvinceState 	String 	No 	No 	0 	32 	Identifies Province/State of origin according to guidelines established by the provider.
//			QString iptc_provincestate[MAX_EXIF_LEN];		/*! IPTC Province/State name, field Iptc.Application2.Provincestate */
//		//0x0064 	100 	Iptc.Application2.CountryCode 	String 	No 	No 	3 	3 	Indicates the code of the country/primary location where the intellectual property of the object data was created
//			QString iptc_countrycode[MAX_EXIF_LEN];		/*! IPTC City name, field Iptc.Application2.City */
//		//0x0065 	101 	Iptc.Application2.CountryName 	String 	No 	No 	0 	64 	Provides full
//			QString iptc_countryname[MAX_EXIF_LEN];		/*! IPTC City name, field Iptc.Application2.City */

	pinfo->width = pinfo->height = 0;
	pinfo->filesize = 0;
	// Movie
	strcpy(pinfo->FourCC, "????");
	pinfo->isMovie = false;
	pinfo->fps = 0.f;
	pinfo->bookmarksList.clear();

	// Image processing data
	pinfo->grayscaled = 0;
	pinfo->sharpness_score =			/*! Sharpness factor in [0..100] */
			pinfo->histo_score = pinfo->score = 0.f;

	memset(&pinfo->thumbImage, 0, sizeof(t_cached_image)); // this struct is only C
	pinfo->sharpnessImage =		/*! Sharpness image for faster display */
			pinfo->hsvImage = NULL;			/*! HSV histogram image for faster display */

	//		float * log_histogram[3];	/*! Log histogram */
	memset(pinfo->log_histogram, 0, 3*sizeof(float *));

}



void ImageInfo::init() {
	m_originalImage = NULL;
	mpFileVA = NULL;
	m_thumbImage = m_scaledImage = m_grayImage = m_HSHistoImage =
	m_HistoImage = m_ColorHistoImage =
	hsvImage = hsvOutImage = h_plane = s_plane = NULL;
	for(int rgb=0; rgb<4; rgb++) {
		m_rgb_plane[rgb] = NULL;
	}


	m_sharp32fImage = m_sobelImage = NULL;
	m_sharpnessImage = NULL;
}


QString rational(const QString & str) {
	if(!(str.contains("/"))) return str;

	QString fract = str;
	// split
	QStringList splitted = fract.split('/');
	bool ok;
	int num = splitted[0].toInt(&ok);
	if(!ok) return fract;
	int den = splitted[1].toInt(&ok);
	if(!ok) return fract;

	float val = (float)num / (float)den;

	return fract.sprintf("%g", val);
}

QString rational_1_div(const QString & str) {
	if(!(str.contains("/"))) return str;

	QString fract = str;
	// split
	QStringList splitted = fract.split('/');
	bool ok;
	int num = splitted[0].toInt(&ok);
	if(!ok) return fract;
	int den = splitted[1].toInt(&ok);
	if(!ok) return fract;

	if(num == 0) return fract;
	float val = (float)den / (float)num;

	return fract.sprintf("1/%g", val);
}

float rational_to_float(const QString & str) {
	if(!(str.contains("/"))) {
		//
		float val;
		bool ok;
		val = str.toFloat(&ok);
		if(!ok) return -1.f;
		return val;
	}
	QString fract = str;

	// split
	QStringList splitted = fract.split('/');
	bool ok;
	int num = splitted[0].toInt(&ok);
	if(!ok) return -1.f;
	int den = splitted[1].toInt(&ok);
	if(!ok) return (float)num;

	if(num == 0) {
		if(den == 0) return 0.f; // 0/0 = 0

		return -1.f;
	}
	float val = (float)num / (float)den;

	return val; //fract.sprintf("1/%g", val);
}

/* Compress raw IplImage to JPEG */
void compressCachedImage(t_cached_image * img) {
	if(img->compressed) { return ; }

	// Compressed directly in RAM
	QImage image = iplImageToQImage(img->iplImage);

	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	//
	image.save(&buffer, "JPEG", 90);

	/*
	// Copy into buffer
	img->compressed_size = ba.size();
	img->compressed = new u8 [ ba.size() ];
	buffer.read((char *)img->compressed, img->compressed_size);

	//
	fprintf(stderr, "[imageinfo] %s:%d: DEBUG : save '/dev/shm/toto.jpg' %p / %d\n",
			__func__, __LINE__,
			img->compressed, img->compressed_size);
	FILE * f=fopen("/dev/shm/toto.jpg", "wb");
	if(f) {
		fwrite(img->compressed, 1, img->compressed_size, f);
		fclose(f);
	}*/
}

/* Uncompress JPEG buffer to raw IplImage */
void uncompressCachedImage(t_cached_image * img) {
	if(img->iplImage) { return ; }

}


void ImageInfo::purgeScaled() {
	fprintf(stderr, "ImageInfo::%s:%d : purging scaled for this=%p\n", __func__, __LINE__, this);
	// purge info data
	if(m_scaledImage == m_grayImage) {
		m_grayImage = NULL;
	} else {
		tmReleaseImage(&m_grayImage);
	}

	tmReleaseImage(&m_scaledImage);
	for(int rgb=0; rgb<4; rgb++) {
		tmReleaseImage(&m_rgb_plane[rgb]);
	}

	tmReleaseImage(&m_sharp32fImage);
	tmReleaseImage(&m_sobelImage);
	tmReleaseImage(&m_HSHistoImage);

	tmReleaseImage(&m_HistoImage);
	tmReleaseImage(&hsvImage);
	tmReleaseImage(&h_plane);
	tmReleaseImage(&s_plane);
}

int ImageInfo::readMetadata(QString filename) {
	// Read metadata
	try {
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open( filename.toStdString() );
		assert(image.get() != 0);
		image->readMetadata();

		Exiv2::ExifData &exifData = image->exifData();
		if (exifData.empty()) {
			std::string error(filename.toStdString());
			error += ": No Exif data found in the file";
			throw Exiv2::Error(1, error);
		}
		Exiv2::ExifData::const_iterator end = exifData.end();

		QString displayStr;

		Exiv2::Exifdatum& exifMaker = exifData["Exif.Image.Make"];
		std::string str = exifMaker.toString();
		m_image_info_struct.exif.maker = QString::fromStdString(str);

		exifMaker = exifData["Exif.Image.Model"];str = exifMaker.toString();
		m_image_info_struct.exif.model = QString::fromStdString(str);

		// DateTime
		exifMaker = exifData["Exif.Photo.DateTimeOriginal"]; str = exifMaker.toString();
		m_image_info_struct.exif.datetime = QString::fromStdString(str);

		// Orientation
		exifMaker = exifData["Exif.Image.Orientation"]; str = exifMaker.toString();
		displayStr = QString::fromStdString(str);
		m_image_info_struct.exif.orientation = (char)( displayStr.contains("0") ? 0 : 1);


		// Focal
		exifMaker = exifData["Exif.Photo.FocalLengthIn35mmFilm"];
		str = exifMaker.toString();
		displayStr = QString::fromStdString(str);

		if(QString::compare(displayStr, "0")) {
			exifMaker = exifData["Exif.Photo.FocalLength"]; str = exifMaker.toString();
			displayStr = QString::fromStdString(str);

			//bool ok;
			//displayStr = rational(displayStr);
			m_image_info_struct.exif.focal_mm = rational_to_float(displayStr);
			m_image_info_struct.exif.focal_eq135_mm = -1.f;
		} else {
			m_image_info_struct.exif.focal_eq135_mm = rational_to_float(displayStr);
			m_image_info_struct.exif.focal_mm = -1.f;

			displayStr += QString("eq. 35mm");
		}
		if(g_debug_ImageInfo) {
			fprintf(stderr, "\t[ImageInfo %p]::%s:%d: Focal : '%s' => %g s %g 35mm\n",
					this, __func__, __LINE__,
					displayStr.toUtf8().data(),
					m_image_info_struct.exif.focal_mm,
					m_image_info_struct.exif.focal_eq135_mm);
		}

		// Aperture
		exifMaker = exifData["Exif.Photo.FNumber"];
		str = exifMaker.toString();
		displayStr = QString::fromStdString(str);
		m_image_info_struct.exif.aperture = rational_to_float(displayStr);

		// Speed
		exifMaker = exifData["Exif.Photo.ExposureTime"];
		str = exifMaker.toString();
		displayStr = QString::fromStdString(str);
		m_image_info_struct.exif.speed_s = rational_to_float(displayStr);
		if(g_debug_ImageInfo) {
			fprintf(stderr, "\t[ImageInfo %p]::%s:%d: Exposure time : '%s' => %g s\n",
					this, __func__, __LINE__,
					displayStr.toUtf8().data(), m_image_info_struct.exif.speed_s);
		}

		// ISO
		exifMaker = exifData["Exif.Photo.ISOSpeedRatings"]; str = exifMaker.toString();
		displayStr = QString::fromStdString(str);
		m_image_info_struct.exif.ISO = (int)rational_to_float(displayStr);

#if 0
		for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {

/*			str.sprintf("%d ", i->key());
			displayStr += QString(i->key()) ;
*/
			QString str; str = (i->tag());

//			displayStr += str+ QString(" = ") + "\n";
//			str.sprintf("%d ", i->value());

			//displayStr += QString(i->value()) + "\n";

			std::cout << std::setw(44) << std::setfill(' ') << std::left
					<< i->key() << " "
					<< "0x" << std::setw(4) << std::setfill('0') << std::right
					<< std::hex << i->tag() << " "
					<< std::setw(9) << std::setfill(' ') << std::left
					<< i->typeName() << " "
					<< std::dec << std::setw(3)
					<< std::setfill(' ') << std::right
					<< i->count() << "  "
					<< std::dec << i->value()
					<< "\n";
		}
#endif

		// IPTC - IPTC - IPTC - IPTC - IPTC - IPTC - IPTC - IPTC - IPTC -
		Exiv2::IptcData &iptcData = image->iptcData();
		if (iptcData.empty()) {
			std::string error(filename.toAscii().data());
			error += ": No IPTC data found in the file";
			if(g_debug_ImageInfo) {
				fprintf(stderr, "\t[ImageInfo %p]::%s:%d: No IPTC data found in the file: '%s' => throw error\n",
						this, __func__, __LINE__,
						filename.toAscii().data() );
			}
			throw Exiv2::Error(1, error);
		}

		Exiv2::IptcData::iterator endIPTC = iptcData.end();
		for (Exiv2::IptcData::iterator md = iptcData.begin(); md != endIPTC; ++md) {
			std::cout << std::setw(44) << std::setfill(' ') << std::left
					  << md->key() << " "
					  << "0x" << std::setw(4) << std::setfill('0') << std::right
					  << std::hex << md->tag() << " "
					  << std::setw(9) << std::setfill(' ') << std::left
					  << md->typeName() << " "
					  << std::dec << std::setw(3)
					  << std::setfill(' ') << std::right
					  << md->count() << "  "
					  << std::dec << md->value()
					  << std::endl;
		}
		return 0;
	}
	catch (Exiv2::AnyError& e) {
		std::cout << "Caught Exiv2 exception '" << e << "'\n";
		fprintf(stderr, "ImageInfo %p::%s:%d : ERROR : caught exception => return 0 (no metadata);\n",
				this, __func__, __LINE__); fflush(stderr);
		return 0;
	}

	fprintf(stderr, "ImageInfo %p::%s:%d : FINE : return 0;\n", this, __func__, __LINE__);

	return 0;
}

int ImageInfo::loadMovieFile(QString filename)
{
	//
	m_image_info_struct.valid = 0;

	clearImageInfoStruct(&m_image_info_struct);
	m_image_info_struct.valid = 0;

	// File info
	QFileInfo fi(filename);
	if(!fi.exists()) {
		PIAF_MSG(SWLOG_ERROR, "cannot open file '%s': it does not exists",
				 fi.absoluteFilePath().toUtf8().data());
		return -1;
	}

	PIAF_MSG(SWLOG_INFO, "open file '%s'", fi.absoluteFilePath().toAscii().data())

	m_image_info_struct.filesize = fi.size();

	// load movie
	tBoxSize size;
	size.x = size.y = 0;
	size.width = 320; size.height = 240; //
	if(!mpFileVA)
	{/// \bug FIXME: use factory
		mpFileVA = new FFmpegFileVideoAcquisition();
	}

	if(mpFileVA->openDevice( fi.absoluteFilePath().toUtf8().data(), size)<0) {
		PIAF_MSG(SWLOG_ERROR, "cannot open file '%s'", fi.absoluteFilePath().toUtf8().data());
		return -1;
	}
	int retgrab = mpFileVA->grab();

	t_video_properties props = mpFileVA->getVideoProperties();

	// Read properties
	m_image_info_struct.width = props.frame_width;
	m_image_info_struct.height = props.frame_height;
	m_image_info_struct.fps = props.fps;
	m_image_info_struct.isMovie = true;

	PIAF_MSG(SWLOG_INFO, "file '%s' : retgrab=%d => %dx%d @ %g fps",
			  fi.absoluteFilePath().toUtf8().data(),
			  retgrab,
			  m_image_info_struct.width, m_image_info_struct.height, m_image_info_struct.fps );

	m_originalImage = tmCloneImage(mpFileVA->readImageRGB32());
	// Invert R<->B
	if(m_originalImage->nChannels == 4)
	{
		cvCvtColor(mpFileVA->readImageRGB32(), m_originalImage, CV_BGRA2RGBA);
	}
	return 0;
}

int ImageInfo::loadFile(QString filename)
{
	tmReleaseImage(&m_originalImage);

	clearImageInfoStruct(&m_image_info_struct);
	m_image_info_struct.valid = 0;

	// File info
	QFileInfo fi(filename);
	if(!fi.exists()) {
		fprintf(stderr, "ImageInfo::%s:%d: file '%s' oes not exists (with Qt)\n",
				__func__, __LINE__, filename.toAscii().data());
		return -1;
	}

	m_image_info_struct.filesize = fi.size();

	fprintf(stderr, "ImageInfo::%s:%d: reading file '%s' ...\n",
			__func__, __LINE__, filename.toAscii().data());

	// LOAD IMAGE PIXMAP
	m_originalImage = cvLoadImage(filename.toUtf8().data());
	m_image_info_struct.filepath = QString( filename );

	if(!m_originalImage) {
		fprintf(stderr, "ImageInfo::%s:%d: cannot load image '%s' (with openCV)\n",
				__func__, __LINE__, filename.toAscii().data());

		// Load as movie
		loadMovieFile(filename);
	}
	else
	{
		// LOAD IMAGE METADATA
		readMetadata(filename);
	}

	if(m_originalImage) {
		m_image_info_struct.width = m_originalImage->width;
		m_image_info_struct.height = m_originalImage->height;
		m_image_info_struct.nChannels = m_originalImage->nChannels;

		if(g_debug_ImageInfo) {
			fprintf(stderr, "\tImageInfo::%s:%d : loaded '%s' : %dx%d x %d\n", __func__, __LINE__,
					filename.toAscii().data(),
					m_originalImage->width, m_originalImage->height,
					m_originalImage->nChannels );
		}
		m_image_info_struct.grayscaled = (m_originalImage->depth == 1);

		m_originalImage = tmAddBorder4x(m_originalImage); // it will purge originalImage
		if(g_debug_ImageInfo) {
			fprintf(stderr, "\tImageInfo::%s:%d => pitchedx4: %dx%d x %d\n", __func__, __LINE__,
				m_originalImage->width, m_originalImage->height,
				m_originalImage->nChannels );
		}

#define IMGINFO_WIDTH	400
#define IMGINFO_HEIGHT	400

		// Scale to analysis size
		//		cvResize(tmpDisplayImage, new_displayImage, CV_INTER_LINEAR );
		int sc_w = IMGINFO_WIDTH;
		int sc_h = IMGINFO_HEIGHT;
		float scale_factor_w = (float)m_originalImage->width / (float)IMGINFO_WIDTH;
		float scale_factor_h = (float)m_originalImage->height / (float)IMGINFO_HEIGHT;
		if(scale_factor_w > scale_factor_h) {
			// limit w
			sc_w = m_originalImage->width * IMGINFO_HEIGHT/m_originalImage->height;
		} else {
			sc_h = m_originalImage->height * IMGINFO_WIDTH/m_originalImage->width;
		}

		while((sc_w % 4) != 0) { sc_w++; }
		while((sc_h % 4) != 0) { sc_h++; }

		if(m_scaledImage
		   && (m_scaledImage->width != sc_w || m_scaledImage->height != sc_h
			   || m_scaledImage->nChannels != m_originalImage->nChannels)) {
			fprintf(stderr, "ImageInfo::%s:%d : this=%p size changed => purge scaled\n",
					__func__, __LINE__, this);
			// purge scaled images
			purgeScaled();

			purgeThumbs();
		}

		// Scale original image to smaller image to fasten later processinggs
		if(!m_scaledImage) {
			m_scaledImage = tmCreateImage(cvSize(sc_w, sc_h),
										  IPL_DEPTH_8U, m_originalImage->nChannels);
		}

		// Scale original image to processing image
		cvResize(m_originalImage, m_scaledImage);

#define IMGTHUMB_WIDTH	80
#define IMGTHUMB_HEIGHT 80

		// Scale rescaled to thumb size
		if(!m_thumbImage) {
			int th_w = IMGTHUMB_WIDTH;
			int th_h = IMGTHUMB_HEIGHT;
			float th_factor_w = (float)m_originalImage->width / (float)IMGTHUMB_WIDTH;
			float th_factor_h = (float)m_originalImage->height / (float)IMGTHUMB_HEIGHT;
			if(th_factor_w > th_factor_h) {
				// limit w
				th_w = m_originalImage->width * IMGTHUMB_HEIGHT / m_originalImage->height;
			} else {
				th_h = m_originalImage->height * IMGTHUMB_WIDTH / m_originalImage->width;
			}

			while((th_w % 4) != 0) { th_w++; }
			while((th_h % 4) != 0) { th_h++; }

			m_thumbImage = tmCreateImage( cvSize(th_w, th_h), IPL_DEPTH_8U, m_originalImage->nChannels);
		}

		cvResize(m_scaledImage, m_thumbImage);

		m_image_info_struct.thumbImage.iplImage = m_thumbImage;

		compressCachedImage(&m_image_info_struct.thumbImage);

		if(g_debug_ImageInfo) {
			fprintf(stderr, "\tImageInfo::%s:%d : scaled to %dx%d\n", __func__, __LINE__,
					m_scaledImage->width, m_scaledImage->height);
			fprintf(stderr, "\tImageInfo::%s:%d : thumb %dx%d\n", __func__, __LINE__,
					m_thumbImage->width, m_thumbImage->height);

			fprintf(stderr, "\tImageInfo::%s:%d : processRGB(m_scaledImage=%dx%d)...\n", __func__, __LINE__,
					m_scaledImage->width, m_scaledImage->height);fflush(stderr);
		}


#ifndef PIAFWORKFLOW
		// process RGB histogram
		processRGB();

		// process color analysis
		processHSV();

		// then sharpness
		if(g_debug_ImageInfo) {
			fprintf(stderr, "\tImageInfo::%s:%d : processSharpness(gray=%dx%d)\n", __func__, __LINE__,
					m_grayImage->width, m_grayImage->height);fflush(stderr);
		}
		processSharpness();

		if(g_debug_ImageInfo) {
			fprintf(stderr, "\tImageInfo::%s:%d : process done (gray=%dx%d)\n", __func__, __LINE__,
					m_grayImage->width, m_grayImage->height);fflush(stderr);
		}

		/* Compute the final score
		Score is the combination of several criteria :
		- sharpness : proportioanl, and best if superior to 50 %
	*/
		float sharpness_score = tmmin(1.f,
									  2.f * m_image_info_struct.sharpness_score / 100.f);
		float histo_score = tmmin(1.f,
								  2.f * m_image_info_struct.histo_score / 100.f);

		m_image_info_struct.score = sharpness_score
									* histo_score
									* 100.f ; // in percent finally
#else
		m_image_info_struct.sharpnessImage = NULL;
		m_image_info_struct.hsvImage = NULL;
		m_image_info_struct.score = -1;
#endif

		// Activate the validation flag
		m_image_info_struct.valid = 1;
	}

	return 0;
}

int ImageInfo::processHSV() {
	// Change to HSV
	if(m_scaledImage->nChannels < 3) {
		// Clear histogram and return
		fprintf(stderr, "ImageInfo::%s:%d : not coloured image : nChannels=%d\n", __func__, __LINE__,
			m_scaledImage->nChannels );
		m_grayImage = m_scaledImage;
		return 0;
	}
/*
void cvCvtColor( const CvArr* src, CvArr* dst, int code );

RGB<=>HSV (CV_BGR2HSV, CV_RGB2HSV, CV_HSV2BGR, CV_HSV2RGB)


// In case of 8-bit and 16-bit images
// R, G and B are converted to floating-point format and scaled to fit 0..1 range
The values are then converted to the destination data type:
	8-bit images:
		V <- V*255, S <- S*255, H <- H/2 (to fit to 0..255)

  */


	if(!hsvImage) {
		hsvImage = tmCreateImage(
			cvSize(m_scaledImage->width, m_scaledImage->height),
			IPL_DEPTH_8U,
			m_scaledImage->nChannels);
	}
	if(m_scaledImage->nChannels == 3) {
		//cvCvtColor(m_scaledImage, hsvImage, CV_RGB2HSV);
		cvCvtColor(m_scaledImage, hsvImage, CV_BGR2HSV);
	} else {
		cvCvtColor(m_scaledImage, hsvImage, CV_BGR2HSV);
	}

	if(!h_plane) h_plane = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	if(!s_plane) s_plane = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	if(!m_grayImage) {
		m_grayImage = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	}

	cvCvtPixToPlane( hsvImage, h_plane, s_plane, m_grayImage, 0 );

	if(g_debug_ImageInfo) {
		tmSaveImage(TMP_DIRECTORY "HImage.pgm", h_plane);
		tmSaveImage(TMP_DIRECTORY "SImage.pgm", s_plane);
		tmSaveImage(TMP_DIRECTORY "VImage.pgm", m_grayImage);
	}


	// save image for debug
	if(g_debug_ImageInfo) {
		tmSaveImage(TMP_DIRECTORY "HSVimage.ppm", hsvImage);
	}

#define HSVHISTO_WIDTH	90
#define HSVHISTO_HEIGHT	64

	float h_scale = (float)HSVHISTO_WIDTH / (float)H_MAX;
	float s_scale = (float)HSVHISTO_HEIGHT / (float)S_MAX;

	if(!m_HSHistoImage) {
		m_HSHistoImage = tmCreateImage(cvSize(HSVHISTO_WIDTH , HSVHISTO_HEIGHT), IPL_DEPTH_8U, 1);
	} else {
		cvZero(m_HSHistoImage);
	}

	// Then process histogram
	for(int r=0; r<hsvImage->height; r++) {
		u8 * hline = (u8 *)(h_plane->imageData
										+ r * h_plane->widthStep);
		u8 * sline = (u8 *)(s_plane->imageData
										+ r * s_plane->widthStep);
		for(int c = 0; c<s_plane->width; c++) {
			int h = (int)(hline[c]);
			int s = (int)(sline[c]);

			// Accumulate in scaled image for saving time
			int histo_h = (int)roundf(h * h_scale);
			int histo_s = (int)roundf(s * s_scale);
			if(histo_h<m_HSHistoImage->widthStep
			   && histo_s<m_HSHistoImage->height) {
				// Increase image
				u8 * pHisto = // H as columns, S as line
					(u8 *)(m_HSHistoImage->imageData + histo_s * m_HSHistoImage->widthStep)
					+ histo_h;
				u8 val = *pHisto;
				if(val < 255) {
					*pHisto = val+1;
				}
			}
		}
	}

	// save image for debug
	if(g_debug_ImageInfo) {
		tmSaveImage(TMP_DIRECTORY "HSHisto.pgm", m_HSHistoImage);
	}

	if(!hsvOutImage) {
		hsvOutImage = tmCreateImage(
			cvSize(m_HSHistoImage->width, m_HSHistoImage->height),
			IPL_DEPTH_8U,
			3);
	}

	// Fill with H,S and use Value for highlighting colors
	for(int r=0; r<hsvOutImage->height; r++) {
		u8 * outline = (u8 *)(hsvOutImage->imageData
						+ r * hsvOutImage->widthStep);
		u8 * histoline = (u8 *)(m_HSHistoImage->imageData
						+ r * m_HSHistoImage->widthStep);
		int c1 = 0;
		for(int c = 0; c1<HSVHISTO_WIDTH;
			c1++, c+=hsvOutImage->nChannels) {
			outline[c] = roundf(c1 / h_scale); // H
			outline[c+1] = roundf(r / h_scale); // S
			outline[c+2] = 64 + (int)tmmin( (float)histoline[c1], 170.f);
					//histoline[c1]>1 ? 255 : 64; //histoline[c1];
		}
	}

	if(!m_ColorHistoImage) {
		m_ColorHistoImage = tmCreateImage(
			cvSize(hsvOutImage->width, hsvOutImage->height),
			IPL_DEPTH_8U,
			3);
	} // else cvZero(m_ColorHistoImage); // don't need to clear, because always the same size

	cvCvtColor(hsvOutImage, m_ColorHistoImage, CV_HSV2BGR);

	m_image_info_struct.hsvImage = m_ColorHistoImage;

	//tmSaveImage(TMP_DIRECTORY "HSHistoColored.ppm", m_ColorHistoImage);
	if(g_debug_ImageInfo) {
		tmSaveImage(TMP_DIRECTORY "HSHistoHSV.ppm", hsvOutImage);
	}

	return 0;
}


int ImageInfo::processRGB() {

	// Compute RGB histogram
	for(int rgb=0; rgb<m_scaledImage->nChannels; rgb++) {
		if(!m_rgb_plane[rgb]){
			m_rgb_plane[rgb] = tmCreateImage(cvGetSize(m_scaledImage),
											 IPL_DEPTH_8U, 1);
		}
	}

	if(m_scaledImage->nChannels >= 3) {
		cvCvtPixToPlane( m_scaledImage, m_rgb_plane[0], m_rgb_plane[1], m_rgb_plane[2], 0 );
	} else {
		cvCopy(	m_scaledImage, m_rgb_plane[0]);
	}

	u32 histo_score = 0;
	u32 hmax = 0;
	u32 grayHisto[3][256];
	for(int rgb=0; rgb<m_scaledImage->nChannels; rgb++)  {
		memset(grayHisto[rgb], 0, sizeof(u32)*256);
		for(int r=0; r<m_rgb_plane[rgb]->height; r++)
		{
			u8 * grayline = (u8 *)(m_rgb_plane[rgb]->imageData + r*m_rgb_plane[rgb]->widthStep);

			for(int c = 0; c<m_rgb_plane[rgb]->width; c++) {
				grayHisto[rgb][ grayline[c] ]++;
			}
		}

		// Store in info structure
		if(!m_image_info_struct.log_histogram[rgb]) {
			m_image_info_struct.log_histogram[rgb] = new float [256];
			memset(m_image_info_struct.log_histogram[rgb], 0, sizeof(float)*256);
		}

		for(int h=0; h<256; h++) {
			if(grayHisto[rgb][h]>hmax) {
				hmax = grayHisto[rgb][h] ; }
			if(grayHisto[rgb][h]>0)
				m_image_info_struct.log_histogram[rgb][h] = log(grayHisto[rgb][h]);
			else
				m_image_info_struct.log_histogram[rgb][h] = 0.f;
		}

		// compute the score for histogram
		float log_hmax = log((float)hmax) / 8.f;
		for(int h=0; h<256; h++) {
			if(grayHisto[rgb][h]>0) {
				if(log(grayHisto[rgb][h])>log_hmax) {
					histo_score++;
				}
			}
		}

	}

	m_image_info_struct.histo_score = (float)histo_score /
									  (float)m_scaledImage->nChannels
									  * 100.f / 256.f;
	return 0;
}


int ImageInfo::processSharpness() {

	// Process sobel
	CvSize size = cvGetSize(m_grayImage);

	CvSize scaledSize;
	if(size.width > size.height) {
		scaledSize.height = 16;
		scaledSize.width = size.width * scaledSize.height / size.height + 1 ;
	} else {
		scaledSize.width = 16;
		scaledSize.height = size.height * scaledSize.width / size.width + 1 ;
	}
	int scale_x = size.width / scaledSize.width;
	int scale_y = size.height / scaledSize.height;

	if(g_debug_ImageInfo) {
		fprintf(stderr, "ImageInfo::%s:%d : size=%dx%d => scaledSize = %dx%d => scale factor=%dx%d\n",
				__func__, __LINE__,
				size.width, size.height,
				scaledSize.width, scaledSize.height,
				scale_x, scale_y); fflush(stderr);
	}

	if(m_sharpnessImage
	   && (m_sharpnessImage->width != scaledSize.width
		   || m_sharpnessImage->height != scaledSize.height) ) {
		tmReleaseImage(&m_sharpnessImage);
	}

	if(!m_sharpnessImage) {
		m_sharpnessImage = tmCreateImage( scaledSize, IPL_DEPTH_8U, 1 );
	} else {
		cvZero(m_sharpnessImage);
	}


	if(m_sobelImage
	   && (m_sobelImage->width != m_grayImage->width
		   || m_sobelImage->height != m_grayImage->height) ) {
		tmReleaseImage(&m_sobelImage);
	}

	if(!m_sobelImage) {
		m_sobelImage = tmCreateImage( size, IPL_DEPTH_16S, 1 );
	} else {
		// no need, because recomputed : cvZero(m_sobelImage);
	}

	if(!m_sobelImage) {
		fprintf(stderr, "ImageInfo::%s:%d : cannot create sobelImage ( size=%dx%d => scaledSize = %dx%d => scale factor=%dx%d)\n",
				__func__, __LINE__,
				size.width, size.height,
				scaledSize.width, scaledSize.height,
				scale_x, scale_y); fflush(stderr);
		return -1;
	}

	if(m_sharp32fImage
	   && (m_sharp32fImage->width != scaledSize.width
		   || m_sharp32fImage->height != scaledSize.height) ) {
		tmReleaseImage(&m_sharp32fImage);
	}

	if(!m_sharp32fImage) {
		m_sharp32fImage = tmCreateImage( scaledSize, IPL_DEPTH_32F, 1 );
	} else {
		cvZero(m_sharp32fImage);
	}

	if(!m_sharp32fImage) {
		fprintf(stderr, "ImageInfo::%s:%d : cannot create sharpImage ( size=%dx%d => scaledSize = %dx%d => scale factor=%dx%d)\n",
				__func__, __LINE__,
				size.width, size.height,
				scaledSize.width, scaledSize.height,
				scale_x, scale_y); fflush(stderr);
		return -1;
	}

	float valmax = 1.f;
	for(int pass=0; pass<2; pass++) {
		if(g_debug_ImageInfo) {
			fprintf(stderr, "ImageInfo::%s:%d : cvSobel(m_grayImage, sobelImage, pass=%d)\n",
				__func__, __LINE__, pass); fflush(stderr);
		}

		cvSobel(m_grayImage, m_sobelImage, pass, 1-pass, 3);

		for(int r=0; r< m_sobelImage->height; r++) {
			short * sobelline = (short *)(m_sobelImage->imageData +
									r * m_sobelImage->widthStep);
			int sc_r = r / scale_y;
			if(sc_r>= m_sharp32fImage->height)
				sc_r = m_sharp32fImage->height - 1;

			float * sharpline32f = (float *)(m_sharp32fImage->imageData +
									sc_r * m_sharp32fImage->widthStep);

			for(int c=0; c<m_sobelImage->width; c++) {
				if(abs(sobelline[c])>80) {
					int sc_c = c/scale_x;

					if(sc_c<m_sharp32fImage->width) {
						sharpline32f[ sc_c ] ++;
						if(sharpline32f[ sc_c ] > valmax) valmax = sharpline32f[ sc_c ];
					}
				}
			}
		}
	}




	// Scale image
	//float scale = 255.f / valmax;
	float scale = 1.f; //255.f / 64.f;
	m_image_info_struct.sharpness_score = 0.f;
	for(int sc_r=0; sc_r< m_sharp32fImage->height; sc_r++) {
		float * sharpline32f = (float *)(m_sharp32fImage->imageData +
								sc_r * m_sharp32fImage->widthStep);
		u8 * sharpline = (u8 *)(m_sharpnessImage->imageData +
								sc_r * m_sharpnessImage->widthStep);
		for(int sc_c=0; sc_c<m_sharp32fImage->width; sc_c++) {
			float val = scale * sharpline32f[sc_c];
			if(val >= 255.f) val = 255.f;
			m_image_info_struct.sharpness_score += sharpline32f[sc_c];
			sharpline[sc_c] = (u8)( val );
		}
	}

	m_image_info_struct.sharpness_score = m_image_info_struct.sharpness_score * 100.f/255.f
				  / (float)(m_sharpnessImage->width * m_sharpnessImage->height);
	m_image_info_struct.sharpnessImage = m_sharpnessImage;

	// Process region growing
/*	void tmGrowRegion(unsigned char * growIn, unsigned char * growOut,
		int swidth, int sheight,
		int c, int r,
		unsigned char threshold,
		unsigned char fillValue,
		CvConnectedComp * areaOut);
*/
	IplImage * growImage = tmCreateImage(cvSize(m_sharpnessImage->width,m_sharpnessImage->height),
										 IPL_DEPTH_8U, 1);
	// clear is done
	int r = 0, c=0;
	CvConnectedComp areaOut;
	tmGrowRegion(
			(u8 *)m_sharpnessImage->imageData, //unsigned char * growIn,
			(u8 *)growImage->imageData, //unsigned char * growOut,
			m_sharpnessImage->widthStep, m_sharpnessImage->height,
			c, r,
			180, //unsigned char threshold,
			255, //unsigned char fillValue,
			&areaOut);


	if(g_debug_ImageInfo) {
		tmSaveImage(TMP_DIRECTORY "growImage.pgm", growImage);
		tmSaveImage(TMP_DIRECTORY "sharpImage.pgm", m_sharpnessImage);
	}
	//
	tmReleaseImage(&growImage);

	return 0;
}



void saveImageInfoStruct(t_image_info_struct * pinfo, QString path)
{
	if(!pinfo) { return; }
	if(path.isEmpty())
	{
		path = pinfo->cache_file;
	}
	fprintf(stderr, "[ImageInfo %s:%d : saving XML cache in '%s'\n",
			__func__, __LINE__, path.toAscii().data());

	QDomDocument infoDoc;
	QDomElement elemFileInfo = infoDoc.createElement("FileInfo");
	// Append data on file
	QDomElement elemFile = infoDoc.createElement("File");
	elemFile.setAttribute("fullpath", pinfo->filepath);
	elemFile.setAttribute("filesize", pinfo->filesize);
	elemFile.setAttribute("valid", pinfo->valid);
	elemFile.setAttribute("width", pinfo->width);
	elemFile.setAttribute("height", pinfo->height);
	elemFile.setAttribute("nChannels", pinfo->nChannels);
	elemFile.setAttribute("isMovie", pinfo->isMovie);
	elemFileInfo.appendChild(elemFile);

	QDomElement elemPicture = infoDoc.createElement("Picture");
	QDomElement elemExif = infoDoc.createElement("EXIF");
	elemExif.setAttribute("aperture", pinfo->exif.aperture);
	elemExif.setAttribute("datetime", pinfo->exif.datetime);
	elemExif.setAttribute("focal_eq35mm", pinfo->exif.focal_eq135_mm);
	elemExif.setAttribute("focal_mm", pinfo->exif.focal_mm);
	elemExif.setAttribute("ISO", pinfo->exif.ISO);
	elemExif.setAttribute("maker", pinfo->exif.maker);
	elemExif.setAttribute("model", pinfo->exif.model);
	elemExif.setAttribute("orientation", pinfo->exif.orientation);
	elemExif.setAttribute("speed_s", pinfo->exif.speed_s);
	elemPicture.appendChild(elemExif);

	QDomElement elemIPTC = infoDoc.createElement("IPTC");
	elemIPTC.setAttribute("city", pinfo->iptc.city);
	elemIPTC.setAttribute("countrycode", pinfo->iptc.countrycode);
	elemIPTC.setAttribute("countryname", pinfo->iptc.countryname);
	elemIPTC.setAttribute("provincestate", pinfo->iptc.provincestate);
	elemIPTC.setAttribute("sublocation", pinfo->iptc.sublocation);
	elemPicture.appendChild(elemIPTC);

	elemFileInfo.appendChild(elemPicture);

	/****************************** MOVIE ***********************************/
	// Add info about movie
	QDomElement elemMovie = infoDoc.createElement("Movie");
	elemMovie.setAttribute("FourCC", pinfo->FourCC);
	elemMovie.setAttribute("fps", pinfo->fps);

	elemFileInfo.appendChild(elemMovie);

	/****************************** DATA ***********************************/
	// ---- Custom tags ----
	QDomElement elemKeywords = infoDoc.createElement("Keywords");
	QStringList::iterator it;
	for(it = pinfo->keywords.begin(); it!=pinfo->keywords.end(); ++it)
	{
		QDomElement elemKeyword = infoDoc.createElement("Keyword");
		elemKeyword.setAttribute("keyword", (*it));
		elemKeywords.appendChild(elemKeyword);
	}
	elemFileInfo.appendChild(elemKeywords);

		/*! List of bookmarks */
	QDomElement elemBookmarks = infoDoc.createElement("Bookmarks");
	QList<t_movie_pos>::iterator b_it;
	for(b_it = pinfo->bookmarksList.begin(); b_it != pinfo->bookmarksList.end(); b_it++)
	{
		QDomElement elemBookmark = infoDoc.createElement("Bookmark");
		t_movie_pos bkmk = (*b_it);
		elemBookmark.setAttribute("name", bkmk.name);
		elemBookmark.setAttribute("prevAbsPosition", bkmk.prevAbsPosition);
		elemBookmark.setAttribute("prevKeyFramePosition", bkmk.prevKeyFramePosition);
		elemBookmark.setAttribute("nbFramesSinceKeyFrame", bkmk.nbFramesSinceKeyFrame);
		elemBookmarks.appendChild(elemBookmark);
	}
	elemFileInfo.appendChild(elemBookmarks);


	QDomElement elemImgProc = infoDoc.createElement("ImgProcessing");
	elemImgProc.setAttribute("grayscaled", pinfo->grayscaled);
	elemImgProc.setAttribute("sharpness_score", pinfo->sharpness_score);
	elemImgProc.setAttribute("histo_score", pinfo->histo_score);
	elemImgProc.setAttribute("score", pinfo->score);
//	elemImgProc.setAttribute("", pinfo-);
//	elemImgProc.setAttribute("", pinfo-);
//	elemImgProc.setAttribute("", pinfo-);
	elemFileInfo.appendChild(elemImgProc);

	infoDoc.appendChild(elemFileInfo);
	PIAF_MSG(SWLOG_TRACE, "saving '%s'", path.toAscii().data());
	QFile fileout( path );
	if( !fileout.open( QFile::WriteOnly ) ) {
		PIAF_MSG(SWLOG_ERROR, "cannot save '%s'", path.toAscii().data());
		return ;
	}

	QTextStream sout(&fileout);
	infoDoc.save(sout, 4);

	fileout.flush();
	fileout.close();

//	t_cached_image thumbImage;		/*! Thumb image for faster display */
//	IplImage * sharpnessImage;		/*! Sharpness image for faster display */
//	IplImage * hsvImage;			/*! HSV histogram image for faster display */

//	float * log_histogram[3];	/*! Log histogram */

//	// Image judgement
//	float score;			/*! Final score factor in [0..100] */
//	QList<t_movie_pos> bookmarksList;	/*! List of bookmarks */

	infoDoc.appendChild(elemFileInfo);

}
void printImageInfoStruct(t_image_info_struct * pinfo)
{
	fprintf(stderr, "[imageinfo] %s:%d : pinfo = %p\n", __func__, __LINE__, pinfo);
	if(!pinfo) { return; }
	// FILE DATA
	fprintf(stderr, "File: '%s' / %ld bytes\n",
			pinfo->filepath.toAscii().data(), (long)pinfo->filesize);
	fprintf(stderr, "\t%d x %d x %d", pinfo->width, pinfo->height, pinfo->nChannels);
	fprintf(stderr, "%s\n", pinfo->isMovie ? "Movie" : "Picture");
	if(!pinfo->isMovie) {
		fprintf(stderr, "\tEXIF: maker='%s', model='%s', date=%s orientation=%c "
				"focal=%gmm=%g mm(eq 35mm), F/%g, %g s\n"
				,
				pinfo->exif.maker.toAscii().data(),
				pinfo->exif.model.toAscii().data(),
				pinfo->exif.datetime.toAscii().data(),
				pinfo->exif.orientation,
				pinfo->exif.focal_mm, pinfo->exif.focal_eq135_mm,
				pinfo->exif.aperture, pinfo->exif.speed_s);
		fprintf(stderr, "\tIPTC: city='%s', sublocation='%s', province/state='%s',"
				" country code='%s', name='%s'\n",
				pinfo->iptc.city.toAscii().data(),
				pinfo->iptc.sublocation.toAscii().data(),
				pinfo->iptc.provincestate.toAscii().data(),
				pinfo->iptc.countrycode.toAscii().data(),
				pinfo->iptc.countryname.toAscii().data() );
	} else {
		fprintf(stderr, "\t%g fps, FourCC='%s'\n", pinfo->fps, pinfo->FourCC);
	}
	fprintf(stderr, "\tKeywords={");
	QStringList::iterator it;
	for(it = pinfo->keywords.begin(); it != pinfo->keywords.end(); ++it)
	{
		fprintf(stderr, "'%s', ", (*it).toAscii().data());
	}
	fprintf(stderr, "}\n\tImage: %s, sharpness=%g, histo=%g\n",
			pinfo->grayscaled ? "gray": "color",
			pinfo->sharpness_score, pinfo->histo_score);

}

int loadImageInfoStruct(t_image_info_struct * pinfo, QString path)
{
	if(!pinfo) { return -1; }
	QFile file( path );
	if (!file.open(QIODevice::ReadOnly))
	{
		PIAF_MSG(SWLOG_ERROR, "could not open file '%s' for reading: err=%s",
				 file.fileName().toAscii().data(),
				 file.errorString().toAscii().data());
		return -1;
	}

	pinfo->cache_file = path;

	QDomDocument infoDoc;
	QString errorMsg;
	int errorLine = 0;
	int errorColumn = 0;
	if( !infoDoc.setContent(&file, &errorMsg, &errorLine, &errorColumn) )
	{
		PIAF_MSG(SWLOG_ERROR,
				 "could not read content of file '%s' as XML doc: "
				 "err='%s' line %d col %d",
				 file.fileName().toAscii().data(),
				 errorMsg.toAscii().data(),
				 errorLine, errorColumn
				 );
	}
	file.close();

	QDomElement docElem = infoDoc.documentElement();
	QDomNode n = docElem.firstChild();
	while(!n.isNull()) {
		QDomElement e = n.toElement(); // try to convert the node to an element.
		if(!e.isNull()) {
			PIAF_MSG(SWLOG_INFO, "\tCategory '%s'", e.tagName().toAscii().data()); // the node really is an element.
			if(e.tagName().compare("File")==0) // Read file info
			{
				pinfo->filepath = e.attribute("fullpath");
				pinfo->filesize = e.attribute("filesize", "0").toULongLong();
				pinfo->valid = e.attribute("valid", "0").toInt();
				pinfo->width = e.attribute("width", "0").toInt();
				pinfo->height = e.attribute("height", "0").toInt();
				pinfo->nChannels = e.attribute("nChannels", "0").toInt();
				pinfo->isMovie = e.attribute("isMovie", "0").toInt();
			}

			// Pictures > IPTC, EXIF...
			if(e.tagName().compare("Picture")==0) // Read bookmarks
			{
				QDomNode pictureNode = e.firstChild();
				while(!pictureNode.isNull()) {
					QDomElement pictureElem = pictureNode.toElement(); // try to convert the node to an element.
					if(!pictureElem.isNull())
					{
						//
						if(pictureElem.tagName().compare("EXIF") == 0)
						{
							pinfo->exif.aperture = pictureElem.attribute("aperture", "0").toFloat();
							pinfo->exif.datetime = pictureElem.attribute("datetime");
							pinfo->exif.focal_eq135_mm = pictureElem.attribute("focal_eq35mm", "0").toFloat();
							pinfo->exif.focal_mm = pictureElem.attribute("focal_mm", "0").toFloat();
							pinfo->exif.ISO = pictureElem.attribute("ISO", "O").toInt();
							pinfo->exif.maker = pictureElem.attribute("maker", "");
							pinfo->exif.model = pictureElem.attribute("model", "");
							pinfo->exif.orientation = pictureElem.attribute("orientation", "0").toUInt();
							pinfo->exif.speed_s = pictureElem.attribute("speed_s", "0").toFloat();
						}

						if(pictureElem.tagName().compare("IPTC") == 0)
						{
							pinfo->iptc.city = pictureElem.attribute("city");
							pinfo->iptc.countrycode = pictureElem.attribute("countrycode");
							pinfo->iptc.countryname = pictureElem.attribute("countryname");
							pinfo->iptc.provincestate = pictureElem.attribute("provincestate");
							pinfo->iptc.sublocation = pictureElem.attribute("sublocation");
						}
					}

					pictureNode = pictureNode.nextSibling();
				}
			}
			if(e.tagName().compare("Movie")==0) // Read movie properties
			{
				pinfo->fps = e.attribute("fps").toFloat();
				unsigned long long fsize =
					e.attribute("filesize", "0").toULong();
				if(fsize > 0) {
					pinfo->filesize = fsize;
				}
				strcpy(pinfo->FourCC, e.attribute("valid", "0").toAscii().data());
			}

			// Keywords
			if(e.tagName().compare("Keywords")==0) // Read Keywords
			{
				QDomNode folderNode = e.firstChild();
				while(!folderNode.isNull()) {
					QDomElement folderElem = folderNode.toElement(); // try to convert the node to an element.
					if(!folderElem.isNull()) {
						// Read parameters
						QString keyword = folderElem.attribute("keyword");
						pinfo->keywords.append(keyword);
					}
					folderNode = folderNode.nextSibling();
				}
			}

			if(e.tagName().compare("Bookmarks")==0) // Read bookmarks
			{
				QDomNode folderNode = e.firstChild();
				while(!folderNode.isNull()) {
					QDomElement folderElem = folderNode.toElement(); // try to convert the node to an element.
					if(!folderElem.isNull()) {
						// Read parameters
						t_movie_pos bkmk;
						bkmk.name = folderElem.attribute("name");
						bkmk.prevAbsPosition = folderElem.attribute("prevAbsPosition").toLongLong();
						bkmk.prevKeyFramePosition = folderElem.attribute("prevKeyFramePosition").toLongLong();
						bkmk.nbFramesSinceKeyFrame = folderElem.attribute("nbFramesSinceKeyFrame").toInt();
						PIAF_MSG(SWLOG_INFO, "\t\tadded bookmark name='%s' "
								 "prevAbsPos=%lld prevKeyFrame=%lld nbFrameSinceKey=%d",
								 bkmk.name.toAscii().data(),
								 bkmk.prevAbsPosition,
								 bkmk.prevKeyFramePosition,
								 bkmk.nbFramesSinceKeyFrame
								 ); // the node really is an element.
						pinfo->bookmarksList.append(bkmk);
					}
					folderNode = folderNode.nextSibling();
				}
			}
		}
		n = n.nextSibling();
	}

	return 0;
}



static u32 * grayToBGR32 = NULL;
static void init_grayToBGR32()
{
	if(grayToBGR32) {
		return;
	}

	grayToBGR32 = new u32 [256];
	for(int c = 0; c<256; c++) {
		int Y = c;
		u32 B = Y;// FIXME
		u32 G = Y;
		u32 R = Y;
		grayToBGR32[c] = (R << 16) | (G<<8) | (B<<0);
	}

}



QImage iplImageToQImage(IplImage * iplImage, bool swap_RB)
{
	if(!iplImage) {
		PIAF_MSG(SWLOG_ERROR, "IplImage is null");
		return QImage();
	}

	int depth = iplImage->nChannels;

	bool rgb24_to_bgr32 = false;
	if(depth == 3  ) {// RGB24 is obsolete on Qt => use 32bit instead
		depth = 4;
		rgb24_to_bgr32 = true;
	}

	u32 * grayToBGR32palette = grayToBGR32;
	bool gray_to_bgr32 = false;


	if(depth == 1) {// GRAY is obsolete on Qt => use 32bit instead
		depth = 4;
		gray_to_bgr32 = true;

		init_grayToBGR32();

		grayToBGR32palette = grayToBGR32;
	}

	int orig_width = iplImage->width;
	int orig_height = iplImage->height;

	QImage qImage(orig_width, orig_height,
				   depth > 1 ? QImage::Format_RGB32 : QImage::Format_Indexed8);
	memset(qImage.bits(), 127, orig_width*orig_height*depth);

	switch(iplImage->depth) {
	default:
		fprintf(stderr, "imageinfowidget %s:%d : Unsupported depth = %d\n", __func__, __LINE__, iplImage->depth);
		break;

	case IPL_DEPTH_8U: {
		if(!rgb24_to_bgr32 && !gray_to_bgr32) {
			if(iplImage->nChannels != 4) {
				//
				if(!swap_RB)
				{
					for(int r=0; r<iplImage->height; r++) {
						// NO need to swap R<->B
						memcpy(qImage.bits() + r*orig_width*depth,
							iplImage->imageData + r*iplImage->widthStep,
							orig_width*depth);
					}
				} else {
					for(int r=0; r<iplImage->height; r++) {
						// need to swap R<->B
						u8 * buf_out = (u8 *)(qImage.bits()) + r*orig_width*depth;
						u8 * buf_in = (u8 *)(iplImage->imageData) + r*iplImage->widthStep;
						memcpy(qImage.bits() + r*orig_width*depth,
							iplImage->imageData + r*iplImage->widthStep,
							orig_width*depth);

						for(int pos3 = 0 ; pos3<orig_width*depth; pos3+=depth,
							buf_out+=3, buf_in+=depth
							 ) {
							buf_out[2] = buf_in[0];
							buf_out[0] = buf_in[2];
						}
					}
				}
			} else {

				for(int r=0; r<iplImage->height; r++) {
					// need to swap R<->B
					u8 * buf_out = (u8 *)(qImage.bits()) + r*orig_width*depth;
					u8 * buf_in = (u8 *)(iplImage->imageData) + r*iplImage->widthStep;
					memcpy(qImage.bits() + r*orig_width*depth,
						iplImage->imageData + r*iplImage->widthStep,
						orig_width*depth);

					if(swap_RB) {
						for(int pos4 = 0 ; pos4<orig_width*depth; pos4+=depth,
							buf_out+=4, buf_in+=depth
							 ) {
							buf_out[2] = buf_in[0];
							buf_out[0] = buf_in[2];
						}
					}
				}
			}
		}
		else if(rgb24_to_bgr32) {
			// RGB24 to BGR32
			u8 * buffer3 = (u8 *)iplImage->imageData;
			u8 * buffer4 = (u8 *)qImage.bits();
			int orig_width4 = 4 * orig_width;

			for(int r=0; r<iplImage->height; r++)
			{
				int pos3 = r * iplImage->widthStep;
				int pos4 = r * orig_width4;
				if(!swap_RB) {

					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4 + 2] = buffer3[pos3+2];
					}
				} else { // SWAP R<->B
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4 + 2] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4    ] = buffer3[pos3+2];
					}
				}
			}
		} else if(gray_to_bgr32) {
			for(int r=0; r<iplImage->height; r++)
			{
				u32 * buffer4 = (u32 *)qImage.bits() + r*qImage.width();
				u8 * bufferY = (u8 *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<orig_width; c++) {
					buffer4[c] = grayToBGR32palette[ (int)bufferY[c] ];
				}
			}
		}
		}break;
	case IPL_DEPTH_16S: {
		if(!rgb24_to_bgr32) {

			u8 * buffer4 = (u8 *)qImage.bits();
			short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				short * buffershort = (short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++)
					if(buffershort[c]>valmax)
						valmax = buffershort[c];
			}

			if(valmax>0)
				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData
									+ r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width;
					for(int c=0; c<orig_width; c++, pos3++, pos4++)
					{
						int val = abs((int)buffer3[pos3]) * 255 / valmax;
						if(val > 255) val = 255;
						buffer4[pos4] = (u8)val;
					}
				}
		}
		else {
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4 + 2] = buffer3[pos3+2];
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0) {
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				}
			}
		}
		}break;
	case IPL_DEPTH_16U: {

		if(!rgb24_to_bgr32) {

			unsigned short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				unsigned short * buffershort = (unsigned short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++) {
					if(buffershort[c]>valmax) {
						valmax = buffershort[c];
					}
				}
			}

			if(valmax>0) {
				if(!gray_to_bgr32) {
					u8 * buffer4 = (u8 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				} else {
					u32 * buffer4 = (u32 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = grayToBGR32palette[ val ];
						}
					}
				}
			}
		}
		else {
			fprintf(stderr, "imageinfowidget %s:%d : U16  depth = %d -> BGR32\n", __func__, __LINE__, iplImage->depth);
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3]/256;
						buffer4[pos4 + 1] = buffer3[pos3+1]/256;
						buffer4[pos4 + 2] = buffer3[pos3+2]/256;
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0)
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
			}
		}
		}break;
	case IPL_DEPTH_32F: {
		// create temp image
		IplImage * image8bit = swCreateImage(cvGetSize(iplImage),
											 IPL_DEPTH_8U, iplImage->nChannels);
		double minVal=0. , maxVal = 255.;
		CvPoint maxPt;
		#ifdef OPENCV_22
		try
		#endif
		{
			cvMinMaxLoc(iplImage, &minVal, &maxVal, &maxPt);
		}
		#ifdef OPENCV_22
		catch(cv::Exception e)
		{
			maxVal = 255.;
		}
		#endif
		// Get dynamic of image
		int limit = 1;
		while (limit < maxVal)
		{
			limit = limit << 1;
		}
		// convert scale
		cvConvertScale(iplImage, image8bit, 255./(double)limit, 0.);

		// convert
		qImage = iplImageToQImage(image8bit).copy();
		swReleaseImage(&image8bit);

		}break;
	}

	if(qImage.depth() == 8) {
		qImage.setNumColors(256);

		for(int c=0; c<256; c++) {
			qImage.setColor(c, qRgb(c,c,c));
		}
	}

	return qImage;
}



