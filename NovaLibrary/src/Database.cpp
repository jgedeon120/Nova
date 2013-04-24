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
#define expectReturn(val) \
if (res != val ) \
{\
	LOG(ERROR, "SQL error: " + string(sqlite3_errmsg(db)), "");\
	return;\
}

// Quick error checking macro so we don't have to copy/paste this over and over
#define SQL_RUN(val, stmt) \
res = stmt; \
if (res != val ) \
{\
	LOG(ERROR, "SQL error: " + string(sqlite3_errmsg(db)), "");\
	return;\
}

// Same as above macro, but returns false instead of no return value
#define expectReturnAndFail(val) \
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
	expectReturnAndFail(SQLITE_OK);

	// Enable foreign keys (useful for keeping database consistency)
	char *err = NULL;

	sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, &err);
	if (err != NULL)
	{
		LOG(ERROR, "Error when trying to enable foreign database keys: " + string(err), "");
	}

	// Set up all our prepared queries
	res = sqlite3_prepare_v2(db,
		"INSERT INTO packet_counts VALUES(?1, ?2, ?3, ?4);",
		-1, &insertPacketCount,  NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"UPDATE packet_counts SET count = count + ?4 WHERE ip = ?1 AND interface = ?2 AND type = ?3;",
		-1, &incrementPacketCount,  NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO suspects (ip, interface, startTime, endTime, lastTime, classification, hostileNeighbors, isHostile, classificationNotes) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
		-1, &insertSuspect,  NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"INSERT INTO ip_port_counts VALUES(?1, ?2, ?3, ?4, ?5, 1)",
		-1, &insertPortContacted, NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
			"UPDATE ip_port_counts SET count = count + ?6 WHERE ip = ?1 AND interface = ?2 AND type = ?3 AND dstip = ?4 AND port = ?5",
			-1, &incrementPortContacted, NULL);
	expectReturnAndFail(SQLITE_OK);


	res = sqlite3_prepare_v2(db,
		"INSERT INTO packet_sizes VALUES(?1, ?2, ?3, 1);",
		-1, &insertPacketSize,  NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"UPDATE packet_sizes SET count = count + ?4 WHERE ip = ?1 AND interface = ?2 AND packetSize = ?3;",
		-1, &incrementPacketSize,  NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO suspect_features VALUES(?1, ?2, ?3, ?4)",
		-1, &setFeatureValue, NULL);
	expectReturnAndFail(SQLITE_OK);



	// Queries for featureset computation
	res = sqlite3_prepare_v2(db,
		"SELECT (1.0*packet_counts.count) / (1.0*SUM(packet_sizes.count))"
		" FROM packet_counts INNER JOIN packet_sizes"
		" ON packet_counts.ip = packet_sizes.ip AND packet_counts.interface = packet_sizes.interface"
		" WHERE packet_counts.type = 'bytes'"
		" AND packet_counts.ip = ?1"
		" AND packet_counts.interface = ?2;",
		-1, &computePacketSizeMean, NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"SELECT SUM((packet_sizes.count)*(packet_sizes.packetSize-?3)*(packet_sizes.packetSize-?3))/(1.0*packet_counts.count)"
		" FROM packet_counts INNER JOIN packet_sizes"
		" ON packet_counts.ip = packet_sizes.ip AND packet_counts.interface = packet_sizes.interface"
		" WHERE packet_counts.type = 'total'"
		" AND packet_counts.ip = ?1"
		" AND packet_counts.interface = ?2",
		-1, &computePacketSizeVariance, NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"SELECT COUNT(DISTINCT dstip) FROM ip_port_counts WHERE ip = ?1 AND interface = ?2",
		-1, &computeDistinctIps, NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"SELECT COUNT(DISTINCT port) FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 AND type = ?3",
		-1, &computeDistinctPorts, NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		"SELECT COUNT(*) FROM (SELECT DISTINCT port, dstip FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 AND type = ?3);",
		-1, &computeDistinctIpPorts, NULL);
	expectReturnAndFail(SQLITE_OK);


	res = sqlite3_prepare_v2(db,
		"SELECT type, count FROM packet_counts WHERE ip = ?1 AND interface = ?2;",
		-1, &selectPacketCounts, NULL);
	expectReturnAndFail(SQLITE_OK);


	res = sqlite3_prepare_v2(db,
		" SELECT MAX(t) FROM (SELECT dstip, SUM(count) AS t FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 AND TYPE = ?3 GROUP BY dstip);",
		-1, &computeMaxPacketsToIp, NULL);
	expectReturnAndFail(SQLITE_OK);

	res = sqlite3_prepare_v2(db,
		" SELECT MAX(t) FROM (SELECT port, SUM(count) AS t FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 AND TYPE = ?3 GROUP BY port);",
		-1, &computeMaxPacketsToPort, NULL);
	expectReturnAndFail(SQLITE_OK);


	return true;
}

