package pku.shengbin.utils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class HttpAccessor {

	/**
	 * 根据URL下载文件，前提是这个文件中的内容是文本，函数的返回值就是文件的内容 1.创建一个URL对象
	 * 2.通过URL对象，创建一个HttpURLConnection对象 3.得到InputStream 4.从InputStream中读取数据
	 */
	public String getContentFromUrl(String strUrl) {
		StringBuffer buffer = new StringBuffer();
		String line = null;
		BufferedReader reader = null;
		try {
			// 创建一个URL对象
			URL url = new URL(strUrl);
			// 创建一个Http连接
			HttpURLConnection urlConn = (HttpURLConnection) url
					.openConnection();
			reader = new BufferedReader(new InputStreamReader(
					urlConn.getInputStream()));
			// 使用IO流读取数据
			while ((line = reader.readLine()) != null) {
				buffer.append(line);
			}

		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				reader.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		return buffer.toString();
	}
}
