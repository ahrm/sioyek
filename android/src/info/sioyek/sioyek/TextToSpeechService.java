package info.sioyek.sioyek;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;

import android.app.NotificationManager;
import android.app.NotificationChannel;
import androidx.core.app.NotificationCompat;
import android.content.Context;

import androidx.annotation.Nullable;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.media3.common.MediaMetadata;
import androidx.media3.common.Player;
import androidx.media3.common.SimpleBasePlayer;
import androidx.media3.exoplayer.source.SilenceMediaSource;
import androidx.media3.session.MediaSession;
import androidx.media3.session.MediaSessionService;
import androidx.media3.session.SessionCommand;
import androidx.media3.session.SessionCommands;

import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.ListenableFuture;

import java.util.Collections;
import java.util.ArrayList;


class CustomPlayer extends SimpleBasePlayer{

    private State state = new State.Builder()
            .setAvailableCommands(new Commands.Builder().addAll(
             COMMAND_PLAY_PAUSE,
             COMMAND_GET_CURRENT_MEDIA_ITEM,
             COMMAND_GET_MEDIA_ITEMS_METADATA).build())
            .setPlayWhenReady(true, PLAY_WHEN_READY_CHANGE_REASON_USER_REQUEST)
            .setPlaylist(Collections.singletonList(new MediaItemData.Builder("test").build()))
            .setPlaylistMetadata(new MediaMetadata.Builder().setMediaType(MediaMetadata.MEDIA_TYPE_PLAYLIST).setTitle("tts test").build())
            .setCurrentMediaItemIndex(0)
            .setContentPositionMs(0)
            .build();
    public CustomPlayer(Looper looper){
        super(looper);
    }

    public void updatePlaybackState(int playbackState, Boolean playWhenReady){
        Handler mainHandler = new Handler(Looper.getMainLooper());
        mainHandler.post(() -> {
            state = state.buildUpon()
                    .setPlaybackState(playbackState)
                    .setPlayWhenReady(playWhenReady, Player.PLAY_WHEN_READY_CHANGE_REASON_USER_REQUEST)
                    .build();
            invalidateState();
        });
    }

    @Override
    protected State getState() {
        return state;

    }

    @Override
    protected ListenableFuture<?> handleSetPlayWhenReady(boolean playWhenReady) {
        return Futures.immediateVoidFuture();

    }
}
public class TextToSpeechService extends MediaSessionService {

    private MediaSession mediaSession = null;
    private SilenceMediaSource mediaSource = null;

    private TextToSpeech tts;
    private boolean ttsInitialized = false;
    private int pauseLocation = 0;
    private int tempPauseLocation = 0;
    private String spokenText = "";
    // private String restOfDocument = "";
    private ArrayList<String> restOfDocument = new ArrayList<String>();
    private int restIndex = 0; 
    boolean shouldHideNotification = false;

    private int MAX_SPEECH_SIZE = 4000;

    private String getPlayerStateString(int state){
        if (state == Player.STATE_READY) return "Ready";
        if (state == Player.STATE_ENDED) return "Ended";
        if (state == Player.STATE_IDLE) return "Idle";
        if (state == Player.STATE_BUFFERING) return "Buffering";
        return "Unknown";
    }

