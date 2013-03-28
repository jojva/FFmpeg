#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

//#define DEINTERLACE 1

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    int y;

    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    for (y = 0; y < height; y++)
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
    fclose(pFile);
}

void DecodePacket(AVCodecContext *pCodecCtx, AVFrame *pFrame, AVFrame *pFrameRGB, AVPacket *pkt)
{
    int frameFinished;
    static int i = 0;
    av_log(NULL, AV_LOG_INFO, "Début paquet, stream %d\n", pkt->stream_index);
    avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, pkt);
    av_log(NULL, AV_LOG_INFO, "Fin paquet, stream %d, outputting %d\n", pkt->stream_index, frameFinished ? i : -1);

    if (frameFinished){
	
#ifdef DEINTERLACE
	avpicture_deinterlace((AVPicture *)pFrame, (AVPicture *)pFrame,
			      pFrame->format, pFrame->width, pFrame->height);
#endif

	struct SwsContext *ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
						pCodecCtx->height, PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	sws_scale(ctx, (const uint8_t * const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
		  pFrameRGB->data, pFrameRGB->linesize);
#ifdef DEINTERLACE
	// When deinterlacing, only extract frames 15-16,27-28,39-40,51-52 which work whereas others don't
	/* if(i >= 15 && ((i - 15) % 12 == 0 || (i - 16) % 12 == 0)) */
	if(i >= 14 && ((i - 14) % 12 == 0 || (i - 15) % 12 == 0))
	    SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
#else
	SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
#endif

	i++;
    }
}

void DumpPacket(AVPacket *pkt)
{  
    static int cpt = 0;
    char fntemplate[100] = "%d_%d_%d.bin";
    char pktfilename[100];
    int fd;
    int err;
    snprintf(pktfilename, sizeof(pktfilename), fntemplate,
	     cpt, pkt->stream_index, pkt->size);
    fd  = open(pktfilename, O_WRONLY | O_CREAT, 0644);
    err = write(fd, pkt->data, pkt->size);
    if (err < 0) {
	fprintf(stderr, "write: error %d\n", err);
	close(fd);
    }
    cpt++;
}

