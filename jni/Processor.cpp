#include "Processor.h"
#include <sys/stat.h>
#include "log.h"
#include <string>
#define LOG_COMPONENT "BubbleProcessor"

using namespace std;

const char* const c_pszCaptureDir = "/sdcard/BubbleBot/capturedImages/";
const char* const c_pszProcessDir = "/sdcard/BubbleBot/processedImages/";
const char* const c_pszDataDir = "/sdcard/BubbleBot/processedText/";
const char* const c_pszJpg= ".jpg";
const char* const c_pszTxt= ".txt";

Processor::Processor() {
}

Processor::~Processor() {
}


// Digitize the given bubble form
char* Processor::ProcessForm(char* filename)
{
	string fullname = c_pszCaptureDir;
	fullname += filename;
	fullname += c_pszJpg;

	LOGI("Entering ProcessForm()");

	//Load image
	IplImage *img=cvLoadImage(fullname.c_str());
	if(!img)
	{
		LOGE("Image load failed");
		return NULL;
	}

	Processor p;
	CvPoint * cornerPoints = new CvPoint[4];
	cornerPoints = p.findCornerPoints(img);

	IplImage* warpImg = cvCreateImage(cvSize(img->width, img->height), img->depth, img->nChannels);
	p.warpImage(img, warpImg, cornerPoints);

	//Detect form squares
	CvPoint* lineValues = new CvPoint[5];
	lineValues = p.findLineValues(warpImg);

	//Find bubbles
	CvPoint * bubbles;
	bubbles = p.findBubbles(warpImg);
	int numBubbles = bubbles[0].x;

	//Count bubbles
	int * count = new int[5];
	count [0] = 0;
	count [1] = 0;
	count [2] = 0;
	count [3] = 0;
	count [4] = 0;

	int i=0;
	for (i=1;i<numBubbles+1;i++)
	{
		//Count Bubbles
		if ((bubbles[i].y > lineValues[0].y) && (bubbles[i].y < lineValues[1].y))
		{
			count [0] ++;
		}

		else if ((bubbles[i].y > lineValues[1].y) && (bubbles[i].y < lineValues[2].y))
		{
			count [1] ++;
		}
		else if ((bubbles[i].y > lineValues[2].y) && (bubbles[i].y < lineValues[3].y))
		{
			count [2] ++;
		}
		else if ((bubbles[i].y > lineValues[3].y) && (bubbles[i].y < lineValues[4].y))
		{
			count [3] ++;
		}
		else if ((bubbles[i].y > lineValues[4].y) && (bubbles[i].y < warpImg->height))
		{
			count [4] ++;
		}

		//Draw on image
		cvCircle(warpImg, bubbles[i], 20, cvScalar(0,0,255), 3, CV_AA, 0);
	}

	//Draw Text
	CvFont font;
	double hScale=3.0;
	double vScale=3.0;
	int    lineWidth=5;
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, hScale,vScale,0,lineWidth);

	for (i=0;i<5;i++)
	{
		int Y = lineValues[i].y + 130;
		char * number = new char[5];
		sprintf(number, "=%i", count[i]);
		cvPutText (warpImg, number, cvPoint(1500,Y), &font, cvScalar(255,0,0));
	}

	//Draw results
	cvLine( img, cornerPoints[0], cornerPoints[1], cvScalar(0,255,0), 5);
	cvLine( img, cornerPoints[1], cornerPoints[2], cvScalar(0,255,0), 5);
	cvLine( img, cornerPoints[2], cornerPoints[3], cvScalar(0,255,0), 5);
	cvLine( img, cornerPoints[3], cornerPoints[0], cvScalar(0,255,0), 5);

	cvCircle( img, cornerPoints[0], 20, cvScalar(0,255,0), 5);
	cvCircle( img, cornerPoints[1], 20, cvScalar(0,255,0), 5);
	cvCircle( img, cornerPoints[2], 20, cvScalar(0,255,0), 5);
	cvCircle( img, cornerPoints[3], 20, cvScalar(0,255,0), 5);

	//Save image
	string saveLocation = c_pszProcessDir;
	saveLocation += filename;
	saveLocation += c_pszJpg;
	cvSaveImage(saveLocation.c_str(), warpImg);

	/*open a file in text mode with write permissions.*/
	string fileLocation = c_pszDataDir;
	fileLocation += filename;
	fileLocation += c_pszTxt;

	FILE *file = fopen(fileLocation.c_str(), "wt");
	if(file==NULL)
	{
		//If unable to open the specified file display error and return.
		LOGE("Failed to save text file");
		return NULL;
	}

	//TODO: Make it print actual stuff
	//Print some random text for now
	fprintf (file,"Vaccination Report\n");
	fprintf (file,"Date: 03/07/2011\n");
	fprintf (file,"Number of years in this slum: 2\n");
	fprintf (file,"Ration card (Y/N): Y\n");
	fprintf (file,"Name on voters roll (Y/N): Y\n");
	fprintf (file,"Voter ID card (Y/N): Y\n");
	fprintf (file,"Voter ID number: 4567891\n");
	fprintf (file,"Mother tongue: Hindi\n");

	//release the file pointer.
	fclose(file);

	cvReleaseImage(&img);
	cvReleaseImage(&warpImg);

	LOGI("Exiting ProcessForm()");
	return filename;
}

