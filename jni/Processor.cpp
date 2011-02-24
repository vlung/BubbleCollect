#include "Processor.h"
#include <sys/stat.h>
#include "log.h"
#include <string>
#define LOG_COMPONENT "BubbleProcessor"

Processor::Processor() {
}

Processor::~Processor() {
}

// Digitize the given bubble form
char* Processor::ProcessForm(char* filename)
{
	char* captureDir = "/sdcard/BubbleBot/capturedImages/";
	char* processDir = "/sdcard/BubbleBot/processedImages/";
	char* dataDir = "/sdcard/BubbleBot/processedText/";

	char fullname[50];
	strcpy (fullname,captureDir);
	strcat (fullname,filename);
	strcat (fullname,".jpg");

	LOGI("Entering ProcessForm()");

	//Load image
	IplImage *img=cvLoadImage(fullname);
	if(!img)
	{
		LOGE("Image load failed");
		return NULL;
	}

	//Create image to use as work canvas
	IplImage *rIm = cvCreateImage(cvGetSize(img),img->depth, img->nChannels);
	cvCopy(img, rIm, NULL);

	//Prepare for thresholding
	IplImage *val = cvCreateImage(cvGetSize(rIm),rIm->depth, 1);
	IplImage *hue = cvCreateImage(cvGetSize(rIm),rIm->depth, 1);
	IplImage *sat = cvCreateImage(cvGetSize(rIm),rIm->depth, 1);

	IplImage *valT = cvCreateImage(cvGetSize(rIm),rIm->depth, 1);
	IplImage *hueT = cvCreateImage(cvGetSize(rIm),rIm->depth, 1);
	IplImage *satT = cvCreateImage(cvGetSize(rIm),rIm->depth, 1);
	IplImage *hsvT = cvCreateImage(cvGetSize(rIm),rIm->depth, 3);

	//Calculate hue, saturation and value images
	IplImage *rImHSV = cvCreateImage(cvGetSize(rIm),rIm->depth, rIm->nChannels);
	cvCvtColor(rIm,rImHSV,CV_BGR2HSV);

	//Separate into single channels
	cvCvtPixToPlane(rImHSV, hue, sat, val, NULL);

	//Thresholding  100  50  150
	cvThreshold(hue, hueT, 50, 255,CV_THRESH_BINARY);
	cvThreshold(sat, satT, 30, 255,CV_THRESH_BINARY);
	cvThreshold(val, valT, 150, 255,CV_THRESH_BINARY_INV);

	//Combine thresholds
	cvCvtPlaneToPix(hueT,satT,valT,NULL,hsvT);
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

		if (area > 0)
		{
			//Get centroid of contour
			cvMoments(contour, &moments, 1);
			double xc = moments.m10/moments.m00;
			double yc = moments.m01/moments.m00;
			CvPoint p = cvPoint(xc, yc);
			//Draw red circle on image
			cvCircle(img, p, 5, cvScalar(0,0,255), 1, CV_AA, 0);
			//Draw contour on HSV image
			cvDrawContours( hsvT , contour, cvScalar(0,0,255), cvScalar(0,0,255), -1, CV_FILLED, 8 );
		}
	}

	//Save image
	char saveLocation[50];
	strcpy (saveLocation,processDir);
	strcat (saveLocation,filename);
	strcat (saveLocation,".jpg");
	cvSaveImage(saveLocation ,img);

	/*open a file in text mode with write permissions.*/
	char fileLocation[50];
	strcpy (fileLocation,dataDir);
	strcat (fileLocation,filename);
	strcat (fileLocation,".txt");

	FILE *file = fopen(fileLocation, "wt");
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

	// Cleanup
	cvReleaseImage(&hue);
	cvReleaseImage(&sat);
	cvReleaseImage(&val);
	cvReleaseImage(&hueT);
	cvReleaseImage(&satT);
	cvReleaseImage(&valT);
	cvReleaseImage(&hsvT);
	cvReleaseImage(&rIm);
	cvReleaseImage(&rImHSV);

	LOGI("Exiting ProcessForm()");
	return filename;
}
