/*
 * Processor.h
 *
 *  Created on: Jun 13, 2010
 *      Author: ethan
 */

#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <vector>

#include "image_pool.h"

class Processor
{
public:

  Processor();
  virtual ~Processor();

  void DetectOutline(int idx, image_pool *pool, double thres1, double thres2);

  void drawText(int i, image_pool* pool, const char* ctext, int row = -2, const cv::Scalar &color = cv::Scalar::all(255),
  			   double fontScale = 1, double thickness = .5);
private:
};

#endif /* PROCESSOR_H_ */