void Database::ComputeFeatures(std::string ip, std::string interface)
{
	int res;

	// Create someplace to store our computed results
	double featureValues[DIM];
	fill(featureValues, featureValues + DIM, 0);

	// Compute the packet size mean
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeMean, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeMean, 2, interface.c_str(), -1, SQLITE_STATIC));

	SQL_RUN(SQLITE_ROW, sqlite3_step(computePacketSizeMean));
	featureValues[PACKET_SIZE_MEAN] = sqlite3_column_double(computePacketSizeMean, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computePacketSizeMean));


	// Compute the packet size deviation
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeVariance, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeVariance, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_double(computePacketSizeVariance, 3, featureValues[PACKET_SIZE_MEAN]));

	// SQL query gives us the variance, take sqrt to get the standard deviation
	SQL_RUN(SQLITE_ROW, sqlite3_step(computePacketSizeVariance));
	featureValues[PACKET_SIZE_DEVIATION] = sqrt(sqlite3_column_double(computePacketSizeVariance, 0));

	SQL_RUN(SQLITE_OK, sqlite3_reset(computePacketSizeVariance));


	// Compute the distinct IPs contacted
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIps, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIps, 2, interface.c_str(), -1, SQLITE_STATIC));

	SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctIps));
	featureValues[DISTINCT_IPS] = sqlite3_column_double(computeDistinctIps, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctIps));


	// Compute the distinct TCP ports contacted
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 3, "tcp", -1, SQLITE_STATIC));

	SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctPorts));
	featureValues[DISTINCT_TCP_PORTS] = sqlite3_column_double(computeDistinctPorts, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctPorts));


	// Compute the distinct UDP ports contacted
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 3, "udp", -1, SQLITE_STATIC));

	SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctPorts));
	featureValues[DISTINCT_UDP_PORTS] = sqlite3_column_double(computeDistinctPorts, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctPorts));


	if (featureValues[DISTINCT_IPS] > 0) {
		// Compute the average TCP ports per host
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 3, "tcp", -1, SQLITE_STATIC));

		SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctIpPorts));
		featureValues[AVG_TCP_PORTS_PER_HOST] = sqlite3_column_double(computeDistinctIpPorts, 0) / featureValues[DISTINCT_IPS];

		SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctIpPorts));


		// Compute the average UDP ports per host
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 3, "udp", -1, SQLITE_STATIC));

		SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctIpPorts));
		featureValues[AVG_UDP_PORTS_PER_HOST] = sqlite3_column_double(computeDistinctIpPorts, 0) / featureValues[DISTINCT_IPS];

		SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctIpPorts));
	}




	// Compute all of the features that are based on packet counts
	// This is kind of ugly, some of it 'could' be moved to more specific SQL queries rather than doing it in C++.
	// I'm not entirely decided on this part of the table schema anyway, so not worrying about it too much yet.
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(selectPacketCounts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(selectPacketCounts, 2, interface.c_str(), -1, SQLITE_STATIC));

	// Did we find the value in the database, and if so what is it?
	pair<bool, double> totalTcpPackets = pair<bool, double>(false, 0);
	pair<bool, double> synPackets = pair<bool, double>(false, 0);
	pair<bool, double> finPackets = pair<bool, double>(false, 0);
	pair<bool, double> rstPackets = pair<bool, double>(false, 0);
	pair<bool, double> synAckPackets = pair<bool, double>(false, 0);
	pair<bool, double> totalPackets = pair<bool, double>(false, 0);

	res = sqlite3_step(selectPacketCounts);
	while (res == SQLITE_ROW)
	{
		string countType = string(reinterpret_cast<const char*>(sqlite3_column_text(selectPacketCounts, 0)));
		if (countType == "tcp")
		{
			totalTcpPackets.first = true;
			totalTcpPackets.second = sqlite3_column_double(selectPacketCounts, 1);
		}
		else if (countType == "tcpSyn")
		{
			synPackets.first = true;
			synPackets.second = sqlite3_column_double(selectPacketCounts, 1);
		}
		else if (countType == "tcpFin")
		{
			finPackets.first = true;
			finPackets.second = sqlite3_column_double(selectPacketCounts, 1);
		}
		else if (countType == "tcpRst")
		{
			rstPackets.first = true;
			rstPackets.second = sqlite3_column_double(selectPacketCounts, 1);
		}
		else if (countType == "tcpSynAck")
		{
			synAckPackets.first = true;
			synAckPackets.second = sqlite3_column_double(selectPacketCounts, 1);
		}
		else if (countType == "total")
		{
			totalPackets.first = true;
			totalPackets.second = sqlite3_column_double(selectPacketCounts, 1);
		}

		res = sqlite3_step(selectPacketCounts);
	}

	SQL_RUN(SQLITE_OK, sqlite3_reset(selectPacketCounts));

	featureValues[TCP_PERCENT_SYN] = synPackets.second / (totalTcpPackets.second + 1);
	featureValues[TCP_PERCENT_FIN] = finPackets.second / (totalTcpPackets.second + 1);
	featureValues[TCP_PERCENT_RST] = rstPackets.second / (totalTcpPackets.second + 1);
	featureValues[TCP_PERCENT_SYNACK] = synAckPackets.second / (totalTcpPackets.second + 1);


	// Compute the "ip traffic distribution", which is basically just (TotalSuspectPackets/TotalSuspectDstIps)/maxPacketsToSingleDstIp
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToIp, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToIp, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToIp, 3, "tcp", -1, SQLITE_STATIC));

	SQL_RUN(SQLITE_ROW, sqlite3_step(computeMaxPacketsToIp));
	featureValues[IP_TRAFFIC_DISTRIBUTION] = totalPackets.second / featureValues[DISTINCT_IPS] / sqlite3_column_double(computeMaxPacketsToIp, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computeMaxPacketsToIp));


	if ((featureValues[DISTINCT_TCP_PORTS] + featureValues[DISTINCT_UDP_PORTS]) > 0)
	{
		// Compute the "port traffic distribution", which is basically just (TotalSuspectPackets/TotalSuspectDstPorts)/maxPacketsToSingleDstPort
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 3, "tcp", -1, SQLITE_STATIC));

		SQL_RUN(SQLITE_ROW, sqlite3_step(computeMaxPacketsToPort));
		double maxPacketsToTcpPort = sqlite3_column_double(computeMaxPacketsToPort, 0);
		SQL_RUN(SQLITE_OK, sqlite3_reset(computeMaxPacketsToPort));

		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 3, "udp", -1, SQLITE_STATIC));

		SQL_RUN(SQLITE_ROW, sqlite3_step(computeMaxPacketsToPort));
		double maxPacketsToUdpPort = sqlite3_column_double(computeMaxPacketsToPort, 0);
		SQL_RUN(SQLITE_OK, sqlite3_reset(computeMaxPacketsToPort));

		featureValues[PORT_TRAFFIC_DISTRIBUTION] = totalPackets.second
				/ (featureValues[DISTINCT_TCP_PORTS] + featureValues[DISTINCT_UDP_PORTS])
				/ max(maxPacketsToTcpPort, maxPacketsToUdpPort);
	}


	// Dump all of our results back into the database
	for (int i = 0; i < DIM; i++)
	{
		SetFeatureSetValue(ip, interface, FeatureSet::m_featureNames[i], featureValues[i]);
	}

}

