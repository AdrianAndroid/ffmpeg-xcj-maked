#include<opencv2/highgui.hpp>
#include<iostream>


#include "XMediaEncode.h"
#include "XRtmp.h"
using namespace cv;
using namespace std;

#pragma comment(lib, "opencv_world320.lib")

int main(int argc, char* argv[])
{
	
	char *inUrl = "rtsp"; //���������rtspurl
	// ������nginx-rtmp����url
	char *outUrl = "rtmp://192.168.114.128/live";

	// �����������ظ�ʽת���Ķ���
	XMediaEncode *me = XMediaEncode::Get(0);

	//��װ����������
	XRtmp *xr = XRtmp::Get(0);

	namedWindow("video");

	VideoCapture cam;
	Mat frame;

	try {
		// 1. ʹ��opencv��rtsp���
		cam.open(0);
		if (!cam.isOpened())
		{
			throw exception("cam open failed!");
		}
		cout << "cam open success" << endl;

		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
		int outWidth = inWidth;
		int outHeight = inHeight;
		int fps = cam.get(CAP_PROP_FPS);
		if (fps < 1) {
			fps = 75;
		}

		// ��ʼ����ʽת��������
		// ��ʼ����������ݽṹ
		me->inWidth = inWidth;
		me->inHeight = inHeight;
		me->outWidth = inWidth;
		me->outHeight = inHeight;
		me->InitScale();
		
		/// 4. ��ʼ������������
		if (!me->InitVideoCodec())
		{
			throw exception("initVideoCodec failed!");
		}

		/// 5. ��װ�����Ƶ������
		xr->Init(outUrl);
		// b.�����Ƶ��
		xr->AddStream(me->vc);

		/// 6. ��rtmp���������IO
		xr->SendHead();

		int vpts = 0;
		for (;;) {
			// ��ȡrtsp��Ƶ֡, ������Ƶ֡
			if (!cam.grab()) //����
			{
				continue; //ʧ��
			}
			// YUVת��ΪRGB
			if (!cam.retrieve(frame)) //��frame
			{
				continue;//ʧ��
			}
			imshow("video", frame);
			waitKey(1);

			// rgb to yuv
			me->inPixSize = frame.elemSize();
			AVFrame *yuv = me->RGBToYUV((char *)frame.data);
			if (!yuv)
			{ //ʧ��
				continue;
			}

			/// h264����
			AVPacket *pack = me->EncodeVideo(yuv);
			if (!pack) continue;
			
			// ����
			xr->SendFrame(pack);
		}
	}
	catch (exception &ex) {
		if (cam.isOpened())
			cam.release();
		cerr << ex.what() << endl; //�������
	}

	getchar();
	return 0;
}

//int main(int argc, char* argv[])
//{
//	VideoCapture cam;
//	//string url = "rtsp://192.168.114.128:8554/live";
//	namedWindow("video");
//	if (cam.open(0))
//	{
//		cout << "open cam sucess!" << endl;
//
//	}
//	else {
//		cout << "open cam failed!" << endl;
//		waitKey(0);
//		return -1;
//	}
//	Mat frame;
//	for (;;) {
//		cam.read(frame);
//		imshow("video", frame);
//		waitKey(1);
//	}
//	getchar();
//	return 0;
//}

