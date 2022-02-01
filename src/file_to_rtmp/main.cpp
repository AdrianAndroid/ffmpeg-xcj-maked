extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

#include<iostream>
using namespace std;
// 引用库
#pragma comment(lib,"avformat.lib")//预处理命令
#pragma comment(lib,"avutil.lib")//预处理命令
#pragma comment(lib,"avcodec.lib")//预处理命令

int XError(int errorNum)
{
	char buf[1024] = { 0 };
	av_strerror(errorNum, buf, sizeof(buf));
	cout << buf << endl;
	getchar();
	return -1;
}

static double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int main(int argc, char *argv[])
{
	//char *inUrl = "test.mp4";
	char *inUrl = "test.flv";
	char *outUrl = "rtmp://192.168.114.128/live";

	// 初始化封装和解封装
	av_register_all();
	//av_register_all(指定的封装器);

	//初始化网络库
	avformat_network_init();

	//1. 打开文件
	// 输入封装上下文
	AVFormatContext *ictx = NULL;
	//char *inUrl = "test1.mp4"; 文件名称不对 No Such of file 
	//char *inUrl = "swscale-5.dll"; //文件格式不对 Invalid data found when processing input

	// 打开文件，解封文件头
	//0 on success, a negative AVERROR on failure.
	int re = avformat_open_input(&ictx, inUrl, 0, 0);
	if (re != 0)
	{
		return XError(re);
	}
	cout << "open file" << inUrl << " Success" << endl;

	//获取音频，视频流信息
	re = avformat_find_stream_info(ictx, 0);
	if (re != 0)
	{
		return XError(re);
	}

	// 打印信息
	av_dump_format(ictx, 0, inUrl, 0);

	//输出流
	// 创建
	AVFormatContext *octx = NULL;
	re = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
	if (!octx) {
		return XError(re);
	}
	cout << "octx create sucess" << endl;

	// 配置输出流
	// 遍历输入的AVStream
	for (int i = 0; i < ictx->nb_streams; i++)
	{
		// 创建输出流
		AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec); //h264,av
		if (!out)
		{
			return XError(0);
		}
		//复制配置信息,用于Mp4
		//re = avcodec_copy_context(out->codec, ictx->streams[i]->codec); // test.mp4
		re = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar); // test.flv
		out->codec->codec_tag = 0;
	}

	av_dump_format(octx, 0, outUrl, 1); // 1是输出

	// rtmp 推流
	// 打开io
	re = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
	if (!octx->pb)
	{
		return XError(re);
	}
	// 写入头信息
	re = avformat_write_header(octx, 0);
	if (re < 0)
	{
		return XError(re);
	}
	cout << "avformat_write_header -> " << re << endl;

	// 推流每一帧数据
	AVPacket pkt;
	long long startTime = av_gettime();
	for (;;)
	{
		re = av_read_frame(ictx, &pkt);
		if (re != 0)
		{
			break;
		}
		cout << pkt.pts << " " << flush;
		//计算转换时间戳
		AVRational itime = ictx->streams[pkt.stream_index]->time_base;
		AVRational otime = octx->streams[pkt.stream_index]->time_base;
		//pkt.pts = (pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		//pkt.dts = (pkt.dts /*dts?*/, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		//pkt.duration = (pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		//pkt.pos = -1;
		pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
		pkt.dts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
		pkt.duration = av_rescale_q_rnd(pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
		pkt.pos = -1;

		if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVRational tb = ictx->streams[pkt.stream_index]->time_base;
			//已经过去的时间
			long long now = av_gettime() - startTime;
			long long dts = 0;
			dts = pkt.dts * (1000 * 1000 * r2d(tb));
			if (dts > now)
				av_usleep(dts - now);
		}

		//发送这一帧数据,自动缓冲排序发送, 也unref操作
		re = av_interleaved_write_frame(octx, &pkt);

		//if (ictx->streams[pkt.stream_index]->codecpar->codec_type
		//	== AVMEDIA_TYPE_VIDEO) { //视频流-存在B帧，P帧
		//	AVRational tb = ictx->streams[pkt.stream_index]->time_base;
		//	// 记时,已经过去的时间
		//	long long now = av_gettime() - startTime;
		//	long long dts = 0;
		//	dts = pkt.dts * (1000 * 1000 * tb.num / tb.den); //微秒数
		//	if(dts > now)
		//		av_usleep(dts-now); //参数是微秒
		//}
		if (re < 0)
		{
			return XError(re);
		}

		// 考虑空间的问题
		// 释放
		//av_packet_unref(&pkt);
	}

	cout << "file to rtmp test" << endl;
	getchar();
	return 0;
}