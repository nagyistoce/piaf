

#include "virtualdeviceacquisition.h"


void printVideoProperties(t_video_properties * props)
{
	fprintf(stderr, "Props = { ");
	fprintf(stderr, "pos_msec=%g, ", props->pos_msec);
	fprintf(stderr, "pos_frames=%g, ", props->pos_frames);
	fprintf(stderr, "pos_avi_ratio=%g, ", props->pos_avi_ratio);
	fprintf(stderr, "frame_width=%d x ", props->frame_width);
	fprintf(stderr, "frame_height=%d, ", props->frame_height);
	fprintf(stderr, "fps=%g, ", props->fps);
	fprintf(stderr, "fourcc_dble=%g, ", props->fourcc_dble);
	fprintf(stderr, "fourcc='%s', ", props->fourcc);
	fprintf(stderr, "norm='%s', ", props->norm);
	fprintf(stderr, "frame_count=%g, ", props->frame_count);
	fprintf(stderr, "format=%g, ", props->format);
	fprintf(stderr, "mode=%g, ", props->mode);
	fprintf(stderr, "brightness=%g, ", props->brightness);
	fprintf(stderr, "contrast=%g, ", props->contrast);
	fprintf(stderr, "saturation=%g, ", props->saturation);
	fprintf(stderr, "hue=%g, ", props->hue);
	fprintf(stderr, "gain=%g, ", props->gain);
	fprintf(stderr, "exposure=%g, ", props->exposure);
	fprintf(stderr, "convert_rgb=%g, ", props->convert_rgb);
	fprintf(stderr, "white_balance=%g, ", props->white_balance);

	fprintf(stderr, " }\n");
}
