package pku.shengbin.hevplayer;

import pku.shengbin.utils.MessageBox;
import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.Color;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.Window;
import android.view.WindowManager;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;

public class GLPlayActivity extends Activity implements SurfaceHolder.Callback, MediaPlayerControl {
	private MediaPlayer				mPlayer;
	private GLSurfaceView 			mGLSurfaceView;
	private MediaController			mMediaController;
    
    private boolean mStartTwoTouchPoint = false;
    private double 	mStartDistance;
    private double 	mZoomScale = 1;
    private int		mError = 0;

    String moviePath;
    TextView display_tv;
    int screenWidth, screenHeight, displayWidth = 0, displayHeight = 0;

	//////////////////////////////////////////
	//implements SurfaceHolder.Callback
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {

    }

    public void surfaceCreated(SurfaceHolder holder) {
    	if (mError != 0) {
    		return ;
    	}
 		setDisplaySizeVideo();
		attachMediaController();
    	mPlayer.start();
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
    	if (mError != 0) {
    		return ;
    	}
    	
		mPlayer.stop();
		if(mMediaController != null && mMediaController.isShowing()) {
			mMediaController.hide();
		}
		mPlayer.close();
		this.finish();
		
    }
    // end of: implements SurfaceHolder.Callback
    /////////////////////////////////////////////
    
	/////////////////////////////////////////////
    //implements MediaPlayerControl
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
	//end of : implements MediaPlayerControl
	//////////////////////////////////////////
	
	
    // Called when the activity is first created
    @Override
    public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		        
		mGLSurfaceView = new GLPlayView(this.getApplication());       
		mGLSurfaceView.getHolder().addCallback(this);
		
		RelativeLayout rl = new RelativeLayout(this);
	    rl.addView(mGLSurfaceView);        
	    display_tv = new TextView(this);
	    RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
	    lp.addRule(RelativeLayout.ALIGN_TOP);
	    display_tv.setLayoutParams(lp);
	    display_tv.setText("");
	    display_tv.setTextSize(20);
	    display_tv.setTextColor(Color.WHITE);
	    rl.addView(display_tv);

	    this.setContentView(rl);
	    
		mError = 0;
		mPlayer = new MediaPlayer(this);
    	
    	moviePath = this.getIntent().getStringExtra("pku.shengbin.hevplayer.strMediaPath");
		mPlayer.setDisplay(mGLSurfaceView, display_tv);
		
