package com.datasoft.ceres;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.MalformedURLException;
import java.net.URL;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManagerFactory;

import org.apache.http.conn.ssl.BrowserCompatHostnameVerifier;

import android.util.Base64;

import android.content.Context;
import android.content.res.Resources.NotFoundException;

public class NetworkHandler {
	private static boolean m_useSelfSigned;
	private static String m_basicAuthEncoded;
	private static SSLContext m_sslctx;
	
	public static void setUseSelfSigned(boolean use)
	{
		m_useSelfSigned = use;
	}
	
	public static boolean usingSelfSigned()
	{
		return m_useSelfSigned;
	}
	
	public static void setSSL(Context ctx, int keyid, String pass, String user)
	{
		try
		{
			if(m_useSelfSigned)
			{
				CertificateFactory cf = CertificateFactory.getInstance("X.509");
				InputStream caInput = ctx.getResources().openRawResource(keyid);
				Certificate ca;
				try
				{
					ca = cf.generateCertificate(caInput);
				}
				finally
				{
					caInput.close();
				}
				
				String keyStoreType = KeyStore.getDefaultType();
				KeyStore keyStore = KeyStore.getInstance(keyStoreType);
				keyStore.load(null, null);
				keyStore.setCertificateEntry("ca", ca);
				
				String tmfAlgorithm = TrustManagerFactory.getDefaultAlgorithm();
				TrustManagerFactory tmf = TrustManagerFactory.getInstance(tmfAlgorithm);
				tmf.init(keyStore);
				
				m_sslctx = SSLContext.getInstance("TLS");
				m_sslctx.init(null, tmf.getTrustManagers(), null);
			}
			m_basicAuthEncoded = Base64.encodeToString((user + ":" + pass).getBytes(), 0);
		}
		catch(KeyStoreException e)
		{
			e.printStackTrace();
		}
		catch(CertificateException e) 
		{
			e.printStackTrace();
		}
		catch(NoSuchAlgorithmException e)
		{
			e.printStackTrace();
		}
		catch(NotFoundException e)
		{
			e.printStackTrace();
		}
		catch (KeyManagementException e)
		{
			e.printStackTrace();
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}
	}
	
	public static String get(String url)
	{
		CeresClient.setMessageReceived(false);
		StringBuilder ret = new StringBuilder();
		URL sslurl;
		try
		{
			sslurl = new URL(url);
			HttpsURLConnection urlconn = (HttpsURLConnection)sslurl.openConnection();
			if(m_useSelfSigned)
			{
				urlconn.setSSLSocketFactory(m_sslctx.getSocketFactory());
				urlconn.setHostnameVerifier(new BrowserCompatHostnameVerifier());
			}
			urlconn.setRequestProperty("Authorization", "Basic " + m_basicAuthEncoded);
			urlconn.connect();
			InputStreamReader in = new InputStreamReader((InputStream)urlconn.getContent());
			BufferedReader buff = new BufferedReader(in);
			String line;
			while((line = buff.readLine()) != null)
			{
				ret.append(line);
			}
			CeresClient.setMessageReceived(true);
		}
		catch(MalformedURLException mue)
		{
			mue.printStackTrace();
		}
		catch(IOException ioe)
		{
			ioe.printStackTrace();
		}
		return ret.toString();
	}
}
