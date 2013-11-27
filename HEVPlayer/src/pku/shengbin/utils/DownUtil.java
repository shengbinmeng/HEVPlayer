package pku.shengbin.utils;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.net.HttpURLConnection;
import java.net.URL;


public class DownUtil {
	private String path;
	private String targetFile;
	private int threadNum;
	private DownloadThread[] threads;
	private int fileSize;

	private boolean stop = false;

	public DownUtil(String path, String targetFile, int threadNum) {
		this.path = path;
		this.threadNum = threadNum;
		threads = new DownloadThread[threadNum];
		this.targetFile = targetFile;
	}

	public void stopDownLoad() {
		stop = true;
	}
	
	public void deleteDownloadFile() {
		File delFile = new File(targetFile);
		if (delFile.exists()) {
			delFile.delete();
		}
	}

	public void download() throws Exception {
		URL url = new URL(path);
		HttpURLConnection conn = (HttpURLConnection) url.openConnection();
		conn.setConnectTimeout(5 * 1000);
		conn.setRequestMethod("GET");
		conn.setRequestProperty(
				"Accept",
				"image/gif, image/jpeg, image/pjpeg, image/pjpeg, application/x-shockwave-flash, application/xaml+xml, application/vnd.ms-xpsdocument, application/x-ms-xbap, application/x-ms-application, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*");
		conn.setRequestProperty("Accept-Language", "zh-CN");
		conn.setRequestProperty("Charset", "UTF-8");
		conn.setRequestProperty(
				"User-Agent",
				"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.2; Trident/4.0; .NET CLR 1.1.4322; .NET CLR 2.0.50727; .NET CLR 3.0.04506.30; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)");
		conn.setRequestProperty("Connection", "Keep-Alive");
		fileSize = conn.getContentLength();
		conn.disconnect();
		int currentPartSize = fileSize / threadNum + 1;
		RandomAccessFile file = new RandomAccessFile(targetFile, "rw");
		file.setLength(fileSize);
		file.close();
		for (int i = 0; i < threadNum; i++) {
			int startPos = i * currentPartSize;
			RandomAccessFile currentPart = new RandomAccessFile(targetFile,
					"rw");
			currentPart.seek(startPos);
			threads[i] = new DownloadThread(startPos, currentPartSize,
					currentPart);
			threads[i].start();
		}
	}

	public double getCompleteRate() {
		int sumSize = 0;
		for (int i = 0; i < threadNum; i++) {
			sumSize += threads[i].length;
		}
		return sumSize * 1.0 / fileSize;
	}

	private class DownloadThread extends Thread {
		private int startPos;
		private int currentPartSize;
		private RandomAccessFile currentPart;
		public int length;

		public DownloadThread(int startPos, int currentPartSize,
				RandomAccessFile currentPart) {
			this.startPos = startPos;
			this.currentPartSize = currentPartSize;
			this.currentPart = currentPart;
		}

		@Override
		public void run() {
			try {
				URL url = new URL(path);
				HttpURLConnection conn = (HttpURLConnection) url
						.openConnection();
				conn.setConnectTimeout(5 * 1000);
				conn.setRequestMethod("GET");
				conn.setRequestProperty(
						"Accept",
						"image/gif, image/jpeg, image/pjpeg, image/pjpeg, application/x-shockwave-flash, "
								+ "application/xaml+xml, application/vnd.ms-xpsdocument, application/x-ms-xbap, "
								+ "application/x-ms-application, application/vnd.ms-excel, application/vnd.ms-powerpoint, "
								+ "application/msword, */*");
				conn.setRequestProperty("Accept-Language", "zh-CN");
				conn.setRequestProperty("Charset", "UTF-8");
				InputStream inStream = conn.getInputStream();
				inStream.skip(this.startPos);
				byte[] buffer = new byte[1024];
				int hasRead = 0;
				while (length < currentPartSize
						&& (hasRead = inStream.read(buffer)) != -1) {
					if (stop)
						break;
					currentPart.write(buffer, 0, hasRead);
					length += hasRead;
				}
				currentPart.close();
				inStream.close();
				if(stop)
					deleteDownloadFile();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
	/*
	 * public void stopDownload(){ stop=true; } public boolean isDownload(){
	 * return !stop; }
	 */
}