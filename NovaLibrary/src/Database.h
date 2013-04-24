//============================================================================
// Name        : Database.h
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
// Description : Wrapper for adding entries to the SQL database
//============================================================================/*

#ifndef DATABASE_H_
#define DATABASE_H_

#include "Suspect.h"
#include "FeatureSet.h"

#include <Lock.h>
#include <string>
#include <sqlite3.h>
#include <stdexcept>

namespace Nova
{

class DatabaseException : public std::runtime_error
{
public:
	DatabaseException(const string& errorcode)
	: runtime_error("Database error code: " + errorcode) {};

};

class Database
{
public:
	static Database *Inst(std::string = "");
	~Database();

	void Connect();
	bool Disconnect();

	void StartTransaction();
	void StopTransaction();

	void InsertSuspect(Suspect *suspect);
	void InsertHoneypotIp(std::string ip);

	void InsertSuspectHostileAlert(Suspect *suspect);
	void WriteClassification(Suspect *s);

	void IncrementPacketCount(std::string ip, std::string interface, std::string type, uint64_t increment = 1);
	void IncrementPacketSizeCount(std::string ip, std::string interface, uint16_t size, uint64_t increment = 1);
	void IncrementPortContactedCount(std::string ip, std::string interface, std::string protocol, std::string dstip, int port, uint64_t increment = 1);

	std::vector<double> ComputeFeatures(std::string ip, std::string interface);

	void SetFeatureSetValue(std::string ip, std::string interface, std::string featureName, double value);

	void ResetPassword();



	static int callback(void *NotUsed, int argc, char **argv, char **azColName);


	// This is just for debugging performance issues
	int m_count;
private:
	Database(std::string databaseFile = "");

	pthread_mutex_t m_lock;

	std::string m_databaseFile;

	static Database * m_instance;

	sqlite3 *db;

	sqlite3_stmt *insertSuspect;

	sqlite3_stmt *insertPacketCount;
	sqlite3_stmt *incrementPacketCount;

	sqlite3_stmt *insertPortContacted;
	sqlite3_stmt *incrementPortContacted;

	sqlite3_stmt *insertPacketSize;
	sqlite3_stmt *incrementPacketSize;

	sqlite3_stmt *setFeatureValue;

	// Query to populate a featureset
	sqlite3_stmt *insertFeatureValue;

	// Queries to compute featuresets;
	sqlite3_stmt *computePacketSizeMean;
	sqlite3_stmt *computePacketSizeVariance;

	sqlite3_stmt *computeDistinctIps;
	sqlite3_stmt *computeDistinctPorts;

	sqlite3_stmt *computeDistinctIpPorts;
	sqlite3_stmt *selectPacketCounts;

	// This gets the max packets sent to any one IP or port
	sqlite3_stmt *computeMaxPacketsToIp;
	sqlite3_stmt *computeMaxPacketsToPort;

	sqlite3_stmt *computeHoneypotsContacted;
	sqlite3_stmt *insertHoneypotIp;

	sqlite3_stmt *updateClassification;

};

} /* namespace Nova */
#endif /* DATABASE_H_ */