		int ret = mPlayer.open(moviePath);
		if (ret != 0) {
			MessageBox.show(this, "Oops!","open media failed! Please check your file or network connection.",
				new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface arg0, int arg1) {
					GLPlayActivity.this.finish();
				}
		    	}
			);
			
			mError = 1;
		}
    }

	@Override
    public boolean onTouchEvent(android.view.MotionEvent event) {
    	
    	if (event.getPointerCount() == 2) {
            if (mStartTwoTouchPoint == false) {
                mStartDistance = getDistance(event.getX(0), event.getY(0), event.getX(1), event.getY(1));
                mStartTwoTouchPoint = true;
            } else {
            	if (event.getAction() == MotionEvent.ACTION_MOVE) {
					double distance = getDistance(event.getX(0), event
					        .getY(0), event.getX(1), event.getY(1));
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
        	if (event.getPointerCount() == 1 && event.getY() > this.getResources().getDisplayMetrics().heightPixels * 0.2) {
        		if (mMediaController != null)
	        		if (mMediaController.isShowing() == false) {
		    			mMediaController.show(3000);
		    		}
        	}
        }
		return true;
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
	        case MENU1:
	        {
	        	if (item.getTitle().equals("Full Screen")) {
		        	setDisplaySizeFullScreen();
		        	item.setTitle("Original Size");
	        	} else if (item.getTitle().equals("Original Size")) {
	        		setDisplaySizeVideo();
		        	item.setTitle("Full Screen");      		
	        	}        	
	        	
	            return true;
	        }
	        case MENU2:
	        {
	        	this.finish();
	            return true;
	        }
	        case MENU3:
	        {
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
    

    /////////////////////////////////////////////////////
    //utility functions 
    private void attachMediaController() {
    	mMediaController = new MediaController(this);
        mMediaController.setMediaPlayer(this);
        mMediaController.setAnchorView(mGLSurfaceView);
        mMediaController.setEnabled(true);
    }
    
    private void setDisplaySizeVideo() {
    	getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	
    	int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer.getVideoHeight();
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
 		
		mZoomScale = (double)displayHeight / videoHeight;
 		mPlayer.setDisplaySize(displayWidth, displayHeight);
    }
    
    private void setDisplaySizeFullScreen() {
    	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	
    	int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer.getVideoHeight();
 		//int screenWidth, screenHeight, displayWidth = 0, displayHeight = 0;
 		screenWidth = this.getResources().getDisplayMetrics().widthPixels;
 		screenHeight = this.getResources().getDisplayMetrics().heightPixels;
 		
 		displayWidth = screenWidth;
 		displayHeight = displayWidth * videoHeight / videoWidth;
 		displayHeight -= displayHeight%4; 
 		if (displayHeight > screenHeight) {
 			displayHeight = screenHeight;
 			displayWidth = displayHeight * videoWidth / videoHeight;
 			displayWidth -= displayWidth%4;
 		}
 		
		mZoomScale = (double)displayHeight / videoHeight;
 		mPlayer.setDisplaySize(displayWidth, displayHeight);
    }
    
    private double getDistance(double x1, double y1, double x2, double y2) {
        return Math.sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }
    
    private void zoomLarge() {
    	int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer.getVideoHeight();
 		int screenWidth = 0, screenHeight = 0, displayWidth = 0, displayHeight = 0;
 		screenWidth = this.getResources().getDisplayMetrics().widthPixels;
 		screenHeight = this.getResources().getDisplayMetrics().heightPixels;
 		
    	mZoomScale += 0.1;
 		displayWidth = (int) (videoWidth * mZoomScale);
 		displayHeight = (int) (videoHeight * mZoomScale);	
 		if (displayHeight > screenHeight - 4 || displayWidth > screenWidth - 4) {
 			setDisplaySizeFullScreen();
 			return ;
 		}
    	getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
 		mPlayer.setDisplaySize(displayWidth, displayHeight);
    }
    
    private void zoomSmall() {
    	int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer.getVideoHeight();
 		int screenWidth = 0, screenHeight = 0, displayWidth = 0, displayHeight = 0;
 		screenWidth = this.getResources().getDisplayMetrics().widthPixels;
 		screenHeight = this.getResources().getDisplayMetrics().heightPixels;
 		
 		mZoomScale -= 0.1;
 		displayWidth = (int) (videoWidth * mZoomScale);
 		displayHeight = (int) (videoHeight * mZoomScale);
 		if (displayHeight > screenHeight || displayWidth > screenWidth) {
 			setDisplaySizeFullScreen();
 			return ;
 		}
 		if (displayHeight < screenHeight * 0.2 || displayWidth < screenWidth * 0.2) {
 			mZoomScale += 0.1;
 			return ;
 		}
    	getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	mPlayer.setDisplaySize(displayWidth, displayHeight);
    }
    // end of: utility functions 
    //////////////////////////////////////////////   

   
	public void replay()
	{
		mPlayer.close();
		mPlayer = new MediaPlayer(this);
    	
    	moviePath = this.getIntent().getStringExtra("pku.shengbin.hevplayer.strMediaPath");
		mPlayer.setDisplay(mGLSurfaceView, display_tv);
		
		int ret = mPlayer.open(moviePath);
		if (ret != 0) {
			MessageBox.show(this, "Oops!","open media failed! Please check your file or network connection.",
				new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface arg0, int arg1) {
					GLPlayActivity.this.finish();
				}
		    	}
			);
			
			mError = 1;
		}
 		//setDisplaySizeVideo();
		mPlayer.setDisplaySize(displayWidth, displayHeight);
		//attachMediaController();
    	mPlayer.start();
	}
}
