package com.datasoft.ceres;

import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;

public class MainActivity extends Activity {
	Context m_ctx;
	Button m_connect;
	EditText m_id;
	EditText m_ip;
	EditText m_port;
	EditText m_passwd;
	EditText m_notify;
	EditText m_success;
	EditText m_keystorePass;
	CheckBox m_keepLogged;
	CheckBox m_useSSC;
	ProgressDialog m_dialog;
	boolean m_keepMeLoggedIn;
	CeresClientConnect m_ceresClient;
	SharedPreferences m_prefs;
	String m_regexIp = "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)";
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        m_ctx = this;
        m_ceresClient = new CeresClientConnect();
        CeresClient.setForeground(true);
        m_id = (EditText)findViewById(R.id.credID);
        m_passwd = (EditText)findViewById(R.id.credPW);
        m_ip = (EditText)findViewById(R.id.credIP);
        m_port = (EditText)findViewById(R.id.credPort);
        m_keepMeLoggedIn = false;
        m_prefs = this.getSharedPreferences(CeresClient.SHAREDPREFS_FILE, Context.MODE_PRIVATE);
        
        if(m_prefs.contains(CeresClient.SHAREDPREFS_IP) 
        && m_prefs.contains(CeresClient.SHAREDPREFS_PORT)
        && m_prefs.contains(CeresClient.SHAREDPREFS_ID))
        {
			m_ip.setText(m_prefs.getString(CeresClient.SHAREDPREFS_IP, ""));
			m_port.setText(m_prefs.getString(CeresClient.SHAREDPREFS_PORT, ""));
			m_id.setText(m_prefs.getString(CeresClient.SHAREDPREFS_ID, ""));
			m_keepMeLoggedIn = true;
        }
        
        m_useSSC = (CheckBox)findViewById(R.id.useSelfSigned);
        m_keystorePass = (EditText)findViewById(R.id.keystorePass);
        NetworkHandler.setUseSelfSigned(false);
        
        if(m_prefs.contains(CeresClient.SHAREDPREFS_USESELFSIGNED))
        {
        	m_useSSC.setChecked(true);
        	NetworkHandler.setUseSelfSigned(true);
        }
        
