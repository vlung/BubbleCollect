#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/legacy/compat.hpp>

class Processor
{
public:
  Processor();
  virtual ~Processor();

  char* ProcessForm(char* filename);
  CvPoint* findCornerPoints(IplImage* img);
  void warpImage(IplImage* img, IplImage* warpImg, CvPoint * cornerPoints);
private:
};
