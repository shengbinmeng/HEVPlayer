package pku.shengbin.hevplayer;
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Bitmap.Config;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.Surface.OutOfResourcesException;
import java.lang.ref.WeakReference;

public class NativeMediaPlayer {   
    public static final int MEDIA_INFO_FRAMERATE_VIDEO = 900;
    public static final int MEDIA_INFO_FRAMERATE_AUDIO = 901;
    public static final int MEDIA_INFO_BYTERATE = 902;
    public static final int MEDIA_INFO_DECODE_FPS = 903;

	private int mNativeContext; // accessed by native methods
    private Activity mOwnerActivity;
    private static Surface mSurface;
    private static Bitmap mFrameBitmap;
    private static	int mDisplayWidth = 0;
	private static int mDisplayHeight = 0;   
    private static	int	mDisplayFPS = -1;
	private static int mDecodeFPS = -1;
    private static int	mBitrateVideo = -1;
	private static int mBitrateAudio = -1; 
    private static boolean mShowInfo = true;
    
    private native final void native_init();
    private native final void native_setup(Object mediaplayer_this);
    private native int native_prepare(int threadNum);
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
    	System.loadLibrary("cpufeature");
    	if((hasNeon()) == 1) System.loadLibrary("jniplayer"); 
    	else System.loadLibrary("jniplayer");
    }
    
    public void setDisplay(SurfaceHolder sh) {
        if (sh != null)
            mSurface = sh.getSurface();
        else
            mSurface = null;
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


    public void prepare(int threadNum) {    
        native_prepare(threadNum);
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
    }
	
	private native static void renderBitmap(Bitmap  bitmap);
	
	public static int drawFrame(int width, int height) {
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
		
    	if (mFrameBitmap.getWidth() != width)
    		// video size has changed, we need to create a new frame bitmap correspondingly
    		mFrameBitmap = Bitmap.createBitmap(width, height, Config.RGB_565);	
    	
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
		    }
		    else canvas.drawBitmap(mFrameBitmap, matrix, null);
		}
		else canvas.drawBitmap(mFrameBitmap,(canvas.getWidth() - mDisplayWidth)/2, (canvas.getHeight() - mDisplayHeight)/2,null);
		
		if (mShowInfo) {
			Paint paint = new Paint();
			paint.setColor(Color.BLUE);
			paint.setTextSize(28);
			String info = "";
			if (width > 0)
				info += ("Video Size:" + width + "x" + height);
			if (mDisplayFPS > 0)
				info += ("    Display FPS:" + mDisplayFPS);
			if (mDecodeFPS > 0)
				info += ("    Decode FPS:" + mDecodeFPS);
			canvas.drawText(info, 20, 60, paint);
			info = "";
			if (mBitrateVideo > 0)
				info += "Bitrate: video " + Integer.toString(mBitrateVideo);
			if (mBitrateAudio > 0)
				info += ", audio " + Integer.toString(mBitrateAudio);
			if (mBitrateVideo > 0 || mBitrateAudio > 0) info += ", total " + Integer.toString(mBitrateVideo + mBitrateAudio) + " kbit/s";
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
	public static void postEventFromNative(
                          int what, int arg1, int arg2) {
    	switch(what) {
    	case MEDIA_INFO_FRAMERATE_VIDEO:
    		mDisplayFPS = arg1;
    		break;
    	case MEDIA_INFO_FRAMERATE_AUDIO:
    		break;
    	case MEDIA_INFO_BYTERATE:
    		mBitrateVideo = arg1;
    		mBitrateAudio = arg2;
    		break;
    	case MEDIA_INFO_DECODE_FPS:
    		mDecodeFPS = arg1;
    		break;
    	}
    }

}
