package com.datasoft.ceres;

import java.util.ArrayList;

import com.loopj.android.http.AsyncHttpResponseHandler;
import com.loopj.android.http.RequestParams;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.app.SearchManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.Toast;

public class GridSearchActivity extends ListActivity {
	private ProgressDialog m_wait;
	private String m_selected;
	private Context m_gridContext;
	private String m_regexIp = "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)";
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_search);
		m_wait = new ProgressDialog(this);
		
		LayoutInflater inf = getLayoutInflater();
		View header = inf.inflate(R.layout.grid_header, (ViewGroup)findViewById(R.id.header_layout_root));
		getListView().addHeaderView(header, null, false);
		
		Intent intent = getIntent();
		if(Intent.ACTION_SEARCH.equals(intent.getAction()))
		{
			String query = intent.getStringExtra(SearchManager.QUERY);
			ClassificationGridAdapter test = doSearch(query);
			
			if(test != null)
			{
				setListAdapter(test);
			}
			else
			{
				AlertDialog.Builder build = new AlertDialog.Builder(this);
				build
				.setTitle("Search returned no results")
				.setCancelable(false)
				.setPositiveButton("Go back", new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which)
					{
		    			Intent nextPage = new Intent(getApplicationContext(), GridActivity.class);
		    			nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
		    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		    			getApplicationContext().startActivity(nextPage);
					}
				});
				AlertDialog ad = build.create();
				ad.show();
			}
		}
	}
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		String[] item = ((String)getListAdapter().getItem(position - 1)).split(":");
		m_selected = item[0] + ":" + item[3];
		Toast.makeText(this, m_selected + " selected", Toast.LENGTH_SHORT).show();

		new CeresSuspectRequest().execute();
	}
	
	private class CeresSuspectRequest extends AsyncTask<Void, Void, Integer> {
		@Override
		protected void onPreExecute()
		{
			CeresClient.clearXmlReceive();
			m_wait.setCancelable(true);
			m_wait.setMessage("Retrieving details for " + m_selected);
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
			super.onPreExecute();
		}
		@Override
		protected Integer doInBackground(Void... vd)
		{
			try
    		{
				RequestParams params = new RequestParams("suspect", m_selected);
    			NetworkHandler.get(("https://" + CeresClient.getURL() + "/getSuspect"), params, new AsyncHttpResponseHandler(){
    				@Override
    				public void onSuccess(String xml)
    				{
    					CeresClient.setXmlBase(xml);
    					CeresClient.setMessageReceived(true);
    				}
    				
    				@Override
    				public void onFailure(Throwable err, String content)
    				{
    					CeresClient.setXmlBase("");
    					CeresClient.setMessageReceived(true);
    				}
    			});
    			while(!CeresClient.checkMessageReceived()){};
    		}
    		catch(Exception e)
    		{
    			e.printStackTrace();
    			return 0;
    		}
    		
    		if(CeresClient.getXmlBase() != "")
    		{
    			return 1;
    		}
    		else
    		{
    			return 0;
    		}
		}
		@Override
		protected void onPostExecute(Integer result)
		{
			if(result == 0)
			{
				m_wait.cancel();
				Toast.makeText(m_gridContext, "Could not get details for " + m_selected, Toast.LENGTH_LONG).show();
			}
			else
			{
				m_wait.cancel();
				Intent nextPage = new Intent(getApplicationContext(), DetailsActivity.class);
				nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				getApplicationContext().startActivity(nextPage);
			}
		}
	}
	
	public ClassificationGridAdapter doSearch(String query)
	{
		ArrayList<String> search = CeresClient.getGridCache();
		ArrayList<String> gridValues = new ArrayList<String>();
		if(!query.contains("*"))
		{
			for(String line : search)
			{
				String[] split = line.split(":");
				for(String cell : split)
				{
					if(cell.equals(query))
					{
						gridValues.add(line);
					}
				}
			}
		}
		else
		{
			for(String line : search)
			{
				if(line.contains(query.substring(0, query.length() - 1)))
				{
					gridValues.add(line);
				}
			}
		}
		if(gridValues.size() > 0)
		{
			ClassificationGridAdapter ret = new ClassificationGridAdapter(this, gridValues);
			return ret;
		}
		return null;
	}
}
