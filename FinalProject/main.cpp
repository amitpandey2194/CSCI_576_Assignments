// GazeBasedControlPlayGround.cpp : Defines the entry point for the console application.
//
#include <thread>
#include "Declarations.h"
using namespace cv;
using namespace std;
Mat dctCoefficientR = Mat(544, 960, CV_32FC1, Scalar(0.0));
Mat dctCoefficientG = Mat(544, 960, CV_32FC1, Scalar(0.0));
Mat dctCoefficientB = Mat(544, 960, CV_32FC1, Scalar(0.0));
Mat QuantizedDctCoefficientR = Mat(544, 960, CV_32SC1, Scalar(0));
Mat QuantizedDctCoefficientG = Mat(544, 960, CV_32SC1, Scalar(0));
Mat QuantizedDctCoefficientB = Mat(544, 960, CV_32SC1, Scalar(0));
Mat DeQuantizedDctCoefficientR = Mat(544, 960, CV_32FC1, Scalar(0));
Mat DeQuantizedDctCoefficientG = Mat(544, 960, CV_32FC1, Scalar(0));
Mat DeQuantizedDctCoefficientB = Mat(544, 960, CV_32FC1, Scalar(0));
vector<Mat>IDctCoefficient(3);
Mat DisplayFrame = Mat(544, 960, CV_32FC3);
Mat BlockTypeMat = Mat(68,120,CV_8UC1,Scalar(0));////this needs to be zero for every frame, improve it//
Mat BlockTypeMatQD = Mat(68, 120, CV_8UC1, Scalar(0));////this needs to be zero for every frame, improve it//
vector<Mat> FrameCatcher(3);
Mat InterMed = Mat(544, 960, 16,Scalar(0,0,0));
Mat FinalDisplayFrame = Mat(544, 960, 16, Scalar(0, 0, 0));
int main(int argc, const char *argv[])
{
	for (int i = 0; i < 3;i++) {
		IDctCoefficient[i].create(544,960,CV_32FC1);
		FrameCatcher[i].create(544, 960, CV_8UC1);
	}
	Decoder obj;//Decoder Class Object
	time_t start, end;
	time(&start);
	thread t1(&Decoder::reader, &obj, argv, ref(dctCoefficientR), ref(dctCoefficientG), ref(dctCoefficientB),ref(BlockTypeMat));
	thread t2(&Decoder::quantizer, &obj,argv, ref(dctCoefficientR), ref(dctCoefficientG), ref(dctCoefficientB),
		ref(QuantizedDctCoefficientR), ref(QuantizedDctCoefficientG), ref(QuantizedDctCoefficientB),ref(BlockTypeMat),ref(BlockTypeMatQD));
	thread t3(&Decoder::deQuantizer, &obj, argv, ref(QuantizedDctCoefficientR), ref(QuantizedDctCoefficientG), ref(QuantizedDctCoefficientB), 
		ref(DeQuantizedDctCoefficientR), ref(DeQuantizedDctCoefficientG), ref(DeQuantizedDctCoefficientB), ref(BlockTypeMat));
	thread t4(&Decoder::iDCT,&obj,argv,ref(DeQuantizedDctCoefficientR), ref(DeQuantizedDctCoefficientG), ref(DeQuantizedDctCoefficientB),ref(IDctCoefficient));
	thread t5(&Decoder::display, &obj, argv,ref(IDctCoefficient),ref(FrameCatcher),ref(InterMed),ref(FinalDisplayFrame),ref(BlockTypeMat));
    t1.join();	t2.join();	t3.join();	t4.join();	t5.join();
	time(&end);
	cout << "Total Time"<<difftime(end, start) / 60.0 << endl;
	return 0;
}





