#include "decoder.h"
#include "log.h"
#include <assert.h>
#include <iostream>

#define kBufferSize 64 * 1024

enum {
    API_MODE_OLD                  = 0, /* old method, deprecated */
    API_MODE_NEW_API_REF_COUNT    = 1, /* new method, using the frame reference counting */
    API_MODE_NEW_API_NO_REF_COUNT = 2, /* new method, without reference counting */
};

static const int api_mode = API_MODE_NEW_API_REF_COUNT;

Decoder::Decoder() :
	format_context(NULL),
	codec_context(NULL),
	audio_stream(NULL),
	frame(NULL),
	decoding(false)
{

}

Decoder::~Decoder() {
	destroy_contexts();
}

void Decoder::destroy_contexts() {
	// if (frame) av_free(frame);
	// if (io_buffer) av_free(io_buffer);
	// if (codec_context) avcodec_close(codec_context);
	// if (format_context) avformat_close_input(&format_context);

	codec_context = NULL;
	format_context = NULL;
}

bool Decoder::is_loaded() const {
	return codec_context && format_context;
}

DecoderInfo Decoder::get_decoder_info() {
	DecoderInfo info;
	if (!is_loaded()) return info;

	info.sample_rate = codec_context->sample_rate;
	info.channels = codec_context->channels;
	info.bytes_per_sample = av_get_bytes_per_sample(codec_context->sample_fmt);

	return info;
}

bool Decoder::load_file(const std::string &filename) {
	decoding = false;
	destroy_contexts();

	if (avformat_open_input(&format_context, filename.c_str(), NULL, NULL) != 0 || avformat_find_stream_info(format_context, NULL) != 0 ||
		(stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) < 0) {
		if (format_context) avformat_close_input(&format_context);
		format_context = NULL;
		fprintf(stderr, "Unable to open file or find stream %s\n", filename.c_str());
		return false;
	}
	AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    st = format_context->streams[stream_index];
    /* find decoder for the stream */
    dec_ctx = st->codec;
    dec = avcodec_find_decoder(dec_ctx->codec_id);
    if (!dec) return false;

    if (api_mode == API_MODE_NEW_API_REF_COUNT)
		av_dict_set(&opts, "refcounted_frames", "1", 0);

	if (avcodec_open2(dec_ctx, dec, &opts) < 0) return false;

	audio_stream = format_context->streams[stream_index];
    codec_context = audio_stream->codec;

    if (api_mode == API_MODE_OLD)
        frame = avcodec_alloc_frame();
    else
        frame = av_frame_alloc();
	assert(frame);

	av_init_packet(&packet);

	decoding = true;

//	swr = swr_alloc();
//	av_opt_set_int(swr, "in_channel_layout",  codec_context->channel_layout, 0);
//	av_opt_set_int(swr, "out_channel_layout", codec_context->channel_layout,  0);
//	av_opt_set_int(swr, "in_sample_rate",     codec_context->sample_rate, 0);
//	av_opt_set_int(swr, "out_sample_rate",    codec_context->sample_rate, 0);
//	av_opt_set_sample_fmt(swr, "in_sample_fmt",  codec_context->sample_fmt, 0);
//	av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLTP,  0);
//	if (swr_init(swr) != 0) {
//		fprintf(stderr, "Unable to open converter\n");
//		return false;
//	}

	return true;
}

void Decoder::stop() {
	destroy_contexts();
	decoding = false;
}

void Decoder::tick(std::function<void(std::vector<float> &samples)> callback) {
	if (!format_context || !codec_context || !decoding) return;
	static std::vector<float> buffer(16384, 0);

	int got_frame = 0;
	if (av_read_frame(format_context, &packet) == 0) {
		if (packet.stream_index == audio_stream->index) {
			// Audio packets can have multiple audio frames in a single packet
			while (packet.size > 0) {
				int result = avcodec_decode_audio4(codec_context, frame, &got_frame, &packet);

				if (result >= 0 && got_frame) {
					packet.size -= result;
					packet.data += result;

					process_audio_frame(codec_context, frame, buffer);
					if (callback) callback(buffer);
				}
				else {
					packet.size = 0;
					packet.data = NULL;
				}
			}
		}

		av_free_packet(&packet);
	}
	else {
		log_text("Done decoding\n");
	 	decoding = false;
	}

	// Flush remaining - if decoding flag not set, we must've finished this frame.
	if (!decoding && ((codec_context->codec->capabilities & CODEC_CAP_DELAY) || got_frame)) {
		av_init_packet(&packet); // needed ?
		int got_frame = 0;
		while (avcodec_decode_audio4(codec_context, frame, &got_frame, &packet) >= 0 && got_frame) {
			process_audio_frame(codec_context, frame, buffer);
			if (callback) callback(buffer);
		}
	}
}