CvPoint * Processor::findLineValues(IplImage* img)
{
	CvPoint* lineValues = new CvPoint[5];
	IplImage *warpImg = cvCloneImage(img);
	CvRect rect;
	rect = cvRect(1500, 0, 100, 300);
	cvSetImageROI(warpImg, rect);

	//params for Canny
	int N = 7;
	double lowThresh = 50;
	double highThresh = 300;

	IplImage* bChannel = cvCreateImage(cvGetSize(warpImg),warpImg->depth, 1);
	cvCvtPixToPlane(warpImg, bChannel, NULL, NULL, NULL);
	IplImage* out = cvCreateImage(cvGetSize(bChannel), bChannel->depth, bChannel->nChannels);
	cvCanny( bChannel, out, lowThresh*N*N, highThresh*N*N, N );

	//Find edge
	int maxWhiteCount = 0;
	int linePoint = 0;
	int j,k;
	int whiteCount;
	CvScalar s;
	for (j=0;j<out->height;j++)
	{
		whiteCount = 0;
		for (k=0;k<out->width;k++)
		{
			s=cvGet2D(out,j,k);
			if (s.val[0] == 255)
			{
				whiteCount++;
			}
		}
		if (whiteCount > maxWhiteCount)
		{
			maxWhiteCount = whiteCount;
			linePoint = j;
		}
	}

	//SEEMS TO BE A BUG HERE: MAY NEED TO FIX THIS
	linePoint = linePoint - 60;
	//////////////////////////////////////////////

	lineValues[0].x = 0;
	lineValues[0].y = linePoint;
	lineValues[1].x = 0;
	lineValues[1].y = linePoint+260;
	lineValues[2].x = 0;
	lineValues[2].y = linePoint+520;
	lineValues[3].x = 0;
	lineValues[3].y = linePoint+780;
	lineValues[4].x = 0;
	lineValues[4].y = linePoint+1040;

	cvReleaseImage(&warpImg);
	cvReleaseImage(&bChannel);
	cvReleaseImage(&out);
	return lineValues;
}

