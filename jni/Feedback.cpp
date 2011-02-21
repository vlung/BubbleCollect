#include "Feedback.h"
#include <sys/stat.h>
#include "log.h"
#define LOG_COMPONENT "BubbleFeedback"

using namespace cv;

Feedback::Feedback() {
}

Feedback::~Feedback() {
}

// How far do we allow a point to be away from the corner
// markers to consider it touches the template
const int c_cornerRange = 50;

// Specify the size of the form template
const int c_templateHeight = 440;
const int c_templateWidth = c_templateHeight / 8.5 * 11;

const int c_borderMargin = 10;
const int c_borderThreshold = 50;

const char* const c_pszCorrectPos = "Correct Position";
const char* const c_pszRotateLeft = "ROTATE LEFT";
const char* const c_pszRotateRight = "ROTATE RIGHT";
const char* const c_pszMoveCloser = "MOVE TOWARDS YOU";
const char* const c_pszMoveAway = "MOVE AWAY FROM FORM";

enum Side {
	None = -1, Top, Right, Bottom, Left
};

// Threshold for proper form area
const int c_minOutlineArea = 180000;
// Number of rows of the camera image
static int g_maxRows = -1, g_maxCols = -1;
static Point g_ptUpperLeft, g_ptLowerRight;
long g_nAcross, g_nTopLeft, g_nTopRight, g_nBottomLeft, g_nBottomRight;
bool g_fMoveForm;

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
bool isNear(const Point &p) {
	return ((abs(p.x - g_ptUpperLeft.x) < c_cornerRange && abs(p.y
			- g_ptUpperLeft.y) < c_cornerRange) || (abs(p.x - g_ptLowerRight.x)
			< c_cornerRange && abs(p.y - g_ptLowerRight.y) < c_cornerRange));
}

Side checkBorder(const Point &p) {
	Side result = None;

	if (p.y < c_borderMargin)
		result = Top;
	else if (p.y > (g_maxRows - c_borderMargin))
		result = Bottom;
	else if (p.x < c_borderMargin)
		result = Left;
	else if (p.x > (g_maxCols - c_borderMargin))
		result = Right;
	return result;
}

void checkContour(Mat &mat, const vector<Point> &contour) {
	if (g_fMoveForm)
		return;
	if (contour.size() == 2) {
		Side side1 = checkBorder(contour[0]);
		Side side2 = checkBorder(contour[1]);
		if (side1 != None && side2 != None) {
			long len = (int) arcLength(Mat(contour), false);

			if (len < c_borderThreshold)
				return;

			if ((side1 % 2) == (side2 % 2)) {
				g_nAcross += len;
				const Point* p = &contour[0];
				int n = contour.size();
				polylines(mat, &p, &n, 1, false, Scalar(255, 0, 0), 3);
			} else {
				Side sideTopBottom = (side1 % 2 == 0) ? side1 : side2;
				Side sideLeftRight = (side1 % 2 == 1) ? side1 : side2;
				const Point* p = &contour[0];
				int n = contour.size();
				polylines(mat, &p, &n, 1, false, Scalar(255, 0, 0), 3);
				if (sideTopBottom == Top) {
					if (sideLeftRight == Right) {
						g_nTopRight = max(g_nTopRight, len);
					} else {
						g_nTopLeft = max(g_nTopLeft, len);
					}
				} else {
					if (sideLeftRight == Right) {
						g_nBottomRight = max(g_nBottomRight, len);
					} else {
						g_nBottomLeft = max(g_nBottomLeft, len);
					}
				}
			}
		}
	} else if (contour.size() == 3 || contour.size() == 4) {
		int len = (int) arcLength(Mat(contour), false);
		if (len < c_borderThreshold)
			return;

		Side side1 = checkBorder(contour[0]);
		Side side2 = checkBorder(contour[contour.size() - 1]);
		if (side1 != None && side2 != None) {
			g_fMoveForm = true;

			const Point* p = &contour[0];
			int n = contour.size();
			polylines(mat, &p, &n, 1, false, Scalar(0, 0, 255), 3);
		}
	}
}