template<typename T>
void convert_samples(const uint8_t **source_data, int num_channels, int size, float divisor, std::vector<float> &output_buffer) {
	T *streams[] = {
		(T*)source_data[0],
		(T*)source_data[1]
	};
	assert(num_channels <= 2);

	output_buffer.resize(size * num_channels);
	for (int i = 0, index = 0; i < size; i++) {
		for (int c = 0; c < num_channels; c++) {
			output_buffer[index++] = streams[c][i] / divisor;
		}
	}
}

// planar - not packed, data in frame->data[0] and next channel is frame->data[1] and so on.
void Decoder::process_planar_audio_frame(const AVCodecContext *context, const AVFrame *frame, std::vector<float> &output_buffer) {
	// swr_convert crashing so we'll just manually convert.
	// swr_convert(swr, &buffer_pointer, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);

	if (context->sample_fmt == AV_SAMPLE_FMT_S16P) {
		convert_samples<int16_t>((const uint8_t**)frame->data, context->channels, frame->nb_samples, 32768.0f, output_buffer);
	}
	else if (context->sample_fmt == AV_SAMPLE_FMT_U8P) {
		// Note we're using signed to get the [-1, 1] resulting range
		convert_samples<int8_t>((const uint8_t**)frame->data, context->channels, frame->nb_samples, 128.0f, output_buffer);
	}
	else if (context->sample_fmt == AV_SAMPLE_FMT_FLTP) {
		convert_samples<float>((const uint8_t**)frame->data, context->channels, frame->nb_samples, 1, output_buffer);
	}
	else {
		fprintf(stderr, "Unhandled planar format\n");
	}
}

// packed, all data of all channels packed in data[0]
void Decoder::process_packed_audio_frame(const AVCodecContext *context, const AVFrame *frame, std::vector<float> &output_buffer) {
	// Note we just use 1 channel and multiply sample count by channels
	if (context->sample_fmt == AV_SAMPLE_FMT_S16) {
		convert_samples<int16_t>((const uint8_t**)frame->data, 1, frame->nb_samples * context->channels, 32768.0f, output_buffer);
	}
	else if (context->sample_fmt == AV_SAMPLE_FMT_U8) {
		// Note we're using signed to get the [-1, 1] resulting range
		convert_samples<int8_t>((const uint8_t**)frame->data, 1, frame->nb_samples * context->channels, 128.0f, output_buffer);
	}
	else if (context->sample_fmt == AV_SAMPLE_FMT_FLT) {
		convert_samples<float>((const uint8_t**)frame->data, 1, frame->nb_samples * context->channels, 1, output_buffer);
	}
	else {
		fprintf(stderr, "Unhandled packed format\n");
	}
}

void Decoder::process_audio_frame(const AVCodecContext *context, const AVFrame *frame, std::vector<float> &output_buffer) {
	if (context->channels > AV_NUM_DATA_POINTERS && av_sample_fmt_is_planar(context->sample_fmt)) {
		fprintf(stderr, "Too many data channels\n");
		return;
	}

	if (av_sample_fmt_is_planar(context->sample_fmt)) {
		process_planar_audio_frame(context, frame, output_buffer);
	}
	else {
		process_packed_audio_frame(context, frame, output_buffer);
	}
}

void Decoder::seek_to(float seconds) {
	seek_to_millis(static_cast<int>(seconds * 1000));
}

void Decoder::seek_to_millis(int millis) {
	int64_t frame = av_rescale(millis, format_context->streams[stream_index]->time_base.den, format_context->streams[stream_index]->time_base.num);
	frame /= 1000;
	seek_to_frame(frame);
}

void Decoder::seek_to_frame(int64_t frame) {
	if (av_seek_frame(format_context, stream_index, frame, 0) < 0) {
		fprintf(stderr, "unable to seek to frame");
	}

//	if (avformat_seek_file(format_context, 0, 0, frame, frame, AVSEEK_FLAG_FRAME) < 0) {
//		fprintf(stderr, "unable to seek to frame");
//	}
	avcodec_flush_buffers(codec_context);
}
