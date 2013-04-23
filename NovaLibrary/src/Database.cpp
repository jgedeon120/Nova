//============================================================================
// Name        : Database.cpp
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
//============================================================================

#include "Config.h"
#include "Database.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "ClassificationEngine.h"
#include "FeatureSet.h"

#include <iostream>
#include <sstream>
#include <string>

// Quick error checking macro so we don't have to copy/paste this over and over
#define expectReturnValue(val) \
if (res != val ) \
{\
	LOG(ERROR, "SQL error: " + string(sqlite3_errmsg(db)), "");\
	return;\
}

// Same as above macro, but returns false instead of no return value
#define expectReturnValueAndFail(val) \
if (res != val) {\
	LOG(ERROR, "SQL error: " + string(sqlite3_errmsg(db)), "");\
	return false;\
}


using namespace std;

namespace Nova
{

Database *Database::m_instance = NULL;

int Database::callback(void *NotUsed, int argc, char **argv, char **azColName){
	int i;
	for(i=0; i<argc; i++){
		cout << azColName[i] << "=" << (argv[i] ? argv[i] : "NULL") << endl;
	}
	cout << endl;
	return 0;
}

Database *Database::Inst(std::string databaseFile)
{
	if(m_instance == NULL)
	{
		m_instance = new Database(databaseFile);
		m_instance->Connect();
	}
	return m_instance;
}

Database::Database(std::string databaseFile)
{
	pthread_mutex_init(&this->m_lock, NULL);
	if (databaseFile == "")
	{
		databaseFile = Config::Inst()->GetPathHome() + "/data/novadDatabase.db";
	}
	m_databaseFile = databaseFile;
	m_count = 0;
}

Database::~Database()
{
	Disconnect();
}

void Database::StartTransaction()
{
	pthread_mutex_lock(&m_lock);
	sqlite3_exec(db, "BEGIN", 0, 0, 0);
}

void Database::StopTransaction()
{
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	pthread_mutex_unlock(&m_lock);
}

bool Database::Connect()
{
	LOG(DEBUG, "Opening database " + m_databaseFile, "");
	int res;
	res = sqlite3_open(m_databaseFile.c_str(), &db);
	expectReturnValueAndFail(SQLITE_OK);

	// Enable foreign keys (useful for keeping database consistency)
	char *err = NULL;

	sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, &err);
	if (err != NULL)
	{
		LOG(ERROR, "Error when trying to enable foreign database keys: " + string(err), "");
	}

	// Set up all our prepared queries
	res = sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO packet_counts VALUES(?1, ?2, ?3, 1);",
		-1, &insertPacketCount,  NULL);
	expectReturnValueAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"UPDATE packet_counts SET count = count + ?4 WHERE ip = ?1 AND interface = ?2 AND type = ?3;",
		-1, &incrementPacketCount,  NULL);
	expectReturnValueAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO suspects (ip, interface, startTime, endTime, lastTime, classification, hostileNeighbors, isHostile, classificationNotes) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
		-1, &insertSuspect,  NULL);
	expectReturnValueAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO ip_port_counts VALUES(?1, ?2, ?3, ?4, ?5, 1)",
		-1, &insertPortContacted, NULL);
	expectReturnValueAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
			"UPDATE ip_port_counts SET count = count + ?6 WHERE ip = ?1 AND interface = ?2 AND type = ?3 AND dstip = ?4 AND port = ?5",
			-1, &incrementPortContacted, NULL);
	expectReturnValueAndFail(SQLITE_OK);


	res = sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO packet_sizes VALUES(?1, ?2, ?3, 1);",
		-1, &insertPacketSize,  NULL);
	expectReturnValueAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"UPDATE packet_sizes SET count = count + ?4 WHERE ip = ?1 AND interface = ?2 AND packetSize = ?3;",
		-1, &incrementPacketSize,  NULL);
	expectReturnValueAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO suspect_features VALUES(?1, ?2, ?3, ?4)",
		-1, &setFeatureValue, NULL);
	expectReturnValueAndFail(SQLITE_OK);


	return true;
}

