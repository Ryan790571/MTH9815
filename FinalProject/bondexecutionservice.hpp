/**
 * executionservice.hpp
 * Defines the data types and Service for executions.
 *
 * @author Breman Thuraisingham
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include "soa.hpp"
#include "algoexecutionservice.hpp"

template<typename T>
class AlgoExecutionListener;

/**
* Service for executing orders in the Market.
* Keyed on product identifier.
* Type T is the product type.
*/
template<typename T>
class ExecutionService : public Service<string, ExecutionOrder<T>>
{

public:

	// Ctor
	ExecutionService();

	// Dtor
	~ExecutionService();

	// Get data on our service given a key
	ExecutionOrder<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(ExecutionOrder<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<ExecutionOrder<T>>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<ExecutionOrder<T>>*>& GetListeners() const;

	// Get the listener of the service
	AlgoExecutionListener<T>* GetListener();

	// Execute an order in the market
	void ExecuteOrder(ExecutionOrder<T>& _executionOrder);

private:
	unordered_map<string, ExecutionOrder<T>> executionOrders;
	vector<ServiceListener<ExecutionOrder<T>>*> listeners;
	AlgoExecutionListener<T>* listener;
};

template<typename T>
ExecutionService<T>::ExecutionService()
{
	executionOrders = unordered_map<string, ExecutionOrder<T>>();
	listeners = vector<ServiceListener<ExecutionOrder<T>>*>();
	listener = new AlgoExecutionListener<T>(this);
}

template<typename T>
ExecutionService<T>::~ExecutionService() {
	delete listener;
}

template<typename T>
ExecutionOrder<T>& ExecutionService<T>::GetData(string _key)
{
	return executionOrders[_key];
}

template<typename T>
void ExecutionService<T>::OnMessage(ExecutionOrder<T>& _data)
{
	executionOrders[_data.GetProduct().GetProductId()] = _data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void ExecutionService<T>::AddListener(ServiceListener<ExecutionOrder<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<ExecutionOrder<T>>*>& ExecutionService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
AlgoExecutionListener<T>* ExecutionService<T>::GetListener()
{
	return listener;
}

template<typename T>
void ExecutionService<T>::ExecuteOrder(ExecutionOrder<T>& _executionOrder)
{
	this->OnMessage(_executionOrder);
}

/**
* Execution Service Listener subscribing data from AlgoExecutionService.
* Type T is the product type.
*/
template<typename T>
class AlgoExecutionListener : public ServiceListener<ExecutionOrder<T>>
{

private:

	ExecutionService<T>* service;

public:

	// Connector and Destructor
	AlgoExecutionListener(ExecutionService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(ExecutionOrder<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(ExecutionOrder<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(ExecutionOrder<T>& _data);

};

template<typename T>
AlgoExecutionListener<T>::AlgoExecutionListener(ExecutionService<T>* _service)
{
	service = _service;
}

template<typename T>
void AlgoExecutionListener<T>::ProcessAdd(ExecutionOrder<T>& _data)
{
	service->ExecuteOrder(_data);
}

template<typename T>
void AlgoExecutionListener<T>::ProcessRemove(ExecutionOrder<T>& _data) {}

template<typename T>
void AlgoExecutionListener<T>::ProcessUpdate(ExecutionOrder<T>& _data) {}

#endif
