/***************************************************************************
 *  colibrithread.cpp - Background processing thread for colibri console
 *
 *  2013-09-29 21:55
 *  Copyright  2013  Christophe Seyve
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

#include "colibrithread.h"

#include "imgutils.h"
#include "swimage_utils.h"

#include <QDateTime>


/**************************************************************************

					BACKGROUND ANALYSIS THREAD

**************************************************************************/
ColibriThread::ColibriThread() {
	m_isRunning = m_run = false;
	m_inputImage = m_outputImage = NULL;
	m_iteration = 0;
}

ColibriThread::~ColibriThread() {
	stop();
	tmReleaseImage(&m_inputImage);
	tmReleaseImage(&m_outputImage);
}

void ColibriThread::setCapture(CvCapture * capture) {
	m_capture = capture;
}

/* Tell the thread to stop */
void ColibriThread::stop() {
	m_run = false;
	while(m_isRunning) {
		sleep(1);
	}
}

/* Thread loop */
void ColibriThread::run()
{
	m_isRunning = m_run = true;

	while(m_run) {
		PIAFFILTER_MSG("hop capture=%p", m_capture);
		if( m_capture ) {
			IplImage * frame = cvQueryFrame( m_capture );

			if( !frame) { //!cvGrabFrame( m_capture ) ) {
				COLIBRI_MSG("capture=%p FAILED", m_capture);
				sleep(1);
				sleep(2);

			} else {
				//IplImage * frame = cvRetrieveFrame( m_capture );
				if(!frame) {
					COLIBRI_MSG("ERROR: capture=%p: cvRetrieveFrame FAILED", m_capture);
					sleep(1);
				} else {
					// Check if size changed
					if(m_outputImage &&
					   (m_outputImage->widthStep*m_outputImage->height != frame->widthStep*frame->height))
					{
						tmReleaseImage(&m_inputImage);
						tmReleaseImage(&m_outputImage);
					}
					if(!m_inputImage) { m_inputImage = cvCloneImage(frame); }
					else { cvCopy(frame, m_inputImage); }

					// then process current image
					int ret = computeImage(m_inputImage);
					fprintf(stderr, "%s:%d : capture=%p ok, iteration=%d frame=%dx%dx%d => ret=%d\n",
							__func__, __LINE__, m_capture, m_iteration,
							frame->width, frame->height, frame->nChannels,
							ret);

					// copy image
					mDisplayMutex.lock();
					if(!m_outputImage) { m_outputImage = cvCloneImage(m_inputImage); }
					else { cvCopy(m_inputImage, m_outputImage); }
					mDisplayMutex.unlock();


					m_iteration++;
				}
			}
		} else {
			sleep(1);
		}
	}

	m_isRunning = false;
}
/* Set the sequence file */
int ColibriThread::setSequenceFile(char * path)
{
	COLIBRI_MSG("Load sequence '%s'", path);
	return mFilterSequencer.loadFilterList(path);
}

int ColibriThread::computeImage(IplImage * iplImage) {
	if(!iplImage) { return -1; }

	// Process this image with filters
	int retproc = mFilterSequencer.processImage(iplImage,
												&m_outputImage);
	if(retproc < 0)
	{
		fprintf(stderr, "[Colibri] %s:%d : process sequence '%s' on image (%dx%dx%d) with ret=%d\n",
				__func__, __LINE__,
				mFilterSequencer.getPluginSequenceFile(),
				iplImage->width, iplImage->height, iplImage->depth,
				retproc
				);
	}
	return retproc;
}