// Detect whether the largest rectangle of an image is well aligned
// with the corner markers on screen
int Feedback::DetectOutline(int idx, image_pool *pool, double thres1,
		double thres2) {
	Mat img = pool->getImage(idx), imgCanny;
	int result = 0;

	LOGI("Entering DetectOutline");

	// Leave if there is no image
	if (img.empty()) {
		LOGE("Failed to get image");
		return result;
	}

	vector < Point > approx;
	vector < Point > maxRect;
	vector < vector<Point> > contours;
	vector < vector<Point> > borderContours;
	vector < Vec2f > lines;
	float maxContourArea = 10000;
	bool fOutlineDetected = false;

	// Initialize the number of rows (height) of the image
	if (g_maxRows == -1) {
		g_maxRows = img.rows;
		g_maxCols = img.cols;
		g_ptUpperLeft = Point((img.cols - c_templateWidth) / 2, (g_maxRows
				- c_templateHeight) / 2);
		g_ptLowerRight = Point(img.cols - g_ptUpperLeft.x, img.rows
				- g_ptUpperLeft.y);
	}

	g_nAcross = g_nTopLeft = g_nTopRight = g_nBottomLeft = g_nBottomRight = 0;
	g_fMoveForm = false;

	// Do Canny transformation on the image
	Canny(pool->getGrey(idx), imgCanny, thres1, thres2, 3);

	// Find all external contours of the image
	findContours(imgCanny, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	// Do Canny transformation on the image
	Canny(pool->getGrey(idx), imgCanny, thres1 * 3, thres2 * 3, 3);

	HoughLines(imgCanny, lines, 1, CV_PI/180, 300);
    for( size_t i = 0; i < lines.size(); i++ )
    {
        float rho = lines[i][0];
        float theta = lines[i][1];
        double a = cos(theta), b = sin(theta);
        double x0 = a*rho, y0 = b*rho;
        Point pt1(cvRound(x0 + 1000*(-b)),
                  cvRound(y0 + 1000*(a)));
        Point pt2(cvRound(x0 - 1000*(-b)),
                  cvRound(y0 - 1000*(a)));
        line( img, pt1, pt2, Scalar::all(255), 3, 8 );
    }

	// Iterate through all detected contours
	for (size_t i = 0; i < contours.size(); ++i) {
		int nMatchCorners = 0;

		// Approximate the perimeter of the contours
		approxPolyDP(Mat(contours[i]), approx,
				arcLength(Mat(contours[i]), true) * 0.01, true);

		if (approx.size() <= 4) {
			checkContour(img, approx);
		}

		// Check whether the perimeter forms a quadrilateral
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
					if (nMatchCorners == 2 && area > c_minOutlineArea) {
						fOutlineDetected = true;
						result = 1;
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
	rectangle(img, g_ptUpperLeft, g_ptLowerRight, guideColor, 1, CV_AA);

	// Draw Status text
	const char *pszMsg = NULL;
	Scalar color = Scalar(255, 0, 0);
	if (fOutlineDetected) {
		pszMsg = c_pszCorrectPos;
		color = Scalar(0, 255, 0);
	} else {
		long cornerLen = g_nTopLeft + g_nTopRight + g_nBottomLeft
				+ g_nBottomRight;

		if (g_fMoveForm || g_nAcross > cornerLen) {
			pszMsg = c_pszMoveCloser;
		} else if (g_nTopLeft + g_nBottomRight > 0 && g_nTopRight
				+ g_nBottomLeft > 0) {
			if (g_nTopLeft + g_nBottomRight > g_nTopRight + g_nBottomLeft) {
				pszMsg = c_pszRotateLeft;
			} else {
				pszMsg = c_pszRotateRight;
			}
		} else if (maxContourArea > 10000 && maxContourArea < 150000) {
			pszMsg = c_pszMoveAway;
		}
	}

	if (pszMsg != NULL)
		drawText(idx, pool, pszMsg, -2, color, 1.6);

	// Draw debug text
	char txt[100];
	sprintf(txt, "A%d C%.2f N%d %.2f %.2f", g_nAcross, thres1, lines.size(),
			lines.size() > 0 ? lines[0][0] : -1.0F,
			lines.size() > 0 ? lines[0][1] : -1.0f);
	drawText(idx, pool, txt, -1);

	LOGI("Exiting DetectOutline");

	return result;
}

// Draw text on screen
void Feedback::drawText(int i, image_pool* pool, const char* ctext, int row,
		const cv::Scalar &color, double fontScale, double thickness) {
	int fontFace = FONT_HERSHEY_COMPLEX_SMALL;
	Mat img = pool->getImage(i);
	string text = ctext;
	Size textSize = getTextSize(text, fontFace, fontScale, thickness, NULL);

	// Center the text
	Point textOrg((img.cols - textSize.width) / 2, (row > 0) ? (textSize.height
			* row) : (img.rows + (row * textSize.height)));

	// Draw a black border for the text and then draw the text
	putText(img, text, textOrg, fontFace, fontScale, Scalar::all(0), thickness
			* 10);
	putText(img, text, textOrg, fontFace, fontScale, color, thickness, CV_AA);
}
