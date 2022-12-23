/**
 * historicaldataservice.hpp
 *
 * Defines the data types and Service for historical data.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */

#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include "soa.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace std;

template<typename T>
class HistoricalDataConnector;
template<typename T>
class ToHistoricalDataListener;

enum PersistType { POSITION, RISK, EXECUTION, STREAMING, INQUIRY };

/**
 * Service for processing and persisting historical data to a persistent store.
 * Keyed on some persistent key.
 * Type T is the data type to persist.
 */
template<typename T>
class HistoricalDataService : Service<string, T>
{

public:

	// Constructor and destructor
	HistoricalDataService(PersistType _type);
	~HistoricalDataService();

	// Get data on our service given a key
	T& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(T& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<T>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<T>*>& GetListeners() const;

	// Get the connector of the service
	HistoricalDataConnector<T>* GetConnector();

	// Get the listener of the service
	ToHistoricalDataListener<T>* GetListener();

	// Get the service type that historical data comes from
	PersistType GetPersistType() const;

	// Persist data to a store
	void PersistData(string _persistKey, T& _data);

private:

	unordered_map<string, T> historicalDatas;
	vector<ServiceListener<T>*> listeners;
	HistoricalDataConnector<T>* connector;
	ToHistoricalDataListener<T>* listener;
	PersistType type;

};

template<typename T>
HistoricalDataService<T>::HistoricalDataService(PersistType _type)
{
	historicalDatas = unordered_map<string, T>();
	listeners = vector<ServiceListener<T>*>();
	connector = new HistoricalDataConnector<T>(this);
	listener = new ToHistoricalDataListener<T>(this);
	type = _type;
}

template<typename T>
HistoricalDataService<T>::~HistoricalDataService() {
	delete connector;
	delete listener;
}

template<typename T>
T& HistoricalDataService<T>::GetData(string _key)
{
	return historicalDatas[_key];
}

template<typename T>
void HistoricalDataService<T>::OnMessage(T& _data)
{ 
	// No need to update to its listeners 
	historicalDatas[_data.GetProduct().GetProductId()] = _data;
	connector->Publish(_data);
}

template<typename T>
void HistoricalDataService<T>::AddListener(ServiceListener<T>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<T>*>& HistoricalDataService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
HistoricalDataConnector<T>* HistoricalDataService<T>::GetConnector()
{
	return connector;
}

template<typename T>
ToHistoricalDataListener<T>* HistoricalDataService<T>::GetListener()
{
	return listener;
}

template<typename T>
PersistType HistoricalDataService<T>::GetPersistType() const
{
	return type;
}

template<typename T>
void HistoricalDataService<T>::PersistData(string _persistKey, T& _data)
{
	this->OnMessage(_data);
}



/**
* Historical Data Connector publishing data to output .txt files
* Type T is the data type to persist.
*/
template<typename T>
class HistoricalDataConnector : public Connector<T>
{

private:

	HistoricalDataService<T>* service;

public:

	// Connector and Destructor
	HistoricalDataConnector(HistoricalDataService<T>* _service);

	// Publish data to the Connector
	void Publish(T& _data);

	// Subscribe data from the Connector
	void Subscribe(ifstream& _data);

};

template<typename T>
HistoricalDataConnector<T>::HistoricalDataConnector(HistoricalDataService<T>* _service)
{
	service = _service;
}

template<typename T>
void HistoricalDataConnector<T>::Publish(T& _data)
{
	// Output different files according to persist data type
	PersistType type = service->GetPersistType();
	ofstream file;
	string fileName;

	if (type == POSITION) fileName = "positions.txt";
	if (type == RISK) fileName = "risk.txt";
	if (type == EXECUTION) fileName = "executions.txt";
	if (type == STREAMING) fileName = "streaming.txt";
	if (type == INQUIRY) fileName = "allinquiries.txt";
	file.open(fileName, ios::app);

	// call back print() for each persist data type
	// also output the timestamp
	boost::posix_time::ptime curTime = boost::posix_time::microsec_clock::local_time();
	file << curTime << ", " << _data.print() << endl;
	file.close();
}

template<typename T>
void HistoricalDataConnector<T>::Subscribe(ifstream& _data) {}


/**
* Historical Data Service Listening data from other service.
* Type T is the data type to persist.
*/
template<typename T>
class ToHistoricalDataListener : public ServiceListener<T>
{

private:

	HistoricalDataService<T>* service;

public:

	// Connector and Destructor
	ToHistoricalDataListener(HistoricalDataService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(T& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(T& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(T& _data);

};

template<typename T>
ToHistoricalDataListener<T>::ToHistoricalDataListener(HistoricalDataService<T>* _service)
{
	service = _service;
}

template<typename T>
void ToHistoricalDataListener<T>::ProcessAdd(T& _data)
{
	service->PersistData("", _data);
}

template<typename T>
void ToHistoricalDataListener<T>::ProcessRemove(T& _data) {}

template<typename T>
void ToHistoricalDataListener<T>::ProcessUpdate(T& _data) {}


#endif