//============================================================================
// Name        : MessageManager.h
// Copyright   : DataSoft Corporation 2011-2012
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
// Description : Manages all incoming messages on sockets
//============================================================================

#ifndef MESSAGEMANAGER_H_
#define MESSAGEMANAGER_H_

#include "messages/UI_Message.h"
#include "MessageQueue.h"

#include <map>
#include "pthread.h"

namespace Nova
{

class MessageManager
{

public:

	MessageManager &Instance();

	Nova::UI_Message *GetMessage(int socketFD);

	void StartSocket(Socket socket);

	//Closes the socket at the given file descriptor
	//	socketFD: The file descriptor of the socket to close
	//	NOTE: This will not immediately destroy the underlying MessageQueue. It will close the socket
	//		such that no new messages can be read on it. At which point the read loop will mark the
	//		queue as closed with an ErrorMessage with the appropriate sub-type and then exit.
	//		The queue will not be actually destroyed until this last message is popped off.
	void CloseSocket(int socketFD);

private:

	static MessageManager *m_instance;

	MessageManager();

	//Mutex for the lock map
	pthread_mutex_t m_queuesLock;

	//These two maps must be kept synchronized
	std::map<int, MessageQueue*> m_queues;
	std::map<int, pthread_mutex_t> m_queueLocks;
};

}

#endif /* MESSAGEMANAGER_H_ */