bool Database::Disconnect()
{
	if (sqlite3_finalize(insertSuspect) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to finalize sql statement: " + string(sqlite3_errmsg(db)), "");
	}
	if (sqlite3_finalize(insertPacketCount) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to finalize sql statement: " + string(sqlite3_errmsg(db)), "");
	}
	if (sqlite3_finalize(insertPortContacted) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to finalize sql statement: " + string(sqlite3_errmsg(db)), "");
	}
	if (sqlite3_finalize(incrementPacketCount) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to finalize sql statement: " + string(sqlite3_errmsg(db)), "");
	}
	if (sqlite3_finalize(incrementPortContacted) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to finalize sql statement: " + string(sqlite3_errmsg(db)), "");
	}

	if (sqlite3_close(db) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to finalize sql statement: " + string(sqlite3_errmsg(db)), "");
	}

	return true;
}

void Database::ResetPassword()
{
	stringstream ss;
	ss << "REPLACE INTO credentials VALUES (\"nova\", \"934c96e6b77e5b52c121c2a9d9fa7de3fbf9678d\", \"root\")";

	char *zErrMsg = 0;
	int state = sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg);
	if (state != SQLITE_OK)
	{
		string errorMessage(zErrMsg);
		sqlite3_free(zErrMsg);
		throw DatabaseException(string(errorMessage));
	}
}

void Database::InsertSuspect(Suspect *suspect)
{
	int res;

	res = sqlite3_bind_text(insertSuspect, 1, suspect->GetIpString().c_str(), -1, SQLITE_TRANSIENT);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(insertSuspect, 2, suspect->GetInterface().c_str(), -1, SQLITE_TRANSIENT);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int64(insertSuspect, 3, static_cast<long int>(suspect->m_features.m_startTime));
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int64(insertSuspect, 4, static_cast<long int>(suspect->m_features.m_endTime));
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int64(insertSuspect, 5, static_cast<long int>(suspect->m_features.m_lastTime));
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_double(insertSuspect, 6, suspect->GetClassification());
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(insertSuspect, 7, suspect->GetHostileNeighbors());
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(insertSuspect, 8, suspect->GetIsHostile());
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(insertSuspect, 9, suspect->m_classificationNotes.c_str(), -1, SQLITE_TRANSIENT);
	expectReturnValue(SQLITE_OK);

	m_count++;
	res = sqlite3_step(insertSuspect);
	expectReturnValue(SQLITE_DONE);

	res = sqlite3_reset(insertSuspect);
	expectReturnValue(SQLITE_OK);
}

