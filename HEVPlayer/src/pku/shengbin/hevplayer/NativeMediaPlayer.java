package pku.shengbin.hevplayer;
import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Bitmap.Config;
import android.opengl.GLSurfaceView;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.Surface.OutOfResourcesException;
import android.widget.TextView;

import java.lang.ref.WeakReference;

public class NativeMediaPlayer {   
    public static final int MEDIA_INFO_FRAMERATE_VIDEO = 900;
    public static final int MEDIA_INFO_END_OF_FILE = 909;

	private int mNativeContext; // accessed by native methods
    private static Activity mOwnerActivity;
    private static Surface mSurface;
    private static GLSurfaceView mGLSurfaceView;
    private static TextView mInfoTextView;
    private static Bitmap mFrameBitmap = null;
    private static int mDisplayWidth = 0;
	private static int mDisplayHeight = 0;   
    private static int mDisplayFPS = -1;
    private static int mDisplayAvgFPS = -1;
	private static int mDecodeFPS = -1;
    private static int mBitrateVideo = -1;
	private static int mBitrateAudio = -1; 
    private static boolean mShowInfo = true;
    private static boolean mShowInfoGL = true;
    private static String mInfo = "";
    
    private native final void native_init();
    private native final void native_setup(Object mediaplayer_this);
    private native int native_prepare(int threadNum, float renderFPS);
    private native int native_start();
    private native int native_stop();
    private native int native_pause(); 
    private native int native_go();
    private native int native_seekTo(int msec);

    public NativeMediaPlayer(Activity activity) {
    	native_init();
    	
        /* Native setup requires a weak reference to our object.
         * It's easier to create it here than in C++.
         */
        native_setup(new WeakReference<NativeMediaPlayer>(this));
        
        mOwnerActivity = activity;
    }
    
    private native static int hasNeon();

    static {
    	System.loadLibrary("lenthevcdec");
    	System.loadLibrary("ffmpeg");
    	System.loadLibrary("jniplayer"); 
    }
    
    public void setDisplay(SurfaceHolder sh) {
        if (sh != null) {
            mSurface = sh.getSurface();
        }
        else
            mSurface = null;
    }
    
    public void setGLDisplay(GLSurfaceView glView, TextView tv) {
            mGLSurfaceView = glView;
            mInfoTextView = tv;
    }
    
    public void setDisplaySize(int w, int h) {
		mDisplayHeight = h;
		mDisplayWidth = w;
	}
    
    public native int setDataSource(String path);
        
    public native int getVideoWidth();

    public native int getVideoHeight();

    public native boolean isPlaying();

    public native int getCurrentPosition();

    public native int getDuration();


    public void prepare() {    
    	// android maintains the preferences for us, so use directly
		SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(mOwnerActivity);  
		int num = Integer.parseInt(settings.getString("multi_thread", "0"));
		if ( 0 == num ) {
			int cores = Runtime.getRuntime().availableProcessors();
			if ( cores <= 1 )
				num = 1;
			else
				num = (cores < 4) ? (cores * 2) : 8;
			Log.d("NativeMediaPlayer", cores + " cores detected! use " + num + " threads.\n");
		}
		
		float fps = Float.parseFloat(settings.getString("render_fps", "0"));
		
        native_prepare(num, fps);
    }
    
    public void start() {
    	int w = getVideoWidth(), h = getVideoHeight();
    	if (w > 0 && h > 0 ) 
    		mFrameBitmap = Bitmap.createBitmap(w, h, Config.RGB_565);   	
    	native_start();
    }

    public void stop() {
    	native_stop();
    }

    public void pause() {
    	native_pause();
    }
    
    public void go() {
    	native_go();
    }
    
    public void seekTo(int msec) {
    	
    }
    
    public void setShowInfo (boolean show) {
    	mShowInfo = show;
    	if (mShowInfo == false && mInfoTextView != null) {
			mInfoTextView.setText("");
    	}
    }
	
	private native static void renderBitmap(Bitmap  bitmap);
	
