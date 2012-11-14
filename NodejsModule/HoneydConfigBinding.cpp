#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "HoneydConfigBinding.h"
#include "HoneydTypesJs.h"
#include "v8Helper.h"

using namespace v8;
using namespace Nova;
using namespace std;

HoneydConfigBinding::HoneydConfigBinding()
{
	m_conf = NULL;
};

HoneydConfigBinding::~HoneydConfigBinding()
{
	delete m_conf;
};

HoneydConfiguration *HoneydConfigBinding::GetChild()
{
	return m_conf;
}

void HoneydConfigBinding::Init(Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("HoneydConfigBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype

	// TODO: Ask ace about doing this the template way. The following segfaults,
	//tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadAllTemplates"),FunctionTemplate::New(InvokeMethod<Boolean, bool, HoneydConfiguration, &HoneydConfiguration::LoadAllTemplates>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::LoadAllTemplates>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::SaveAllTemplates>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("WriteHoneydConfiguration"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::WriteHoneydConfiguration>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGeneratedProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetGeneratedProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNodeNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetNodeNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGroups"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetGroups>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGeneratedNodeNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetGeneratedNodeNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetScriptNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetScriptNames>));

	//tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteProfile"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::DeleteProfile>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteNode"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::DeleteNode>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("CheckNotInheritingEmptyProfile"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::CheckNotInheritingEmptyProfile>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("UpdateNodeMacs"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::UpdateNodeMacs>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNewNodes"),FunctionTemplate::New(AddNewNodes)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNode"),FunctionTemplate::New(GetNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNewNode"),FunctionTemplate::New(AddNewNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfile"),FunctionTemplate::New(GetProfile)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPorts"),FunctionTemplate::New(GetPorts)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddScript"),FunctionTemplate::New(AddScript)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("RemoveScript"),FunctionTemplate::New(RemoveScript)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("RemoveScriptPort"),FunctionTemplate::New(RemoveScriptPort)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAll"),FunctionTemplate::New(SaveAll)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteProfile"),FunctionTemplate::New(DeleteProfile)->GetFunction());

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydConfigBinding"), constructor);
}


Handle<Value> HoneydConfigBinding::New(const Arguments& args)
{
	HandleScope scope;

	HoneydConfigBinding* obj = new HoneydConfigBinding();
	obj->m_conf = new HoneydConfiguration();
	obj->Wrap(args.This());

	return args.This();
}

Handle<Value> HoneydConfigBinding::DeleteProfile(const Arguments& args)
{
  HandleScope scope;
  HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());
  
  if(args.Length() != 1)
  {
    return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
  }
  
  std::string profileToDelete = cvv8::CastFromJS<string>(args[0]);
  bool otherwise = true;
  
  if(obj->m_conf->m_profiles.keyExists(profileToDelete))
  {
    return scope.Close(Boolean::New(obj->m_conf->DeleteProfile(profileToDelete)));
  }
  else
  {
    return scope.Close(Boolean::New(otherwise));
  }
}

Handle<Value> HoneydConfigBinding::AddNewNodes(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 5 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 5 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	string ipAddress = cvv8::CastFromJS<string>( args[1] );
	string interface = cvv8::CastFromJS<string>( args[2] );
	string subnet = cvv8::CastFromJS<string>( args[3] );
	int count = cvv8::JSToInt32( args[4] );

	return scope.Close(Boolean::New(obj->m_conf->AddNewNodes(profile,ipAddress,interface,subnet,count)));
}


Handle<Value> HoneydConfigBinding::AddNewNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 5 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 5 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	string ipAddress = cvv8::CastFromJS<string>( args[1] );
	string mac = cvv8::CastFromJS<string>( args[2] );
	string interface = cvv8::CastFromJS<string>( args[3] );
	string subnet = cvv8::CastFromJS<string>( args[4] );

	return scope.Close(Boolean::New(obj->m_conf->AddNewNode(profile,ipAddress,mac, interface,subnet)));
}

Handle<Value> HoneydConfigBinding::GetNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string name = cvv8::CastFromJS<string>(args[0]);

	Nova::Node *ret = obj->m_conf->GetNode(name);

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapNode(ret));
	}
	else
	{
		return scope.Close( Null() );
	}
}

Handle<Value> HoneydConfigBinding::GetProfile(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string name = cvv8::CastFromJS<string>(args[0]);
	Nova::NodeProfile *ret = obj->m_conf->GetProfile(name);

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapProfile(ret));
	}
	else
	{
		return scope.Close( Null() );
	}

}

