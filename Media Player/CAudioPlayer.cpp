#include "CAudioPlayer.h"
#include <Windows.h>
int CAudioPlayer2::stream_component_open(){
    AVDictionaryEntry *t = NULL;
    int64_t wanted_channel_layout = 0;

    if (audioStream < 0 || audioStream >= pFormatCtx->nb_streams)
        return -1;
    pCodecCtx = pFormatCtx->streams[audioStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (!pCodec)
        return -1;

    if(pCodecCtx->lowres > pCodec->max_lowres){
        av_log(pCodecCtx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
                pCodec->max_lowres);
        pCodecCtx->lowres= pCodec->max_lowres;
    }

    if(pCodec->capabilities & CODEC_CAP_DR1)
        pCodecCtx->flags |= CODEC_FLAG_EMU_EDGE;

    if (pCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        wanted_channel_layout = (pCodecCtx->channel_layout && pCodecCtx->channels == av_get_channel_layout_nb_channels(pCodecCtx->channels)) ? pCodecCtx->channel_layout : av_get_default_channel_layout(pCodecCtx->channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
        wanted_spec.channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
        wanted_spec.freq = pCodecCtx->sample_rate;
        if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
            fprintf(stderr, "Invalid sample rate or channel count!\n");
            return -1;
        }
    }

    if (!pCodec ||
        avcodec_open(pCodecCtx, pCodec) < 0)
        return -1;

    if (pCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.silence = 0;
        wanted_spec.samples = 1024;
        wanted_spec.callback = CallBack;
        wanted_spec.userdata = this;
        if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
            fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
            return -1;
        }
        as.audio_hw_buf_size = spec.size;
        if (spec.format != AUDIO_S16SYS) {
            fprintf(stderr, "SDL advised audio format %d is not supported!\n", spec.format);
            return -1;
        }
        if (spec.channels != wanted_spec.channels) {
            wanted_channel_layout = av_get_default_channel_layout(spec.channels);
            if (!wanted_channel_layout) {
                fprintf(stderr, "SDL advised channel count %d is not supported!\n", spec.channels);
                return -1;
            }
        }
        as.audio_src_fmt = as.audio_tgt_fmt = AV_SAMPLE_FMT_S16;
        as.audio_src_freq = as.audio_tgt_freq = spec.freq;
        as.audio_src_channel_layout = as.audio_tgt_channel_layout = wanted_channel_layout;
        as.audio_src_channels = as.audio_tgt_channels = spec.channels;
    }

    pFormatCtx->streams[audioStream]->discard = AVDISCARD_DEFAULT;
    as.audio_buf_size = 0;
    as.audio_buf_index = 0;	

    memset(&as.audio_pkt, 0, sizeof(as.audio_pkt));
    InitPacketQueue();
    SDL_PauseAudio(0);
    return 0;
}
void CAudioPlayer2::stream_component_close(){
    if (audioStream < 0 || audioStream >= pFormatCtx->nb_streams)
        return;
    AbortPacketQueue();
    SDL_CloseAudio();
    EndPacketQueue();
    if (as.swr_ctx)
        swr_free(&as.swr_ctx);
    av_free_packet(&as.audio_pkt);
	if (as.audio_buf1)
		av_freep(&as.audio_buf1);
    as.audio_buf = NULL;
	if (as.pFrame)
		av_free(as.pFrame);
    pFormatCtx->streams[audioStream]->discard = AVDISCARD_ALL;
	if(pCodecCtx){
		avcodec_close(pCodecCtx);
	}
    audioStream = -1;
	if (pFormatCtx)
		av_close_input_file(pFormatCtx);
	pFormatCtx = NULL;
	pCodecCtx = NULL;
	pCodec = NULL;
	avio_set_interrupt_cb(NULL);
}
int CAudioPlayer2::PlayThread(void* param){
	CAudioPlayer2* p = (CAudioPlayer2*)param;
	p->status = PLAY;
	SDL_PauseAudio(0);
	AVPacket pkt;
	while (!p->queue.abort_request){
        if (p->as.seek_req) {
			p->as.paused = 1;
            int64_t seek_target= p->as.seek_pos;			
            //if (avformat_seek_file(p->pFormatCtx, -1, INT64_MIN, seek_target, INT64_MAX, 0) < 0) {
			if(av_seek_frame(p->pFormatCtx, -1, seek_target, 0)){
				fprintf(stderr, "%s: error while seeking\n", p->pFormatCtx->filename);
            }else{
                if (p->audioStream >= 0) {
					p->FlushPacketQueue();
					p->PutPacket(&p->flush_pkt);
					//p->as.refresh_clock2 = 1;
                }
            }
			p->as.paused = 0;
			p->as.seek_req = 0;
        }
		if (av_read_frame(p->pFormatCtx, &pkt) >= 0){
			if (pkt.stream_index == p->audioStream){
				p->PutPacket(&pkt);
			}else{
				av_free_packet(&pkt);
			}
		}else{
			SDL_Delay(100);
		}
	}
	return 0;
}
int CAudioPlayer2::Open(char* filename){
	if (av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL) != 0)
		return ERR_OPEN_FILE;
	if (av_find_stream_info(pFormatCtx) < 0)
		return ERR_STREAM_NOT_FOUND;
	for (int i = 0; i < pFormatCtx->nb_streams; i++){
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			audioStream = i;
			break;
		}
	}
	if (audioStream == -1)
		return ERR_STREAM_NOT_FOUND;
	pCodecCtx = pFormatCtx->streams[audioStream]->codec;
	memset(&as, 0, sizeof(VideoState));
	stream_component_open();
	av_dump_format(pFormatCtx, 0, filename, 0);
}

