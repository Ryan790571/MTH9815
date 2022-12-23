/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */
#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "algostreamingservice.hpp"


template<typename T>
class AlgoStreamingListener;

/**
* Streaming service to publish two-way prices.
* Keyed on product identifier.
* Type T is the product type.
*/

template<typename T>
class StreamingService : public Service<string, PriceStream<T>>
{

private:

	unordered_map<string, PriceStream<T>> priceStreams;
	vector<ServiceListener<PriceStream<T>>*> listeners;
	AlgoStreamingListener<T>* listener;

public:

	// Ctor
	StreamingService();

	// Dtor
	~StreamingService();

	// Get data on our service given a key
	PriceStream<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(PriceStream<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<PriceStream<T>>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<PriceStream<T>>*>& GetListeners() const;

	// Get the listener of the service
	AlgoStreamingListener<T>* GetListener();

	// Publish two-way prices
	void PublishPrice(PriceStream<T>& _priceStream);

};

template<typename T>
StreamingService<T>::StreamingService()
{
	priceStreams = unordered_map<string, PriceStream<T>>();
	listeners = vector<ServiceListener<PriceStream<T>>*>();
	listener = new AlgoStreamingListener<T>(this);
}

template<typename T>
StreamingService<T>::~StreamingService() {
	delete listener;
}

template<typename T>
PriceStream<T>& StreamingService<T>::GetData(string _key)
{
	return priceStreams[_key];
}

template<typename T>
void StreamingService<T>::OnMessage(PriceStream<T>& _data)
{
	priceStreams[_data.GetProduct().GetProductId()] = _data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void StreamingService<T>::AddListener(ServiceListener<PriceStream<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<PriceStream<T>>*>& StreamingService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
AlgoStreamingListener<T>* StreamingService<T>::GetListener()
{
	return listener;
}

template<typename T>
void StreamingService<T>::PublishPrice(PriceStream<T>& _priceStream)
{
	this->OnMessage(_priceStream);
}


/**
* Streaming Service Listener subscribing data from Algo Streaming Service
* Type T is the product type.
*/
template<typename T>
class AlgoStreamingListener : public ServiceListener<PriceStream<T>>
{

private:

	StreamingService<T>* service;

public:

	// Connector and Destructor
	AlgoStreamingListener(StreamingService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(PriceStream<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(PriceStream<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(PriceStream<T>& _data);

};

template<typename T>
AlgoStreamingListener<T>::AlgoStreamingListener(StreamingService<T>* _service)
{
	service = _service;
}

template<typename T>
void AlgoStreamingListener<T>::ProcessAdd(PriceStream<T>& _data)
{
	service->PublishPrice(_data);
}

template<typename T>
void AlgoStreamingListener<T>::ProcessRemove(PriceStream<T>& _data) {}

template<typename T>
void AlgoStreamingListener<T>::ProcessUpdate(PriceStream<T>& _data) {}


#endif
