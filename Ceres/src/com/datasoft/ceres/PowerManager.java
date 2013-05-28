package com.datasoft.ceres;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.TaskStackBuilder;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;

public class PowerManager extends BroadcastReceiver {
	@Override
	public void onReceive(Context ctx, Intent intent) {
		String action = intent.getAction();
		if(action != null)
		{
			if(action.equalsIgnoreCase(Intent.ACTION_BATTERY_LOW))
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
						.setContentText("Battery is low, halting refresh. Refreshing can still be performed manually.")
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
			else if(action.equalsIgnoreCase(Intent.ACTION_BATTERY_OKAY))
			{
				if(CeresClient.isInForeground())
				{
					Toast.makeText(ctx, "Restarting grid refresh", Toast.LENGTH_SHORT).show();
				}
				else
				{
					Notification.Builder build = new Notification.Builder(ctx)
						.setSmallIcon(R.drawable.ic_launcher)
						.setContentTitle("Restarting grid refresh")
						.setContentText("Battery is out of critical area, restarting grid refresh")
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
				GridActivity.startThread();
			}
		}
	}
}
