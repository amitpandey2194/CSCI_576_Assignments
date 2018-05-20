#include <time.h>
#include "Declarations.h"
#include "Semaphore.h"
#include "BinarySemaphore.h"
using namespace cv;
using namespace std;
mutex m_;
Point coordinates;
////////////////////////////////////////////////
int readerFrame = 1, quant_Counter = 0, deQuant_Counter = 0, idct_Counter=0,display_Counter=0;
///////////////////////////////////////////////
Semaphore rq_Empty(120 * 68); Semaphore rq_Occupied(0);
Semaphore qdq_Empty(120 * 68);Semaphore qdq_Occupied(0);
Semaphore dq_Idct_Empty(120 * 68);Semaphore dq_Idct_Occupied(0);
BinSem dis_Idct(0); BinSem dis_Reader_Bin_Sem(0); BinSem idct_Dis_Bin_Sem(0);
//////////////////For Pause Play Functionality/////////////////////////////////
BinSem dis_Read_P_P(0); BinSem dis_Quant_P_P(0); BinSem dis_De_Quant_P_P(0); BinSem dis_Idct_P_P(0);
bool P_P = true;//P_P is an abbrevation for play and pause//
//////////////////////////////////////////////////////////////////////////////
unsigned long len = 0; int noOfFrames = 363;
//this function is pointing other member function
void on_mouse(int ev, int x, int y, int, void* obj)
{

	Decoder* app = static_cast<Decoder*>(obj);
	if (app)
		app->mouseSampler(ev, x, y);
}
void Decoder::mouseSampler(int event, int x, int y) {

	if (event == EVENT_MOUSEMOVE) {
		coordinates = Point(x, y);
		//cout << "x:" << coordinates.x << "y:" << coordinates.y << endl;
	}
}
void Decoder::reader(const char*argv[], Mat & dctCoefficientR, Mat & dctCoefficientG, Mat & dctCoefficientB, Mat & blockTypeMat) {
	FILE *file; String arg;
	errno_t err; time_t start, end;
	int height = 544; int width = 960; unsigned long len = frameLength(argv,len); 
	unsigned long arrayLength = len / 4;//TODO: it can be int 16 as well be mindful about that
	float * video= dynamicMemoryAllocation1d(arrayLength);
	int totalBlocks = 120 * 68;////(number of blocks per frame);
	int counter=0, block_Type_X=0, block_Type_Y = 0;int block_Type_Stream=0; 
	time(&start);
	for (readerFrame = 1; readerFrame <= noOfFrames;) {//starting with 1 because the name of the frames start from 1.dct
		arg = argv[1] + to_string(readerFrame) + String(".dct");
		if ((err = fopen_s(&file, arg.c_str(), "rb") != 0)) {
			cout << "Cannot open file: " << arg << endl;
			exit(1);
		}
		fread(video, sizeof(float), arrayLength, file);
		block_Type_Y = 0; counter = 0;
			if (atoi(argv[4]) == 1) {
				dis_Reader_Bin_Sem.lock();//unlocking will be done by display and that will be after every Frame
			}
		for (int h = 0; h <= height - 8; h += 8) {////Height and width incrementing by 8
				block_Type_X = 0;
				for (int w = 0; w <= width - 8; w += 8) {
					rq_Empty.lock();////Check for Activation From Quantizer Thread and Decrement Empty Locations
					block_Type_Stream= int(video[counter]);////reading the block type from the file stream
					counter++;//skipping one location as there is our Block Type Info
						for (int m = 0; m < 8; m++) {///////8*8 window loop//////
							for (int n = 0; n < 8; n++) {
								dctCoefficientR.at<float>(h + m, n + w)= video[counter];
								dctCoefficientG.at<float>(h + m, n + w) = video[counter+64];
								dctCoefficientB.at<float>(h + m, n + w) = video[counter+(64*2)];
								counter++;//reading counter for dctcoeff rgb from one D file, it is also taking care of the Block Type Location on the last iteration///
							}
						}
						counter += 128;
					//////one block reading finished//////
					if (atoi(argv[4]) == 1) {//// if gaze is ON
						if (!(blockTypeMat.at<uchar>(block_Type_Y, block_Type_X) == 2)) {
							blockTypeMat.at<uchar>(block_Type_Y, block_Type_X) = uchar(block_Type_Stream);
						}
					}
					else {//if gaze is OFF
						blockTypeMat.at<uchar>(block_Type_Y, block_Type_X) = uchar(block_Type_Stream);//then no BlockTypeChange
					}
					
					rq_Occupied.unlock();//one block done Waking Up the Quantizer Thread
					block_Type_X++;//Block Column counter		
				}
				block_Type_Y++;//Block Row counter
			}//one frame finish here
		fclose(file);
		m_.lock();
		//cout << "Frame no at Reader:" << reader << endl;
		m_.unlock();
		dis_Read_P_P.lock();//Waiting For Display To Unlock The Reader Thread//
		if (P_P) {
			readerFrame++;//Only if After Verifying it From Display Thread That Pause is Not Pressed We will increment the Frame Count
		}
		
		
	}//All Frames Read//
	delete[] video;//deallocating the Memory
	time(&end);
	cout << "Reader's Job Done in:"<<difftime(end, start) / 60.0 <<" minutes"<< endl;
};
void Decoder::quantizer(const char*argv[],Mat & dctCoefficientR, Mat & dctCoefficientG, Mat & dctCoefficientB,
				Mat & quantizedDctCoeffR, Mat & quantizedDctCoeffG, Mat & quantizedDctCoeffB, Mat & blockTypeMat,Mat &blockTypeMatDQ) {
	int height = 544; int width = 960;
	unsigned char quant[8][8] =
	{ { 16,11,10,16,24,40,51,61 },
	{ 12,12,14,19,26,58,60,55 },
	{ 14,13,16,24,40,57,69,56 },
	{ 14,17,22,29,51,87,80,62 },
	{ 18,22,37,56,68,109,103,77 },
	{ 24,35,55,64,81,104,113,92 },
	{ 49,64,78,87,103,121,120,101 },
	{ 72,92,95,98,112,100,103,99 } };
	int n1 = atoi(argv[2]);
	int n2 = atoi(argv[3]); int mainQuant=0;
	int totalBlocks = 120 * 68 * noOfFrames;////(number of blocks per frame)*(number of frames);
	int counter=0, block_Type_X=0, block_Type_Y = 0; 
	time_t start, end;
	time(&start);
for (quant_Counter = 0; quant_Counter < noOfFrames;){//this loop will run noOfFrames times
	
	block_Type_Y = 0;
	//The First Two Loops Are Running For 120*68 times/////
	for (int i = 0; i <= height - 8; i += 8) {
		block_Type_X = 0;
		for (int j = 0; j <= width - 8; j += 8) {
			rq_Occupied.lock();////Quantizer thread wake up by Reader////
			qdq_Empty.lock();////Dequantizer will Unlock This Lock//////
			/////////Here taking care of the foreground and Background Quantization Factor/////////////////////////
			if (blockTypeMat.at<uchar>(block_Type_Y, block_Type_X) == 1) { mainQuant = n1; }
			else if(blockTypeMat.at<uchar>(block_Type_Y, block_Type_X) == 0) {mainQuant = n2; }
			else { mainQuant = 1; }
			//////////////////////////////////////////////////////////////////////////////////////////////////////
			if (atoi(argv[4])==1 && int(blockTypeMat.at<uchar>(block_Type_Y, block_Type_X)) == 2) {
				//if Gaze is ON and also The Mouse Sampling causes the blockType to change to 2 which does not require QUANTIZATION 
					for (int l = 0; l < 8; l++) {
						for (int m = 0; m < 8; m++) {
							quantizedDctCoeffR.at<int>(i + l, j + m) = int(dctCoefficientR.at<float>(i + l, j + m));
							quantizedDctCoeffG.at<int>(i + l, j + m) = int(dctCoefficientG.at<float>(i + l, j + m));
							quantizedDctCoeffB.at<int>(i + l, j + m) = int(dctCoefficientB.at<float>(i + l, j + m));
						}
					}
			}
			else {////else quantize it based on Foreground or Background
					for (int l = 0; l < 8; l++) {
						for (int m = 0; m < 8; m++) {
							quantizedDctCoeffR.at<int>(i + l, j + m) = round(dctCoefficientR.at<float>(i + l, j + m) / ((mainQuant)*quant[l][m]));
							quantizedDctCoeffG.at<int>(i + l, j + m) = round(dctCoefficientG.at<float>(i + l, j + m) / ((mainQuant)*quant[l][m]));
							quantizedDctCoeffB.at<int>(i + l, j + m) = round(dctCoefficientB.at<float>(i + l, j + m) / ((mainQuant)*quant[l][m]));
						}
					}
			}
			//blockTypeMatDQ = blockTypeMat.clone();//Be Mindful about this operation
			//////one block reading finished//////
			block_Type_X++;
			qdq_Occupied.unlock();
			rq_Empty.unlock();
		}
		block_Type_Y++;//Block Row Counter
	}
	m_.lock();
	//cout << "Frame no at Quantizer:" << quant_Counter << endl;
	m_.unlock();
	dis_Quant_P_P.lock(); // Waiting For Display To Unlock The Quantizer Thread//
	if (P_P) {
		quant_Counter++;//Only if After Verifying it From Display Thread That Pause is Not Pressed We will increment the Frame Count
	}
}	
time(&end);
cout << "Quantizer's Job Done in:" << difftime(end, start) / 60.0 << " minutes" << endl;
		
};
void Decoder::deQuantizer(const char*argv[], Mat& quantizedDctCoeffR, Mat & quantizedDctCoeffG, Mat& quantizedDctCoeffB,
							Mat & dequantizedDctCoeffR, Mat & dequantizedDctCoeffG, Mat & dequantizedDctCoeffB,
												Mat &blockTypeMatQD){
	int Height = 544; int Width = 960;
	unsigned char deQuant[8][8] =
	{ { 16,11,10,16,24,40,51,61 },
	{ 12,12,14,19,26,58,60,55 },
	{ 14,13,16,24,40,57,69,56 },
	{ 14,17,22,29,51,87,80,62 },
	{ 18,22,37,56,68,109,103,77 },
	{ 24,35,55,64,81,104,113,92 },
	{ 49,64,78,87,103,121,120,101 },
	{ 72,92,95,98,112,100,103,99 } };
	int n1 = atoi(argv[2]);
	int n2 = atoi(argv[3]); int mainQuant=0;
	int counter=0, block_Type_X=0, block_Type_Y = 0; 
	time_t start, end;
	time(&start);
	for (deQuant_Counter = 0;deQuant_Counter <noOfFrames;) {//this loop will run noOfFrames times
		 block_Type_Y = 0;
		for (int i = 0; i <= Height - 8; i += 8) {
			block_Type_X = 0;
			for (int j = 0; j <= Width - 8; j += 8) {
				qdq_Occupied.lock();////deQuantizer thread wake up by Quantizer////
				dq_Idct_Empty.lock();////IDCT will Unlock This Lock//////
				/////////Here taking care of the foreground and Background deQuantization Factor/////////////////////////
				if (blockTypeMatQD.at<uchar>(block_Type_Y, block_Type_X) == 1) { mainQuant = n1; }
				else if (blockTypeMatQD.at<uchar>(block_Type_Y, block_Type_X) == 0) { mainQuant = n2; }
				else { mainQuant = 1; }
				//////////////////////////////////////////////////////////////////////////////////////////////////////
				if (atoi(argv[4]) == 1 && int(blockTypeMatQD.at<uchar>(block_Type_Y, block_Type_X)) == 2) {
					//if Gaze is ON and also The Mouse Sampling causes the blockType to change to 2 which does not require deQuantIZATION 
						for (int l = 0; l < 8; l++) {
							for (int m = 0; m < 8; m++) {
								dequantizedDctCoeffR.at<float>(i + l, j + m) = float(quantizedDctCoeffR.at<int>(i + l, j + m));
								dequantizedDctCoeffG.at<float>(i + l, j + m) = float(quantizedDctCoeffG.at<int>(i + l, j + m));
								dequantizedDctCoeffB.at<float>(i + l, j + m) = float(quantizedDctCoeffB.at<int>(i + l, j + m));
							}
						}
				}
				else {////else DeQuantize it based on Foreground or Background
					for (int l = 0; l < 8; l++) {
							for (int m = 0; m < 8; m++) {
								dequantizedDctCoeffR.at<float>(i + l, j + m) = float(quantizedDctCoeffR.at<int>(i + l, j + m)*(mainQuant)*deQuant[l][m]);
								dequantizedDctCoeffG.at<float>(i + l, j + m) = float(quantizedDctCoeffG.at<int>(i + l, j + m)*(mainQuant)*deQuant[l][m]);
								dequantizedDctCoeffB.at<float>(i + l, j + m) = float(quantizedDctCoeffB.at<int>(i + l, j + m)*(mainQuant)*deQuant[l][m]);

							}
						}
					}
		        //////one block reading finished//////
				block_Type_X++;
				dq_Idct_Occupied.unlock();//again revisit just to check the order//
				qdq_Empty.unlock();
			}
			//cout << endl;
			block_Type_Y++;//Block Row Counter
		}
		m_.lock();
		//cout << "Frame no at DeQuantizer:" << deQuant_Counter << endl;
		m_.unlock();
		dis_De_Quant_P_P.lock(); // Waiting For Display To Unlock The DeQuantizer Thread//
		if (P_P) {
			deQuant_Counter++;//Only if After Verifying it From Display Thread That Pause is Not Pressed We will increment the Frame Count
		}
		
	}
	time(&end);
	cout << "DeQuantizer's Job Done in:" << difftime(end, start) / 60.0 << " minutes" << endl;
}
void Decoder::iDCT(const char*argv[],Mat & dequantizedDctCoeffR, Mat & dequantizedDctCoeffG, Mat & dequantizedDctCoeffB,
									vector<Mat> & IDctCoefficient) {
	int height = 544; int width = 960;
	vector<Mat> idctPlanes(3); vector<Mat> idctBlock(3); int count = 0;
	time_t start, end;
	time(&start);
	for ( idct_Counter = 0; idct_Counter < noOfFrames;) {

		if ((count != 0)) {
			idct_Dis_Bin_Sem.lock();//It will Wait for the indication from the Display that It Has Displayed The Frame Now You Can Edit It//
			count--;
		}
		for (int h = 0; h <= height - 8; h += 8) {
			for (int w = 0; w <= width - 8; w += 8) {
				dq_Idct_Occupied.lock();//The IDCT thread will be Woken Up by the DeQuantizer Thread/////

				idctPlanes[0] = dequantizedDctCoeffR(Rect(w, h, 8, 8));
				idctPlanes[1] = dequantizedDctCoeffG(Rect(w, h, 8, 8));
				idctPlanes[2] = dequantizedDctCoeffB(Rect(w, h, 8, 8));
				for (int k = 0; k < 3; k++) {
					idct(idctPlanes[k], idctBlock[k]);
					idctBlock[k].copyTo(IDctCoefficient[k](Rect(w, h, 8, 8)));//insertion of the idct block in the frame to be displayed//*/
				}
				////////one block separated//////////
				dq_Idct_Empty.unlock();//Indicate the DeQuantizer Thread That I Have Read the Block///	
			}
		}
		m_.lock();
		//cout << "Frame no at IDCT:" << idct_Counter << endl;
		m_.unlock();
		dis_Idct.unlock();////IDCT of one Frame is Completed Here, Lets Now Call the Display Thread///
		count++;
		dis_Idct_P_P.lock();
		if (P_P) {
			idct_Counter++;//Only if After Verifying it From Display Thread That Pause is Not Pressed We will increment the Frame Count
		}
		//cout << "Frame:"<<i<<"IDCT Done"<< endl;			
	}
	time(&end);
	cout << "IDCT's Job Done in:" << difftime(end, start) / 60.0 << " minutes" << endl;
}
//////////////////////////Display Function//////////////////////////////////////////
void Decoder::display(const char*argv[],vector<Mat> & Frame,vector<Mat> &FrameCatcher,Mat &InterMed,Mat &FinalDisplayFrame,Mat &BlockMatType) {
	namedWindow("Frames", 1); int x = 0; int y = 0; moveWindow("Frames", x, y);//Display parameters
	if (atoi(argv[4]) == 1) {
		setMouseCallback("Frames", on_mouse, this);
		imshow("Frames", FinalDisplayFrame);//Dummy First Frame
		blockNumberIdentifier_Gaze(BlockMatType);// Editing the Block Type//
		dis_Reader_Bin_Sem.unlock();/////activating the Reader Thread
		waitKey(30);
	}//// if gaze based is ON then only start sampling*/	
	int count = 0;
	for (display_Counter = 0; display_Counter < noOfFrames; ) {
		dis_Idct.lock();//Display Will Get Triggered Only When IDCT Done With One Complete Frame
		Frame[0].convertTo(FrameCatcher[0],CV_8UC1);
		Frame[1].convertTo(FrameCatcher[1], CV_8UC1);
		Frame[2].convertTo(FrameCatcher[2], CV_8UC1);
		merge(FrameCatcher, InterMed);
		cvtColor(InterMed, FinalDisplayFrame, CV_RGB2BGR);
		imshow("Frames", FinalDisplayFrame);
		if (waitKey(50) == 27) //wait for 'esc' key press for 30 ms. If 'esc' key is pressed, break loop
		{//Still Not Sure How Should I Make My Imshow Wait
			cout << "esc key is pressed by user" << endl;
			break;
		}
		if (atoi(argv[4]) == 1) {
			BlockMatType.setTo(Scalar(0));//setting the Block Type Everytime to Zero
			blockNumberIdentifier_Gaze(BlockMatType);// Editing the Block Type//
			dis_Reader_Bin_Sem.unlock();/////activating the Reader Thread
		}
		if (waitKey(1) == 'p') { //press P to pause video
			cout << "Pausing Video" << endl;
			P_P = false;
			dis_Read_P_P.unlock(); dis_Quant_P_P.unlock(); dis_De_Quant_P_P.unlock(); dis_Idct_P_P.unlock();
		}
		else if (waitKey(1) == 'g') {
			cout << "Resuming Video" << endl;
			P_P = true;
			dis_Read_P_P.unlock(); dis_Quant_P_P.unlock(); dis_De_Quant_P_P.unlock(); dis_Idct_P_P.unlock();
		}
		else {
			dis_Read_P_P.unlock(); dis_Quant_P_P.unlock(); dis_De_Quant_P_P.unlock(); dis_Idct_P_P.unlock();
		}
		idct_Dis_Bin_Sem.unlock();//waking up the IDCT thread, indicating that now you can Start Editing the Frame As I have Displayed It.
		m_.lock();
		//cout << "Frame no at Display:" << display_Counter << endl;
		m_.unlock();
		if (P_P) {
			display_Counter++;//Only if After Verifying it From Display Thread That Pause is Not Pressed We will increment the Frame Count
		}
		
	}
}
////////////////////Display Function End////////////////////////////////////////////////////////////
void Decoder::blockNumberIdentifier_Gaze(Mat & BlockMatType) {
	int BlockNumber_x=0, BlockNumber_y = 0;
	int BlockNumStart_X=0, BlockNumStart_Y=0, BlockNumEnd_X=0, BlockNumEnd_Y = 0;
	//////Identification of the center Block to which the x,y belongs to//////
	BlockNumber_x = coordinates.x / 8; BlockNumber_y = coordinates.y / 8;
	//cout << BlockNumber_x << endl;
	//cout << BlockNumber_y << endl;
	/////////For Loop Initializer/////////////////
	BlockNumStart_X = BlockNumber_x - 3;
	BlockNumEnd_X = BlockNumber_x + 4;
	BlockNumStart_Y = BlockNumber_y - 3;
	BlockNumEnd_Y = BlockNumber_y + 4;
	////////////Taking Care Of the Boundary Condition////////////////////
	if (!(BlockNumStart_X >= 0)) {
		BlockNumStart_X = 0;
	}
	if (!(BlockNumStart_Y >= 0)) {
		BlockNumStart_Y = 0;
	}
	if (!(BlockNumEnd_X <= 119)) {
		BlockNumEnd_X = 119;
	}
	if (!(BlockNumEnd_Y <= 67)) {
		BlockNumEnd_Y = 67;
	}
	for (int bI = BlockNumStart_Y; bI <= BlockNumEnd_Y; bI++) {
		for (int bJ = BlockNumStart_X; bJ <= BlockNumEnd_X; bJ++) {
			BlockMatType.at<uchar>(bI, bJ) = 2;
		}
	}
	//waitKey(0);//to be deleted

}
unsigned char* Decoder::dynamicMemoryAllocation1dUC(unsigned long len) {
	unsigned char * ImagePointer = new unsigned char[len];
	for (unsigned long i = 0; i<len; i++) {
		ImagePointer[i] = 0;
	}
	return ImagePointer;

}
unsigned long & Decoder::frameLength(const char*argv[],unsigned long &len) {
	FILE *file; String arg; errno_t err;
	arg = argv[1] + to_string(1) + String(".dct");//linking just the first frame and extracting the len from there
	if ((err = fopen_s(&file, arg.c_str(), "rb") != 0)) {
		cout << "Cannot open file: " << arg << endl;
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	len = (unsigned long)ftell(file);//this need not be edited again//
	fseek(file, 0, SEEK_SET);
	fclose(file);
	return len;
}
float* Decoder::dynamicMemoryAllocation1d(unsigned long len) {
	float * ImagePointer = new float[len];
	return ImagePointer;
}