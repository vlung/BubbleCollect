package com.opencv.camera;

import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;

import android.content.Context;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Handler;
import android.text.format.Time;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.opencv.camera.NativeProcessor.NativeProcessorCallback;
import com.opencv.camera.NativeProcessor.PoolCallback;

public class NativePreviewer extends SurfaceView implements
		SurfaceHolder.Callback, Camera.PreviewCallback, Camera.PictureCallback,
		NativeProcessorCallback {

	public interface PictureSavedCallback {
		void OnPictureSaved(String filename);
	}

	private PictureSavedCallback pictureSavedCallback = null;
	private Handler mHandler = new Handler();

	private boolean mHasAutoFocus = false;
	private boolean mIsPreview = false;
	private SurfaceHolder mHolder;
	private Camera mCamera;
	private NativeProcessor mProcessor;

	private int mPreview_width, mPreview_height;
	private int mPixelformat;

	private byte[] mPreviewBuffer = null;
	private Boolean mFnExistSetPreviewCallbackWithBuffer = false;
	private Method mFnAddCallbackBuffer = null;
	private Method mFnSetPreviewCallbackWithBuffer = null;
	
	public long initTime;

	private void init() {
		// Install a SurfaceHolder.Callback so we get notified when the
		// underlying surface is created and destroyed.
		mHolder = getHolder();
		mHolder.addCallback(this);
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

		mProcessor = new NativeProcessor();
		setZOrderMediaOverlay(false);

		mFnExistSetPreviewCallbackWithBuffer = false;

		// Check if camera APIs are available
		try {
			mFnSetPreviewCallbackWithBuffer = Class.forName(
					"android.hardware.Camera").getMethod(
					"setPreviewCallbackWithBuffer", PreviewCallback.class);
		} catch (Exception e) {
			Log.e("NativePreviewer",
					"This device does not support setPreviewCallbackWithBuffer.");
			mFnSetPreviewCallbackWithBuffer = null;
		}
		try {
			mFnAddCallbackBuffer = Class.forName("android.hardware.Camera")
					.getMethod("addCallbackBuffer", byte[].class);
		} catch (Exception e) {
			Log.e("NativePreviewer",
					"This device does not support addCallbackBuffer.");
			mFnAddCallbackBuffer = null;
		}
		if (mFnSetPreviewCallbackWithBuffer != null
				&& mFnAddCallbackBuffer != null) {
			mFnExistSetPreviewCallbackWithBuffer = true;
		}
	}

	public NativePreviewer(Context context, AttributeSet attributes,
			PictureSavedCallback callback) {
		super(context, attributes);

		init();
		pictureSavedCallback = callback;

		/*
		 * TODO get this working! Can't figure out how to define these in xml
		 */
		mPreview_width = attributes.getAttributeIntValue("opencv",
				"preview_width", 600);
		mPreview_height = attributes.getAttributeIntValue("opencv",
				"preview_height", 600);
	}

	/**
	 * 
	 * @param context
	 * @param preview_width
	 *            the desired camera preview width - will attempt to get as
	 *            close to this as possible
	 * @param preview_height
	 *            the desired camera preview height
	 */
	public NativePreviewer(Context context, int preview_width,
			int preview_height, PictureSavedCallback callback) {
		super(context);

		init();
		pictureSavedCallback = callback;

		this.mPreview_width = preview_width;
		this.mPreview_height = preview_height;
	}

	public void onPictureTaken(byte[] data, Camera camera) {
		final String captureImageFolder = "/sdcard/BubbleBot/capturedImages/";
		try {
			Time curTime = new Time();
			curTime.setToNow();

			String filename = curTime.format2445() + ".jpg";

			FileOutputStream jpgFile = new FileOutputStream(captureImageFolder
					+ filename, false);
			jpgFile.write(data);
			jpgFile.close();
			Log.i("NativePreviewer", "Picture saved to " + captureImageFolder
					+ filename);
			if (pictureSavedCallback != null) {
				pictureSavedCallback.OnPictureSaved(filename);
			}
		} catch (Exception e) {
			Log.e("NativePreviewer", "Failed to save photo", e);
		} finally {
			camera.stopPreview();
		}
	}

	public void takePicture() {
		Log.i("NativePreviewer", "Taking picture...");
		try {
			// Indicate that we are not in preview mode anymore
			// so we will ignore any autofocus event
			mIsPreview = false;
			mHandler.removeCallbacks(autofocusrunner);
			// Clear preview callback. Otherwise, camera will
			// use the preview buffer for saving the photo and it
			// will fail because the buffer is not big enough.
			mCamera.setPreviewCallback(null);
			mCamera.takePicture(null, null, this);
		} catch (Exception e) {
			Log.e("NativePreviewer", "takePicture", e);
		}
	}

	public void setPreviewSize(int width, int height) {
		mPreview_width = width;
		mPreview_height = height;
	}

	public void setParamsFromPrefs(Context ctx) {
		int size[] = { 0, 0 };
		CameraConfig.readImageSize(ctx, size);
		int mode = CameraConfig.readCameraMode(ctx);
		setPreviewSize(size[0], size[1]);
		setGrayscale(mode == CameraConfig.CAMERA_MODE_BW ? true : false);
	}

	public void surfaceCreated(SurfaceHolder holder) {
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
		releaseCamera();
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		try {
			initCamera(mHolder);
		} catch (InterruptedException e) {
			Log.e("NativePreviewer", "Failed to open camera", e);
			return;
		}

		// Now that the size is known, set up the camera parameters and begin
		// the preview.

		Camera.Parameters parameters = mCamera.getParameters();
		List<Camera.Size> pvsizes = mCamera.getParameters()
				.getSupportedPreviewSizes();
		int best_width = 1000000;
		int best_height = 1000000;
		int bdist = 100000;
		for (Size x : pvsizes) {
			if (Math.abs(x.width - mPreview_width) < bdist) {
				bdist = Math.abs(x.width - mPreview_width);
				best_width = x.width;
				best_height = x.height;
			}
		}
		mPreview_width = best_width;
		mPreview_height = best_height;

		Log.d("NativePreviewer", "Determined compatible preview size is: ("
				+ mPreview_width + "," + mPreview_height + ")");

		List<String> fmodes = mCamera.getParameters().getSupportedFocusModes();

		int idx = fmodes.indexOf(Camera.Parameters.FOCUS_MODE_INFINITY);
		if (idx != -1) {
			parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY);
		} else if (fmodes.indexOf(Camera.Parameters.FOCUS_MODE_FIXED) != -1) {
			parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
		}

		if (fmodes.indexOf(Camera.Parameters.FOCUS_MODE_AUTO) != -1) {
			mHasAutoFocus = true;
		}

		List<String> scenemodes = mCamera.getParameters()
				.getSupportedSceneModes();
		if (scenemodes != null) {
			if (scenemodes.indexOf(Camera.Parameters.SCENE_MODE_STEADYPHOTO) != -1) {
				parameters
						.setSceneMode(Camera.Parameters.SCENE_MODE_STEADYPHOTO);
				Log.d("NativePreviewer", "***Steady mode");
			}
		}

		if (mCamera.getParameters().getFlashMode() != null) {
			parameters.setFlashMode(Camera.Parameters.FLASH_MODE_ON);
		}

		parameters.setPictureFormat(PixelFormat.JPEG);

		parameters.setPreviewSize(mPreview_width, mPreview_height);

		mCamera.setParameters(parameters);

		setPreviewCallbackBuffer();
		mCamera.startPreview();
		mIsPreview = true;
	}

	public void postautofocus(int delay) {
		if (mHasAutoFocus)
		{
			mHandler.postDelayed(autofocusrunner, delay);
		}
	}

	private Date start = null;
	private long fcount = 0;
	
	/**
	 * Demonstration of how to use onPreviewFrame. In this case I'm not
	 * processing the data, I'm just adding the buffer back to the buffer queue
	 * for re-use
	 */
	public void onPreviewFrame(byte[] data, Camera camera) {
        if (start == null) {
            start = new Date();
        }

		if (mProcessor.isActive()) {
			if (mFnExistSetPreviewCallbackWithBuffer) {
				mProcessor.post(data, mPreview_width, mPreview_height,
						mPixelformat, System.nanoTime(), this);
			} else {
				System.arraycopy(data, 0, mPreviewBuffer, 0, data.length);
				mProcessor.post(mPreviewBuffer, mPreview_width,
						mPreview_height, mPixelformat, System.nanoTime(), this);
			}
		} else {
			Log.i("NativePreviewer",
					"Ignoring preview frame since processor is inactive.");
		}
		
        fcount++;
        if (fcount % 100 == 0) {
            double ms = (new Date()).getTime() - start.getTime();
            Log.i("NativePreviewer", "fps:" + fcount / (ms / 1000.0));
            start = new Date();
            fcount = 0;
        }
	}

	public void onDoneNativeProcessing(byte[] buffer) {
		if (mFnExistSetPreviewCallbackWithBuffer) {
			try {
				mFnAddCallbackBuffer.invoke(mCamera, buffer);
			} catch (Exception e) {
				Log.e("NativePreviewer", "Invoking addCallbackBuffer failed: "
						+ e.toString());
			}
		}
	}

	public void addCallbackStack(LinkedList<PoolCallback> callbackstack) {
		mProcessor.addCallbackStack(callbackstack);
	}

	/**
	 * This must be called when the activity pauses, in Activity.onPause This
	 * has the side effect of clearing the callback stack.
	 * 
	 */
	public void onPause() {
		try {
			mIsPreview = false;
			mCamera.setPreviewCallback(null);
			mCamera.stopPreview();
			addCallbackStack(null);
		} catch (Exception e) {
			Log.e("NativePreviewer", "Error during onPause()", e);
		} finally {
			releaseCamera();
			mProcessor.stop();
		}
	}

	public void onResume() {
		try {
			initCamera(mHolder);
		} catch (InterruptedException e) {
			Log.e("NativePreviewer", "Failed to open camera", e);
			return;
		}
		mProcessor.start();
		setPreviewCallbackBuffer();
		mCamera.startPreview();
		mIsPreview = true;
	}

	private void setPreviewCallbackBuffer() {

		PixelFormat pixelinfo = new PixelFormat();
		mPixelformat = mCamera.getParameters().getPreviewFormat();
		PixelFormat.getPixelFormatInfo(mPixelformat, pixelinfo);

		Size previewSize = mCamera.getParameters().getPreviewSize();
		mPreviewBuffer = new byte[previewSize.width * previewSize.height
				* pixelinfo.bitsPerPixel / 8];

		if (mFnExistSetPreviewCallbackWithBuffer) {
			try {
				mFnAddCallbackBuffer.invoke(mCamera, mPreviewBuffer);
			} catch (Exception e) {
				Log.e("NativePreviewer", "Invoking addCallbackBuffer failed: "
						+ e.toString());
			}
			try {
				mFnSetPreviewCallbackWithBuffer.invoke(mCamera, this);
			} catch (Exception e) {
				Log.e("NativePreviewer", "Invoking setPreviewCallbackWithBuffer failed: "
						+ e.toString());
			}
		} else {
			mCamera.setPreviewCallback(this);
		}
	}

	private Runnable autofocusrunner = new Runnable() {
		public void run() {
			if (mIsPreview)
			{
				mCamera.autoFocus(autocallback);
			}
		}
	};

	private Camera.AutoFocusCallback autocallback = new Camera.AutoFocusCallback() {
		public void onAutoFocus(boolean success, Camera camera) {
			if (!success)
				postautofocus(1000);
		}
	};

	private void initCamera(SurfaceHolder holder) throws InterruptedException {
		if (mCamera == null) {
			// The Surface has been created, acquire the camera and tell it
			// where
			// to draw.
			for (int i = 0; i < 10; i++)
				try {
					mCamera = Camera.open();
					break;
				} catch (RuntimeException e) {
					Log.e("NativePreviewer", "Failed to open camera", e);
					Thread.sleep(200);
					if (i == 9)
					{
						throw e;
					}
				}
		}
		try {
			mCamera.setPreviewDisplay(holder);
		} catch (IOException exception) {
			mCamera.release();
			mCamera = null;
		} catch (RuntimeException e) {
			Log.e("camera", "stacktrace", e);
		}
	}

	private void releaseCamera() {
		if (mCamera != null) {
			// Surface will be destroyed when we return, so stop the preview.
			// Because the CameraDevice object is not a shared resource, it's
			// very
			// important to release it when the activity is paused.
			mCamera.stopPreview();
			mCamera.release();
		}

		mCamera = null;
	}

	public void setGrayscale(boolean b) {
		mProcessor.setGrayscale(b);
	}

}
