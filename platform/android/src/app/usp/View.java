/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2011 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package app.usp;

import java.nio.ByteBuffer;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.media.AudioTrack;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.content.Context;

public class View extends SurfaceView  implements Callback
{
	private SurfaceHolder sh = null;
	private Bitmap bmp = Bitmap.createBitmap(320, 240, Bitmap.Config.RGB_565);
	private ByteBuffer buf_video = ByteBuffer.allocateDirect(320*240*2);
	private ByteBuffer buf_audio = ByteBuffer.allocateDirect(32768);
	private static class Lock {};
	private final Object lock_scr = new Lock();
	private AudioTrack audio;
	private byte[] aud = new byte[32768];
	private boolean paused = false;
	public View(Context context)
	{
		super(context);
		getHolder().addCallback(this);
		final int freq = 44100;
		final int channels = AudioFormat.CHANNEL_CONFIGURATION_STEREO;
		final int format = AudioFormat.ENCODING_PCM_16BIT;
		final int buf_size = AudioTrack.getMinBufferSize(freq, channels, format);
		audio = new AudioTrack(	AudioManager.STREAM_MUSIC,
								freq, channels, format, buf_size*4,
								AudioTrack.MODE_STREAM);
		audio.play();
		StartRenderThread();
	}
	private void StartRenderThread()
	{
		Thread t = new Thread(new Runnable()
		{
			public void run()
			{
				for(;;)
				{
					if(!paused && sh != null)
						Draw();
					else
						Thread.yield();
				}
			}
		},"draw");
		t.start();
	}
	public void surfaceCreated(SurfaceHolder holder)
	{
		synchronized(lock_scr) { sh = holder; }
	}
	public void surfaceDestroyed(SurfaceHolder holder)
	{
		synchronized(lock_scr) { sh = null; }
	}
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
	{
		synchronized(lock_scr) { sh = holder; }
	}
	@Override
	protected void onMeasure(int w, int h)
	{
		setMeasuredDimension(320, 240);
	}
	private void Draw()
	{
		Emulator.the.UpdateVideo(buf_video);
		buf_video.rewind();
		bmp.copyPixelsFromBuffer(buf_video);
		final int audio_bytes_ready = Emulator.the.UpdateAudio(buf_audio);
		if(audio_bytes_ready != 0)
		{
			buf_audio.rewind();
			buf_audio.get(aud);
			audio.write(aud, 0, audio_bytes_ready);
		}
		synchronized(lock_scr)
		{
			if(sh != null)
			{
				Canvas c = sh.lockCanvas();
				c.drawBitmap(bmp, 0, 0, null);
				sh.unlockCanvasAndPost(c);
			}
		}
	}
	public void OnPause()
	{
		synchronized(lock_scr) { paused = true; }
	}
	public void OnResume()
	{
		synchronized(lock_scr) { paused = false; }
	}
}
