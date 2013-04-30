//============================================================================
// Name        : tester_Database.h
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
// Description : This file contains unit tests for the class Database
//============================================================================/*

#include "gtest/gtest.h"

#include "Database.h"

using namespace Nova;

// The test fixture for testing class Database.
class DatabaseTest : public ::testing::Test {
protected:
	// Unused methods here may be deleted
	DatabaseTest() {
		// You can do set-up work for each test here.
		Database::Inst()->Connect();
	}
	virtual ~DatabaseTest() {
		// You can do clean-up work that doesn't throw exceptions here.
	}
};


TEST_F(DatabaseTest, testInsertSuspect)
{

	struct timeval start, end;
	long mtime, seconds, useconds;
	gettimeofday(&start, NULL);

	Database::Inst()->StartTransaction();

	for (uint32_t iip = 1; iip < 20; iip++)
	{
		in_addr iiip;
		iiip.s_addr = htonl(iip);
		string ip(inet_ntoa(iiip));

		SuspectID_pb m_id;
		m_id.set_m_ip(iip);
		m_id.set_m_ifname("test");
		Suspect s;
		s.SetIdentifier(m_id);

		Database::Inst()->InsertSuspect(&s);

		// Something that looks about like a port scan
		Database::Inst()->IncrementPacketCount(ip, "test", "tcp", 65535);
		Database::Inst()->IncrementPacketCount(ip, "test", "tcpSyn", 65535);
		Database::Inst()->IncrementPacketCount(ip, "test", "total", 65535);
		Database::Inst()->IncrementPacketSizeCount(ip, "test", 120, 65535);

		for (int i = 0; i < 65535; i++)
		{
			Database::Inst()->IncrementPortContactedCount(ip, "test", "tcp", "192.168.42.42", i);
		}

		vector<double> featureset = Database::Inst()->ComputeFeatures(ip, "test");
		Database::Inst()->WriteClassification(&s);
	}

	Database::Inst()->StopTransaction();

	gettimeofday(&end, NULL);
	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	cout << "Elapsed time in milliseconds: " << mtime << endl;
}
