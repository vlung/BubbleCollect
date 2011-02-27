package com.bubblebot;

import java.util.LinkedList;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.bubblebot.jni.Feedback;
import com.opencv.camera.CameraConfig;
import com.opencv.camera.NativePreviewer;
import com.opencv.camera.NativePreviewer.PictureSavedCallback;
import com.opencv.camera.NativeProcessor;
import com.opencv.camera.NativeProcessor.PoolCallback;
import com.opencv.jni.image_pool;
import com.opencv.opengl.GL2CameraViewer;

public class BubbleCollect extends Activity implements SensorEventListener,
		PictureSavedCallback {

	// Default Canny threshold and threshold2 multiplier
	static private double c_CannyMultiplier = 2.5;
	private double mCannyThres1 = 50;

	// Members for tracking accelerometer
	static private float c_StationaryThreshold = 0.25F;
	// Refresh accelerometer reading no less than .5 seconds
	static private long c_AccelrefreshInterval = 500000000L;
	// Auto focus no more than once every 5 seconds
	static private long c_AutoFocusInterval = 5000000000L;
	private final float alpha = (float) 0.8;
	private float[] mAccelMA = new float[3];
	private float[] mGravity = new float[3];
	private boolean mStationary = false;
	private TextView mDebugTextView = null;
	private long mLastAutoFocusTime = 0;
	private long mLastRefreshTime = 0;

	// final processor so that these processor callbacks can access it
	private final Feedback mFeedback = new Feedback();

	// OpenCV previewer
	private NativePreviewer mPreview;

	// GL view for OpenCV previewer
	private GL2CameraViewer glview;

	private final Handler mHandler = new Handler();

	private final Runnable mTakePhoto = new Runnable() {
		public void run() {
			mPreview.takePicture();
		}
	};

	public void OnPictureSaved(String filename) {
		// Start activity to handle actions after taking photo
		Intent intent = new Intent(getApplication(), AfterPhotoTaken.class);
		intent.putExtra("file", filename);
		Log.i("BubbleCollect", "Starting AfterPhotoTaken activity with "
				+ filename + "...");
		startActivity(intent);
		finish();
	}

	private void takePhoto() {
		mHandler.post(mTakePhoto);
	}

	// The OutlineProcessor guides the user to align the camera properly
	// for capturing a bubble form image
	class OutlineProcessor implements NativeProcessor.PoolCallback {

		public void process(int idx, image_pool pool, long timestamp,
				NativeProcessor nativeProcessor) {
			mFeedback.DetectOutline(idx, pool, mCannyThres1, mCannyThres1
					* c_CannyMultiplier);
		}
	}

	// Avoid that the screen get's turned off by the system.
	public void disableScreenTurnOff() {
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
				WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}

	// Use full screen
	public void setFullscreen() {
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
	}

	// Hide application title
	public void setNoTitle() {
		requestWindowFeature(Window.FEATURE_NO_TITLE);
	}

	// Create options menu
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add("*2");
		menu.add("+20");
		menu.add("-50");
		menu.add("Settings");
		return true;
	}

	// Handle options menu actions
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		if (item.getTitle().equals("+20")) {
			mCannyThres1 += 20;
		} else if (item.getTitle().equals("-50")) {
			mCannyThres1 -= 50;
		} else if (item.getTitle().equals("*2")) {
			mCannyThres1 *= 2;
		} else if (item.getTitle().equals("Settings")) {
			// Start the CameraConfig activity for camera settings
			Intent intent = new Intent(this, CameraConfig.class);
			startActivity(intent);
		}
		return true;
	}

	// Initialize the application
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setFullscreen();
		disableScreenTurnOff();

		FrameLayout frame = new FrameLayout(this);

		// Create our Preview view and set it as the content of our activity.
		mPreview = new NativePreviewer(getApplication(), 480, 640, this);

		LayoutParams params = new LayoutParams(LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT);
		params.height = getWindowManager().getDefaultDisplay().getHeight();
		params.width = getWindowManager().getDefaultDisplay().getWidth();
		// params.width = (int) (params.height * 4.0 / 2.88);

		LinearLayout vidlay = new LinearLayout(getApplication());

		vidlay.setGravity(Gravity.CENTER);
		vidlay.addView(mPreview, params);
		frame.addView(vidlay);

		// make the glview overlay ontop of video preview
		mPreview.setZOrderMediaOverlay(false);

		glview = new GL2CameraViewer(getApplication(), false, 0, 0);
		glview.setZOrderMediaOverlay(true);

		LinearLayout gllay = new LinearLayout(getApplication());

		gllay.setGravity(Gravity.CENTER);
		gllay.addView(glview, params);
		frame.addView(gllay);

		LinearLayout buttons = new LinearLayout(getApplicationContext());
		buttons.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT,
				LayoutParams.WRAP_CONTENT));
		buttons.setGravity(Gravity.RIGHT);
		buttons.setOrientation(1);

		ImageButton capture_button = new ImageButton(getApplicationContext());
		capture_button.setImageDrawable(getResources().getDrawable(
				android.R.drawable.ic_menu_camera));
		capture_button.setLayoutParams(new LayoutParams(
				LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		capture_button.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				takePhoto();
			}
		});
		buttons.addView(capture_button);

		// Add a new view for debug messages
		LinearLayout debugView = new LinearLayout(getApplicationContext());
		debugView.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT,
				LayoutParams.WRAP_CONTENT));
		debugView.setGravity(Gravity.CENTER);
		mDebugTextView = new TextView(getApplicationContext());
		debugView.addView(mDebugTextView);

		// Setup frame content
		frame.addView(buttons);
		frame.addView(debugView);
		setContentView(frame);

		// Use color camera
		SharedPreferences settings = getApplication().getSharedPreferences(
				CameraConfig.CAMERA_SETTINGS, 0);
		Editor editor = settings.edit();
		editor.putInt(CameraConfig.CAMERA_MODE, CameraConfig.CAMERA_MODE_COLOR);
		editor.commit();
	}

	@Override
	protected void onPause() {
		super.onPause();

		// clears the callback stack
		mPreview.onPause();
		glview.onPause();

		// Unregister sensor events
		SensorManager sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
		sensorManager.unregisterListener(this);
	}

	@Override
	protected void onResume() {
		super.onResume();

		// Register for accelerometer events
		mAccelMA[0] = mAccelMA[1] = mAccelMA[2] = 1.0F;
		SensorManager sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
		sensorManager.registerListener(this,
				sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
				SensorManager.SENSOR_DELAY_NORMAL);

		glview.onResume();
		mPreview.setParamsFromPrefs(getApplicationContext());
		// add an initial callback stack to the preview on resume...
		// this one will just draw the frames to opengl
		LinkedList<NativeProcessor.PoolCallback> cbstack = new LinkedList<PoolCallback>();
		cbstack.addFirst(glview.getDrawCallback());
		cbstack.addFirst(new OutlineProcessor());
		mPreview.addCallbackStack(cbstack);
		mPreview.onResume();
	}

	// We don't need to track accuracy but we must override this fuction to
	// avoid compiler error.
	public void onAccuracyChanged(Sensor arg0, int arg1) {
	}

	// Callback function for sensor events
	public void onSensorChanged(SensorEvent event) {
		// Handle accelerometer events
		if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
			// Skip this event if we have just handled one not so long ago
			if (event.timestamp - mLastRefreshTime < c_AccelrefreshInterval)
				return;

			float[] accel = new float[3];

			// Update timestamp
			mLastRefreshTime = event.timestamp;

			// Calculate linear acceleration
			mGravity[0] = alpha * mGravity[0] + (1 - alpha) * event.values[0];
			mGravity[1] = alpha * mGravity[1] + (1 - alpha) * event.values[1];
			mGravity[2] = alpha * mGravity[2] + (1 - alpha) * event.values[2];
			accel[0] = Math.abs(event.values[0] - mGravity[0]);
			accel[1] = Math.abs(event.values[1] - mGravity[1]);
			accel[2] = Math.abs(event.values[2] - mGravity[2]);

			// Calculate the moving averages of the acceleration
			for (int i = 0; i < 3; ++i) {
				mAccelMA[i] = (mAccelMA[i] + accel[i]) / 2;
			}

			// Check whether the moving averages exceed the threshold
			if (mAccelMA[0] > c_StationaryThreshold
					|| mAccelMA[1] > c_StationaryThreshold
					|| mAccelMA[2] > c_StationaryThreshold) {
				if (mStationary) {
					// The phone is moving, reset the moving averages to
					// allow smooth stationary detection
					mStationary = false;
					mAccelMA[0] = mAccelMA[1] = mAccelMA[2] = 1.0F;
				}
			} else {
				// If the phone has just become stationary, trigger
				// the autofocus action of the camera unless we
				// already triggered autofocus not too long ago.
				if (mStationary == false
						&& event.timestamp - mLastAutoFocusTime > c_AutoFocusInterval) {
					mLastAutoFocusTime = event.timestamp;
					mPreview.postautofocus(100);
				}
				mStationary = true;
			}

			// Show debug messages
			// String s = String.format("%.2f %.2f %.2f", accel[0], accel[1],
			// accel[2]);
			// mDebugTextView.setText(s);
		}
	}
}