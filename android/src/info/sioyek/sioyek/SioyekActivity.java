
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

import java.lang.String;
import android.content.Intent;
import java.io.File;
import android.net.Uri;
import android.util.Log;
import android.content.ContentResolver;
import android.webkit.MimeTypeMap;

public class SioyekActivity extends QtActivity{
    public static native void setFileUrlReceived(String url);
    public static native void qDebug(String msg);

    public static boolean isIntentPending;
    public static boolean isInitialized;

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        if (intent != null){
            String action = intent.getAction();
            if (action != null){
                isIntentPending = true;
            }
        }
        if(!Environment.isExternalStorageManager()){

            // Uri uri = Uri.parse("package:" + BuildConfig.APPLICATION_ID);
            Uri uri = Uri.parse("package:" + "org.qtproject.example");
            try {
                startActivity(
                        new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, uri)
                );
            }
            catch(Exception e){
            }
        }

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

    public static String getRealPathFromUri(Context context, Uri contentUri) {
        Cursor cursor = null;
        try {
            String[] proj = { MediaStore.Images.Media.DATA };
            cursor = context.getContentResolver().query(contentUri, proj, null, null, null);
            int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
            cursor.moveToFirst();
            return cursor.getString(column_index);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    private void processIntent(){

        Intent intent = getIntent();
        if (intent.getAction().equals("android.intent.action.VIEW")){
            //qDebug("jsioyek: " + intent.getAction());
            Uri intentUri = intent.getData();
            //String realPath = getRealPathFromUri(getApplicationContext(), intentUri);
            //Uri newUri = Uri.fromFile(new File(realPath));

            setFileUrlReceived(intentUri.toString());
        }
        return;
    }
}