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
private:
};
