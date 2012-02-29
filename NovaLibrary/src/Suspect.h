//============================================================================
// Name        : Suspect.h
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
// Description : A Suspect object with identifying information and traffic
//					features so that each entity can be monitored and classified.
//============================================================================/*

#ifndef SUSPECT_H_
#define SUSPECT_H_

#include "FeatureSet.h"
#include "Point.h"

using namespace std;
using namespace Nova;

namespace Nova{

// A Suspect represents a single actor on the network, whether good or bad.
// Suspects are the target of classification and a major part of Nova.
class Suspect
{

public:

	Suspect();

	//	Destructor. Has to delete the FeatureSet object within.
	~Suspect();

	//	Constructor from a Packet
	//		packet - Used to set the IP address and initial evidence of the suspect
	Suspect(Packet packet);

	//	Converts suspect into a human readable string
	//		featureEnabled: Array of size DIM that specifies which features to return in the string
	// 	Returns: Human readable string of the given feature
	string ToString(bool featureEnabled[]);

	//	Add an additional piece of evidence to this suspect
	// Does not take actions like reclassifying or calculating features.
	//		packet - Packet headers to extract evidence from
	void AddEvidence(Packet packet);

	//Returns a copy of the evidence vector so that it can be read.
	vector <Packet> GetEvidence(); //TODO

	//Clears the evidence vector, returns 0 on success
	void ClearEvidence(); //TODO

	// Calculates the feature set for this suspect
	// 		isTraining - True for training data gathering mode
	//		featuresEnabled - bitmask of features to calculate
	void CalculateFeatures(uint32_t featuresEnabled);

	// Stores the Suspect information into the buffer, retrieved using deserializeSuspect
	//		buf - Pointer to buffer where serialized data will be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeSuspect(u_char * buf);

	// Reads Suspect information from a buffer originally populated by serializeSuspect
	//		buf - Pointer to buffer where the serialized suspect is
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeSuspect(u_char * buf);

	// Reads Suspect information from a buffer originally populated by serializeSuspect
	// expects featureSet data appended by serializeFeatureData after serializeSuspect
	//		buf - Pointer to buffer where serialized data resides
	//		isLocal -
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeSuspectWithData(u_char * buf, bool isLocal);


	//Returns a copy of the suspects in_addr, must not be locked or is locked by the owner
	//Returns: Suspect's in_addr or NULL on failure
	in_addr_t GetIpAddress(); //TODO
	//Sets the suspects in_addr, must have the lock to perform this operation
	//Returns: 0 on success
	void SetIpAddress(in_addr_t ip); //TODO


	//Returns a copy of the Suspects classification double, must not be locked or is locked by the owner
	// Returns -1 on failure
	double GetClassification(); //TODO
	//Sets the suspect's classification, must have the lock to perform this operation
	//Returns 0 on success
	void SetClassification(double n); //TODO


	//Returns the number of hostile neighbors, must not be locked or is locked by the owner
	int GetHostileNeighbors(); //TODO
	//Sets the number of hostile neighbors, must have the lock to perform this operation
	void SetHostileNeighbors(int i); //TODO


	//Returns the hostility bool of the suspect, must not be locked or is locked by the owner
	bool GetIsHostile(); //TODO
	//Sets the hostility bool of the suspect, must have the lock to perform this operation
	void SetIsHostile(bool b); //TODO


	//Returns the needs classification bool, must not be locked or is locked by the owner
	bool GetNeedsClassificationUpdate(); //TODO
	//Sets the needs classification bool, must have the lock to perform this operation
	void SetNeedsClassificationUpdate(bool b); //TODO


	//Returns the needs feature update bool, must not be locked or is locked by the owner
	bool GetNeedsFeatureUpdate(); //TODO
	//Sets the neeeds feature update bool, must have the lock to perform this operation
	void SetNeedsFeatureUpdate(bool b); //TODO


	//Returns the flagged by silent alarm bool, must not be locked or is locked by the owner
	bool GetFlaggedByAlarm(); //TODO
	//Sets the flagged by silent alarm bool, must have the lock to perform this operation
	void SetFlaggedByAlarm(bool b); //TODO


	//Returns the 'from live capture' bool, must not be locked or is locked by the owner
	bool GetIsLive(); //TODO
	//Sets the 'from live capture' bool, must have the lock to perform this operation
	void SetIsLive(bool b); //TODO


	//Returns a copy of the suspects FeatureSet, must not be locked or is locked by the owner
	FeatureSet GetFeatures(); //TODO
	//Sets or overwrites the suspects FeatureSet, must have the lock to perform this operation
	void SetFeatures(FeatureSet fs); //TODO

	//Adds the feature set 'fs' to the suspect's feature set
	void AddFeatureSet(FeatureSet fs); //TODO
	//Subtracts the feature set 'fs' from the suspect's feature set
	void SubtractFeatureSet(FeatureSet fs); //TODO

	//Clears the feature set of the suspect
	void ClearFeatures(); //TODO


	//Returns the accuracy double of the feature using featureIndex fi, must not be locked or is locked by the owner
	double GetFeatureAccuracy(featureIndex fi); //TODO
	//Sets the accuracy double of the feature using featureIndex fi, must have the lock to perform this operation
	void SetFeatureAccuracy(featureIndex fi, double d); //TODO

	//Returns a copy of the suspect's ANNpoint, must not be locked or is locked by the owner
	ANNpoint GetAnnPoint(); //TODO

	//Returns the pthread_t owner, returns NULL if suspect is not checked out
	pthread_t GetOwner();

	//Returns true if the suspect is checked out by a thread
	bool HasOwner();

	//Sets the pthread_t 'owner'
	//		tid: unique thread identifier retrieved from pthread_self();
	int SetOwner(pthread_t tid);

	//Flags the suspect as no longer 'checked out'
	int UnsetOwner();

//private: //TODO Uncomment private and ensure suspects are fully accessible using thread-safe accessors.
	//Develop some method for making concurrent changes without redundantly locking and unlocking the suspects

	// The IP address of the suspect. This field serves as a unique identifier for the Suspect
	struct in_addr m_IpAddress;

	// The current classification assigned to this suspect.
	//		0-1, where 0 is almost surely benign, and 1 is almost surely hostile.
	//		-1 indicates no classification or error.
	double m_classification;
	int m_hostileNeighbors;

	// Is the classification above the current threshold? IE: What conclusion has the CE come to?
	bool m_isHostile;

	// Does the classification need updating?
	//		IE: Has the evidence changed since last it was calculated?
	bool m_needsClassificationUpdate;

	// Does the FeatureSet need updating?
	//		IE: Has the evidence changed since last it was calculated?
	bool m_needsFeatureUpdate;

	// Has this suspect been the subject of an alarm from another Nova instance?
	bool m_flaggedByAlarm;

	// Is this a live capture or is NOVA reading from a pcap file?
	bool m_isLive;

	// The Feature Set for this Suspect
	FeatureSet m_features;
	double m_featureAccuracy[DIM];

	// The feature set in the format that ANN requires.
	ANNpoint m_annPoint;

	// A listing of all the events (evidence) that originated from this suspect
	vector <Packet> m_evidence;

private:

	//Lock used to maintain concurrency between threads
	pthread_rwlock_t m_lock;

	pthread_t m_owner;
	bool m_hasOwner;

	//Write locks the suspect
	void WrlockSuspect(); //TODO

	//Read Locks the suspect
	void RdlockSuspect(); //TODO

	//Unlocks the suspect
	void UnlockSuspect(); //TODO

};

}

#endif /* SUSPECT_H_ */