//��Ƶ��ݼ����������գ���
//ctrl + , �� �����ļ���������� ����  �༭->��λ��     ע�⣺, ��Ӧ < �Ǹ������ϵĶ��ţ�������"�Ǹ����Ķ���
//	ctrl + g  : ��λ����  ����  �༭->ת��
//	ctrl + -: ���ص���һ�ι�����   ע�⣺ - �Ų���С��(����)���ϵ��Ǹ���
//	ctrl + +: ���ص���һ�ι�����   ע�⣺ + �Ų���С��(����)���ϵ��Ǹ���
//	ctrl + c : ���ƹ��������    ע�⣺ֻ��Ҫ�������һ�У�����Ҫѡ������
//	ctrl + x : ���й��������  ע�⣺ֻ�轫����������У�����Ҫѡ������
//	ctrl + l(ע����L��) : ɾ�����������  ע�⣺ֻ�轫����������У�����Ҫѡ������
//	ctrl + k + c : ע����ѡ�������������   ע�⣺�Ȱ�k, �ٰ�c     ���� ctrl + k, ctrl + c
//	ctrl + k + u : ȡ��ע����ѡ�������������   ע�⣺�Ȱ�k, �ٰ�u  ���� ctrl + k, ctrl + u
//	ctrl + f ���ڱ��ļ��в���
//	ctrl + h : �ڱ��ļ����滻
//	f2 �� ת������, �������Connect���������ﶨ�壬����˫��ѡ�У�Ȼ��f12
//	shift + f12 : ������������ ���������Connect��������Щ�ط������ˣ�����ѡ��Connect��Ȼ��shift + f12
//	f5 : ��ʼ���� ���� ��ʼ����
//
//	�����ʽ��
//	��ѡ�д��룬Ȼ����ctrl + k, �����ٰ���ctrl + f
//	��β鿴��ǰ�ļ�ĳ���ඨ������Щ������
//	�ڱ༭���ڶ���ѡ�ж�Ӧ�࣬Ȼ���ں���鿴����Щ����������������
//	�� : https://blog.csdn.net/niusiqiang/article/details/43116283


//��Ŀ��صĿ�ݼ�
//Ctrl + Shift + B = ������Ŀ
//Ctrl + Alt + L = ��ʾSolution Explorer�����������Դ��������
//Shift + Alt + C = �������
//Shift + Alt + A = �������Ŀ����Ŀ
//
//�༭��صļ��̿�ݼ�
//Ctrl + Enter = �ڵ�ǰ�в������
//Ctrl + Shift + Enter = �ڵ�ǰ���·��������
//Ctrl + �ո�� = ʹ��IntelliSense�����ܸ�֪���Զ����
//Alt + Shift + ��ͷ��(��, ��, ��, ��) = ѡ�������Զ��岿��
//Ctrl + } = ƥ������š�����
//Ctrl + Shift + } = ��ƥ������š�������ѡ���ı�
//Ctrl + Shift + S = ���������ļ�����Ŀ
//Ctrl + K��Ctrl + C = ע��ѡ����
//Ctrl + K��Ctrl + U = ȡ��ѡ���е�ע��
//Ctrl + K��Ctrl + D = ��ȷ�������д���
//Ctrl + D ��һ��ƥ���Ҳ��ѡ��
//Ctrl + C �� Ctrl + V ���ƻ���е�ǰ�� / ��ǰѡ������
//Shift + Alt + F���� Ctrl + Shift + P ������ format code �����ʽ��
//Shift + End = ��ͷ��βѡ������
//Shift + Home = ��β��ͷѡ������
//Ctrl + Delete = ɾ������Ҳ��������
//
//������صļ��̿�ݼ�
//Ctrl + Up / Down = �������ڵ����ƶ����
//Ctrl + -= �ù���ƶ�������ǰ��λ��
//Ctrl++ = �ù���ƶ�����һ��λ��
//F12 = ת������
//
//������صļ��̿�ݼ�
//Ctrl + Alt + P = ���ӵ�����
//F10 = ���Ե���ִ��
//F5 = ��ʼ����
//Shift + F5 = ֹͣ����
//Ctrl + Alt + Q = ��ӿ��ƥ��
//F9 = ���û�ɾ���ϵ�
//
//������صļ��̿�ݼ�
//Ctrl + K  Ctrl + K = ����ǰ�������ǩ
//Ctrl + K  Ctrl + N = ��������һ����ǩ
//Ctrl + . = ��������һ��������Collection<string>���������ռ䵼�벻��ȷ�Ļ�����ô�����ݷ�ʽ��Ͻ��Զ����뵼��
//Ctrl + Shift + F = ���ļ��в���
//Shift + F12 = ������������
//Ctrl + F = ��ʾ���ҶԻ���
//Ctrl + H = ��ʾ�滻�Ի���
//Ctrl + G = ��ת���кŻ���
//Ctrl + Shift + F = ������ѡ��Ŀ��������������е�����
//��������������������������������
//��Ȩ����������ΪCSDN��������Ҷ��硹��ԭ�����£���ѭCC 4.0 BY - SA��ȨЭ�飬ת���븽��ԭ�ĳ������Ӽ���������
//ԭ�����ӣ�https ://blog.csdn.net/PZ0605/article/details/89577438