CvPoint * Processor::findBubbles(IplImage* img)
{
	list<CvPoint> pointList;

	//Prepare for thresholding
	IplImage *val = cvCreateImage(cvGetSize(img),img->depth, 1);
	IplImage *hue = cvCreateImage(cvGetSize(img),img->depth, 1);
	IplImage *sat = cvCreateImage(cvGetSize(img),img->depth, 1);

	IplImage *valT = cvCreateImage(cvGetSize(img),img->depth, 1);
	IplImage *hueT = cvCreateImage(cvGetSize(img),img->depth, 1);
	IplImage *satT = cvCreateImage(cvGetSize(img),img->depth, 1);
	IplImage *hsvT = cvCreateImage(cvGetSize(img),img->depth, 3);

	//Calculate hue, saturation and value images
	IplImage *imgHSV = cvCreateImage(cvGetSize(img),img->depth, img->nChannels);
	cvCvtColor(img,imgHSV,CV_BGR2HSV);

	//Separate into single channels
	cvCvtPixToPlane(imgHSV, hue, sat, val, NULL);

	//Thresholding  100  50  150
	cvThreshold(hue, hueT, 40, 255,CV_THRESH_BINARY);
	cvThreshold(sat, satT, 30, 255,CV_THRESH_BINARY);
	cvThreshold(val, valT, 200, 255,CV_THRESH_BINARY_INV);

	//Combine thresholds
	cvCvtPlaneToPix(satT,valT,hueT,NULL,hsvT);
	CvScalar s;
	int i,j;
	for (i=0;i<hsvT->height;i++)
	{
		for (j=0;j<hsvT->width;j++)
		{
			s=cvGet2D(hsvT,i,j);
			if(s.val[0] != 255)
			{
				cvSet2D(hsvT,i,j,cvScalar(0));
			}
			if(s.val[1] != 255)
			{
				cvSet2D(hsvT,i,j,cvScalar(0));
			}
			if(s.val[2] != 255)
			{
				cvSet2D(hsvT,i,j,cvScalar(0));
			}
		}
	}

	//Take one channel of combined thresholds (all the same anyway)
	cvCvtPixToPlane(hsvT, hue, NULL, NULL, NULL);

	//Find contours
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contour = 0;
	CvMoments moments;
	cvFindContours( hue, storage, &contour, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );

	int totalContours = 0;
	for( ; contour != 0; contour = contour->h_next )
	{
		totalContours ++;
		double area = fabs(cvContourArea(contour));

		if (area > 50)
		{
			//Get centroid of contour
			cvMoments(contour, &moments, 1);
			double xc = moments.m10/moments.m00;
			double yc = moments.m01/moments.m00;
			CvPoint p = cvPoint(xc, yc);
			pointList.push_back(p);
		}
	}

	CvPoint * bubblesFound = new CvPoint[pointList.size()+1];
	CvPoint num;
	num.x = pointList.size();
	num.y = 0;
	bubblesFound[0] = num;
	for (i=0;i<num.x;i++)
	{
		bubblesFound[i+1] = pointList.back();
		pointList.pop_back();
	}

	//Get rid of duplicates - set detected duplicates to x=5000, y=5000
	for (i=1;i<num.x+1;i++)
	{
		for (j=1;j<num.x+1 ;j++)
		{
			if (i != j)
			{
				int X = bubblesFound[i].x;
				int Y = bubblesFound[i].y;

				if ((bubblesFound[j].x == 5000) && (bubblesFound[j].y == 5000))
				{	}
				else if ((bubblesFound[j].x > X-50) && (bubblesFound[j].x < X+50))
				{
					if ((bubblesFound[j].y > Y-50) && (bubblesFound[j].y < Y+50))
					{
						//SAME BUBBLE
						bubblesFound[j].x = 5000;
						bubblesFound[j].y = 5000;
					}
				}
			}
		}
	}

	cvReleaseImage(&val);
	cvReleaseImage(&hue);
	cvReleaseImage(&sat);
	cvReleaseImage(&valT);
	cvReleaseImage(&hueT);
	cvReleaseImage(&satT);
	cvReleaseImage(&hsvT);
	cvReleaseImage(&imgHSV);

	return bubblesFound;
}


