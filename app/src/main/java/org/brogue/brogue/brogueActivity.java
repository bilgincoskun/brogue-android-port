package org.brogue.brogue;

import android.content.Intent;
import android.net.Uri;

import org.brogue.brogueCore.brogueCore;
import org.json.JSONException;
import org.json.JSONObject;
import org.brogue.brogue.BuildConfig;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.SocketTimeoutException;
import java.net.URL;

import javax.net.ssl.HttpsURLConnection;

public class brogueActivity extends brogueCore
{
    private String download_link;
    private String gitVersionCheck(){//first character of return value indicates error
        String git_url = "https://api.github.com/repos/bilgincoskun/brogue-android-port/releases/latest";
        try {
            URL url = new URL(git_url);
            HttpsURLConnection urlConnection = (HttpsURLConnection) url.openConnection();
            urlConnection.setConnectTimeout(1500);
            urlConnection.setReadTimeout(1500);
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(urlConnection.getInputStream()));
            String line = bufferedReader.readLine();
            JSONObject json = new JSONObject(line);
            String versions[] = {(String) json.get("tag_name"), BuildConfig.VERSION_NAME};
            for (int i = 0; i < 2; i++) {
                String v = versions[i];
                v = v.toLowerCase().trim();
                if (v.charAt(0) == 'v') {
                    v = v.substring(1);
                }
                versions[i] = v;
            }
            if (!versions[0].equals(versions[1])) {
                download_link = (String) json.get("html_url");
                return ' ' + versions[0];
            }
        }catch (SocketTimeoutException e){
            return "1";
        }catch (JSONException e ) {
            return "2";
        }catch (IOException e){
            return "3";
        }
        return " ";
    }

    private void open_download_link(){
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(download_link));
        startActivity(intent);
    }
}
