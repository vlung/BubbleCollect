#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include "image_pool.h"

class Feedback
{
public:

  Feedback();
  virtual ~Feedback();

  void DetectOutline(int idx, image_pool *pool, double thres1, double thres2);

  void drawText(int i, image_pool* pool, const char* ctext, int row = -2, const cv::Scalar &color = cv::Scalar::all(255),
  			   double fontScale = 1, double thickness = .5);
private:
};