void Processor::warpImage(IplImage* img, IplImage* warpImg, CvPoint * cornerPoints)
{
	CvPoint2D32f templatePoint[4], currentPoint[4];

	templatePoint[0].x = 0;
	templatePoint[0].y = 0;
	templatePoint[1].x = img->width;;
	templatePoint[1].y = 0;
	templatePoint[2].x = img->width;
	templatePoint[2].y = img->height;
	templatePoint[3].x = 0;
	templatePoint[3].y = img->height;

	currentPoint[0].x = cornerPoints[0].x;
	currentPoint[0].y = cornerPoints[0].y;
	currentPoint[1].x = cornerPoints[1].x;
	currentPoint[1].y = cornerPoints[1].y;
	currentPoint[2].x = cornerPoints[2].x;
	currentPoint[2].y = cornerPoints[2].y;
	currentPoint[3].x = cornerPoints[3].x;
	currentPoint[3].y = cornerPoints[3].y;

	CvMat* map = cvCreateMat(3, 3, CV_32FC1);
	cvGetPerspectiveTransform(templatePoint, currentPoint, map);
	cvWarpPerspective(img, warpImg, map, CV_WARP_FILL_OUTLIERS+CV_WARP_INVERSE_MAP, cvScalar(0, 0, 0));

	//Reduce search space - cut white off form edge
	CvRect rect;
	rect = cvRect(100,50,warpImg->width-200, warpImg->height-75);
	cvSetImageROI(warpImg, rect);
	cvReleaseMat(&map);
}


