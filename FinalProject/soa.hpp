/**
 * soa.hpp
 * Definition of our Service Oriented Architecture (SOA) Service base class
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */

#ifndef SOA_HPP
#define SOA_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <string>
#include "products.hpp"

using namespace std;

/**
 * Definition of a generic base class ServiceListener to listen to add, update, and remve
 * events on a Service. This listener should be registered on a Service for the Service
 * to notify all listeners for these events.
 */
template<typename V>
class ServiceListener
{

public:

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(V &data) = 0;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(V &data) = 0;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(V &data) = 0;

};


/**
 * Definition of a generic base class Service.
 * Uses key generic type K and value generic type V.
 */
template<typename K, typename V>
class Service
{

public:

  // Get data on our service given a key
  virtual V& GetData(K key) = 0;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(V &data) = 0;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<V> *listener) = 0;

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<V>* >& GetListeners() const = 0;

};  


/**
 * Definition of a Connector class.
 * This will invoke the Service.OnMessage() method for subscriber Connectors
 * to push data to the Service.
 * Services can invoke the Publish() method on this Service to publish data to the Connector
 * for a publisher Connector.
 * Note that a Connector can be publisher-only, subscriber-only, or both publisher and susbcriber.
 */
template<typename V>
class Connector
{

public:

  // Publish data to the Connector
  virtual void Publish(V &data) = 0;

  // Subscribe data from Connector
  virtual void Subscribe(ifstream& data) = 0;

};


/* 
	Some useful functions
*/

// Return Bond product type given CUSIP
Bond GetProductType(string cusip) {
	if (cusip == "91282CFX4") return Bond("91282CFX4", CUSIP, "T", 0.04500, from_string("2024/11/30"));
	if (cusip == "91282CGA3") return Bond("91282CGA3", CUSIP, "T", 0.04000, from_string("2025/12/15"));
	if (cusip == "91282CFZ9") return Bond("91282CFZ9", CUSIP, "T", 0.03875, from_string("2027/11/30"));
	if (cusip == "91282CFY2") return Bond("91282CFY2", CUSIP, "T", 0.03875, from_string("2029/11/30"));
	if (cusip == "91282CFV8") return Bond("91282CFV8", CUSIP, "T", 0.04125, from_string("2032/11/15"));
	if (cusip == "912810TM0") return Bond("912810TM0", CUSIP, "T", 0.04000, from_string("2042/11/15"));
	if (cusip == "912810TL2") return Bond("912810TL2", CUSIP, "T", 0.04000, from_string("2052/11/15"));

}

// Convert fractional price to numerical price.
double GetNormalPrice(string price)
{
	string fp, sp, tp;
	auto pos_dot = price.find("-");
	fp = price.substr(0, pos_dot);
	sp = price.substr(pos_dot + 1, 2);
	tp = price.substr(pos_dot + 3, 1);
	if (tp == "+") tp = "4";

	return stod(fp) + stod(sp) / 32.0 + stod(tp) / 256.0;
}


// Convert numerical price to fractional price.
string GetQuotePrice(double price)
{
	int fp = floor(price);
	int tp = floor((price - fp) * 256.0);
	int sp = floor(tp / 8.0);
	tp = tp % 8;

	string tmp_sp = to_string(sp), tmp_tp = to_string(tp);
	if (sp < 10) tmp_sp = "0" + tmp_sp;
	if (tp == 4) tmp_tp = "+";
	return to_string(fp) + "-" + tmp_sp + tmp_tp;
}

// Get PV01 for different bonds
double GetPV01(string cusip)
{
	double pv01;
	if (cusip == "91282CFX4") pv01 = 0.0188;
	if (cusip == "91282CGA3") pv01 = 0.0276;
	if (cusip == "91282CFZ9") pv01 = 0.0452;
	if (cusip == "91282CFY2") pv01 = 0.0617;
	if (cusip == "91282CFV8") pv01 = 0.0862;
	if (cusip == "912810TM0") pv01 = 0.1442;
	if (cusip == "912810TL2") pv01 = 0.1992;
	return pv01;
}

#endif
