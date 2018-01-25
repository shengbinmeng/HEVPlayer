package pku.shengbin.hevplayer;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;

/**
 * The main activity of the application.
 * It's not used in this project, because the user will be presented 
 * with the local file list when they enter this application.
 * It exists for possible extension in the future.
 */
public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.activity_main, menu);
        return true;
    }
    
}
