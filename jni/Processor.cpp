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

	//Print some random text for now
	fprintf (file,"Date: 2/20/2011\n");
	fprintf (file,"Number of years in Thane: 5\n");
	fprintf (file,"Number of years in this slum: 2\n");
	fprintf (file,"Ration card (Y/N): Y\n");
	fprintf (file,"Name on voters roll (Y/N): Y\n");
	fprintf (file,"Voter ID card (Y/N): Y\n");
	fprintf (file,"Voter ID number: 4567891\n");
	fprintf (file,"Mother tongue: Hindi\n");

	//release the file pointer.
	fclose(file);

	//cvReleaseImage(&img);
	//cvReleaseImage(&warpImage);

	LOGI("Exiting ProcessForm()");
	return filename;
}

//Locate the corners of the form
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
	//cvGetAffineTransform(templatePoint, currentPoint, map);
	cvGetPerspectiveTransform(templatePoint, currentPoint, map);
	cvWarpPerspective(img, warpImg, map, CV_WARP_FILL_OUTLIERS+CV_WARP_INVERSE_MAP, cvScalar(0, 0, 0));

	//Load template of form
	/*IplImage* temp = 0;
	//Load image - 2048 x 1536
	temp=cvLoadImage("bubbleTemplate.jpg");
	if(!temp)
	{
		//Could not load the image
		printf("Could not load image");
		exit(1);
	}

	IplImage* bubbleTemplate = cvCreateImage(cvSize(temp->width, temp->height), temp->depth, 1);
	cvCvtPixToPlane(temp, bubbleTemplate, NULL, NULL, NULL);

	cvReleaseImage(&temp);

	int k=0, j=0;
	CvScalar s;

	for (j=0;j<bubbleTemplate->height;j++)
	{
		for (k=0;k<bubbleTemplate->width;k++)
		{
			s=cvGet2D(bubbleTemplate,j,k);
			if (s.val[0] != 255)
			{
				cvSet2D(warpImg,j,k,cvScalar(255,0,0));
			}
		}
	}

	cvReleaseImage(&bubbleTemplate);*/
}


