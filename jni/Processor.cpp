/*
 * Processor.cpp
 *
 *  Created on: Jun 13, 2010
 *      Author: ethan
 */

#include "Processor.h"
#include <sys/stat.h>

using namespace cv;

Processor::Processor() {
}

Processor::~Processor() {
}

// How far are the corner markers away from the edge of screen?
const int c_cornerMargin = 20;
// How long should the markers be?
const int c_cornerLength = c_cornerMargin + 30;
// How far do we allow a point to be distanced from the corner
// markers to consider it touches the markers
const int c_cornerRange = 20;
// Threshold for proper form area
const int c_minOutlineArea = 200000;
// Number of rows of the camera image
static int g_maxRows = -1;

// Calculate the angle between 2 lines
float angle(Point pt1, Point pt2, Point pt0) {
	float dx1 = pt1.x - pt0.x;
	float dy1 = pt1.y - pt0.y;
	float dx2 = pt2.x - pt0.x;
	float dy2 = pt2.y - pt0.y;
	return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2
			+ dy2 * dy2) + 1e-10);
}

// Evaluate whether a point is close to the corner markers
bool isNear(Point p) {
	return (abs(p.x - c_cornerMargin) < c_cornerRange && (abs(p.y - c_cornerMargin)
			< c_cornerRange || abs(p.y - (g_maxRows - c_cornerMargin)) < c_cornerRange));
}

// Detect whether the largest rectangle of an image is well aligned
// with the corner markers on screen
void Processor::DetectOutline(int idx, image_pool *pool, double thres1,
		double thres2) {
	Mat img = pool->getImage(idx), imgCanny;

	// Leave if there is no image
	if (img.empty())
		return;

	vector < Point > approx;
	vector < Point > maxRect;
	vector < vector<Point> > contours;
	float maxContourArea = 10000;
	bool fOutlineDetected = false;
	bool fMatchCorners = false;

	// Initialize the number of rows (height) of the image
	if (g_maxRows == -1)
		g_maxRows = img.rows;

	// Do Canny transformation on the image
	Canny(pool->getGrey(idx), imgCanny, thres1, thres2, 3);

	// Find all external contours of the image
	findContours(imgCanny, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	// Iterate through all detected contours
	for (size_t i = 0; i < contours.size(); i++) {
		int nMatchCorners = 0;

		// Approximate the perimmeter of the contours
		approxPolyDP(Mat(contours[i]), approx,
				arcLength(Mat(contours[i]), true) * 0.01, true);

		// Check whether the perimmeter forms a quadrilateral
		// Note: absolute value of an area is used because
		// area may be positive or negative - in accordance with the
		// contour orientation
		if (approx.size() == 4 && isContourConvex(Mat(approx))) {
			float area = fabs(contourArea(Mat(approx)));

			// Skip if its area is less that the maximum area
			// that we have detected.
			if (area > maxContourArea) {
				float maxCosine = 0;

				// Find the maximum cosine of the angle between joint edges
				// and check if the quadrilateral touches the corner markers
				for (int j = 2; j < 6; ++j) {
					float cosine = fabs(angle(approx[j % 4], approx[j - 2],
							approx[j - 1]));
					maxCosine = MAX(maxCosine, cosine);
					if (isNear(approx[j % 4]))
						++nMatchCorners;
				}

				// If cosines of all angles are small
				// (ie. all angles are between 75-105 degree),
				// this quadrilateral becomes the largest one
				// we have detected so far.
				if (maxCosine < 0.25) {
					maxRect = approx;
					maxContourArea = area;
					if (nMatchCorners == 2) {
						fMatchCorners = true;
						if (area > c_minOutlineArea)
							fOutlineDetected = true;
					}
				}
			}
		}
	}

	// Draw corner markers
	const Point* p = &maxRect[0];
	int n = (int) maxRect.size();
	Scalar guideColor = fOutlineDetected ? Scalar(0, 255, 0)
			: Scalar(255, 0, 0);
	polylines(img, &p, &n, 1, true, guideColor, 3, CV_AA);
	line(img, Point(c_cornerMargin, c_cornerMargin), Point(c_cornerLength,
			c_cornerMargin), guideColor, 5, CV_AA);
	line(img, Point(c_cornerMargin, c_cornerMargin), Point(c_cornerMargin,
			c_cornerLength), guideColor, 3, CV_AA);
	line(img, Point(c_cornerMargin, img.rows - c_cornerMargin), Point(
			c_cornerLength, img.rows - c_cornerMargin), guideColor, 5, CV_AA);
	line(img, Point(c_cornerMargin, img.rows - c_cornerMargin), Point(
			c_cornerMargin, img.rows - c_cornerLength), guideColor, 3, CV_AA);

	// Draw Status text
	char txt[100] = "";
	Scalar color = Scalar(255,0,0);
	if (maxContourArea > 10000 && maxContourArea < 200000) {
		sprintf(txt, "MOVE AWAY FROM FORM");
	}
	if (fMatchCorners) {
		if (fOutlineDetected) {
			sprintf(txt, "Correct Position");
			color = Scalar(0, 255, 0);
		} else {
			sprintf(txt, "FORM AREA TOO SMALL, PLEASE REALIGN FORM.");
		}
	}
	if (txt[0])
		drawText(idx, pool, txt, -2, color, 1.6);

	// Draw debug text
	sprintf(txt, "Area: %0.2f  %0.2f", maxContourArea, thres1);
	drawText(idx, pool, txt, -1);
}

// Draw text on screen
void Processor::drawText(int i, image_pool* pool, const char* ctext, int row,
		const cv::Scalar &color, double fontScale, double thickness) {
	int fontFace = FONT_HERSHEY_COMPLEX_SMALL;
	Mat img = pool->getImage(i);
	string text = ctext;
	Size textSize =	getTextSize(text, fontFace, fontScale, thickness, NULL);

	// Center the text
	Point textOrg((img.cols - textSize.width) / 2, (row > 0) ? (textSize.height
			* row) : (img.rows + (row * textSize.height)));

	// Draw a black border for the text and then draw the text
	putText(img, text, textOrg, fontFace, fontScale, Scalar::all(0), thickness
			* 10);
	putText(img, text, textOrg, fontFace, fontScale, color, thickness, CV_AA);
}