	public static int drawFrame(int width, int height) {
		
		SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(mOwnerActivity);
        boolean useGL = settings.getBoolean("opengl_switch", true);
        
        if (useGL) {
        	
        	mGLSurfaceView.requestRender();
            
            if (mShowInfoGL) {
            	mInfo = "";
    			Paint paint = new Paint();
    			paint.setColor(Color.WHITE);
    			paint.setTextSize(40);
    			if (width > 0) {
    				mInfo += ("Video Size:" + width + "x" + height);
    			}
    			if (mDisplayFPS > 0) {
    				mInfo += ("    Display FPS:" + mDisplayFPS);
    			}
    			if (mDisplayAvgFPS > 0) {
    				mInfo += String.format("    Average FPS:%.2f", mDisplayAvgFPS / 4096.0);
    			}
    			
    			mOwnerActivity.runOnUiThread(new Runnable() {
    				@Override
    				public void run() {
    					// TODO Auto-generated method stub
    					mInfoTextView.setText(mInfo);
    				}
    			});

    			mShowInfoGL = false;
    		}
    		
            return 0;
        }
        
        // draw without OpenGL
		Canvas canvas = null;
		try {
			canvas = mSurface.lockCanvas(null);
		} catch (IllegalArgumentException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (OutOfResourcesException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}	
		
		canvas.drawColor(Color.BLACK);
		
    	if ( null == mFrameBitmap || mFrameBitmap.getWidth() != width) {
    		// video size has changed, we need to create a new frame bitmap correspondingly
    		mFrameBitmap = Bitmap.createBitmap(width, height, Config.RGB_565);	
    	}
    	
    	renderBitmap(mFrameBitmap);
		
		if (mDisplayWidth != mFrameBitmap.getWidth()) {
			Matrix matrix = new Matrix();
		    float scaleWidth = ((float) mDisplayWidth) / width;
		    float scaleHeight = ((float) mDisplayHeight) / height;
		    matrix.postScale(scaleWidth, scaleHeight);
		    matrix.postTranslate((canvas.getWidth() - mDisplayWidth)/2, (canvas.getHeight() - mDisplayHeight)/2);
		    if (mFrameBitmap.getWidth() < 640) {
		    	// small bitmap, able to use filter
			    Paint paint = new Paint();
		    	paint.setFilterBitmap(true);
		    	canvas.drawBitmap(mFrameBitmap, matrix, paint);
		    } else {
		    	canvas.drawBitmap(mFrameBitmap, matrix, null);
		    }
		} else {
			canvas.drawBitmap(mFrameBitmap,(canvas.getWidth() - mDisplayWidth)/2, (canvas.getHeight() - mDisplayHeight)/2,null);
		}
		
		if (mShowInfo) {
			Paint paint = new Paint();
			paint.setColor(Color.WHITE);
			paint.setTextSize(40);
			String info = "";
			if (width > 0) {
				info += ("Video Size:" + width + "x" + height);
			}
			if (mDisplayFPS > 0) {
				info += ("    Display FPS:" + mDisplayFPS);
			}
			if (mDisplayAvgFPS > 0) {
				info += String.format("    Average FPS:%.2f", mDisplayAvgFPS / 4096.0);
			}
			if (mDecodeFPS > 0) {
				info += ("    Decode FPS:" + mDecodeFPS);
			}
			canvas.drawText(info, 20, 60, paint);
			info = "";
			if (mBitrateVideo > 0) {
				info += "Bitrate: video " + Integer.toString(mBitrateVideo);
			}
			if (mBitrateAudio > 0) {
				info += ", audio " + Integer.toString(mBitrateAudio);
			}
			if (mBitrateVideo > 0 || mBitrateAudio > 0) {
				info += ", total " + Integer.toString(mBitrateVideo + mBitrateAudio) + " kbit/s";
			}
			canvas.drawText(info, 20, 100, paint);
		}

        mSurface.unlockCanvasAndPost(canvas);
        
        return 0;
	}
	
	
	/**
     * Called from native code when an interesting event happens.  This method
     * just uses the EventHandler system to post the event back to the main app thread.
     * We use a weak reference to the original MediaPlayer object so that the native
     * code is safe from the object disappearing from underneath it.  (This is
     * the cookie passed to native_setup().)
     */
	public static void postEventFromNative(int what, int arg1, int arg2) {
    	switch(what) {
    	case MEDIA_INFO_FRAMERATE_VIDEO:
    		mDisplayFPS = arg1;
    		mDisplayAvgFPS = arg2;
    		if (mShowInfo) {
    			mShowInfoGL = true;
    		}
    		break;
    	case MEDIA_INFO_END_OF_FILE:
    		mOwnerActivity.finish();
    		break;
    	}
    }

}
