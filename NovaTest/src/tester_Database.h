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
	SuspectID_pb m_id;
	m_id.set_m_ip(42);
	m_id.set_m_ifname("test");
	Suspect s;
	s.SetIdentifier(m_id);

	std::string ip = Suspect::GetIpString(m_id);

	Database::Inst()->StartTransaction();
	Database::Inst()->InsertSuspect(&s);
	Database::Inst()->StopTransaction();

	struct timeval start, end;
	long mtime, seconds, useconds;
	gettimeofday(&start, NULL);

	// Something that looks about like a port scan
	Database::Inst()->StartTransaction();
	for (int i = 0; i < 65535; i++)
	{
		Database::Inst()->IncrementPacketCount(ip, "test", "tcp");
		Database::Inst()->IncrementPacketCount(ip, "test", "tcpSyn");
		Database::Inst()->IncrementPacketCount(ip, "test", "total");
		Database::Inst()->IncrementPacketSizeCount(ip, "test", 120);
		Database::Inst()->IncrementPortContactedCount(ip, "test", "tcp", "192.168.42.42", i);
	}
	Database::Inst()->StopTransaction();

	gettimeofday(&end, NULL);
	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	cout << "Elapsed time in milliseconds: " << mtime << endl;
}

// Tests go here. Multiple small tests are better than one large test, as each test
// will get a pass/fail and debugging information associated with it.

/*
TEST_F(DatabaseTest, testInsertHostileSuspectEvent)
{
	FeatureSet f;
	for (int i = 0; i < DIM; i++)
	{
		f.m_features[i] = i;
	}
	Suspect s;
	s.SetFeatureSet(&f, MAIN_FEATURES);
	s.SetClassification(0.42);

	testObject.InsertSuspectHostileAlert(&s);
}*/
