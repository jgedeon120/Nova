package com.datasoft.ceres;
import com.datasoft.ceres.R;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.TaskStackBuilder;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.widget.Toast;


public class NetworkUpdateReceiver extends BroadcastReceiver {
	@Override
	public void onReceive(Context ctx, Intent intent) {
		ConnectivityManager cm = (ConnectivityManager)ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo netInfo = cm.getActiveNetworkInfo();
		if(netInfo == null || !netInfo.isConnected())
		{
			if(CeresClient.isInForeground())
			{
				Toast.makeText(ctx, "Halting grid refresh", Toast.LENGTH_SHORT).show();
			}
			else
			{
				Notification.Builder build = new Notification.Builder(ctx)
					.setSmallIcon(R.drawable.ic_launcher)
					.setContentTitle("Halting grid refresh")
					.setContentText("Device is not connected to the network")
					.setAutoCancel(true);
				Intent target = new Intent(ctx, MainActivity.class);
				TaskStackBuilder tsbuild = TaskStackBuilder.create(ctx);
				tsbuild.addParentStack(MainActivity.class);
				tsbuild.addNextIntent(target);
				PendingIntent rpintent = tsbuild.getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT);
				build.setContentIntent(rpintent);
				NotificationManager nm = (NotificationManager)ctx.getSystemService(Context.NOTIFICATION_SERVICE);
				nm.notify(0, build.build());
			}
			GridActivity.cancelThread();
		}
	}
}
