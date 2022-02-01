#include "XMediaEncode.h"


#include<opencv2/highgui.hpp>
#include<iostream>

extern "C"
{
#include <libswresample/swresample.h> //��Ƶ�ز���
#include<libswscale/swscale.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
}

//using namespace cv;
//vs2015 ����opencv��initVideoCodec failed!�������� https://www.cnblogs.com/baldermurphy/p/6123687.html
using namespace std;

#pragma comment(lib, "swresample.lib") //��Ƶ�ز���
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "opencv_world320.lib")


#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif
//��ȡCPU����
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
		// ���̰߳�ȫ
		if (vsc) {
			sws_freeContext(vsc);
			vsc = NULL; // ��ֹ��ε���close�����������ͷ�
		}
		if (asc)  {
			swr_free(&asc);
		}
		if (yuv)
		{
			av_frame_free(&yuv); 
			//yuv = NULL;
		}
		if (vc) //�ͷţ��ɶԱ��
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
		/// 4. ��ʼ������������
		//a.�ҵ�������
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			cout << "Cannot find H264 encoder" << endl;
			return false;
		}
		//b.����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			cout << "avcodec_alloc_context3 failed!" << endl;
			return false;
		}
		//c. ���ñ���������
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = XGetCpuNum(); //ͨ��cpu������

		vc->bit_rate = 50 * 1024 * 8;//ѹ����ÿ����Ƶ��bitλ��С 50kB ѹ����
		vc->width = outWidth;
		vc->height = outHeight;
		vc->time_base = { 1, fps };//ʱ�����
		vc->framerate = { fps, 1 };//֡��
								   // ������Ĵ�С,����֡һ���ؼ�֡
		vc->gop_size = 50;
		vc->max_b_frames = 0; // 
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//d.�򿪱�����������
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
		av_packet_unref(&pack); //�ͷſռ�
		yuv->pts = vpts;
		vpts++;
		int ret = avcodec_send_frame(vc, yuv);
		if (ret != 0) {
			return NULL; //������
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
		//2. ��ʼ����ʽת��������
		int inPixSize = 3;
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,// Դ���,���ظ�ʽ
			outWidth, outHeight, AV_PIX_FMT_YUV420P,//Ŀ����,���ظ�ʽ
			SWS_BICUBIC, //�ߴ�仯ʹ���㷨
			0, 0, 0
		);
		if (!vsc)
		{
			cout << "sws_getCachedContext failed!" << endl;
			return false;
		}
		// 3. ��������ݽṹ
		// �������ռ�
		yuv = av_frame_alloc(); //��̬���ӿ��е�
								//������Ԥ��
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0; //ʱ�䣺������
					  // ����yuv�ռ�
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
		// ��������ݽṹ
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		//bgrbgrbgr
		//plane indata[0] bbbbb indata[1] ggggg indata[2] rrrrr
		indata[0] = (uint8_t*)rgb;
		int insize[AV_NUM_DATA_POINTERS] = { 0 };
		// һ��(��)���ݵ��ֽ��� ,����
		insize[0] =  inWidth * inPixSize;//frame.cols * frame.elemSize();

		int h = sws_scale(vsc, indata, insize, 0, inHeight, //Դ����
			yuv->data, yuv->linesize);
		if (h <= 0)
		{
			return NULL; //�����κδ���
		}
		//cout << h << " " << flush;
		return yuv;
	}


	bool InitResample()
	{
		//��Ƶ�ز��� //�����ĳ�ʼ��
		//struct SwrContext *s,
		//int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
		//int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
		//int log_offset, void *log_ctx
		asc = NULL;
		asc = swr_alloc_set_opts(asc,
			av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt, sampleRate, // �����ʽ
			av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate,  // �����ʽ
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
		cout << "��Ƶ�ز��� �����ĳ�ʼ���ɹ�!" << endl;

		// ��Ƶ�ز����������
		pcm = av_frame_alloc(); //Ԥ�������
		pcm->format = outSampleFmt;
		pcm->channels = channels;
		pcm->channel_layout = av_get_default_channel_layout(channels);
		pcm->nb_samples = nbSample;//һ֡��Ƶһͨ���Ĳ�������
		ret = av_frame_get_buffer(pcm, 0); //��pcm�����˴洢�ռ�
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
		//�Ѿ���һ֡Դ����
		//cout << size << " ";
		//�ز���Դ����
		const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		indata[0] = (uint8_t *)data;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples, //�������������洢��ַ����������
			indata, pcm->nb_samples
		);
		if (len <= 0)
		{
			return NULL;
		}
		return pcm;
	}

private:
	SwsContext *vsc = NULL;    //���ظ�ʽת��������c++11
	SwrContext *asc = NULL;    //�ز���������
	AVFrame *yuv = NULL;       //�����YUV
	AVFrame *pcm = NULL;       //�ز��������PCM
	AVPacket pack = {0};       //
	int vpts = 0;
	int ret = 0;

};

XMediaEncode * XMediaEncode::Get(unsigned char index)
{
	static bool isFirst = true;
	if (isFirst) {
		//ע�����еı������
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
