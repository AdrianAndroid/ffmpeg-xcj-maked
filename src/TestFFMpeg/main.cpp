/*******************************************************************************
**                                                                            **
**                     Jiedi(China nanjing)Ltd.                               **
**	               �������Ĳܿ����˴��������Ϊѧϰ�ο�                       **
*******************************************************************************/

/*****************************FILE INFOMATION***********************************
**
** Project       : FFmpeg
** Description   : FFMPEG��Ŀ����ʾ��
** Contact       : xiacaojun@qq.com
**        ����   : http://blog.csdn.net/jiedichina
**		��Ƶ�γ� 
** �����ƿ���	http://study.163.com/u/xiacaojun
** ��Ѷ����		https://jiedi.ke.qq.com/
** csdnѧԺ		http://edu.csdn.net/lecturer/lecturer_detail?lecturer_id=961
** 51ctoѧԺ	    http://edu.51cto.com/lecturer/index/user_id-12016059.html
** �������µ�ffmpeg�汾 http://www.ffmpeg.club
**
**   ���Ŀ��ý���Ⱥ ��296249312 
**   ���Ŀ�����ַ��www.laoxiaketang.com
**   ΢�Ź��ں�  : jiedi2007
**		ͷ����	 : �Ĳܿ�
**
*******************************************************************************/
//������������������ ���Ŀ��ý���Ⱥ ��296249312 


#include <iostream>
extern "C" 
{
#include <libavcodec/avcodec.h>
}
#pragma comment(lib,"avcodec.lib")
using namespace std;
int main(int argc, char *argv[])
{
	cout << "Test FFmpeg.club " << endl;
#ifdef WIN32
	cout << "32 λ����" << endl;
#else
	cout << "64 λ����" << endl;
#endif
	cout << avcodec_configuration() << endl;
	getchar();
	return 0;
}