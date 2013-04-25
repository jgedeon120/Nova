//============================================================================
// Name        : FeatureSet.cpp
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
// Description : Maintains and calculates distinct features for individual Suspects
//					for use in classification of the Suspect.
//============================================================================

#include "SerializationHelper.h"
#include "FeatureSet.h"
#include "Logger.h"
#include "Config.h"
#include "Database.h"

#include <time.h>
#include <math.h>
#include <sstream>
#include <sys/un.h>

using namespace std;

namespace Nova
{

string FeatureSet::m_featureNames[] =
{
	"IP Traffic Distribution",
	"Port Traffic Distribution",
	"Packet Size Mean",
	"Packet Size Deviation",
	"Protected IPs Contacted",
	"Distinct TCP Ports Contacted",
	"Distinct UDP Ports Contacted",
	"Average TCP Ports Per Host",
	"Average UDP Ports Per Host",
	"TCP Percent SYN",
	"TCP Percent FIN",
	"TCP Percent RST",
	"TCP Percent SYN ACK",
	"Haystack Percent Contacted",
};

FeatureSet::FeatureSet()
{
	//Temp variables
	m_startTime = 2147483647; //2^31 - 1 (IE: Y2.038K bug) (IE: The largest standard Unix time value)
	m_endTime = 0;

	m_rstCount = 0;
	m_ackCount = 0;
	m_synCount = 0;
	m_finCount = 0;
	m_synAckCount = 0;

	m_packetCount = 0;
	m_tcpPacketCount = 0;
	m_udpPacketCount = 0;
	m_icmpPacketCount = 0;
	m_otherPacketCount = 0;
	m_bytesTotal = 0;
	m_lastTime = 0;

	//Features
	for(int i = 0; i < DIM; i++)
	{
		m_features[i] = 0;
	}
}

FeatureSet::~FeatureSet()
{
}

string FeatureSet::toString()
{
	stringstream ss;

	time_t start = m_startTime;
	time_t end = m_endTime;
	ss << "First packet seen at: " << ctime(&start) << endl;
	ss << "Last packet seen at: " << ctime(&end) << endl;
	ss << endl;
	ss << "Total bytes in IP packets: " << m_bytesTotal << endl;
	ss << "Packets seen: " << m_packetCount << endl;
	ss << "TCP Packets Seen: " << m_tcpPacketCount << endl;
	ss << "UDP Packets Seen: " << m_udpPacketCount << endl;
	ss << "ICMP Packets Seen: " << m_icmpPacketCount << endl;
	ss << "Other protocol Packets Seen: " << m_otherPacketCount << endl;
	ss << endl;
	ss << "TCP RST Packets: " << m_rstCount << endl;
	ss << "TCP ACK Packets: " << m_ackCount << endl;
	ss << "TCP SYN Packets: " << m_synCount << endl;
	ss << "TCP FIN Packets: " << m_finCount << endl;
	ss << "TCP SYN ACK Packets: " << m_synAckCount << endl;
	ss << endl;

	return ss.str();
}


void FeatureSet::UpdateEvidence(const Evidence &evidence)
{
	// Ensure our assumptions about valid packet fields are true
	if(evidence.m_evidencePacket.ip_dst == 0)
	{
		LOG(DEBUG, "Got packet with invalid source IP address of 0. Skipping.", "");
		return;
	}
	switch(evidence.m_evidencePacket.ip_p)
	{
		//If UDP
		case 17:
		{
			m_udpPacketCount++;

			IpPortCombination t;
			t.m_ip = evidence.m_evidencePacket.ip_dst;
			t.m_port = evidence.m_evidencePacket.dst_port;
			if (!m_hasUdpPortIpBeenContacted.keyExists(t))
			{
				m_hasUdpPortIpBeenContacted[t] = 1;
			}
			else
			{
				m_hasUdpPortIpBeenContacted[t]++;
			}

			break;
		}
		//If TCP
		case 6:
		{
			m_tcpPacketCount++;

			// Only count as an IP/port contacted if it looks like a scan (SYN or NULL packet)
			if ((evidence.m_evidencePacket.tcp_hdr.syn && !evidence.m_evidencePacket.tcp_hdr.ack)
					|| (!evidence.m_evidencePacket.tcp_hdr.syn && !evidence.m_evidencePacket.tcp_hdr.ack
							&& !evidence.m_evidencePacket.tcp_hdr.rst))
			{
				IpPortCombination t;
				t.m_ip = evidence.m_evidencePacket.ip_dst;
				t.m_port = evidence.m_evidencePacket.dst_port;
				if (!m_hasTcpPortIpBeenContacted.keyExists(t))
				{
					m_hasTcpPortIpBeenContacted[t] = 1;
				}
				else
				{
					m_hasTcpPortIpBeenContacted[t]++;
				}
			}

			if(evidence.m_evidencePacket.tcp_hdr.syn && evidence.m_evidencePacket.tcp_hdr.ack)
			{
				m_synAckCount++;
			}
			else if(evidence.m_evidencePacket.tcp_hdr.syn)
			{
				m_synCount++;
			}
			else if(evidence.m_evidencePacket.tcp_hdr.ack)
			{
				m_ackCount++;
			}

			if(evidence.m_evidencePacket.tcp_hdr.rst)
			{
				m_rstCount++;
			}

			if(evidence.m_evidencePacket.tcp_hdr.fin)
			{
				m_finCount++;
			}

			break;
		}
		//If ICMP
		case 1:
		{
			m_icmpPacketCount++;
			m_IPTable[evidence.m_evidencePacket.ip_dst]++;
			break;
		}
		//If untracked IP protocol or error case ignore it
		default:
		{
			m_otherPacketCount++;
			m_IPTable[evidence.m_evidencePacket.ip_dst]++;
			break;
		}
	}

	m_packetCount++;
	m_bytesTotal += evidence.m_evidencePacket.ip_len;


	m_packTable[evidence.m_evidencePacket.ip_len]++;
	m_lastTime = evidence.m_evidencePacket.ts;

	//Accumulate to find the lowest Start time and biggest end time.
	if(evidence.m_evidencePacket.ts < m_startTime)
	{
		m_startTime = evidence.m_evidencePacket.ts;
	}
	if(evidence.m_evidencePacket.ts > m_endTime)
	{
		m_endTime =  evidence.m_evidencePacket.ts;
	}
}


}
