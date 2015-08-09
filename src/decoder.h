#pragma once

#include <string>
#include <functional>
#include <vector>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/mem.h>
	#include <libavutil/opt.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/common.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/samplefmt.h>
	#include <libswresample/swresample.h>
}

//
// Decoder decodes an audio file into raw audio using libavcodec
//

struct DecoderInfo {
	int sample_rate;
	int channels;
	int bytes_per_sample;
};

class Decoder {
public:
	Decoder();
	~Decoder();

	bool load_file(const std::string &filename);
	bool is_loaded() const;
	bool is_done_decoding() const;
	void tick(std::function<void(std::vector<float> &samples)> callback);

	// Call only after loading a file for accuracy
	DecoderInfo get_decoder_info();

private:
	AVPacket          packet;
	AVFrame          *frame;

	AVFormatContext  *format_context;
	AVCodecContext   *codec_context;
	AVStream         *audio_stream;

	bool             decoding;

	// For conversions
	SwrContext       *swr;

	void destroy_contexts();

	void process_planar_audio_frame(const AVCodecContext *context, const AVFrame *frame, std::vector<float> &output_buffer);
	void process_packed_audio_frame(const AVCodecContext *context, const AVFrame *frame, std::vector<float> &output_buffer);
	void process_audio_frame(const AVCodecContext *context, const AVFrame *frame, std::vector<float> &output_buffer);
};

inline bool Decoder::is_done_decoding() const {
	return !decoding;
}