        m_keepLogged = (CheckBox)findViewById(R.id.keepLogged);
        m_keepLogged.setChecked(m_keepMeLoggedIn);
        m_connect = (Button)findViewById(R.id.ceresConnect);
        m_notify = (EditText)findViewById(R.id.notify);
        m_dialog = new ProgressDialog(this);
        m_connect.setOnClickListener(new Button.OnClickListener() {
        	public void onClick(View v){
        		if(m_ip.getText().toString().equals("") || m_port.getText().toString().equals(""))
        		{
        			AlertDialog.Builder builder = new AlertDialog.Builder(m_ctx);
        			builder.setMessage("Ip address or port fields were empty")
        			       .setCancelable(false)
        			       .setPositiveButton("OK", new DialogInterface.OnClickListener() {
        			           public void onClick(DialogInterface dialog, int id) {
        			                dialog.cancel();
        			           }
        			       });
        			AlertDialog alert = builder.create();
        			alert.show();
        		}
        		else if(Integer.parseInt(m_port.getText().toString()) < 0 
        			 || Integer.parseInt(m_port.getText().toString()) > 65536)
        		{
        			AlertDialog.Builder builder = new AlertDialog.Builder(m_ctx);
        			builder.setMessage("The port number you provided is invalid")
        			       .setCancelable(false)
        			       .setPositiveButton("OK", new DialogInterface.OnClickListener() {
        			           public void onClick(DialogInterface dialog, int id) {
        			                dialog.cancel();
        			           }
        			       });
        			AlertDialog alert = builder.create();
        			alert.show();
        		}
        		else if(!m_ip.getText().toString().matches(m_regexIp))
        		{
        			AlertDialog.Builder builder = new AlertDialog.Builder(m_ctx);
        			builder.setMessage("The provided IP address format is invalid")
        			       .setCancelable(false)
        			       .setPositiveButton("OK", new DialogInterface.OnClickListener() {
        			           public void onClick(DialogInterface dialog, int id) {
        			                dialog.cancel();
        			           }
        			       });
        			AlertDialog alert = builder.create();
        			alert.show();
        		}
        		else
        		{
	        		m_keepMeLoggedIn = ((CheckBox)findViewById(R.id.keepLogged)).isChecked();
	        		if(m_keepMeLoggedIn)
	        		{
	        			SharedPreferences.Editor editor = m_prefs.edit();
	        			editor.putString(CeresClient.SHAREDPREFS_IP, m_ip.getText().toString());
	        			editor.putString(CeresClient.SHAREDPREFS_PORT, m_port.getText().toString());
	        			editor.putString(CeresClient.SHAREDPREFS_ID, m_id.getText().toString());
	        			editor.putBoolean(CeresClient.SHAREDPREFS_USESELFSIGNED, m_useSSC.isChecked());
	        			editor.commit();
	        		}
	        		else
	        		{
	        			SharedPreferences.Editor editor = m_prefs.edit();
	        			editor.clear();
	        			editor.commit();
	        		}
	        		String netString = (m_ip.getText().toString() + ":" + m_port.getText().toString());
	        		if(m_id.getText().toString().equals("") || m_passwd.getText().toString().equals(""))
	        		{
	        			AlertDialog.Builder builder = new AlertDialog.Builder(m_ctx);
	        			builder.setMessage("Please fill out both Username and Password!")
	        			       .setCancelable(false)
	        			       .setPositiveButton("OK", new DialogInterface.OnClickListener() {
	        			           public void onClick(DialogInterface dialog, int id) {
	        			                dialog.cancel();
	        			           }
	        			       });
	        			AlertDialog alert = builder.create();
	        			alert.show();
	        		}
	        		else
	        		{
	        			if(m_ceresClient.getStatus() == AsyncTask.Status.FINISHED)
	        			{
	        				m_ceresClient = new CeresClientConnect();
	        			}
	        			if(m_ceresClient.getStatus() != AsyncTask.Status.RUNNING)
	        			{
	        				m_ceresClient.execute(netString);
	        			}
	        		}
	        		m_notify.setVisibility(View.INVISIBLE);
        		}
        	}
        });
    }
    
    public void onCheckboxClicked(View view)
    {
    	m_keepMeLoggedIn = ((CheckBox)view).isChecked();
    }
    
    public void onUseSelfSigned(View view)
    {
    	boolean checkValue = ((CheckBox)view).isChecked();
    	if(checkValue)
    	{
    		AlertDialog.Builder builder = new AlertDialog.Builder(m_ctx);
    		builder.setMessage(
    				"WARNING: You are electing to use a self signed certificate!" 
    				+ " In order for this to work, you must use your server's certificate.crt file," 
    				+ " place it in res/raw, and sideload the app onto your device."
    				+ " It is not recommended to use this feature.")
    		       .setCancelable(false)
    		       .setPositiveButton("I understand.", new DialogInterface.OnClickListener() {
    		           public void onClick(DialogInterface dialog, int id) {
    		                dialog.cancel();
    		                NetworkHandler.setUseSelfSigned(true);
    		           }
    		       })
    		       .setNegativeButton("Scary! Take me back!", new DialogInterface.OnClickListener() {
    					@Override
    					public void onClick(DialogInterface dialog, int which) {
    						((CheckBox)findViewById(R.id.useSelfSigned)).setChecked(false);
    						NetworkHandler.setUseSelfSigned(false);
    						dialog.cancel();
    					}
    		       });
    		AlertDialog alert = builder.create();
    		alert.show();
    	}
    }
    
    @Override
    protected void onPause()
    {
    	super.onPause();
    	CeresClient.setForeground(false);
    }
    
    @Override
    protected void onResume()
    {
    	super.onResume();
    	m_notify.setVisibility(View.INVISIBLE);
    	CeresClient.setForeground(true);
    }
    
    @Override
    protected void onRestart()
    {
        if(m_prefs.contains(CeresClient.SHAREDPREFS_IP) 
        && m_prefs.contains(CeresClient.SHAREDPREFS_PORT)
        && m_prefs.contains(CeresClient.SHAREDPREFS_ID))
        {
			m_ip.setText(m_prefs.getString(CeresClient.SHAREDPREFS_IP, ""));
			m_port.setText(m_prefs.getString(CeresClient.SHAREDPREFS_PORT, ""));
			m_id.setText(m_prefs.getString(CeresClient.SHAREDPREFS_ID, ""));
			m_keepMeLoggedIn = true;
	        m_keepLogged.setChecked(m_keepMeLoggedIn);
        }
        if(m_prefs.contains(CeresClient.SHAREDPREFS_USESELFSIGNED))
        {
        	m_useSSC.setChecked(true);
        	NetworkHandler.setUseSelfSigned(true);
        }
        super.onRestart();
    }
 
    private class CeresClientConnect extends AsyncTask<String, Void, Integer> {
    	@Override
    	protected void onPreExecute()
    	{
    		// Display spinner here
    		if(m_dialog == null)
    		{
    			m_dialog = new ProgressDialog(m_ctx);
    		}
    		m_dialog.setCancelable(false);
    		m_dialog.setCanceledOnTouchOutside(false);
    		m_dialog.setMessage("Attempting to connect");
    		m_dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    		m_dialog.show();
    		super.onPreExecute();
    	}
    	
    	@Override
    	protected Integer doInBackground(String... params)
    	{
    		try
    		{
    			CeresClient.setURL(params[0]);
    			CeresClient.clearXmlReceive();
    			NetworkHandler.setSSL(m_ctx, R.raw.certificate, m_passwd.getText().toString(), m_id.getText().toString());
    			CeresClient.setXmlBase(NetworkHandler.get(("https://" + CeresClient.getURL() + "/getAll")));
    		}
    		catch(Exception e)
    		{
    			e.printStackTrace();
    			return 0;
    		}
    		
    		if(CeresClient.getXmlBase() != "")
    		{
    			System.out.println("returning 1");
    			return 1;
    		}
    		else
    		{
    			System.out.println("returning 0");
    			return 0;
    		}
    	}
    	@Override
    	protected void onPostExecute(Integer result)
    	{
    		if(result == 0)
    		{   
    			m_dialog.dismiss();
    			m_notify.setText(R.string.error);
    			m_notify.setTextColor(Color.RED);
    			m_notify.setVisibility(View.VISIBLE);
    		}
    		else
    		{
    			m_dialog.dismiss();
    			m_notify.setText(R.string.success);
    			m_notify.setTextColor(Color.GREEN);
    			m_notify.setVisibility(View.VISIBLE);
    			Intent nextPage = new Intent(getApplicationContext(), GridActivity.class);
    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    			nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    			getApplicationContext().startActivity(nextPage);
    		}
    	}
    }
}