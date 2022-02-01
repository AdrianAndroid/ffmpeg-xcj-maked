#include<opencv2/highgui.hpp>
#include<iostream>


#include "XMediaEncode.h"
#include "XRtmp.h"
using namespace cv;
using namespace std;

#pragma comment(lib, "opencv_world320.lib")

int main(int argc, char* argv[])
{
	
	char *inUrl = "rtsp"; //海康相机的rtspurl
	// 服务器nginx-rtmp推流url
	char *outUrl = "rtmp://192.168.114.128/live";

	// 编码器和像素格式转换的对象
	XMediaEncode *me = XMediaEncode::Get(0);

	//封装和推流对象
	XRtmp *xr = XRtmp::Get(0);

	namedWindow("video");

	VideoCapture cam;
	Mat frame;

	try {
		// 1. 使用opencv打开rtsp相机
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

		// 初始化格式转换上下文
		// 初始化输出的数据结构
		me->inWidth = inWidth;
		me->inHeight = inHeight;
		me->outWidth = inWidth;
		me->outHeight = inHeight;
		me->InitScale();
		
		/// 4. 初始化编码上下文
		if (!me->InitVideoCodec())
		{
			throw exception("initVideoCodec failed!");
		}

		/// 5. 封装其和视频流配置
		xr->Init(outUrl);
		// b.添加视频流
		xr->AddStream(me->vc);

		/// 6. 打开rtmp的网络输出IO
		xr->SendHead();

		int vpts = 0;
		for (;;) {
			// 读取rtsp视频帧, 解码视频帧
			if (!cam.grab()) //解码
			{
				continue; //失败
			}
			// YUV转换为RGB
			if (!cam.retrieve(frame)) //读frame
			{
				continue;//失败
			}
			imshow("video", frame);
			waitKey(1);

			// rgb to yuv
			me->inPixSize = frame.elemSize();
			AVFrame *yuv = me->RGBToYUV((char *)frame.data);
			if (!yuv)
			{ //失败
				continue;
			}

			/// h264编码
			AVPacket *pack = me->EncodeVideo(yuv);
			if (!pack) continue;
			
			// 推流
			xr->SendFrame(pack);
		}
	}
	catch (exception &ex) {
		if (cam.isOpened())
			cam.release();
		cerr << ex.what() << endl; //错误输出
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

//高频快捷键（必须掌握）：
//ctrl + , ： 搜索文件或函数或变量 或者  编辑->定位到     注意：, 对应 < 那个按键上的逗号，而不是"那个键的逗号
//	ctrl + g  : 定位到行  或者  编辑->转到
//	ctrl + -: 返回到上一次光标浏览   注意： - 号不是小键(数字)盘上的那个键
//	ctrl + +: 返回到后一次光标浏览   注意： + 号不是小键(数字)盘上的那个键
//	ctrl + c : 复制光标所在行    注意：只需要光标在这一行，不需要选中整行
//	ctrl + x : 剪切光标所在行  注意：只需将光标移至该行，不需要选中整行
//	ctrl + l(注意是L键) : 删除光标所在行  注意：只需将光标移至该行，不需要选中整行
//	ctrl + k + c : 注释所选代码或光标所在行   注意：先按k, 再按c     或者 ctrl + k, ctrl + c
//	ctrl + k + u : 取消注释所选代码或光标所在行   注意：先按k, 再按u  或者 ctrl + k, ctrl + u
//	ctrl + f ：在本文件中查找
//	ctrl + h : 在本文件中替换
//	f2 ： 转到定义, 如想查找Connect函数在哪里定义，可以双击选中，然后f12
//	shift + f12 : 查找所有引用 。如想查找Connect函数在哪些地方调用了，可以选中Connect，然后shift + f12
//	f5 : 开始运行 或者 开始调试
//
//	代码格式：
//	先选中代码，然后按下ctrl + k, 马上再按下ctrl + f
//	如何查看当前文件某个类定义了哪些方法：
//	在编辑窗口顶部选中对应类，然后在后面查看有哪些方法（可以搜索）
//	如 : https://blog.csdn.net/niusiqiang/article/details/43116283


//项目相关的快捷键
//Ctrl + Shift + B = 生成项目
//Ctrl + Alt + L = 显示Solution Explorer（解决方案资源管理器）
//Shift + Alt + C = 添加新类
//Shift + Alt + A = 添加新项目到项目
//
//编辑相关的键盘快捷键
//Ctrl + Enter = 在当前行插入空行
//Ctrl + Shift + Enter = 在当前行下方插入空行
//Ctrl + 空格键 = 使用IntelliSense（智能感知）自动完成
//Alt + Shift + 箭头键(←, ↑, ↓, →) = 选择代码的自定义部分
//Ctrl + } = 匹配大括号、括号
//Ctrl + Shift + } = 在匹配的括号、括号内选择文本
//Ctrl + Shift + S = 保存所有文件和项目
//Ctrl + K，Ctrl + C = 注释选定行
//Ctrl + K，Ctrl + U = 取消选定行的注释
//Ctrl + K，Ctrl + D = 正确对齐所有代码
//Ctrl + D 下一个匹配的也被选中
//Ctrl + C 、 Ctrl + V 复制或剪切当前行 / 当前选中内容
//Shift + Alt + F，或 Ctrl + Shift + P 后输入 format code 代码格式化
//Shift + End = 从头到尾选择整行
//Shift + Home = 从尾到头选择整行
//Ctrl + Delete = 删除光标右侧的所有字
//
//导航相关的键盘快捷键
//Ctrl + Up / Down = 滚动窗口但不移动光标
//Ctrl + -= 让光标移动到它先前的位置
//Ctrl++ = 让光标移动到下一个位置
//F12 = 转到定义
//
//调试相关的键盘快捷键
//Ctrl + Alt + P = 附加到进程
//F10 = 调试单步执行
//F5 = 开始调试
//Shift + F5 = 停止调试
//Ctrl + Alt + Q = 添加快捷匹配
//F9 = 设置或删除断点
//
//搜索相关的键盘快捷键
//Ctrl + K  Ctrl + K = 将当前行添加书签
//Ctrl + K  Ctrl + N = 导航至下一个书签
//Ctrl + . = 如果你键入一个类名如Collection<string>，且命名空间导入不正确的话，那么这个快捷方式组合将自动插入导入
//Ctrl + Shift + F = 在文件中查找
//Shift + F12 = 查找所有引用
//Ctrl + F = 显示查找对话框
//Ctrl + H = 显示替换对话框
//Ctrl + G = 跳转到行号或行
//Ctrl + Shift + F = 查找所选条目在整个解决方案中的引用
//————————————————
//版权声明：本文为CSDN博主「绿叶清风」的原创文章，遵循CC 4.0 BY - SA版权协议，转载请附上原文出处链接及本声明。
//原文链接：https ://blog.csdn.net/PZ0605/article/details/89577438