    @Override
    public void onCreate() {
        super.onCreate();
        CustomPlayer player = new CustomPlayer(getMainLooper());

        player.prepare();

        mediaSession = new MediaSession.Builder(this, player)
                .setId("sioyek")
                .setCallback(new MediaSession.Callback() {

                    @Override
                    public int onPlayerCommandRequest(MediaSession session, MediaSession.ControllerInfo controller, int playerCommand) {

                        boolean isFromSioyek = controller.getPackageName().equals(getApplicationContext().getPackageName()); 

                        if (playerCommand == SimpleBasePlayer.COMMAND_PLAY_PAUSE){

                            if (tts.isSpeaking()){
                                pauseLocation = tempPauseLocation + pauseLocation;
                                tts.stop();
                                player.updatePlaybackState(Player.STATE_ENDED, true);
                                publishExternalTtsState(getPlayerStateString(Player.STATE_ENDED));
                            }
                            else{
                                String currentText = spokenText;
                                if (restIndex > 0){
                                    currentText = restOfDocument.get(restIndex - 1);
                                }

                                tts.speak(currentText.substring(pauseLocation), TextToSpeech.QUEUE_FLUSH, null, "hi");
                                player.updatePlaybackState(Player.STATE_READY, true);
                                publishExternalTtsState(getPlayerStateString(Player.STATE_READY));
                            }
                        }
                        return MediaSession.Callback.super.onPlayerCommandRequest(session, controller, playerCommand);
                    }
                })
                .build();

        tts = new TextToSpeech(this, new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int status) {
                if (status == TextToSpeech.SUCCESS) {
                    MAX_SPEECH_SIZE = tts.getMaxSpeechInputLength();
                    ttsInitialized = true;
                    tts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                        @Override
                        public void onStart(String s) {
                            publishTtsState("Speaking");
                        }

                        @Override
                        public void onDone(String s) {
                            publishTtsState("Ready");

                            if (restIndex < restOfDocument.size()){
                                tts.speak(restOfDocument.get(restIndex), TextToSpeech.QUEUE_FLUSH, null, "hi");
                                pauseLocation = 0;
                                tempPauseLocation = 0;
                                restIndex++;
                            }
                            // if (restOfDocument.length() > 0){
                            //     tts.speak(restOfDocument, TextToSpeech.QUEUE_FLUSH, null, "hi");
                            //     restOfDocument = "";
                            // }
                        }

                        @Override
                        public void onRangeStart(String utteranceId, int start, int end, int frame){
                            tempPauseLocation = start;
                            sendMessage(pauseLocation + start, pauseLocation + end);
                        }

                        @Override
                        public void onError(String s) {
                            publishTtsState("Error");
                        }
                    });
                }
            }
        });
    }


    @Override
    public int onStartCommand (Intent intent, int flags, int startId) {


        if (intent.getStringExtra("text") != null){
            spokenText = intent.getStringExtra("text");
            pauseLocation = 0;
            tempPauseLocation = 0;
        }
        if (intent.hasExtra("rate")){
            float rate = intent.getFloatExtra("rate", 1.0f);
            tts.setSpeechRate(rate);
        }
        if (intent.hasExtra("resume")){
            handleResumeState();
        }
        if (intent.hasExtra("stop")){
        }
        if (intent.hasExtra("rest")){
            String rest = intent.getStringExtra("rest");
            setRestText(rest);
        }
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onTaskRemoved(@Nullable Intent rootIntent) {
        Player player = mediaSession.getPlayer();
        if (!player.getPlayWhenReady() || player.getMediaItemCount() == 0) {
            stopForeground(true);
            stopSelf();
        }
    }

    @Override
    public void onDestroy() {
        mediaSession.release();
        mediaSession = null;
        super.onDestroy();
    }

    @Override
    public MediaSession onGetSession(MediaSession.ControllerInfo controllerInfo){
        SessionCommands commands = new SessionCommands.Builder().add(new SessionCommand("set_text", new Bundle())).build();
        Player.Commands playerCommands = new Player.Commands.Builder().addAll(
            Player.COMMAND_PLAY_PAUSE,
             Player.COMMAND_GET_CURRENT_MEDIA_ITEM,
             Player.COMMAND_GET_MEDIA_ITEMS_METADATA).build();
        mediaSession.setAvailableCommands(controllerInfo, commands, playerCommands);
        return mediaSession;
    }

    private void sendMessage(int begin, int end){
        Intent intent = new Intent("sioyek_tts");
        intent.putExtra("begin", begin);
        intent.putExtra("end", end);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void publishTtsState(String state){
        Intent intent = new Intent("sioyek_tts_state");
        intent.putExtra("state", state);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void publishExternalTtsState(String state){
        Intent intent = new Intent("sioyek_external_tts_state");
        intent.putExtra("state", state);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    int findFirstSpaceBefore(String text, int index){
        for (int i = index; i > 0; i--){
            if (text.charAt(i) == ' '){
                return i;
            }
        }
        return 0;
    }

    void setRestText(String rest){
        while (rest.length() > MAX_SPEECH_SIZE){
            int index = findFirstSpaceBefore(rest, MAX_SPEECH_SIZE);
            restOfDocument.add(rest.substring(0, index));
            rest = rest.substring(index);
        }
        if (rest.length() > 0){
            restOfDocument.add(rest);
        }
        restIndex = 0;
    }

    void handleResumeState(){
        boolean isOnRest = restIndex > 0;
        boolean isPlaying = tts.isSpeaking();
        int offset = tempPauseLocation + pauseLocation;
        if (isOnRest){
            for (int i = 0; i < restIndex-1; i++){
                offset += restOfDocument.get(i).length();
            }
        }
        tts.stop();
        pauseLocation = 0;
        tempPauseLocation = 0;
        restOfDocument.clear();
        restIndex = 0;
        SioyekActivity.onResumeState(isPlaying, isOnRest, offset);
    }

}
