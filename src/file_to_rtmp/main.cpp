extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

#include<iostream>
using namespace std;
// ���ÿ�
#pragma comment(lib,"avformat.lib")//Ԥ��������
#pragma comment(lib,"avutil.lib")//Ԥ��������
#pragma comment(lib,"avcodec.lib")//Ԥ��������

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

	// ��ʼ����װ�ͽ��װ
	av_register_all();
	//av_register_all(ָ���ķ�װ��);

	//��ʼ�������
	avformat_network_init();

	//1. ���ļ�
	// �����װ������
	AVFormatContext *ictx = NULL;
	//char *inUrl = "test1.mp4"; �ļ����Ʋ��� No Such of file 
	//char *inUrl = "swscale-5.dll"; //�ļ���ʽ���� Invalid data found when processing input

	// ���ļ�������ļ�ͷ
	//0 on success, a negative AVERROR on failure.
	int re = avformat_open_input(&ictx, inUrl, 0, 0);
	if (re != 0)
	{
		return XError(re);
	}
	cout << "open file" << inUrl << " Success" << endl;

	//��ȡ��Ƶ����Ƶ����Ϣ
	re = avformat_find_stream_info(ictx, 0);
	if (re != 0)
	{
		return XError(re);
	}

	// ��ӡ��Ϣ
	av_dump_format(ictx, 0, inUrl, 0);

	//�����
	// ����
	AVFormatContext *octx = NULL;
	re = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
	if (!octx) {
		return XError(re);
	}
	cout << "octx create sucess" << endl;

	// ���������
	// ���������AVStream
	for (int i = 0; i < ictx->nb_streams; i++)
	{
		// ���������
		AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec); //h264,av
		if (!out)
		{
			return XError(0);
		}
		//����������Ϣ,����Mp4
		//re = avcodec_copy_context(out->codec, ictx->streams[i]->codec); // test.mp4
		re = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar); // test.flv
		out->codec->codec_tag = 0;
	}

	av_dump_format(octx, 0, outUrl, 1); // 1�����

	// rtmp ����
	// ��io
	re = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
	if (!octx->pb)
	{
		return XError(re);
	}
	// д��ͷ��Ϣ
	re = avformat_write_header(octx, 0);
	if (re < 0)
	{
		return XError(re);
	}
	cout << "avformat_write_header -> " << re << endl;

	// ����ÿһ֡����
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
		//����ת��ʱ���
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
			//�Ѿ���ȥ��ʱ��
			long long now = av_gettime() - startTime;
			long long dts = 0;
			dts = pkt.dts * (1000 * 1000 * r2d(tb));
			if (dts > now)
				av_usleep(dts - now);
		}

		//������һ֡����,�Զ�����������, Ҳunref����
		re = av_interleaved_write_frame(octx, &pkt);

		//if (ictx->streams[pkt.stream_index]->codecpar->codec_type
		//	== AVMEDIA_TYPE_VIDEO) { //��Ƶ��-����B֡��P֡
		//	AVRational tb = ictx->streams[pkt.stream_index]->time_base;
		//	// ��ʱ,�Ѿ���ȥ��ʱ��
		//	long long now = av_gettime() - startTime;
		//	long long dts = 0;
		//	dts = pkt.dts * (1000 * 1000 * tb.num / tb.den); //΢����
		//	if(dts > now)
		//		av_usleep(dts-now); //������΢��
		//}
		if (re < 0)
		{
			return XError(re);
		}

		// ���ǿռ������
		// �ͷ�
		//av_packet_unref(&pkt);
	}

	cout << "file to rtmp test" << endl;
	getchar();
	return 0;
}