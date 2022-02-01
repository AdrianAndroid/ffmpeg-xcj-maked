#include "XRtmp.h"
extern "C"
{
#include<libavformat/avformat.h>
}
#include<iostream>
#include<string>
#pragma comment(lib, "avformat.lib")

using namespace std;

class CXRtmp :public XRtmp
{
public:
	void Close()
	{
		if(ic)
		{
			avformat_close_input(&ic);
			vs = NULL;
		}
		vc = NULL;
		url = "";
	}

	bool Init(const char *url)
	{
		this->url = url;
		int ret = avformat_alloc_output_context2(&ic, 0, "flv", url);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}
		return true;
	}

	bool AddStream(const AVCodecContext *c)
	{
		if (!c) return false;
		AVStream *st = avformat_new_stream(ic, NULL);
		if (!st)
		{
			cout << "avformat_new_stream failed!" << endl;
			return false;
		}
		st->codecpar->codec_tag = 0;//指定编码格式
									// 从编码器复制参数
		avcodec_parameters_from_context(st->codecpar, c);

		av_dump_format(ic, 0, this->url.c_str(), 1);

		if (c->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vc = c;
			vs = st;
		}
		return true;
	}

	bool SendHead()
	{
		int ret = avio_open(&ic->pb, this->url.c_str(), AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}

		//写入封装头
		ret = avformat_write_header(ic, NULL);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}
		return true;
	}

	bool SendFrame(AVPacket *pack)
	{
		pack->pts = av_rescale_q(pack->pts, vc->time_base, vs->time_base);
		pack->dts = av_rescale_q(pack->dts, vc->time_base, vs->time_base);
		int ret = av_interleaved_write_frame(ic, pack);
		if (ret == 0)
		{
			cout << "#" << flush;
		}
		return true;
	}
private:
	// rtmp flv 封装器
	AVFormatContext *ic = NULL;
	//视频编码器流
	const AVCodecContext *vc = NULL;

	AVStream *vs = NULL;
	string url = "";
};

XRtmp * XRtmp::Get(unsigned char index)
{
	static CXRtmp cxr[255];

	static bool isFirst = true;
	if (isFirst)
	{
		// 注册所有的封装器
		av_register_all();
		// 网络接口
		avformat_network_init();
		isFirst = false;
	}

	return &cxr[index];
}


XRtmp::XRtmp()
{
}



XRtmp::~XRtmp()
{
}
