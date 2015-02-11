#ifndef _CAUDIOPLAYER_H
#define _CAUDIOPLAYER_H
#include <Windows.h>
#include <iostream>
#include "IMediaPlayer.h"
using namespace std;

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <SDL.h>
#include "libswresample/swresample.h"

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"SDL.lib")
//#pragma comment(lib,"SDLmain.lib")

enum _AudioErr{
	ERR_OPEN_FILE = 1, 
	ERR_STREAM_NOT_FOUND, 
	ERR_UNSUPPORT_CODEC, 
	ERR_OPEN_CODEC, 
	ERR_OPEN_DEVICE
};

struct PacketQueue{
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int abort_request;
	SDL_mutex* mutex;
	SDL_cond* cond;
};

class CAudioPlayer2 : public CAudioPlayer{
private:
	AVFormatContext* pFormatCtx;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	PacketQueue queue;
	int audioStream;
	int stop;
	SDL_AudioSpec wanted_spec, spec;
	SDL_Thread* hPlayThread;
	AVPacket flush_pkt;
	struct VideoState{
		SwrContext *swr_ctx;
		AVSampleFormat audio_src_fmt;
		AVSampleFormat audio_tgt_fmt;
		int audio_src_channels;
		int audio_tgt_channels;
		int64_t audio_src_channel_layout;
		int64_t audio_tgt_channel_layout;
		int audio_src_freq;
		int audio_tgt_freq;	
		AVFrame* pFrame;
		AVPacket audio_pkt_temp;
		AVPacket audio_pkt;
		uint8_t *audio_buf;
		uint8_t *audio_buf1;
		int audio_hw_buf_size;
		DECLARE_ALIGNED(16,uint8_t,audio_buf2)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
		uint8_t silence_buf[1024];
		unsigned int audio_buf_size; /* in bytes */
		int audio_buf_index; /* in bytes */
		int audio_write_buf_size;
		double audio_current_pts;
		double audio_current_pts_drift;
		double audio_clock;
		int paused;
		int seek_req;
		int64_t seek_pos;
		double seek_second;
		int refresh_clock;
		int refresh_clock2;
	}as;
	int64_t audio_callback_time;
	int volume;
	int stream_component_open();
	void stream_component_close();
	void InitPacketQueue();
	void FlushPacketQueue();
	void AbortPacketQueue();
	void EndPacketQueue();
	int PutPacket(AVPacket* pkt);
	int GetPacket(AVPacket *pkt, int block);
	int AudioDecodeFrame(double *pts_ptr);
	static int PlayThread(void* param);
	static void CallBack(void *opaque, Uint8 *stream, int len);
	static int decode_inte(void* param){
		CAudioPlayer2* p = (CAudioPlayer2*)param;
		return p->stop;
	}
	int status;
public:
	CAudioPlayer2():pFormatCtx(NULL), pCodecCtx(NULL), pCodec(NULL), audioStream(-1), stop(0), volume(SDL_MIX_MAXVOLUME){
		av_register_all();
		av_init_packet(&flush_pkt);
		flush_pkt.data = (uint8_t*)"FLUSH";
		status = STOP;
	}
	CAudioPlayer2(char* Filename):pFormatCtx(NULL), pCodecCtx(NULL), pCodec(NULL), audioStream(-1), stop(0), volume(SDL_MIX_MAXVOLUME), CAudioPlayer(Filename){
		av_register_all();
		av_init_packet(&flush_pkt);
		flush_pkt.data = (uint8_t*)"FLUSH";
		status = STOP;
	}
	~CAudioPlayer2(){}
	int Open(char* filename);
	DWORD Open(){
		Open(path);
		return 0;
	}
	DWORD Play(){
		if (!pFormatCtx)
			Open();
		hPlayThread = SDL_CreateThread(PlayThread, this);
		return 1;
	}
	int GetStatus(){return status;}
	DWORD Close(){
		if (!pFormatCtx)
			return 0;
		queue.abort_request = 1;
		SDL_WaitThread(hPlayThread, NULL);
		stream_component_close();
		return 0;
	}
	DWORD Pause(){status = PAUSE; as.paused = 1;return 0;}
	DWORD Resume(){status = PLAY; as.paused = 0;return 0;}
	DWORD Stop() {status = STOP;Close();return 0;}
	DWORD Replay(){Stop();Close();Open();Play();return 0;}
	int GetVolume() {return (double)volume / 128 * 100 ;}
	void SetVolume(int num){
		int temp = (double)num / 100 * 128;
		if (temp > 128)
			volume = SDL_MIX_MAXVOLUME;
		else if (temp < 0)
			volume = 0;
		else volume = temp;
	}
	void IncreaseVolume(int vol) {SetVolume(volume + vol);}
	void DecreaseVolume(int vol) {SetVolume(volume - vol);}
	void IncreasePosition(int pos) {
		SetPosition(GetPosition() + pos);
	}
	void DecreasePosition(int pos) {
		SetPosition(GetPosition() - pos);
	}
	long GetTotalLength(){
		if (pFormatCtx){
			return pFormatCtx->streams[audioStream]->duration * pFormatCtx->streams[audioStream]->time_base.num / pFormatCtx->streams[audioStream]->time_base.den;  
		}
		return 0;
	}
	long GetPosition() {
		if (pFormatCtx){ 
			if (!as.refresh_clock)
				return as.audio_clock;
			else
				return as.seek_second; 
		}else 
			return 0;
	}
	double getclock(){return as.audio_clock;}
	void SetPosition(int second) {
		if (!pFormatCtx)
			return;
		as.refresh_clock = 2;
		int64_t ts;
		double p = (double)second / (double)GetTotalLength();
		ts = p * pFormatCtx->duration;
		as.seek_pos = ts;
		as.seek_second = ts / 1000000.0;
		as.seek_req = 1;
	}
};

}
#endif
