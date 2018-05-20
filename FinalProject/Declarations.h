#ifndef Declarations_H_
#define Declarations_H_

#include <stdlib.h>
#include <stdio.h>
#include<string>
#include <iostream>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <math.h>
#include <fstream>
using namespace cv;
using namespace std;

class Decoder {

	void mouseSampler(int, int, int);
	friend void on_mouse(int ev, int x, int y, int, void* obj);

public:
	void reader(const char*[], Mat &,Mat &,Mat&,Mat &);
	void quantizer(const char*[],Mat &,Mat &,Mat&,Mat&,Mat&, Mat &, Mat&,Mat&);
	void deQuantizer(const char*[], Mat &, Mat &, Mat&, Mat&, Mat&, Mat &,Mat&);
	void iDCT(const char*[], Mat &, Mat &, Mat &, vector<Mat>&);
	
	void display(const char*[],vector<Mat> &,vector<Mat>&,Mat &,Mat &,Mat&);
	void blockNumberIdentifier_Gaze(Mat&);
	unsigned long & frameLength(const char*[],unsigned long &);
	float * dynamicMemoryAllocation1d(unsigned long);
	unsigned char * dynamicMemoryAllocation1dUC(unsigned long);










};



#endif