bool Database::Disconnect()
{
	// TODO: Should probably check return values. Oh the joys of C style APIs with no exceptions...
	sqlite3_finalize(insertSuspect);
	sqlite3_finalize(insertPacketCount);
	sqlite3_finalize(incrementPacketCount);
	sqlite3_finalize(insertPortContacted);
	sqlite3_finalize(incrementPortContacted);
	sqlite3_finalize(insertPacketSize);
	sqlite3_finalize(incrementPacketSize);
	sqlite3_finalize(setFeatureValue);
	sqlite3_finalize(insertFeatureValue);
	sqlite3_finalize(computePacketSizeMean);
	sqlite3_finalize(computePacketSizeVariance);
	sqlite3_finalize(computeDistinctIps);
	sqlite3_finalize(computeDistinctPorts);
	sqlite3_finalize(computeDistinctIpPorts);
	sqlite3_finalize(selectPacketCounts);
	sqlite3_finalize(computeMaxPacketsToIp);
	sqlite3_finalize(computeMaxPacketsToPort);

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

	SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertSuspect, 1, suspect->GetIpString().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertSuspect, 2, suspect->GetInterface().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertSuspect, 3, static_cast<long int>(suspect->m_features.m_startTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertSuspect, 4, static_cast<long int>(suspect->m_features.m_endTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertSuspect, 5, static_cast<long int>(suspect->m_features.m_lastTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_double(insertSuspect, 6, suspect->GetClassification()));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertSuspect, 7, suspect->GetHostileNeighbors()));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertSuspect, 8, suspect->GetIsHostile()));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertSuspect, 9, suspect->m_classificationNotes.c_str(), -1, SQLITE_TRANSIENT));

	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(insertSuspect));
	SQL_RUN(SQLITE_OK, sqlite3_reset(insertSuspect));
}