void Database::IncrementPacketCount(std::string ip, std::string interface, std::string type, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	res = sqlite3_bind_text(incrementPacketCount, 1, ip.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(incrementPacketCount, 2, interface.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(incrementPacketCount, 3, type.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(incrementPacketCount, 4, increment);
	expectReturnValue(SQLITE_OK);

	m_count++;
	res = sqlite3_step(incrementPacketCount);
	expectReturnValue(SQLITE_DONE);
	res = sqlite3_reset(incrementPacketCount);
	expectReturnValue(SQLITE_OK);


	// If the update failed, we need to insert the port contacted count and set it to 1
	if (sqlite3_changes(db) == 0)
	{
		res = sqlite3_bind_text(insertPacketCount, 1, ip.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_text(insertPacketCount, 2, interface.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_text(insertPacketCount, 3, type.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);

		m_count++;
		res = sqlite3_step(insertPacketCount);
		expectReturnValue(SQLITE_DONE);

		res = sqlite3_reset(insertPacketCount);
		expectReturnValue(SQLITE_OK);
	}
}

void Database::IncrementPacketSizeCount(std::string ip, std::string interface, uint16_t size, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	res = sqlite3_bind_text(incrementPacketSize, 1, ip.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(incrementPacketSize, 2, interface.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(incrementPacketSize, 3, size);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(incrementPacketSize, 4, increment);
	expectReturnValue(SQLITE_OK);


	m_count++;
	res = sqlite3_step(incrementPacketSize);
	expectReturnValue(SQLITE_DONE);
	res = sqlite3_reset(incrementPacketSize);
	expectReturnValue(SQLITE_OK);

	if (sqlite3_changes(db) == 0)
	{
		res = sqlite3_bind_text(insertPacketSize, 1, ip.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_text(insertPacketSize, 2, interface.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_int(insertPacketSize, 3, size);
		expectReturnValue(SQLITE_OK);

		m_count++;
		res = sqlite3_step(insertPacketSize);
		expectReturnValue(SQLITE_DONE);
		res = sqlite3_reset(insertPacketSize);
		expectReturnValue(SQLITE_OK);
	}
}

void Database::IncrementPortContactedCount(std::string ip, std::string interface, string protocol, string dstip, int port, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	// Try to increment the port count if it exists
	res = sqlite3_bind_text(incrementPortContacted, 1, ip.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(incrementPortContacted, 2, interface.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(incrementPortContacted, 3, protocol.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(incrementPortContacted, 4, dstip.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(incrementPortContacted, 5, port);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(incrementPortContacted, 6, increment);
	expectReturnValue(SQLITE_OK);

	m_count++;
	res = sqlite3_step(incrementPortContacted);
	expectReturnValue(SQLITE_DONE);
	res = sqlite3_reset(incrementPortContacted);
	expectReturnValue(SQLITE_OK);

	// If the update failed, we need to insert the port contacted count and set it to 1
	if (sqlite3_changes(db) == 0)
	{
		res = sqlite3_bind_text(insertPortContacted, 1, ip.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_text(insertPortContacted, 2, interface.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_text(insertPortContacted, 3, protocol.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_text(insertPortContacted, 4, dstip.c_str(), -1, SQLITE_STATIC);
		expectReturnValue(SQLITE_OK);
		res = sqlite3_bind_int(insertPortContacted, 5, port);
		expectReturnValue(SQLITE_OK);

		m_count++;
		res = sqlite3_step(insertPortContacted);
		expectReturnValue(SQLITE_DONE);
		res = sqlite3_reset(insertPortContacted);
		expectReturnValue(SQLITE_OK);
	}
}

void Database::SetFeatureSetValue(std::string ip, std::string interface, std::string featureName, double value)
{
	int res;

	res = sqlite3_bind_text(setFeatureValue, 1, ip.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(setFeatureValue, 2, interface.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(setFeatureValue, 3, featureName.c_str(), -1, SQLITE_STATIC);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_double(setFeatureValue, 4, value);
	expectReturnValue(SQLITE_OK);

	m_count++;
	res = sqlite3_step(setFeatureValue);
	expectReturnValue(SQLITE_DONE);
	res = sqlite3_reset(setFeatureValue);
	expectReturnValue(SQLITE_OK);
}


void Database::InsertSuspectHostileAlert(Suspect *suspect)
{
	FeatureSet features = suspect->GetFeatureSet(MAIN_FEATURES);

	stringstream ss;
	ss << "INSERT INTO statistics VALUES (NULL";

	for (int i = 0; i < DIM; i++)
	{
		ss << ", ";

		ss << features.m_features[i];

	}
	ss << ");";


	ss << "INSERT INTO suspect_alerts VALUES (NULL, '";
	ss << suspect->GetIpString() << "', '" << suspect->GetInterface() << "', datetime('now')" << ",";
	ss << "last_insert_rowid()" << "," << suspect->GetClassification() << ")";

	char *zErrMsg = 0;
	int state = sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg);
	if (state != SQLITE_OK)
	{
		string errorMessage(zErrMsg);
		sqlite3_free(zErrMsg);
		throw DatabaseException(string(errorMessage));
	}
}


}/* namespace Nova */
