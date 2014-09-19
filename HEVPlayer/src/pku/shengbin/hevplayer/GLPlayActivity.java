package pku.shengbin.hevplayer;

import pku.shengbin.utils.MessageBox;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.Color;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;

/**
 * This class will new a MediaPlayer instance to play the media file.
 * It contains a GLPlayView which use a GLRenderer to render video frames.
 * Media player control is implemented by this class. Users interact with this class
 * when the media is playing.
 */
public class GLPlayActivity extends Activity implements SurfaceHolder.Callback,
		MediaPlayerControl {
	private MediaPlayer mPlayer;
	private GLSurfaceView mGLSurfaceView;
	private MediaController mMediaController;

	private boolean mStartTwoTouchPoint = false;
	private double mStartDistance;
	private double mZoomScale = 1;
	private int mError = 0;

	String moviePath;
	int screenWidth, screenHeight, displayWidth = 0, displayHeight = 0;

	// ////////////////////////////////////////
	// implements SurfaceHolder.Callback
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {

	}

	public void surfaceCreated(SurfaceHolder holder) {
		if (mError != 0) {
			return;
		}
		setDisplaySizeVideo();
		attachMediaController();
		mPlayer.start();
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
		if (mError != 0) {
			return;
		}

		mPlayer.stop();
		if (mMediaController != null && mMediaController.isShowing()) {
			mMediaController.hide();
		}
		mPlayer.close();
		this.finish();

	}

	// end of: implements SurfaceHolder.Callback
	// ///////////////////////////////////////////

	// ///////////////////////////////////////////
	// implements MediaPlayerControl
	public void start() {
		mPlayer.go();
	}

	public void seekTo(int pos) {
		mPlayer.seekTo(pos);
	}

	public void pause() {
		mPlayer.pause();
	}

	public boolean isPlaying() {
		boolean ret = mPlayer.isPlaying();
		return ret;
	}

	public int getDuration() {
		int ret = mPlayer.getDuration();
		return ret;
	}

	public int getCurrentPosition() {
		int ret = mPlayer.getCurrentPosition();
		return ret;
	}

	public int getBufferPercentage() {
		return 0;
	}

	public boolean canPause() {
		return true;
	}

	public boolean canSeekBackward() {
		return true;
	}

	public boolean canSeekForward() {
		return true;
	}

	public int getAudioSessionId() {
		return 0;
	}

	// end of : implements MediaPlayerControl
	// ////////////////////////////////////////

	// Called when the activity is first created
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		View decorView = getWindow().getDecorView();
		decorView.setOnSystemUiVisibilityChangeListener
		        (new View.OnSystemUiVisibilityChangeListener() {
		    @Override
		    public void onSystemUiVisibilityChange(int visibility) {
		        // Note that system bars will only be "visible" if none of the
		        // LOW_PROFILE, HIDE_NAVIGATION, or FULLSCREEN flags are set.
		        if ((visibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) {
		            // The system bars are visible
		        	mMediaController.show(300000);
		        	//GLPlayActivity.this.getActionBar().show();
					getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
					
					// The system bar becoming visible may have eaten the touch event, so we need a reset here.
		        	resetTimer();
		        } else {
		            // The system bars are NOT visible
					mMediaController.hide();
		        	//GLPlayActivity.this.getActionBar().hide();
					getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
							WindowManager.LayoutParams.FLAG_FULLSCREEN);
		        }
		    }
		});

		mGLSurfaceView = new GLPlayView(this.getApplication());
		mGLSurfaceView.getHolder().addCallback(this);

		RelativeLayout relativeLayout = new RelativeLayout(this);
		relativeLayout.addView(mGLSurfaceView);
		TextView textView = new TextView(this);
		RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
				LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.ALIGN_TOP);
		textView.setLayoutParams(lp);
		textView.setText("");
		textView.setTextSize(20);
		textView.setTextColor(Color.WHITE);
		relativeLayout.addView(textView);

		this.setContentView(relativeLayout);

		mError = 0;
		mPlayer = new MediaPlayer(this);

		moviePath = this.getIntent().getStringExtra(
				"pku.shengbin.hevplayer.strMediaPath");
		mPlayer.setDisplay(mGLSurfaceView, textView);

		int ret = mPlayer.open(moviePath);
		if (ret != 0) {
			MessageBox.show(this, "Oops!",
							"open media failed! Please check your file or network connection.",
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface arg0,
										int arg1) {
									GLPlayActivity.this.finish();
								}
							});

			mError = 1;
		}
	}

	@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
	@Override
	/**
	 * Handles user touch, mainly pinch to zoom.
	 */
	public boolean onTouchEvent(android.view.MotionEvent event) {

		if (event.getPointerCount() == 8) {
			if (mStartTwoTouchPoint == false) {
				mStartDistance = getDistance(event.getX(0), event.getY(0),
						event.getX(1), event.getY(1));
				mStartTwoTouchPoint = true;
			} else {
				if (event.getAction() == MotionEvent.ACTION_MOVE) {
					double distance = getDistance(event.getX(0), event.getY(0),
							event.getX(1), event.getY(1));
					if (distance - mStartDistance > 10) {
						zoomLarge();
					} else if (distance - mStartDistance < -10) {
						zoomSmall();
					}
					mStartDistance = distance;
				}
			}
		} else {
			mStartTwoTouchPoint = false;
			if (event.getPointerCount() == 1
					&& event.getAction() == MotionEvent.ACTION_DOWN && event.getAction() != MotionEvent.ACTION_POINTER_DOWN) {
				if (mMediaController != null) {
					if (mMediaController.isShowing()) {
			        	mMediaController.hide();
						if (Build.VERSION.SDK_INT >= 16) {
							View decorView = getWindow().getDecorView();
							int uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
							decorView.setSystemUiVisibility(uiOptions);
						}
					} else {
			        	mMediaController.show(300000);
					}
				}
				
	        	resetTimer();
			}
		}
		return true;
	}

	public static final long TIMEUP_PERIOD = 5000; // 3000 ms

    private Handler timeupHandler = new Handler(){
        public void handleMessage(Message msg) {
        }
    };

    private Runnable timeupCallback = new Runnable() {
        @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
		@Override
        public void run() {
            // Perform any required operation
        	if (mMediaController != null) {
	        	mMediaController.hide();
				if (Build.VERSION.SDK_INT >= 16) {
					View decorView = getWindow().getDecorView();
					int uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
					decorView.setSystemUiVisibility(uiOptions);
				}
			}
        }
    };

    public void resetTimer() {
        timeupHandler.removeCallbacks(timeupCallback);
        timeupHandler.postDelayed(timeupCallback, TIMEUP_PERIOD);
    }

    public void stopTimer() {
        timeupHandler.removeCallbacks(timeupCallback);
    }
	
	@Override
    public void onResume() {
        super.onResume();
        resetTimer();
    }

    @Override
    public void onStop() {
        super.onStop();
        stopTimer();
    }
	
	// Menu item Ids
	public static final int MENU1 = Menu.FIRST;
	public static final int MENU2 = Menu.FIRST + 1;
	public static final int MENU3 = Menu.FIRST + 2;

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);

		menu.add(0, MENU1, 0, "Original Size");
		menu.add(0, MENU2, 1, "Choose Media");
		menu.add(0, MENU3, 2, "Hide Info");

		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case MENU1: {
			if (item.getTitle().equals("Full Screen")) {
				setDisplaySizeFullScreen();
				item.setTitle("Original Size");
			} else if (item.getTitle().equals("Original Size")) {
				setDisplaySizeVideo();
				item.setTitle("Full Screen");
			}

			return true;
		}
		case MENU2: {
			this.finish();
			return true;
		}
		case MENU3: {
			if (item.getTitle().equals("Show Info")) {
				mPlayer.setShowInfo(true);
				item.setTitle("Hide Info");
			} else if (item.getTitle().equals("Hide Info")) {
				mPlayer.setShowInfo(false);
				item.setTitle("Show Info");
			}
		}
		}

		return super.onOptionsItemSelected(item);
	}

	// ///////////////////////////////////////////////////
	// utility functions
	private void attachMediaController() {
		mMediaController = new MediaController(this);
		mMediaController.setMediaPlayer(this);
		mMediaController.setAnchorView(mGLSurfaceView);
		mMediaController.setEnabled(true);
    	mMediaController.show(300000);
	}

	private void setDisplaySizeVideo() {
		getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

		int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer
				.getVideoHeight();
		int screenWidth, screenHeight, displayWidth = 0, displayHeight = 0;
		screenWidth = this.getResources().getDisplayMetrics().widthPixels;
		screenHeight = this.getResources().getDisplayMetrics().heightPixels;

		displayWidth = videoWidth;
		displayHeight = videoHeight;
		if (displayHeight > screenHeight) {
			displayHeight = screenHeight;
			displayWidth = displayHeight * videoWidth / videoHeight;
			displayWidth -= displayWidth % 4;
		}
		if (displayWidth > screenWidth) {
			displayWidth = screenWidth;
			displayHeight = displayWidth * videoHeight / videoWidth;
			displayHeight -= displayHeight % 4;
		}

		mZoomScale = (double) displayHeight / videoHeight;
		mPlayer.setDisplaySize(displayWidth, displayHeight);
	}

	private void setDisplaySizeFullScreen() {
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);

		int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer
				.getVideoHeight();
		// int screenWidth, screenHeight, displayWidth = 0, displayHeight = 0;
		screenWidth = this.getResources().getDisplayMetrics().widthPixels;
		screenHeight = this.getResources().getDisplayMetrics().heightPixels;

		displayWidth = screenWidth;
		displayHeight = displayWidth * videoHeight / videoWidth;
		displayHeight -= displayHeight % 4;
		if (displayHeight > screenHeight) {
			displayHeight = screenHeight;
			displayWidth = displayHeight * videoWidth / videoHeight;
			displayWidth -= displayWidth % 4;
		}

		mZoomScale = (double) displayHeight / videoHeight;
		mPlayer.setDisplaySize(displayWidth, displayHeight);
	}

	private double getDistance(double x1, double y1, double x2, double y2) {
		return Math.sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
	}

	private void zoomLarge() {
		int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer
				.getVideoHeight();
		int screenWidth = 0, screenHeight = 0, displayWidth = 0, displayHeight = 0;
		screenWidth = this.getResources().getDisplayMetrics().widthPixels;
		screenHeight = this.getResources().getDisplayMetrics().heightPixels;

		mZoomScale += 0.1;
		displayWidth = (int) (videoWidth * mZoomScale);
		displayHeight = (int) (videoHeight * mZoomScale);
		if (displayHeight > screenHeight - 4 || displayWidth > screenWidth - 4) {
			setDisplaySizeFullScreen();
			return;
		}
		getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
		mPlayer.setDisplaySize(displayWidth, displayHeight);
	}

	private void zoomSmall() {
		int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer
				.getVideoHeight();
		int screenWidth = 0, screenHeight = 0, displayWidth = 0, displayHeight = 0;
		screenWidth = this.getResources().getDisplayMetrics().widthPixels;
		screenHeight = this.getResources().getDisplayMetrics().heightPixels;

		mZoomScale -= 0.1;
		displayWidth = (int) (videoWidth * mZoomScale);
		displayHeight = (int) (videoHeight * mZoomScale);
		if (displayHeight > screenHeight || displayWidth > screenWidth) {
			setDisplaySizeFullScreen();
			return;
		}
		if (displayHeight < screenHeight * 0.2
				|| displayWidth < screenWidth * 0.2) {
			mZoomScale += 0.1;
			return;
		}
		getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
		mPlayer.setDisplaySize(displayWidth, displayHeight);
	}
	// end of: utility functions
	// ////////////////////////////////////////////
}
