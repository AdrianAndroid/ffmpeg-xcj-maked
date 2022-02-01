#include "XMediaEncode.h"


#include<opencv2/highgui.hpp>
#include<iostream>

extern "C"
{
#include <libswresample/swresample.h> //音频重采样
#include<libswscale/swscale.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
}

//using namespace cv;
//vs2015 配置opencv遇initVideoCodec failed!到的问题 https://www.cnblogs.com/baldermurphy/p/6123687.html
using namespace std;

#pragma comment(lib, "swresample.lib") //音频重采样
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "opencv_world320.lib")


#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif
//获取CPU数量
static int XGetCpuNum()
{
#if defined WIN32 || defined _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	return (int)sysinfo.dwNumberOfProcessors;
#elif defined __linux__
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined __APPLE__
	int numCPU = 0;
	int mib[4];
	size_t len = sizeof(numCPU);

	// set the mib for hw.ncpu
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

						   // get the number of CPUs from the system
	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if (numCPU < 1)
	{
		mib[1] = HW_NCPU;
		sysctl(mib, 2, &numCPU, &len, NULL, 0);

		if (numCPU < 1)
			numCPU = 1;
	}
	return (int)numCPU;
#else
	return 1;
#endif
}
class CXMediaEncode : public XMediaEncode {
public:

	void Close()
	{
		// 非线程安全
		if (vsc) {
			sws_freeContext(vsc);
			vsc = NULL; // 防止多次调用close，导致两次释放
		}
		if (asc)  {
			swr_free(&asc);
		}
		if (yuv)
		{
			av_frame_free(&yuv); 
			//yuv = NULL;
		}
		if (vc) //释放，成对编程
		{
			//avio_closep(&ic->pb);
			//ic->pb = NULL;
			avcodec_free_context(&vc);
			//vc = NULL;
		}
		if (pcm)
		{
			av_frame_free(&pcm);
		}
		vpts = 0;
		av_packet_unref(&pack);
	}

	bool InitVideoCodec()
	{
		/// 4. 初始化编码上下文
		//a.找到编码器
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			cout << "Cannot find H264 encoder" << endl;
			return false;
		}
		//b.创建编码器上下文
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			cout << "avcodec_alloc_context3 failed!" << endl;
			return false;
		}
		//c. 配置编码器参数
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //全局参数
		vc->codec_id = codec->id;
		vc->thread_count = XGetCpuNum(); //通过cpu的数量

		vc->bit_rate = 50 * 1024 * 8;//压缩后每秒视频的bit位大小 50kB 压缩率
		vc->width = outWidth;
		vc->height = outHeight;
		vc->time_base = { 1, fps };//时间基数
		vc->framerate = { fps, 1 };//帧率
								   // 画面组的大小,多少帧一个关键帧
		vc->gop_size = 50;
		vc->max_b_frames = 0; // 
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//d.打开编码器上下文
		int ret = avcodec_open2(vc, 0, 0);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf;
			return false;
		}
		cout << "avcodec_open2 success!" << endl;
		return true;
	}

	AVPacket * EncodeVideo(AVFrame *frame)
	{
		av_packet_unref(&pack); //释放空间
		yuv->pts = vpts;
		vpts++;
		int ret = avcodec_send_frame(vc, yuv);
		if (ret != 0) {
			return NULL; //不处理
		}

		ret = avcodec_receive_packet(vc, &pack);
		if (ret != 0 || pack.size <= 0)
		{
			return NULL;//cout << "*"<<pack.size << flush;
		}

		return &pack;
	}

	bool InitScale()
	{
		//2. 初始化格式转换上下文
		int inPixSize = 3;
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,// 源宽高,像素格式
			outWidth, outHeight, AV_PIX_FMT_YUV420P,//目标宽高,像素格式
			SWS_BICUBIC, //尺寸变化使用算法
			0, 0, 0
		);
		if (!vsc)
		{
			cout << "sws_getCachedContext failed!" << endl;
			return false;
		}
		// 3. 输出的数据结构
		// 分配对象空间
		yuv = av_frame_alloc(); //动态链接库中的
								//参数的预置
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0; //时间：连续的
					  // 分配yuv空间
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		return true;
	}

	AVFrame* RGBToYUV(char *rgb) 
	{
		// 输入的数据结构
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		//bgrbgrbgr
		//plane indata[0] bbbbb indata[1] ggggg indata[2] rrrrr
		indata[0] = (uint8_t*)rgb;
		int insize[AV_NUM_DATA_POINTERS] = { 0 };
		// 一行(宽)数据的字节数 ,输入
		insize[0] =  inWidth * inPixSize;//frame.cols * frame.elemSize();

		int h = sws_scale(vsc, indata, insize, 0, inHeight, //源数据
			yuv->data, yuv->linesize);
		if (h <= 0)
		{
			return NULL; //不做任何处理
		}
		//cout << h << " " << flush;
		return yuv;
	}


	bool InitResample()
	{
		//音频重采样 //上下文初始化
		//struct SwrContext *s,
		//int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
		//int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
		//int log_offset, void *log_ctx
		asc = NULL;
		asc = swr_alloc_set_opts(asc,
			av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt, sampleRate, // 输出格式
			av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate,  // 输入格式
			0, 0
		);
		if (!asc)
		{
			cout << "swr_alloc_set_opts failed!" << endl;
			return false;
		}
		int ret = swr_init(asc);
		if (ret != 0)
		{
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;
			return false;
		}
		cout << "音频重采样 上下文初始化成功!" << endl;

		// 音频重采样输出分配
		pcm = av_frame_alloc(); //预先申请好
		pcm->format = outSampleFmt;
		pcm->channels = channels;
		pcm->channel_layout = av_get_default_channel_layout(channels);
		pcm->nb_samples = nbSample;//一帧音频一通道的采样数量
		ret = av_frame_get_buffer(pcm, 0); //给pcm分配了存储空间
		if (ret != 0)
		{
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;
			return false;
		}

		return true;
	}
	AVFrame* Resample(char* data)
	{
		//已经读一帧源数据
		//cout << size << " ";
		//重采样源数据
		const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		indata[0] = (uint8_t *)data;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples, //输出参数，输出存储地址和样本数量
			indata, pcm->nb_samples
		);
		if (len <= 0)
		{
			return NULL;
		}
		return pcm;
	}

private:
	SwsContext *vsc = NULL;    //像素格式转换上下文c++11
	SwrContext *asc = NULL;    //重采样上下文
	AVFrame *yuv = NULL;       //输出的YUV
	AVFrame *pcm = NULL;       //重采样输出的PCM
	AVPacket pack = {0};       //
	int vpts = 0;
	int ret = 0;

};

XMediaEncode * XMediaEncode::Get(unsigned char index)
{
	static bool isFirst = true;
	if (isFirst) {
		//注册所有的编解码器
		avcodec_register_all();
		isFirst = false;
	}
	static CXMediaEncode cxm[255];
	return &cxm[index];
}


XMediaEncode::XMediaEncode()
{
}


XMediaEncode::~XMediaEncode()
{
}
