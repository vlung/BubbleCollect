#pragma once

#include <opencv2/core/core.hpp>

class Processor
{
public:
  Processor();
  virtual ~Processor();

  void ProcessForm(cv::Mat img);
private:
};