int main(int argc, char *argv[])
{
    AVFormatContext *pFormatCtx = NULL;
    int i, j, k, l;
    int videoStream;
    int avc = 1;
    int cnt[2];
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
    AVPacket packet;
    AVPacket *avcpkt;
    AVPacket *mvcpkt;
    AVPacket mvcsplit1;
    AVPacket mvcsplit2;
    int avc_cnt, mvc_cnt;
    int must_split;
    int numBytes;
    uint8_t *buffer;

    if (argc < 2)
    {
        printf("Please provide a movie file\n");
        return -1;
    }
    // Register all formats and codecs
    av_register_all();

    // Open video file
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
        return -1; // Couldn't open file

    // Retrieve stream information 
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1; // Couldn't find stream information

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    // Find the first video stream
    videoStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
      if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    if (videoStream == -1)
        return -1; // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }
    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return -1; // Could not open codec

    pFormatCtx->streams[3]->codec = pCodecCtx;

    // Allocate video frame
    pFrame = avcodec_alloc_frame();

    // Allocate an AVFrame structure
    pFrameRGB = avcodec_alloc_frame();
    if (pFrameRGB == NULL)
        return -1;

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
				  pCodecCtx->height);
    buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *) pFrameRGB, buffer, PIX_FMT_RGB24,
		   pCodecCtx->width, pCodecCtx->height);  

    av_log_set_level(AV_LOG_DEBUG);
    pCodecCtx->debug |= FF_DEBUG_PICT_INFO;
    pCodecCtx->debug |= FF_DEBUG_MMCO;
    
    avcpkt = malloc(50 * sizeof(AVPacket));
    mvcpkt = malloc(50 * sizeof(AVPacket));
    avc_cnt = mvc_cnt = 0;
    for(;;) {
	while(avc_cnt < 2 || mvc_cnt < 1) {
	    if(av_read_frame(pFormatCtx, &packet) < 0)
		goto end;
	    if (packet.stream_index == 0)
		avcpkt[avc_cnt++] = packet;
	    else if (packet.stream_index == 3) {
		// Determine which following MVC access unit is a real one because it starts with 00 00 00 01 (18?)
		if(packet.data[0] == 0
		   && packet.data[1] == 0
		   && packet.data[2] == 0
		   && packet.data[3] == 1)
		    mvcpkt[mvc_cnt++] = packet;
	    }
	}
	
	// Split MVC access unit into a couple of AVPacket
	for(k = 4; k < mvcpkt[0].size - 5; k++) {
	    // Find the next NAL Unit with nal_unit_type == 24
	    if(mvcpkt[0].data[k] == 0
	       && mvcpkt[0].data[k+1] == 0
	       && mvcpkt[0].data[k+2] == 0
	       && mvcpkt[0].data[k+3] == 1
	       && (mvcpkt[0].data[k+4] & 0x1F) == 24) {
		break;
	    }
	}

	av_copy_packet(&mvcsplit1, &mvcpkt[0]);
	mvcsplit1.size = k;
	av_copy_packet(&mvcsplit2, &mvcpkt[0]);
	for(l = 0; l < mvcpkt[0].size - mvcsplit1.size; l++) {
	    mvcsplit2.data[l] = mvcpkt[0].data[k+l];
	}
	mvcsplit2.size = l;
	av_log(NULL, AV_LOG_DEBUG, "mvcsplit1.size : %d, mvcsplit2.size : %d, k : %d\n\n", mvcsplit1.size, mvcsplit2.size, k);

	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &avcpkt[0]);
    	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &avcpkt[1]);
    	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &mvcsplit1);
    	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &mvcsplit2);	
	
	/* av_free(&avcpkt[0]); */
	/* av_free(&avcpkt[1]); */
	/* av_free(&mvcpkt[0]); */

	for(i = 0; i < avc_cnt - 2; i++) {
	    avcpkt[i] = avcpkt[i+2];
	}
	for(i = 0; i < mvc_cnt - 1; i++) {
	    mvcpkt[i] = mvcpkt[i+1];
	}       
	avc_cnt -= 2;
	mvc_cnt -= 1;
    }

    /* avcpkt = (AVPacket*)malloc(100 * sizeof(AVPacket)); */
    /* mvcpkt = (AVPacket*)malloc(100 * sizeof(AVPacket)); */
    /* avc_cnt = mvc_cnt = 0; */
    /* while (av_read_frame(pFormatCtx, &packet) >= 0) { */
    /* 	if (packet.stream_index == 0) */
    /* 	    avcpkt[avc_cnt++] = packet; */
    /* 	else if (packet.stream_index == 3) */
    /* 	    mvcpkt[mvc_cnt++] = packet; */
    /* }    */
    /* av_log_set_level(AV_LOG_DEBUG); */
    /* pCodecCtx->debug |= FF_DEBUG_PICT_INFO; */
    /* pCodecCtx->debug |= FF_DEBUG_MMCO; */
    /* for(i = 0, j = 0; i < avc_cnt && j < mvc_cnt; i++, j++) { */
    /* 	// Decode AVC access unit 1 */
    /* 	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &avcpkt[i]); */
    /* 	i++; */
	
    /* 	// Determine which following MVC access unit is a real one because it starts with 00 00 00 01 (18?) */
    /* 	av_log(NULL, AV_LOG_DEBUG, "j1 : %d\n\n", j); */
    /* 	while(!(mvcpkt[j].data[0] == 0 */
    /* 		&& mvcpkt[j].data[1] == 0 */
    /* 		&& mvcpkt[j].data[2] == 0 */
    /* 		&& mvcpkt[j].data[3] == 1)) */
    /* 	    j++; */
    /* 	av_log(NULL, AV_LOG_DEBUG, "j2 : %d\n\n", j); */

    /* 	// Split MVC access unit into a couple of AVPacket */
    /* 	must_split = 0; */
    /* 	for(k = 4; k < mvcpkt[j].size - 5; k++) { */
    /* 	    // Find the next NAL Unit with nal_unit_type == 24 */
    /* 	    if(mvcpkt[j].data[k] == 0 */
    /* 	       && mvcpkt[j].data[k+1] == 0 */
    /* 	       && mvcpkt[j].data[k+2] == 0 */
    /* 	       && mvcpkt[j].data[k+3] == 1 */
    /* 	       && (mvcpkt[j].data[k+4] & 0x1F) == 24) { */
    /* 	    	must_split = 1; */
    /* 	    	break; */
    /* 	    }	     */
    /* 	} */
    /* 	if(must_split) { */
    /* 	    av_copy_packet(&mvcsplit1, &mvcpkt[j]); */
    /* 	    mvcsplit1.size = k; */
    /* 	    av_copy_packet(&mvcsplit2, &mvcpkt[j]); */
    /* 	    for(l = 0; l < mvcpkt[j].size - mvcsplit1.size; l++) { */
    /* 		mvcsplit2.data[l] = mvcpkt[j].data[k+l]; */
    /* 	    } */
    /* 	    mvcsplit2.size = l; */
    /* 	    av_log(NULL, AV_LOG_DEBUG, "mvcsplit1.size : %d, mvcsplit2.size : %d, k : %d\n\n", mvcsplit1.size, mvcsplit2.size, k); */
    /* 	} */
    /* 	else { */
    /* 	    av_log(NULL, AV_LOG_ERROR, "Joris. MVC Packet doesn't have to be split. Exiting...\n"); */
    /* 	    exit(1); */
    /* 	} */
	
    /* 	/\* DumpPacket(&mvcsplit1); *\/ */
    /* 	/\* DumpPacket(&mvcsplit2); *\/ */

	
    /* 	/\* DecodePacket(pCodecCtx, pFrame, pFrameRGB, &avcpkt[i]); *\/ */
    /* 	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &mvcsplit1);	 */
    /* 	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &avcpkt[i]);	 */
    /* 	DecodePacket(pCodecCtx, pFrame, pFrameRGB, &mvcsplit2); */


    /* 	//av_free(avcpkt[i]); */
    /* } */

 end:

    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    //avformat_close_input(&pFormatCtx);

    return 0;
}
