
package info.sioyek.sioyek;

import org.qtproject.qt.android.QtNative;

import org.qtproject.qt.android.bindings.QtActivity;
import android.os.*;
import android.provider.Settings;
import android.os.Environment;
import android.database.Cursor;
import android.provider.MediaStore;
import android.provider.DocumentsContract;
import android.content.*;
import android.app.*;
import android.view.WindowManager;
import android.widget.Toast;
import android.net.Uri;
import android.provider.OpenableColumns;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.String;
import android.content.Intent;
import java.io.File;

import java.lang.String;
import android.content.Intent;
import java.io.File;
import android.net.Uri;
import android.util.Log;
import android.content.ContentResolver;
import android.webkit.MimeTypeMap;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.media3.session.MediaController;
import androidx.media3.session.SessionToken;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.MoreExecutors;

import android.view.Menu;
import android.view.MenuItem;



public class SioyekActivity extends QtActivity{
    public static native void setFileUrlReceived(String url);
    public static native void qDebug(String msg);
    public static native void onTts(int begin, int end);
    public static native void onTtsStateChange(String newState);
    public static native void onExternalTtsStateChange(String newState);
    public static native String getRestOnPause();
    public static native void onResumeState(boolean isPlaying, boolean readingRest, int offset);

    public static boolean isIntentPending;
    public static boolean isInitialized;
    public static boolean isPaused = true;

    private static SioyekActivity instance = null;

    private MediaController mediaController = null;
    private SessionToken ttsSessionToken = null;

