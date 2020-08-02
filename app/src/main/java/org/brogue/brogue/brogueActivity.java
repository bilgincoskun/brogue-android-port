package org.brogue.brogue;

import android.Manifest;
import android.annotation.TargetApi;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;

import org.json.JSONException;
import org.json.JSONObject;
import org.brogue.brogue.BuildConfig;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.SocketTimeoutException;
import java.net.URL;

import javax.net.ssl.HttpsURLConnection;


import org.libsdl.app.SDLActivity;

public class brogueActivity extends SDLActivity
{
   @Override
   protected String[] getLibraries() {
       if( Build.VERSION.SDK_INT <= 17){
           return new String[] {
                   "hidapi",
                   "SDL2",
                   "SDL2_ttf",
                   "main"
           };
       }else{
           return super.getLibraries();
       }


   }
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

    private void openDownloadLink(){
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(download_link));
        startActivity(intent);
    }

    private void openManual(){
        String manual_url = "https://github.com/bilgincoskun/brogue-android-port/blob/master/README.md";
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(manual_url));
        startActivity(intent);
    }

    private boolean needsWritePermission(){
        return Build.VERSION.SDK_INT >= 23;
    }

    @TargetApi(23)
    private void grantPermission(){
        if(checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED){
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},10);
        }
    }

    private String configFolder(){

        File f = new File(Environment.getExternalStorageDirectory() + "/Brogue");
        if(!f.exists()){
            if(!f.mkdir()){
                return null;
            }
        }
        return f.toString();
    }
}