Handle<Value> HoneydConfigBinding::RemoveScriptPort(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 parameters")));
	}

	string portName = cvv8::CastFromJS<string>(args[0]);
	// For later, if we want to do more complex things
	// for script removal, like make this a SetPort method
	// or something.
	/*string newPortNumber = cvv8::CastFromJS<string>(args[1]);
	string newPortType = cvv8::CastFromJS<string>(args[2]);
	string newPortBehavior = cvv8::CastFromJS<string>(args[3]);
	string newPortService = cvv8::CastFromJS<string>(args[4]);
	string newPortScriptName = cvv8::CastFromJS<string>(args[5]);*/
	string profileName = cvv8::CastFromJS<string>(args[1]);

	if(!obj->m_conf->m_profiles.keyExists(profileName))
	{
		cout << "Profile " << profileName << " does not exist in the m_profiles NodeProfile table." << endl;
		return scope.Close(Boolean::New(false));
	}

	vector<pair<string, pair<bool, double> > > temp = obj->m_conf->m_profiles[profileName].m_ports;

	for(uint i = 0; i < temp.size(); i++)
	{
		if(!string(temp[i].first).compare(portName))
		{
			string replacement = temp[i].first;
			uint numIdx = replacement.find_first_of('_');
			uint idx = replacement.find_last_of('_');
			string num = replacement.substr(0, numIdx);
			string type = replacement.substr(numIdx + 1, idx - 3);
			replacement.replace(idx + 1, replacement.length(), "open");
			if(!obj->m_conf->m_ports.keyExists(replacement))
			{
				Nova::Port add;
				add.m_portName = replacement;
				add.m_portNum = num;
				add.m_type = type;
				add.m_service = "";
				add.m_behavior = "open";
				add.m_scriptName = "";
				obj->m_conf->m_ports[add.m_portName] = add;
				pair<string, pair<bool, double> > pushPair;
				pushPair.first = replacement;
				pushPair.second.first = temp[i].second.first;
				pushPair.second.second = temp[i].second.second;
				obj->m_conf->m_profiles[profileName].m_ports.erase(obj->m_conf->m_profiles[profileName].m_ports.begin() + i);
				obj->m_conf->m_profiles[profileName].m_ports.push_back(pushPair);
			}
			else
			{
				pair<string, pair<bool, double> > changePort = obj->m_conf->m_profiles[profileName].m_ports[i];
				changePort.first = replacement;
				obj->m_conf->m_profiles[profileName].m_ports.erase(obj->m_conf->m_profiles[profileName].m_ports.begin() + i);
				obj->m_conf->m_profiles[profileName].m_ports.push_back(changePort);
			}
			break;
		}
	}

	obj->m_conf->UpdateProfile(profileName);

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::AddScript(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() > 4 || args.Length() < 2)
	{
		// The service should be found dynamically, but unfortunately
		// within the nmap services file the services are linked to ports,
		// whereas our scripts have no such discrete affiliation.
		// I'm thinking of adding a searchable dropdown (akin to the
		// dojo select within the edit profiles page) that contains
		// all the services, and allow the user to associate the
		// uploaded script with one of these services.
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 to 4 parameters (3rd parameter is optional service, 4th parameter is optional osclass")));
	}

	Nova::Script script;
	script.m_name = cvv8::CastFromJS<string>(args[0]);
	script.m_path = cvv8::CastFromJS<string>(args[1]);

	if(args.Length() == 4)
	{
		script.m_service = cvv8::CastFromJS<string>(args[2]);
		script.m_osclass = cvv8::CastFromJS<string>(args[3]);
	}
	else
	{
		script.m_service = "";
		script.m_osclass = "";
	}

	if(!obj->m_conf->m_scripts.keyExists(script.m_name))
	{
		obj->m_conf->m_scripts[script.m_name] = script;
	}
	else
	{
		cout << "Script already present, doing nothing" << endl;
	}

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::RemoveScript(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	string scriptToRemove = cvv8::CastFromJS<string>(args[0]);

	if(!obj->m_conf->m_scripts.keyExists(scriptToRemove))
	{
		cout << "No registered script with name " << scriptToRemove << endl;
		return scope.Close(Boolean::New(false));
	}

	obj->m_conf->m_scripts.erase(scriptToRemove);

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::SaveAll(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	bool success = true;


	if (!obj->m_conf->SaveAllTemplates())
	{
		cout << "ERROR saving honeyd templates " << endl;
		success = false;
	}

	if (!obj->m_conf->WriteHoneydConfiguration()) {
		cout << "ERROR saving honeyd templates " << endl;
		success = false;
	}

	return scope.Close(Boolean::New(success));
}