CvPoint* Processor::findCornerPoints(IplImage* img)
{
	int WIDTH = 30;
	int HEIGHT = 250;
	int P1 = 400;
	int P2 = 1200;

	IplImage *img2, *img3, *bChannel, *out;

	//Define places in the image to detect form edges
	CvRect * top = new CvRect [2];
	CvRect * bottom = new CvRect [2];
	CvRect * left = new CvRect [2];
	CvRect * right = new CvRect [2];

	top[0] = cvRect(P1,0,WIDTH,HEIGHT);
	top[1] = cvRect(P2,0,WIDTH,HEIGHT);

	bottom[0] = cvRect(P1,img->height-HEIGHT,WIDTH,HEIGHT);
	bottom[1] = cvRect(P2,img->height-HEIGHT,WIDTH,HEIGHT);

	left[0] = cvRect(0,P1,HEIGHT,WIDTH);
	left[1] = cvRect(0,P2,HEIGHT,WIDTH);

	right[0] = cvRect(img->width-HEIGHT,P1,HEIGHT,WIDTH);
	right[1] = cvRect(img->width-HEIGHT,P2,HEIGHT,WIDTH);

	//params for Canny
	int N = 7;
	double lowThresh = 50;
	double highThresh = 400;

	int *topPoints = new int[2];
	int *bottomPoints = new int[2];
	int *leftPoints = new int[2];
	int *rightPoints = new int[2];

	int i=0, j=0, k=0;
	int maxWhiteCount = 0, whiteCount = 0;

	//FIND THE TOP OF THE FORM
	for (i=0;i<2;i++)
	{
		cvSetImageROI(img, top[i]);
		img2 = cvCreateImage(cvGetSize(img),img->depth, img->nChannels);
		cvCopy(img, img2, NULL);
		cvResetImageROI(img);

		img3 = cvCreateImage(cvGetSize(img2),img2->depth, img2->nChannels);
		cvSmooth( img2, img3, CV_GAUSSIAN, 1, 13);

		bChannel = cvCreateImage(cvGetSize(img2),img2->depth, 1);
		cvCvtPixToPlane(img3, bChannel, NULL, NULL, NULL);
		out = cvCreateImage(cvGetSize(bChannel), bChannel->depth, bChannel->nChannels);
		cvCanny( bChannel, out, lowThresh*N*N, highThresh*N*N, N );

		maxWhiteCount = 0;
		CvScalar s;
		for (j=0;j<out->height;j++)
		{
			whiteCount = 0;
			for (k=0;k<out->width;k++)
			{
				s=cvGet2D(out,j,k);
				if (s.val[0] == 255)
				{
					whiteCount++;
				}
			}
			if (whiteCount > maxWhiteCount)
			{
				maxWhiteCount = whiteCount;
				topPoints[i] = j;
			}
		}
	}

	//FIND THE BOTTOM OF THE FORM
	for (i=0;i<2;i++)
	{
		cvSetImageROI(img, bottom[i]);
		img2 = cvCreateImage(cvGetSize(img),img->depth, img->nChannels);
		cvCopy(img, img2, NULL);
		cvResetImageROI(img);

		img3 = cvCreateImage(cvGetSize(img2),img2->depth, img2->nChannels);
		cvSmooth( img2, img3, CV_GAUSSIAN, 1, 13);

		bChannel = cvCreateImage(cvGetSize(img2),img2->depth, 1);
		cvCvtPixToPlane(img3, bChannel, NULL, NULL, NULL);
		out = cvCreateImage(cvGetSize(bChannel), bChannel->depth, bChannel->nChannels);
		cvCanny( bChannel, out, lowThresh*N*N, highThresh*N*N, N );

		maxWhiteCount = 0;
		CvScalar s;
		for (j=0;j<out->height;j++)
		{
			whiteCount = 0;
			for (k=0;k<out->width;k++)
			{
				s=cvGet2D(out,j,k);
				if (s.val[0] == 255)
				{
					whiteCount++;
				}
			}
			if (whiteCount > maxWhiteCount)
			{
				maxWhiteCount = whiteCount;
				bottomPoints[i] = j;
			}
		}
	}

	//FIND THE LEFT OF THE FORM
	for (i=0;i<2;i++)
	{
		cvSetImageROI(img, left[i]);
		img2 = cvCreateImage(cvGetSize(img),img->depth, img->nChannels);
		cvCopy(img, img2, NULL);
		cvResetImageROI(img);

		img3 = cvCreateImage(cvGetSize(img2),img2->depth, img2->nChannels);
		cvSmooth( img2, img3, CV_GAUSSIAN, 1, 13);

		bChannel = cvCreateImage(cvGetSize(img2),img2->depth, 1);
		cvCvtPixToPlane(img3, bChannel, NULL, NULL, NULL);
		out = cvCreateImage(cvGetSize(bChannel), bChannel->depth, bChannel->nChannels);
		cvCanny( bChannel, out, lowThresh*N*N, highThresh*N*N, N );

		maxWhiteCount = 0;
		CvScalar s;
		for (j=0;j<out->width;j++)
		{
			whiteCount = 0;
			for (k=0;k<out->height;k++)
			{
				s=cvGet2D(out,k,j);
				if (s.val[0] == 255)
				{
					whiteCount++;
				}
			}
			if (whiteCount > maxWhiteCount)
			{
				maxWhiteCount = whiteCount;
				leftPoints[i] = j;
			}
		}
	}

	//FIND THE RIGHT OF THE FORM
	for (i=0;i<2;i++)
	{
		cvSetImageROI(img, right[i]);
		img2 = cvCreateImage(cvGetSize(img),img->depth, img->nChannels);
		cvCopy(img, img2, NULL);
		cvResetImageROI(img);

		img3 = cvCreateImage(cvGetSize(img2),img2->depth, img2->nChannels);
		cvSmooth( img2, img3, CV_GAUSSIAN, 1, 13);

		bChannel = cvCreateImage(cvGetSize(img2),img2->depth, 1);
		cvCvtPixToPlane(img3, bChannel, NULL, NULL, NULL);
		out = cvCreateImage(cvGetSize(bChannel), bChannel->depth, bChannel->nChannels);
		cvCanny( bChannel, out, lowThresh*N*N, highThresh*N*N, N );

		maxWhiteCount = 0;
		CvScalar s;
		for (j=0;j<out->width;j++)
		{
			whiteCount = 0;
			for (k=0;k<out->height;k++)
			{
				s=cvGet2D(out,k,j);
				if (s.val[0] == 255)
				{
					whiteCount++;
				}
			}
			if (whiteCount > maxWhiteCount)
			{
				maxWhiteCount = whiteCount;
				rightPoints[i] = j;
			}
		}
	}

	//Get equation of top line
	// y = slopeTop * x + interceptTop
	double firstPointX = P2 + (WIDTH/2);
	double firstPointY = topPoints[1];
	double secondPointX = P1 + (WIDTH/2);
	double secondPointY = topPoints[0];

	double slopeTop = (firstPointY-secondPointY)/(firstPointX-secondPointX);
	double interceptTop = secondPointY - (slopeTop * secondPointX);

	//Get equation of bottom line
	// y = slopeBottom * x + interceptBottom
	firstPointX = P2 + (WIDTH/2);
	firstPointY = img->height-HEIGHT+bottomPoints[1];
	secondPointX = P1 + (WIDTH/2);
	secondPointY = img->height-HEIGHT+bottomPoints[0];

	double slopeBottom = (firstPointY-secondPointY)/(firstPointX-secondPointX);
	double interceptBottom = secondPointY - (slopeBottom * secondPointX);

	//Get equation of left line
	// y = slopeLeft * x + interceptLeft
	firstPointY = P2 + (WIDTH/2);
	firstPointX = leftPoints[1];
	secondPointY = P1 + (WIDTH/2);
	secondPointX = leftPoints[0];

	double slopeLeft = (firstPointY-secondPointY)/(firstPointX-secondPointX);
	double interceptLeft = secondPointY - (slopeLeft * secondPointX);

	//Get equation of right line
	// y = slopeRight * x + interceptRight
	firstPointY = P2 + (WIDTH/2);
	firstPointX = img->width-HEIGHT+rightPoints[1];
	secondPointY = P1 + (WIDTH/2);
	secondPointX = img->width-HEIGHT+rightPoints[0];

	double slopeRight = (firstPointY-secondPointY)/(firstPointX-secondPointX);
	double interceptRight = secondPointY - (slopeRight * secondPointX);

	//Find where lines intersect to get corners of form
	//Top and left lines
	double topLeftCornerX = (interceptTop - interceptLeft)/(slopeLeft - slopeTop);
	double topLeftCornerY = (slopeTop * topLeftCornerX) + interceptTop;

	//Bottom and left lines
	double bottomLeftCornerX = (interceptBottom - interceptLeft)/(slopeLeft - slopeBottom);
	double bottomLeftCornerY = (slopeBottom * bottomLeftCornerX) + interceptBottom;

	//Top and right lines
	double topRightCornerX = (interceptTop - interceptRight)/(slopeRight - slopeTop);
	double topRightCornerY = (slopeTop * topRightCornerX) + interceptTop;

	//Bottom and right lines
	double bottomRightCornerX = (interceptBottom - interceptRight)/(slopeRight - slopeBottom);
	double bottomRightCornerY = (slopeBottom * bottomRightCornerX) + interceptBottom;

	//To return
	CvPoint * cornerPoints = new CvPoint[4];
	cornerPoints[0].x = topLeftCornerX;
	cornerPoints[0].y = topLeftCornerY;
	cornerPoints[1].x = topRightCornerX;
	cornerPoints[1].y = topRightCornerY;
	cornerPoints[3].x = bottomLeftCornerX;
	cornerPoints[3].y = bottomLeftCornerY;
	cornerPoints[2].x = bottomRightCornerX;
	cornerPoints[2].y = bottomRightCornerY;

	// Cleanup
	cvReleaseImage(&img2);
	cvReleaseImage(&img3);
	cvReleaseImage(&out);
	cvReleaseImage(&bChannel);

	return cornerPoints;
}