    private BroadcastReceiver messageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int begin = intent.getIntExtra("begin", 0);
            int end = intent.getIntExtra("end", 0);
            onTts(begin, end);
        }
    };

    private BroadcastReceiver stateMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String state = intent.getStringExtra("state");
            onTtsStateChange(state);
        }
    };

    private BroadcastReceiver externalStateMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String state = intent.getStringExtra("state");
            onExternalTtsStateChange(state);
            //onTtsStateChange(state);
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);


        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Intent intent = getIntent();

        if (intent != null){
            String action = intent.getAction();
            if (action != null){
                Uri intentUri = intent.getData();
                if (intentUri != null){
                    if (intentUri.toString().startsWith("content://") && (!intentUri.toString().startsWith("content://com.android")) && (!intentUri.toString().startsWith("content://media")) && (intentUri.toString().indexOf("@media") == -1)){
                        //Toast.makeText(this, "Opening files from other apps is not supported. Download the file and open it from file manager.", Toast.LENGTH_LONG).show();

                        Intent viewIntent = new Intent(getApplicationContext(), SioyekActivity.class);
                        // viewIntent.setUri(intentUri);
                        viewIntent.setAction(Intent.ACTION_VIEW);
                        viewIntent.putExtra("sharedData", intentUri.toString());

                        startActivity(viewIntent);
                        if (instance != null){
                            finish();
                        }
                    }
                    isIntentPending = true;
                }
            }
        }

        instance = this;
        if(!Environment.isExternalStorageManager()){

            // Uri uri = Uri.parse("package:" + BuildConfig.APPLICATION_ID);
            Uri uri = Uri.parse("package:" + "org.qtproject.example");
            try {
                Intent newActivityIntent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, uri);

                startActivity(
                    newActivityIntent
                );
            }
            catch(Exception e){
            }
        }

    }

    @Override
    public void onStart(){
        super.onStart();
        ttsSessionToken = new SessionToken(getApplicationContext(), new ComponentName(getApplicationContext(), TextToSpeechService.class));
        ListenableFuture<MediaController> controllerFuture = new MediaController.Builder(getApplicationContext(), ttsSessionToken).buildAsync();
        controllerFuture.addListener(() -> {
            try{
                mediaController = controllerFuture.get();
            }
            catch(Exception e){
                qDebug("sioyek: could not get media controller");
            }
        }, MoreExecutors.directExecutor());
        LocalBroadcastManager.getInstance(this).registerReceiver(messageReceiver, new IntentFilter("sioyek_tts"));
        LocalBroadcastManager.getInstance(this).registerReceiver(stateMessageReceiver, new IntentFilter("sioyek_tts_state"));
        LocalBroadcastManager.getInstance(this).registerReceiver(externalStateMessageReceiver, new IntentFilter("sioyek_external_tts_state"));
    }

    @Override
    public void onStop(){

        super.onStop();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(messageReceiver);
        LocalBroadcastManager.getInstance(this).unregisterReceiver(stateMessageReceiver);
        LocalBroadcastManager.getInstance(this).unregisterReceiver(externalStateMessageReceiver);
    }

    @Override
    public void onResume(){

        isPaused = false;
        Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
        intent.putExtra("resume", true);
        startService(intent);

        super.onResume();
    }

    @Override
    public void onPause(){
        isPaused = true;
        String rest = getRestOnPause();

        if (rest.length() > 0){
            setTtsRestOfDocument(rest);
        }

        super.onPause();
    }

    @Override
    public void onNewIntent(Intent intent){
        super.onNewIntent(intent);
        setIntent(intent);
        if (isInitialized){
            processIntent();
        }
        else{
            isIntentPending = true;
        }
    }

    public void checkPendingIntents(String workingDir){
        isInitialized = true;
        if (isIntentPending){
            isIntentPending = false;
            processIntent();
        }
    }

    public static File getFile(Context context, Uri uri) throws IOException {
        File destinationFilename = new File(context.getFilesDir().getPath() + File.separatorChar + queryName(context, uri));

        if (destinationFilename.exists()){
            return destinationFilename;
        }

        try (InputStream ins = context.getContentResolver().openInputStream(uri)) {
            createFileFromStream(ins, destinationFilename);
        } catch (Exception ex) {
            Log.e("Save File", ex.getMessage());
            ex.printStackTrace();
        }
        return destinationFilename;
    }

    public static void createFileFromStream(InputStream ins, File destination) {
        try (OutputStream os = new FileOutputStream(destination)) {
            byte[] buffer = new byte[4096];
            int length;
            while ((length = ins.read(buffer)) > 0) {
                os.write(buffer, 0, length);
            }
            os.flush();
        } catch (Exception ex) {
            Log.e("Save File", ex.getMessage());
            ex.printStackTrace();
        }
    }

    private static String queryName(Context context, Uri uri) {
        Cursor returnCursor =
                context.getContentResolver().query(uri, null, null, null, null);
        assert returnCursor != null;
        int nameIndex = returnCursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
        returnCursor.moveToFirst();
        String name = returnCursor.getString(nameIndex);
        returnCursor.close();
        return name;
    }

    public static String getRealPathFromUri(Context context, Uri contentUri) throws IOException{
        Cursor cursor = null;
        try {
            String[] proj = { MediaStore.Images.Media.DATA };
            cursor = context.getContentResolver().query(contentUri, proj, null, null, null);
            int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
            cursor.moveToFirst();
            return cursor.getString(column_index);
        }
        catch(Exception e){
            String newFileName=  getFile(context, contentUri).getPath();
            return getFile(context, contentUri).getPath();
        }
        finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    public static String getPathFromUri(Context context, Uri uri) {
        final boolean isKitKat = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;

        // DocumentProvider
        if (isKitKat && DocumentsContract.isDocumentUri(context, uri)) {
            // ExternalStorageProvider
            if (isExternalStorageDocument(uri)) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                if ("primary".equalsIgnoreCase(type)) {
                    return Environment.getExternalStorageDirectory() + "/" + split[1];
                }

                // TODO handle non-primary volumes
            }
            // DownloadsProvider
            else if (isDownloadsDocument(uri)) {

                final String id = DocumentsContract.getDocumentId(uri);
                final Uri contentUri = ContentUris.withAppendedId(
                        Uri.parse("content://downloads/public_downloads"), Long.valueOf(id));

                return getDataColumn(context, contentUri, null, null);
            }
            // MediaProvider
            else if (isMediaDocument(uri)) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                Uri contentUri = null;
                if ("image".equals(type)) {
                    contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                } else if ("video".equals(type)) {
                    contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                } else if ("audio".equals(type)) {
                    contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                }

                final String selection = "_id=?";
                final String[] selectionArgs = new String[] {
                        split[1]
                };

                return getDataColumn(context, contentUri, selection, selectionArgs);
            }
        }
        // MediaStore (and general)
        else if ("content".equalsIgnoreCase(uri.getScheme())) {

            // Return the remote address
            if (isGooglePhotosUri(uri))
                return uri.getLastPathSegment();

            return getDataColumn(context, uri, null, null);
        }
        // File
        else if ("file".equalsIgnoreCase(uri.getScheme())) {
            return uri.getPath();
        }

        return null;
    }

    public static String getDataColumn(Context context, Uri uri, String selection,
                                       String[] selectionArgs) {

        Cursor cursor = null;
        final String column = "_data";
        final String[] projection = {
                column
        };

        try {
            cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs,
                    null);
            if (cursor != null && cursor.moveToFirst()) {
                final int index = cursor.getColumnIndexOrThrow(column);
                return cursor.getString(index);
            }
        } finally {
            if (cursor != null)
                cursor.close();
        }
        return null;
    }


    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is ExternalStorageProvider.
     */
    public static boolean isExternalStorageDocument(Uri uri) {
        return "com.android.externalstorage.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is DownloadsProvider.
     */
    public static boolean isDownloadsDocument(Uri uri) {
        return "com.android.providers.downloads.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is MediaProvider.
     */
    public static boolean isMediaDocument(Uri uri) {
        return "com.android.providers.media.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is Google Photos.
     */
    public static boolean isGooglePhotosUri(Uri uri) {
        return "com.google.android.apps.photos.content".equals(uri.getAuthority());
    }

    private void processIntent(){

        Intent intent = getIntent();
        if (intent.getAction().equals("android.intent.action.VIEW")){
            Uri intentUri = intent.getData();
            if (intentUri == null){
                intentUri = Uri.parse(intent.getStringExtra("sharedData"));
            }
            //String realPath = getRealPathFromUri(getApplicationContext(), intentUri);
            //Uri newUri = Uri.fromFile(new File(realPath));

            //setFileUrlReceived(intentUri.toString());
            String realPath = "";
            try{
                realPath = getRealPathFromUri(this, intentUri);
                //Toast.makeText(this, "trying to open " + realPath, Toast.LENGTH_LONG).show();
                setFileUrlReceived(realPath);
            }
            catch(IOException e){
                qDebug("sioyek: could not open" + realPath);
            }
        }
        return;
    }

    public void ttsPause(){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mediaController.pause();
            }
        });

    } 

    public void ttsStop(){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mediaController.stop();
            }
        });
    } 

    public void ttsSetRate(float rate){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                setTtsRate(rate);
            }
        });
    } 

    public void ttsSetRestOfDocument(String text){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                setTtsRestOfDocument(text);
            }
        });
    } 


    public void ttsSay(String text){
        Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
        intent.putExtra("text", text);
        startService(intent);

        Handler handler = new Handler(Looper.getMainLooper());
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mediaController.play();
            }
        }, 100);
    }

    public void setTtsRate(float rate){
        Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
        intent.putExtra("rate", rate);
        startService(intent);
    }

    public void setTtsRestOfDocument(String rest){
        Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
        intent.putExtra("rest", rest);
        startService(intent);
    }

}
