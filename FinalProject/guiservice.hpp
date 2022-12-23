/**
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */

#ifndef GUI_SERVICE_HPP
#define GUI_SERVICE_HPP

#include "soa.hpp"
#include "pricingservice.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace std;

template<typename T>
class GUIConnector;
template<typename T>
class GUIPricingListener;

/**
* Service for outputing GUI with a certain throttle.
* Keyed on product identifier.
* Type T is the product type.
*/
template<typename T>
class GUIService : Service<string, Price<T>>
{

public:

	// Constructor and destructor
	GUIService();
	~GUIService();

	// Get data on our service given a key
	Price<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Price<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<Price<T>>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<Price<T>>*>& GetListeners() const;

	// Get the connector of the service
	GUIConnector<T>* GetConnector();

	// Get the listener of the service
	GUIPricingListener<T>* GetListener();

	// listens to streaming prices that should be throettled
	void ThroettleStreamingPrices(Price<T>& _price);


private:

	unordered_map<string, Price<T>> guis;
	vector<ServiceListener<Price<T>>*> listeners;
	GUIConnector<T>* connector;
	GUIPricingListener<T>* listener;
	boost::posix_time::ptime lastTime;
	boost::posix_time::time_duration throttle;
};

template<typename T>
GUIService<T>::GUIService()
{
	guis = unordered_map<string, Price<T>>();
	listeners = vector<ServiceListener<Price<T>>*>();
	connector = new GUIConnector<T>(this);
	listener = new GUIPricingListener<T>(this);
	lastTime = boost::posix_time::microsec_clock::local_time();
	throttle = boost::posix_time::millisec(300);
}

template<typename T>
GUIService<T>::~GUIService() {
	delete connector;
	delete listener;
}

template<typename T>
Price<T>& GUIService<T>::GetData(string _key)
{
	return guis[_key];
}

template<typename T>
void GUIService<T>::OnMessage(Price<T>& _data)
{
	guis[_data.GetProduct().GetProductId()] = _data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void GUIService<T>::AddListener(ServiceListener<Price<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<Price<T>>*>& GUIService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
GUIConnector<T>* GUIService<T>::GetConnector()
{
	return connector;
}

template<typename T>
GUIPricingListener<T>* GUIService<T>::GetListener()
{
	return listener;
}

template<typename T>
void GUIService<T>::ThroettleStreamingPrices(Price<T>& _price) {

	// calculate the time duration between two price updates
	boost::posix_time::ptime curTime = boost::posix_time::microsec_clock::local_time();
	auto time_duration = curTime - lastTime;

	if (time_duration > throttle) {
		connector->PublishGUI(curTime, _price); // since we have timestamp, we add a new publish()
		this->OnMessage(_price);
		lastTime = curTime;
	}
}

/**
* GUI Service Listener updating data to GUI Service.
* Type T is the product type.
*/
template<typename T>
class GUIPricingListener : public ServiceListener<Price<T>>
{

private:

	GUIService<T>* service;

public:

	// Connector and Destructor
	GUIPricingListener(GUIService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Price<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Price<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Price<T>& _data);

};

template<typename T>
GUIPricingListener<T>::GUIPricingListener(GUIService<T>* _service)
{
	service = _service;
}


template<typename T>
void GUIPricingListener<T>::ProcessAdd(Price<T>& _data)
{
	service->ThroettleStreamingPrices(_data);
}

template<typename T>
void GUIPricingListener<T>::ProcessRemove(Price<T>& _data) {}

template<typename T>
void GUIPricingListener<T>::ProcessUpdate(Price<T>& _data) {}


/**
* GUI Connector publishing data to gui.txt.
* Publish-only connector
* Type T is the product type.
*/
template<typename T>
class GUIConnector : public Connector<Price<T>>
{

private:

	GUIService<T>* service;

public:

	// Ctor
	GUIConnector(GUIService<T>* _service);

	// Publish data to the Connector
	void Publish(Price<T>& _data);

	// Subscribe data from the Connector
	void Subscribe(ifstream& _data);

	// new Publish function since we have timestamp now
	void PublishGUI(boost::posix_time::ptime time, Price<T> _data);

};

template<typename T>
GUIConnector<T>::GUIConnector(GUIService<T>* _service)
{
	service = _service;
}


template<typename T>
void GUIConnector<T>::Publish(Price<T>& _data){}

template<typename T>
void GUIConnector<T>::Subscribe(ifstream& _data) {}

template<typename T>
void GUIConnector<T>::PublishGUI(boost::posix_time::ptime time, Price<T> _data)
{
	string productId = _data.GetProduct().GetProductId();
	double midPrice = _data.GetMid();
	double spread = _data.GetBidOfferSpread();

	ofstream GUIOutput;
	GUIOutput.open("gui.txt", ios::app);
	GUIOutput << time << ", " << "CUSIP: " << productId << ", " << midPrice << ", " << spread << endl;
	GUIOutput.close();
}







#endif