void CAudioPlayer2::InitPacketQueue(){
	PacketQueue *q = &queue;
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
	PutPacket(&flush_pkt);
}
void CAudioPlayer2::FlushPacketQueue()
{
	PacketQueue *q = &queue;
    AVPacketList *pkt, *pkt1;
    SDL_LockMutex(q->mutex);
    for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    SDL_UnlockMutex(q->mutex);
}
int CAudioPlayer2::PutPacket(AVPacket* pkt){
	PacketQueue *q = &queue;
	AVPacketList *pkt1;

    /* duplicate the packet */
    if (pkt!=&flush_pkt && av_dup_packet(pkt) < 0)
        return -1;

    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;


    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)

        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    /* XXX: should duplicate packet data in DV case */
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}
void CAudioPlayer2::EndPacketQueue(){
	PacketQueue* q = &queue; 
    FlushPacketQueue();
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}
void CAudioPlayer2::AbortPacketQueue(){
	PacketQueue* q = &queue; 
    SDL_LockMutex(q->mutex);
    q->abort_request = 1;
    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);
}
int CAudioPlayer2::GetPacket(AVPacket *pkt, int block){
	PacketQueue *q = &queue;
	AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for(;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

void CAudioPlayer2::CallBack(void *opaque, Uint8 *stream, int len){
	CAudioPlayer2* p = (CAudioPlayer2*)opaque;
	VideoState *is = &p->as;
    int audio_size, len1;
    int bytes_per_sec;
    double pts;

    p->audio_callback_time = av_gettime();
	if (p->queue.size <= 0)
		p->status = STOP;
    while (len > 0) {
        if (is->audio_buf_index >= is->audio_buf_size) {
           audio_size = p->AudioDecodeFrame(&pts);
           if (audio_size < 0) {
               is->audio_buf      = is->silence_buf;
               is->audio_buf_size = sizeof(is->silence_buf);
           } else {
               is->audio_buf_size = audio_size;
			   p->status = PLAY;
           }
           is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len)
            len1 = len;
		SDL_MixAudio(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1, p->volume);
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
    bytes_per_sec = is->audio_tgt_freq * is->audio_tgt_channels * av_get_bytes_per_sample(is->audio_tgt_fmt);
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    is->audio_current_pts = is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / bytes_per_sec;
    is->audio_current_pts_drift = is->audio_current_pts - p->audio_callback_time / 1000000.0;
}

int CAudioPlayer2::AudioDecodeFrame(double *pts_ptr){
	VideoState* is = &as;
	AVPacket *pkt_temp = &is->audio_pkt_temp;
    AVPacket *pkt = &is->audio_pkt;
    int len1, len2, data_size, resampled_data_size, got_frame;
    int64_t dec_channel_layout;
    double pts;
    int new_packet = 0;
    int flush_complete = 0;
    for(;;) {
        while (pkt_temp->size > 0 || (!pkt_temp->data && new_packet)) {
            if (!is->pFrame) {
                if (!(is->pFrame = avcodec_alloc_frame()))
                    return AVERROR(ENOMEM);
            } else
                avcodec_get_frame_defaults(is->pFrame);
            if (flush_complete)
                break;
            new_packet = 0;
            len1 = avcodec_decode_audio4(pCodecCtx, is->pFrame, &got_frame, pkt_temp);
            if (len1 < 0) {
                pkt_temp->size = 0;
                break;
            }
            pkt_temp->data += len1;
            pkt_temp->size -= len1;

            if (!got_frame) {
                if (!pkt_temp->data && pCodec->capabilities & CODEC_CAP_DELAY)
                    flush_complete = 1;
                continue;
            }
            data_size = av_samples_get_buffer_size(NULL, pCodecCtx->channels,
                                                   is->pFrame->nb_samples,
                                                   pCodecCtx->sample_fmt, 1);

            dec_channel_layout = (pCodecCtx->channel_layout && pCodecCtx->channels == av_get_channel_layout_nb_channels(pCodecCtx->channel_layout)) ? pCodecCtx->channel_layout : av_get_default_channel_layout(pCodecCtx->channels);

            if (pCodecCtx->sample_fmt != is->audio_src_fmt || dec_channel_layout != av_get_default_channel_layout(spec.channels) || pCodecCtx->sample_rate != spec.freq) {
                if (is->swr_ctx)
                    swr_free(&is->swr_ctx);
                is->swr_ctx = swr_alloc_set_opts(NULL,
												is->audio_tgt_channel_layout, is->audio_tgt_fmt, is->audio_tgt_freq,
                                                 dec_channel_layout,           pCodecCtx->sample_fmt,   pCodecCtx->sample_rate,
                                                 0, NULL);
                if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
                    fprintf(stderr, "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                        pCodecCtx->sample_rate,
                        av_get_sample_fmt_name(pCodecCtx->sample_fmt),
                        pCodecCtx->channels,
                        is->audio_tgt_freq,
                        av_get_sample_fmt_name(is->audio_tgt_fmt),
                        is->audio_tgt_channels);
                    break;
                }
                is->audio_src_channel_layout = dec_channel_layout;
                is->audio_src_channels = pCodecCtx->channels;
                is->audio_src_freq = pCodecCtx->sample_rate;
                is->audio_src_fmt = pCodecCtx->sample_fmt;
	
            }
            resampled_data_size = data_size;
            if (is->swr_ctx) {
                const uint8_t *in[] = { is->pFrame->data[0] };
                uint8_t *out[] = {is->audio_buf2};
                len2 = swr_convert(is->swr_ctx, out, sizeof(is->audio_buf2) / is->audio_tgt_channels / av_get_bytes_per_sample(is->audio_tgt_fmt),
                                                in, data_size / pCodecCtx->channels / av_get_bytes_per_sample(pCodecCtx->sample_fmt));
                if (len2 < 0) {
                    fprintf(stderr, "audio_resample() failed\n");
                    break;
                }
                if (len2 == sizeof(is->audio_buf2) / is->audio_tgt_channels / av_get_bytes_per_sample(is->audio_tgt_fmt)) {
                    fprintf(stderr, "warning: audio buffer is probably too small\n");
                    swr_init(is->swr_ctx);
                }
                is->audio_buf = is->audio_buf2;
                resampled_data_size = len2 * is->audio_tgt_channels * av_get_bytes_per_sample(is->audio_tgt_fmt);
            } else {
                is->audio_buf = is->pFrame->data[0];
            }

            /* if no pts, then compute it */
            pts = is->audio_clock;
            *pts_ptr = pts;
			is->audio_clock += (double)data_size / (pCodecCtx->channels * pCodecCtx->sample_rate * av_get_bytes_per_sample(pCodecCtx->sample_fmt));
			if (is->refresh_clock)
				is->seek_second += (double)data_size / (pCodecCtx->channels * pCodecCtx->sample_rate * av_get_bytes_per_sample(pCodecCtx->sample_fmt));
			return resampled_data_size;
        }

        /* free the current packet */
        if (pkt->data)
            av_free_packet(pkt);

        if (is->paused || queue.abort_request) {
            return -1;
        }

        /* read next packet */
        if ((new_packet = GetPacket(pkt, 1)) < 0)
            return -1;

        if (pkt->data == flush_pkt.data)
           avcodec_flush_buffers(pCodecCtx);

        *pkt_temp = *pkt;

        /* if update the audio clock with the pts */
        if (pkt->pts != AV_NOPTS_VALUE) {
            is->audio_clock = av_q2d(pFormatCtx->streams[audioStream]->time_base)*pkt->pts;
			if (as.refresh_clock)
				as.refresh_clock--;
        }
    }
}
