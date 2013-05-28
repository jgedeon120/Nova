package com.datasoft.ceres;

import java.io.IOException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import org.apache.http.Header;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.conn.ssl.SSLSocketFactory;
import org.apache.http.conn.ssl.X509HostnameVerifier;
import org.apache.http.impl.auth.BasicScheme;
import android.content.Context;
import android.content.res.Resources.NotFoundException;
import com.loopj.android.http.*;

public class NetworkHandler {
	private static AsyncHttpClient m_client = new AsyncHttpClient();
	private static Header m_headerParam;
	private static Context m_ctx;
	private static char[] m_kspass;
	private static Boolean m_useSelfSigned;
	
	public static void setKSPass(String kspass)
	{
		m_kspass = kspass.toCharArray();
	}
	
	public static void setUseSelfSigned(Boolean use)
	{
		m_useSelfSigned = use;
	}
	
	public static Boolean usingSelfSigned()
	{
		return m_useSelfSigned;
	}
	
	public static void setSSL(Context ctx, int keyid, String pass, String user)
	{
		try
		{
			m_client = new AsyncHttpClient();
			m_ctx = ctx;
			if(m_useSelfSigned)
			{
				KeyStore keystore = KeyStore.getInstance("BKS");
				keystore.load(ctx.getResources().openRawResource(keyid), m_kspass);
				SSLSocketFactory sslsf = new SSLSocketFactory(keystore);
				sslsf.setHostnameVerifier((X509HostnameVerifier)SSLSocketFactory.ALLOW_ALL_HOSTNAME_VERIFIER);
				m_client.setSSLSocketFactory(sslsf);
			}
			UsernamePasswordCredentials creds = new UsernamePasswordCredentials(user, pass);
			m_headerParam = BasicScheme.authenticate(creds, "UTF-8", false);
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
		catch (UnrecoverableKeyException e)
		{
			e.printStackTrace();
		}
	}
	
	public static void get(String url, RequestParams params, AsyncHttpResponseHandler response)
	{
		Header[] pass = {m_headerParam};
		m_client.get(m_ctx, url, pass, params, response);
	}
}
