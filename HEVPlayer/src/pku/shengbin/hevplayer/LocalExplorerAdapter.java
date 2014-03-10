package pku.shengbin.hevplayer;

import java.io.File;
import java.io.FileInputStream;

import pku.shengbin.hevplayer.LocalExploreActivity;
import pku.shengbin.hevplayer.R;
import pku.shengbin.utils.md5;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.media.MediaMetadataRetriever;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

public class LocalExplorerAdapter extends BaseAdapter {
	
	private File[] 							mFiles;
	private LayoutInflater 					mInflater;
	private Context 						mContext;
	private boolean							mHasParrent;
	private String 	rootpath = Environment.getExternalStorageDirectory().getPath();

	public native int getframe(String path, String savename, String dir);
	static {
		System.loadLibrary("lenthevcdec");
		System.loadLibrary("ffmpeg");
		System.loadLibrary("getframe");
	}
	
	public LocalExplorerAdapter(Context context, File[] files, boolean isRoot) {
		mFiles = files;
        mInflater = LayoutInflater.from(context);
        mContext = context;
        mHasParrent = !isRoot;
	}
	
	public int getCount() {
		return mFiles.length;
	}

	public Object getItem(int position) {
		return mFiles[position];
	}

	public long getItemId(int position) {
		return position;
	}
	
	private void setRow(ViewHolder holder, int position) {
		File file = mFiles[position];
		holder.text.setText(file.getName());
		if(position == 0 && mHasParrent) {
			holder.icon.setImageDrawable(mContext.getResources().getDrawable(R.drawable.ic_menu_back));
			holder.text.setText("< Parent Directory > ");
		} 
		else if (file.isDirectory()) {
			holder.icon.setImageDrawable(mContext.getResources().getDrawable(R.drawable.ic_menu_archive));
		} 
		else {
			Drawable d = null;
			if(LocalExploreActivity.checkExtension(file)) {
				d = mContext.getResources().getDrawable(R.drawable.ic_menu_gallery);
				holder.icon.setImageDrawable(d);
				//String rootpath = Environment.getExternalStorageDirectory().getPath();
				LoadImageTask imagetask = new LoadImageTask(file.getPath(),"."+md5.MD5(file.getPath()),rootpath+"/hevplayer",holder.icon);
				imagetask.execute();
			}
			else {
				d = mContext.getResources().getDrawable(R.drawable.ic_menu_block);
				holder.icon.setImageDrawable(d);
			}
			/*
			holder.icon.setImageDrawable(d);
			Bitmap bitmap = null;
			bitmap = createVideoThumbnail(file.getPath());
			if(bitmap != null) {
				holder.icon.setImageBitmap(bitmap);
			}
			*/
		}
	}

	public View getView(int position, View convertView, ViewGroup parent) {
		// A ViewHolder keeps references to children views to avoid unneccessary calls
        // to findViewById() on each row.
        ViewHolder holder;

        // When convertView is not null, we can reuse it directly, there is no need
        // to re-inflate it. We only inflate a new View when the convertView supplied
        // by ListView is null.
        if (convertView == null) {
            convertView = mInflater.inflate(R.layout.movie_explorer_row, null);

            // Creates a ViewHolder and store references to the two children views
            // we want to bind data to.
            holder = new ViewHolder();
            holder.text = (TextView) convertView.findViewById(R.id.textview_rowtext);
            holder.icon = (ImageView) convertView.findViewById(R.id.imageview_rowicon);

            convertView.setTag(holder);
        } else {
            // Get the ViewHolder back to get fast access to the TextView
            // and the ImageView.
            holder = (ViewHolder) convertView.getTag();
        }

		// Bind the data efficiently with the holder.
		setRow(holder, position);
		
        return convertView;
	}
	
	private static class ViewHolder {
	    TextView text;
	    ImageView icon;
	}
    private Bitmap createVideoThumbnail(String filePath) {
        Bitmap bitmap = null;
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        try {
            //retriever.set
            retriever.setDataSource(filePath);
            Log.v("test1", filePath);
            bitmap = retriever.getFrameAtTime();
        } catch(IllegalArgumentException ex) {
            // Assume this is a corrupt video file
        } catch (RuntimeException ex) {
            // Assume this is a corrupt video file.
        } finally {
            try {
                retriever.release();
            } catch (RuntimeException ex) {
                // Ignore failures while cleaning up.
            }
        }
        return bitmap;
    }
    
    class LoadImageTask extends AsyncTask<String, Integer, String> {
    	String file_path;
    	String save_name;
    	String save_dir;
    	ImageView img;
    	File saved_file;
    	
    	public LoadImageTask(String file_path, String save_name, String save_dir, ImageView image)
    	{
    		this.file_path = file_path;
    		this.save_dir = save_dir;
    		this.save_name = save_name;
    		this.img = image;
    		saved_file = new File(save_dir+"/"+save_name+".jpg");
    	}
    	
    	@Override  
        protected void onPreExecute() {  
    		
    	}

		@Override
		protected String doInBackground(String... arg0) {
			// TODO Auto-generated method stub
			if(saved_file.exists())
			{
				return null;
			}
			getframe(file_path,save_name,save_dir);
			return null;
		}
		@Override  
        protected void onPostExecute(String result) {
			FileInputStream fis = null;
			if(saved_file.exists())
			{
					//Bitmap bitmap  = BitmapFactory.decodeStream(fis);
				BitmapFactory.Options opts = new BitmapFactory.Options();  
				opts.inSampleSize = 5;  
				Bitmap bitmap  =  BitmapFactory.decodeFile(saved_file.getPath());
				img.setImageBitmap(bitmap);
			}
		}
		@Override  
        protected void onCancelled() {  
    		
    	}
    }
    
}