void Database::IncrementPacketCount(std::string ip, std::string interface, std::string type, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPacketCount, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPacketCount, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPacketCount, 3, type.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(incrementPacketCount, 4, increment));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(incrementPacketCount));
	SQL_RUN(SQLITE_OK, sqlite3_reset(incrementPacketCount));


	// If the update failed, we need to insert the port contacted count and set it
	if (sqlite3_changes(db) == 0)
	{
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketCount, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketCount, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketCount, 3, type.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertPacketCount, 4, increment));

		m_count++;
		SQL_RUN(SQLITE_DONE, sqlite3_step(insertPacketCount));
		SQL_RUN(SQLITE_OK, sqlite3_reset(insertPacketCount));
	}
}

void Database::IncrementPacketSizeCount(std::string ip, std::string interface, uint16_t size, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(incrementPacketSize, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(incrementPacketSize, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_int(incrementPacketSize, 3, size));
	SQL_RUN(SQLITE_OK, sqlite3_bind_int(incrementPacketSize, 4, increment));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(incrementPacketSize));
	SQL_RUN(SQLITE_OK, sqlite3_reset(incrementPacketSize));

	if (sqlite3_changes(db) == 0)
	{
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketSize, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketSize, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertPacketSize, 3, size));

		m_count++;
		SQL_RUN(SQLITE_DONE, sqlite3_step(insertPacketSize));
		SQL_RUN(SQLITE_OK, sqlite3_reset(insertPacketSize));
	}
}

void Database::IncrementPortContactedCount(std::string ip, std::string interface, string protocol, string dstip, int port, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	// Try to increment the port count if it exists
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 3, protocol.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 4, dstip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(incrementPortContacted, 5, port));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(incrementPortContacted, 6, increment));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(incrementPortContacted));
	SQL_RUN(SQLITE_OK, sqlite3_reset(incrementPortContacted));

	// If the update failed, we need to insert the port contacted count and set it to 1
	if (sqlite3_changes(db) == 0)
	{
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 3, protocol.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 4, dstip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertPortContacted, 5, port));

		m_count++;
		SQL_RUN(SQLITE_DONE, sqlite3_step(insertPortContacted));
		SQL_RUN(SQLITE_OK, sqlite3_reset(insertPortContacted));
	}
}

void Database::SetFeatureSetValue(std::string ip, std::string interface, std::string featureName, double value)
{
	int res;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(setFeatureValue, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(setFeatureValue, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(setFeatureValue, 3, featureName.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_double(setFeatureValue, 4, value));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(setFeatureValue));
	SQL_RUN(SQLITE_OK, sqlite3_reset(setFeatureValue));
}


// TODO DTC: hostile event table got removed. Would like to refactor this so it just makes a copy of the suspect into a different
// table along with all of the forien key references
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
