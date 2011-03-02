#you may override this if you move the build
#just define it before including this or on the command line - or with
#an environment variable
#this points to the root of the opencv trunk - where the original opencv 
#sources are - with modules 3rparty ...
ifndef OPENCV_ROOT
OPENCV_ROOT := /Users/nixdell/BubbleCollect/opencv
endif

#you may override this same as above
#this points to the actually directory that you built opencv for android from
#maybe in under opencv/android/build
ifndef OPENCV_BUILD_ROOT
OPENCV_BUILD_ROOT := $(OPENCV_ROOT)/android/build
endif

OPENCV_INCLUDES :=  $(OPENCV_ROOT)/modules/calib3d/include $(OPENCV_ROOT)/modules/contrib/include $(OPENCV_ROOT)/modules/core/include $(OPENCV_ROOT)/modules/features2d/include $(OPENCV_ROOT)/modules/ffmpeg/include $(OPENCV_ROOT)/modules/flann/include $(OPENCV_ROOT)/modules/gpu/include $(OPENCV_ROOT)/modules/gtest/include $(OPENCV_ROOT)/modules/haartraining/include $(OPENCV_ROOT)/modules/highgui/include $(OPENCV_ROOT)/modules/imgproc/include $(OPENCV_ROOT)/modules/legacy/include $(OPENCV_ROOT)/modules/ml/include $(OPENCV_ROOT)/modules/objdetect/include $(OPENCV_ROOT)/modules/traincascade/include $(OPENCV_ROOT)/modules/video/include $(OPENCV_ROOT)/3rdparty/include $(OPENCV_BUILD_ROOT)/include $(OPENCV_ROOT)/include

ANDROID_OPENCV_INCLUDES := $(OPENCV_ROOT)/android/android-jni/jni

ARMOBJS := local/armeabi
ARMOBJS_V7A := local/armeabi-v7a

#OPENCV_LIB_DIRS := -L$(OPENCV_BUILD_ROOT)/obj/$(ARMOBJS_V7A) \
#    -L$(OPENCV_BUILD_ROOT)/obj/$(ARMOBJS) -L$(OPENCV_BUILD_ROOT)/bin/ndk/$(ARMOBJS) \
#    -L$(OPENCV_BUILD_ROOT)/bin/ndk/$(ARMOBJS_V7A)
OPENCV_LIB_DIRS := -L$(OPENCV_BUILD_ROOT)/obj/$(ARMOBJS_V7A) \
    -L$(OPENCV_BUILD_ROOT)/obj/$(ARMOBJS) 

ANDROID_OPENCV_LIB_DIRS := -L$(OPENCV_ROOT)/android/android-jni/libs/armeabi-v7a \
    -L$(OPENCV_ROOT)/android/android-jni/libs/armeabi

#order of linking very important ---- may have stuff out of order here, but
#important that modules that are more dependent come first...

OPENCV_LIBS := $(OPENCV_LIB_DIRS) -lopencv_calib3d -lopencv_features2d -lopencv_objdetect -lopencv_imgproc \
     -lopencv_video  -lopencv_highgui -lopencv_ml -lopencv_legacy -lopencv_core -lopencv_lapack -lopencv_flann \
    -lzlib -lpng -ljpeg -ljasper
ANDROID_OPENCV_LIBS := $(ANDROID_OPENCV_LIB_DIRS) -landroid-opencv
    
