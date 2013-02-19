package pku.shengbin.hevplayer;

import pku.shengbin.utils.MessageBox;
import android.app.Activity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

public class PlayActivity extends Activity implements SurfaceHolder.Callback, MediaPlayerControl {
	private static final String 	TAG = "PlayActivity";
	
	private NativeMediaPlayer			mPlayer;
	private SurfaceView 			mSurfaceView;
	private MediaController			mMediaController;
    
    private boolean mStartTwoTouchPoint = false;
    private double 	mStartDistance;
    private double 	mZoomScale = 1;
    
	//////////////////////////////////////////
	//implements SurfaceHolder.Callback
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {

    }

    public void surfaceCreated(SurfaceHolder holder) {
 		setDisplaySizeVideo();	
		attachMediaController();
    	mPlayer.start();
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
		mPlayer.stop();
		if(mMediaController != null && mMediaController.isShowing()) {
			mMediaController.hide();
		}
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
		return false;
	}

	public boolean canSeekForward() {
		return false;
	}
	
	//end of : implements MediaPlayerControl
	//////////////////////////////////////////
	
	
    // Called when the activity is first created
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
    	requestWindowFeature(Window.FEATURE_NO_TITLE);
    	getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.play_view);
                
        mSurfaceView = (SurfaceView)this.findViewById(R.id.surface_view);       
        mSurfaceView.getHolder().addCallback(this);
    	
        String moviePath = this.getIntent().getStringExtra("pku.shengbin.hevplayer.strMediaPath");
        mPlayer = new NativeMediaPlayer(this);
		int ret = mPlayer.setDataSource(moviePath);	
		if (ret != 0) {
			MessageBox.show(this, "Oops!","Get data source failed! Please check your file or network connection.",
			new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface arg0, int arg1) {
				PlayActivity.this.finish();
			}
        	});
		}
	 	mPlayer.setDisplay(mSurfaceView.getHolder());
	 	mPlayer.prepare(1);
		
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
					}else if (distance - mStartDistance < -10) {
						zoomSmall();
					}				
					mStartDistance = distance;
            	}
            }
       } else {
        	mStartTwoTouchPoint = false;
        	if(event.getPointerCount() == 1 && event.getY() > getWindowManager().getDefaultDisplay().getHeight() - 40){
        		if(mMediaController != null)
	        		if(!mMediaController.isShowing()) {
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
    public static final int MENU4 = Menu.FIRST + 3;
    public static final int MENU5 = Menu.FIRST + 4;
    public static final int MENU6 = Menu.FIRST + 5;

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        
        menu.add(0, MENU1, 0, "Original Size");   
        menu.add(0, MENU2, 1, "Choose Media");
        menu.add(0, MENU3, 2, "Show Info");

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
        mMediaController.setAnchorView(mSurfaceView);
        mMediaController.setEnabled(true);
    }
    
    private void setDisplaySizeVideo()
    {
    	getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	
    	int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer.getVideoHeight();
 		int screenWidth, screenHeight, displayWidth = 0, displayHeight = 0;
 		screenWidth = getWindowManager().getDefaultDisplay().getWidth();
 		screenHeight = getWindowManager().getDefaultDisplay().getHeight();
 		
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
    
    private void setDisplaySizeFullScreen()
    {
    	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	
    	int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer.getVideoHeight();
 		int screenWidth, screenHeight, displayWidth = 0, displayHeight = 0;
 		screenWidth = getWindowManager().getDefaultDisplay().getWidth();
 		screenHeight = getWindowManager().getDefaultDisplay().getHeight();
 		
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
    
    
    private static double getDistance(double x1, double y1, double x2, double y2) {
        return Math.sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }
    
    private void zoomLarge() {
    	int videoWidth = mPlayer.getVideoWidth(), videoHeight = mPlayer.getVideoHeight();
 		int screenWidth = 0, screenHeight = 0, displayWidth = 0, displayHeight = 0;
 		screenWidth = getWindowManager().getDefaultDisplay().getWidth();
 		screenHeight = getWindowManager().getDefaultDisplay().getHeight();
 		
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
 		screenWidth = getWindowManager().getDefaultDisplay().getWidth();
 		screenHeight = getWindowManager().getDefaultDisplay().getHeight();
 		
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
   
}