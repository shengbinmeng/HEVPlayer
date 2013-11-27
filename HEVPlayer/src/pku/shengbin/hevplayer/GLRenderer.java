package pku.shengbin.hevplayer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.opengl.GLSurfaceView;

/**
 * @author shengbin
 *
 */
public class GLRenderer implements GLSurfaceView.Renderer {

    private native int nativeInit();
    private native int nativeSetup(int w, int h);
    private native void nativeDrawFrame();
    
	@Override
	public void onDrawFrame(GL10 arg0) {
		nativeDrawFrame();
	}

	@Override
	public void onSurfaceChanged(GL10 arg0, int w, int h) {
	    nativeSetup(w, h);
	}

	@Override
	public void onSurfaceCreated(GL10 arg0, EGLConfig arg1) {
		nativeInit();
	}
}
