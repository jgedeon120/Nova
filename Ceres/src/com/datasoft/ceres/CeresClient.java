package com.datasoft.ceres;

import java.io.StringReader;
import java.util.ArrayList;
import android.app.Application;

public class CeresClient extends Application {
	private static String m_xmlBase;
	private static StringReader m_xmlReceive;
	private static boolean m_messageReceived;
	private static ArrayList<String> m_gridCache;
	private static boolean m_isInForeground;
	private static String m_serverUrl;	
	public static final String SHAREDPREFS_FILE = "com.datasoft.ceres";
	public static final String SHAREDPREFS_IP = "com.datasoft.ceres.ip";
	public static final String SHAREDPREFS_PORT = "com.datasoft.ceres.port";
	public static final String SHAREDPREFS_ID = "com.datasoft.ceres.id";
	public static final String SHAREDPREFS_USESELFSIGNED = "com.datasoft.ceres.usessc";
	public static final String SHAREDPREFS_CERTNAME = "com.datasoft.ceres.certname";
	
	@Override
	public void onCreate()
	{
		m_messageReceived = false;
		m_gridCache = new ArrayList<String>();
		super.onCreate();
	}
	
	public static ArrayList<String> getGridCache()
	{
		return m_gridCache;
	}
	
	public static void setGridCache(ArrayList<String> newCache)
	{
		m_gridCache.clear();
		for(String s : newCache)
		{
			m_gridCache.add(s);
		}
	}
	
	public static void setURL(String url)
	{
		m_serverUrl = url;
	}
	
	public static String getURL()
	{
		return m_serverUrl;
	}
	
	public static boolean checkMessageReceived()
	{
		return m_messageReceived;
	}
	
	public static void setMessageReceived(boolean msgRecv)
	{
		m_messageReceived = msgRecv;
	}
	
	public static StringReader getXmlReceive()
	{
		m_xmlReceive = new StringReader(m_xmlBase);
		return m_xmlReceive;
	}
	
	public static void setXmlBase(String base)
	{
		m_xmlBase = base;
	}
	
	public static String getXmlBase()
	{
		return m_xmlBase;
	}
	
	public static void clearXmlReceive()
	{
		m_xmlReceive = null;
		m_messageReceived = false;
	}
	
	public static boolean isInForeground()
	{
		return m_isInForeground;
	}
	
	public static void setForeground(boolean inForeground)
	{
		m_isInForeground = inForeground;
	}
}
