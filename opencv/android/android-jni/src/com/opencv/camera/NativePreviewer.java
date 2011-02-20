package com.opencv.camera;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.Size;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.opencv.camera.NativeProcessor.NativeProcessorCallback;
import com.opencv.camera.NativeProcessor.PoolCallback;

public class NativePreviewer extends SurfaceView implements
		SurfaceHolder.Callback, Camera.PreviewCallback, Camera.PictureCallback, NativeProcessorCallback {

	private Handler handler = new Handler();

	private boolean hasAutoFocus = false;
	private SurfaceHolder mHolder;
	private Camera mCamera;
	private NativeProcessor processor;
	
	private int preview_width, preview_height;
	private int pixelformat;
	
	private byte[] previewBuffer = null;
	
	/**
	 * Constructor useful for defining a NativePreviewer in android layout xml
	 * 
	 * @param context
	 * @param attributes
	 */
	public NativePreviewer(Context context, AttributeSet attributes) {
		super(context, attributes);

		// Install a SurfaceHolder.Callback so we get notified when the
		// underlying surface is created and destroyed.
		mHolder = getHolder();
		mHolder.addCallback(this);
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

		/*
		 * TODO get this working! Can't figure out how to define these in xml
		 */
		preview_width = attributes.getAttributeIntValue("opencv",
				"preview_width", 600);
		preview_height = attributes.getAttributeIntValue("opencv",
				"preview_height", 600);

		processor = new NativeProcessor();

		setZOrderMediaOverlay(false);
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
			int preview_height) {
		super(context);

		// Install a SurfaceHolder.Callback so we get notified when the
		// underlying surface is created and destroyed.
		mHolder = getHolder();
		mHolder.addCallback(this);
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

		this.preview_width = preview_width;
		this.preview_height = preview_height;

		processor = new NativeProcessor();
		setZOrderMediaOverlay(false);
	}

	public void onPictureTaken(byte[] data, Camera camera) {
		try {
			FileOutputStream jpgFile = new FileOutputStream(
					"/sdcard/temp.jpg", false);
			jpgFile.write(data);
			jpgFile.close();
			Log.i("NativePreviewer", "Picture saved to /sdcard/temp.jpg");
		} catch (Exception e) {
			Log.e("NativePreviewer", "Failed to save photo", e);
		} finally {
			// Set up the preview buffer and resume preview
			mCamera.setPreviewCallbackWithBuffer(this);
			setPreviewCallbackBuffer();
			mCamera.startPreview();
		}
	}

	public void takePicture() {
		Log.i("NativePreviewer", "Taking picture...");
		try {
			// Clear preview callback. Otherwise, camera will
			// use the preview buffer for saving the photo and it
			// will fail because the buffer is not big enough.
			mCamera.setPreviewCallback(null);
			mCamera.takePicture(null, null, this);
			mCamera.stopPreview();
		} catch (Exception e) {
			Log.e("NativePreviewer", "takePicture", e);
		}
	}

	public void setPreviewSize(int width, int height) {
		preview_width = width;
		preview_height = height;
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
			if (Math.abs(x.width - preview_width) < bdist) {
				bdist = Math.abs(x.width - preview_width);
				best_width = x.width;
				best_height = x.height;
			}
		}
		preview_width = best_width;
		preview_height = best_height;

		Log.d("NativePreviewer", "Determined compatible preview size is: ("
				+ preview_width + "," + preview_height + ")");

		List<String> fmodes = mCamera.getParameters().getSupportedFocusModes();

		int idx = fmodes.indexOf(Camera.Parameters.FOCUS_MODE_INFINITY);
		if (idx != -1) {
			parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY);
		} else if (fmodes.indexOf(Camera.Parameters.FOCUS_MODE_FIXED) != -1) {
			parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
		}

		if (fmodes.indexOf(Camera.Parameters.FOCUS_MODE_AUTO) != -1) {
			hasAutoFocus = true;
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

		parameters.setPreviewSize(preview_width, preview_height);

		mCamera.setParameters(parameters);

		setPreviewCallbackBuffer();
		mCamera.setPreviewCallbackWithBuffer(this);
		mCamera.startPreview();
	}

	public void postautofocus(int delay) {
		if (hasAutoFocus)
			handler.postDelayed(autofocusrunner, delay);

	}

	/**
	 * Demonstration of how to use onPreviewFrame. In this case I'm not
	 * processing the data, I'm just adding the buffer back to the buffer queue
	 * for re-use
	 */
	public void onPreviewFrame(byte[] data, Camera camera) {
		if (processor.isActive()) {
			processor.post(data, preview_width, preview_height, pixelformat,
					System.nanoTime(), this);
		}
		else
		{
			Log.i("NativePreviewer", "Ignoring preview frame since processor is inactive.");
		}
	}

	@Override
	public void onDoneNativeProcessing(byte[] buffer) {
		mCamera.addCallbackBuffer(previewBuffer);
	}

	public void addCallbackStack(LinkedList<PoolCallback> callbackstack) {
		processor.addCallbackStack(callbackstack);
	}

	/**
	 * This must be called when the activity pauses, in Activity.onPause This
	 * has the side effect of clearing the callback stack.
	 * 
	 */
	public void onPause() {
		releaseCamera();
		addCallbackStack(null);
		processor.stop();
	}

	public void onResume() {
//
//		setPreviewCallbackBuffer();
//		mCamera.setPreviewCallbackWithBuffer(this);
		processor.start();
	}

	private void setPreviewCallbackBuffer() {

		try {
			PixelFormat pixelinfo = new PixelFormat();
			pixelformat = mCamera.getParameters().getPreviewFormat();
			PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);
			
			Size previewSize = mCamera.getParameters().getPreviewSize();

			previewBuffer = new byte[previewSize.width * previewSize.height * pixelinfo.bitsPerPixel / 8];
			mCamera.addCallbackBuffer(previewBuffer);
		} catch (Exception e) {
			Log.e("NativePreviewer",
					"invoking addCallbackBuffer failed: " + e.toString());
		}
	}

	private Runnable autofocusrunner = new Runnable() {
		@Override
		public void run() {
			mCamera.autoFocus(autocallback);
		}
	};

	private Camera.AutoFocusCallback autocallback = new Camera.AutoFocusCallback() {
		@Override
		public void onAutoFocus(boolean success, Camera camera) {
			if (!success)
				postautofocus(1000);
		}
	};


//	private void listAllCameraMethods() {
//		try {
//			Class<?> c = Class.forName("android.hardware.Camera");
//			Method[] m = c.getMethods();
//			for (int i = 0; i < m.length; i++) {
//				Log.d("NativePreviewer", "  method:" + m[i].toString());
//			}
//		} catch (Exception e) {
//			// TODO Auto-generated catch block
//			Log.e("NativePreviewer", e.toString());
//		}
//	}

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
					Thread.sleep(200);
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
		processor.setGrayscale(b);
	}

}
