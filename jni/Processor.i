/*
 * include the headers required by the generated cpp code
 */
%{
#include "Processor.h"
#include "image_pool.h"
using namespace cv;
%}

//import the android-cv.i file so that swig is aware of all that has been previous defined
//notice that it is not an include....
%import "android-cv.i"

//make sure to import the image_pool as it is 
//referenced by the Processor java generated
//class
%typemap(javaimports) Processor "
import com.opencv.jni.image_pool;// import the image_pool interface for playing nice with
								 // android-opencv

/** Processor - for processing images that are stored in an image pool
*/"

class Processor {
public:
	Processor();
	virtual ~Processor();

	void DetectOutline(int idx, image_pool *pool, double thres1, double thres2);

	void drawText(int i, image_pool* pool, const char* ctext, int row = -2, const Scalar &color = Scalar::all(255),
				  double fontScale = 1, double thickness = .5);
};
