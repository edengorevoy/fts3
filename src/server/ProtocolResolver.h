/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ProtocolResolver.h
 *
 *  Created on: Dec 3, 2012
 *      Author: Michal Simon
 */

#ifndef PROTOCOLRESOLVER_H_
#define PROTOCOLRESOLVER_H_

#include "server_dev.h"

#include "db/generic/SingleDbInstance.h"

#include <list>
#include <string>
#include <utility>

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

FTS3_SERVER_NAMESPACE_START

using namespace std;
using namespace db;
using namespace boost;

/**
 * The class aims to resolve the protocol parameters for a given transfer job.
 */
class ProtocolResolver {

	/**
	 * protocol data:
	 * - auto protocol (true means that auto was used)
	 * - number of streams
	 * - no activity timeout
	 * - TCP buffer size
	 * - url-copy timeout
	 */
	typedef tuple<bool, int, int, int, int> protocol;

	/**
	 * The link type
	 */
	enum LinkType {
		SE_PAIR = 0, //< SE pair
		GROUP_PAIR, //< SE group pair
		SOURCE_SE, //< standalonoe source SE
		SOURCE_GROUP, //< standalone source SE group
		SOURCE_WILDCARD, //< standalone default source SE
		DESTINATION_SE, //< standalone destination SE
		DESTINATION_GROUP, //< standalone destination SE group
		DESTINATION_WILDCARD //< standalone defalt destination SE
	};

	/**
	 * triplet that uniquely defines share configuration
	 * such triplets are assigned to transfer-job at submission time
	 */
	enum {
		SOURCE = 0,
		DESTINATION,
		VO
	};

	/**
	 * A tuple with all the information about a protocol
	 */
	enum {
		AUTO_PROTOCOL = 0,
		NOSTREAMS,
		NO_TX_ACTIVITY_TO,
		TCP_BUFFER_SIZE,
		URLCOPY_TX_TO
	};

public:

	/**
	 * Constructor.
	 *
	 * Loads the configurations assigned to the transfer job from the DB.
	 * Adds respective entries to the link table.
	 *
	 * @param job_id - transfer job ID
	 */
	ProtocolResolver(string &job_id);

	/**
	 * Destructor.
	 */
	~ProtocolResolver();

	/**
	 * Resolves the protocol parameters.
	 *
	 * @return an object containing protocol parameters (the memory has to be released by the user)
	 */
	bool resolve();

	/**
	 * checks if the configuration says to use auto tuning
	 *
	 * @return true if auto tuning should be used, false otherwise
	 */
	bool isAuto();

	/**
	 * gets the number of streams that should be used
	 *
	 * @return number of streams
	 */
	int getNoStreams();

	/**
	 * gets no activity timeout
	 *
	 * @return no activity timeout
	 */
	int getNoTxActiveTo();

	/**
	 * gets TCP buffer size
	 *
	 * @return TCP buffer size
	 */
	int getTcpBufferSize();

	/**
	 * gets url-copy timeout
	 *
	 * @return url-copy timeout
	 */
	int getUrlCopyTxTo();


private:

	/**
	 * Checks if given entity is a SE group
	 *
	 * @param entity name
	 *
	 * @return true if the entity is a group, false otherwise
	 */
	bool isGr(string name);

	/**
	 * Gets the first object from the link sublist that was initialized.
	 *
	 * @param l - sublist of link array
	 *
	 * @return first initialized object in the sublist, or an uninitialized object if non was found
	 */
	optional< pair<string, string> > getFirst(list<LinkType> l);

	/**
	 * Gets the protocol parameters for the given link
	 *
	 * @param link - source and destination pair
	 *
	 * @return an object containing protocol parameters
	 */
	optional<protocol> getProtocolCfg(optional< pair<string, string> > link);

	/**
	 * Merges two sets of protocol parameters.
	 *
	 * @param source_ptr - the protocol parameters of the source link
	 * @param destination_ptr - the protocol parameters of the destination link
	 *
	 * @return an object containing protocol parameters (the memory has to be released by the user)
	 */
	optional<protocol> merge(optional<protocol> source, optional<protocol> destination);

	/**
	 * Does the auto tuning in case the protocol was set to 'auto'
	 *
	 * @param source - the source hostname
	 * @param destination - the destination hostname
	 */
	void autotune();

	/// DB singleton instance
	GenericDbIfce* db;

	/// array containing respective source-destination pairs (corresponds to the LinkType enumeration)
	optional< pair<string, string> > link[8];

	/// stores the protocol parameters that have been resolved
	optional<protocol> prot;

	// the transfer job ID
	string& job_id;
};

FTS3_SERVER_NAMESPACE_END

#endif /* PROTOCOLRESOLVER_H_ */
