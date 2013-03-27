//============================================================================
// Name        : StatusQueries.cpp
// Copyright   : DataSoft Corporation 2011-2013
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Handles requests for information from Novad
//============================================================================

#include "Commands.h"
#include "messaging/MessageManager.h"
#include "Logger.h"
#include "Lock.h"

#include <iostream>

using namespace Nova;
using namespace std;

extern bool isConnected;

namespace Nova
{

bool IsNovadUp()
{
	return isConnected;
}

void Ping()
{
	Message ping;
	ping.m_contents.set_m_type(REQUEST_PING);
	MessageManager::Instance().WriteMessage(&ping, ping.m_contents.m_sessionindex());
}

void RequestStartTime()
{
	Message getUptime;
	getUptime.m_contents.set_m_type(REQUEST_UPTIME);
	MessageManager::Instance().WriteMessage(&getUptime, getUptime.m_contents.m_sessionindex());
}

void RequestSuspectList(enum SuspectListType listType)
{
	Message request;
	request.m_contents.set_m_type(REQUEST_SUSPECTLIST);
	request.m_contents.set_m_listtype(listType);
	MessageManager::Instance().WriteMessage(&request, request.m_contents.m_sessionindex());
}

void RequestSuspect(SuspectID_pb address)
{
	Message request;
	request.m_contents.set_m_type(REQUEST_SUSPECT);
	*request.m_contents.mutable_m_suspectid() = address;
	request.m_contents.set_m_featuremode(NO_FEATURE_DATA);
	MessageManager::Instance().WriteMessage(&request, request.m_contents.m_sessionindex());
}

void RequestSuspectWithData(SuspectID_pb address)
{
	Message request;
	request.m_contents.set_m_type(REQUEST_SUSPECT);
	*request.m_contents.mutable_m_suspectid() = address;
	request.m_contents.set_m_featuremode(MAIN_FEATURE_DATA);
	MessageManager::Instance().WriteMessage(&request, request.m_contents.m_sessionindex());
}

void RequestSuspects(enum SuspectListType listType)
{
	Message request;
	request.m_contents.set_m_type(REQUEST_ALL_SUSPECTS);
	request.m_contents.set_m_listtype(listType);
	MessageManager::Instance().WriteMessage(&request, request.m_contents.m_sessionindex());
}

}
