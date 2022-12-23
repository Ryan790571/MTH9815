/**
 * algoexecutionservice.hpp
 * Defines the data types and Service for executions.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */
#ifndef ALGO_EXECUTION_SERVICE_HPP
#define ALGO_EXECUTION_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include <algorithm>
#include "marketdataservice.hpp"

enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

enum Market { BROKERTEC, ESPEED, CME };

/**
 * An execution order that can be placed on an exchange.
 * Type T is the product type.
 */
template<typename T>
class ExecutionOrder
{

public:

  // ctor for an order
  ExecutionOrder(const T &_product, PricingSide _side, string _orderId, 
	  OrderType _orderType, double _price, double _visibleQuantity, 
	  double _hiddenQuantity, string _parentOrderId, bool _isChildOrder);
  ExecutionOrder() = default;

  // Get the product
  const T& GetProduct() const;

  // Get the pricing side
  PricingSide GetPricingSide() const;

  // Get the order ID
  const string& GetOrderId() const;

  // Get the order type on this order
  OrderType GetOrderType() const;

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity
  long GetHiddenQuantity() const;

  // Get the parent order ID
  const string& GetParentOrderId() const;

  // Is child order?
  bool IsChildOrder() const;

  // Print
  string print();

private:
  T product;
  PricingSide side;
  string orderId;
  OrderType orderType;
  double price;
  double visibleQuantity;
  double hiddenQuantity;
  string parentOrderId;
  bool isChildOrder;

};


template<typename T>
ExecutionOrder<T>::ExecutionOrder(const T& _product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder) :
	product(_product)
{
	side = _side;
	orderId = _orderId;
	orderType = _orderType;
	price = _price;
	visibleQuantity = _visibleQuantity;
	hiddenQuantity = _hiddenQuantity;
	parentOrderId = _parentOrderId;
	isChildOrder = _isChildOrder;
}

template<typename T>
const T& ExecutionOrder<T>::GetProduct() const
{
	return product;
}

template<typename T>
PricingSide ExecutionOrder<T>::GetPricingSide() const
{
	return side;
}

template<typename T>
const string& ExecutionOrder<T>::GetOrderId() const
{
	return orderId;
}

template<typename T>
OrderType ExecutionOrder<T>::GetOrderType() const
{
	return orderType;
}

template<typename T>
double ExecutionOrder<T>::GetPrice() const
{
	return price;
}

template<typename T>
long ExecutionOrder<T>::GetVisibleQuantity() const
{
	return visibleQuantity;
}

template<typename T>
long ExecutionOrder<T>::GetHiddenQuantity() const
{
	return hiddenQuantity;
}

template<typename T>
const string& ExecutionOrder<T>::GetParentOrderId() const
{
	return parentOrderId;
}

template<typename T>
bool ExecutionOrder<T>::IsChildOrder() const
{
	return isChildOrder;
}

template<typename T>
string ExecutionOrder<T>::print()
{
	stringstream output;
	output << "CUSIP: " << product.GetProductId() << ", ";

	string orderSide;
	if (side == BID) orderSide = "bid";
	if (side == OFFER) orderSide = "offer";
	output << "Side: " << orderSide << ", ";
	output << "Order ID: " << orderId << ", ";

	string type;
	if (orderType == FOK) type = "FOK";
	if (orderType == IOC) type = "IOC";
	if (orderType == MARKET) type = "MARKET";
	if (orderType == LIMIT) type = "LIMIT";
	if (orderType == STOP) type = "STOP";
	output << "Order type: " << type << ", ";

	output << "Price: " << GetQuotePrice(price) << ", ";
	output << "Visible quantity: " << to_string(visibleQuantity) << ", ";
	output << "Hidden quantity: " << to_string(hiddenQuantity) << ", ";
	output << "Parent order ID: " << parentOrderId << ", ";
	output << "Is child order: " << to_string(isChildOrder) << ", ";

	return output.str();
}


template<typename T>
class MarketDataListener;

/**
 * Service for executing orders on an exchange.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class AlgoExecutionService : public Service<string,ExecutionOrder <T> >
{

public:

	// Ctor
	AlgoExecutionService();

	// Dtor
	~AlgoExecutionService();

	// Get data on our service given a key
	ExecutionOrder<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(ExecutionOrder<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<ExecutionOrder<T>>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<ExecutionOrder<T>>*>& GetListeners() const;

	// Get the listener of the service
	MarketDataListener<T>* GetListener();

	// Execute an order on a market
	void AlgoExecuteOrder(OrderStacks<T>& _orderBook);

private:
	unordered_map<string, ExecutionOrder<T>> algoExecutions;
	vector<ServiceListener<ExecutionOrder<T>>*> listeners;
	MarketDataListener<T>* listener;
	bool isBuy; // help to decide whether buy or sell  
	int numID; // help to generate unique ID for trades
};


template<typename T>
AlgoExecutionService<T>::AlgoExecutionService()
{
	algoExecutions = unordered_map<string, ExecutionOrder<T>>();
	listeners = vector<ServiceListener<ExecutionOrder<T>>*>();
	listener = new MarketDataListener<T>(this);
	isBuy = true;
	numID = 0;
}

template<typename T>
AlgoExecutionService<T>::~AlgoExecutionService() {
	delete listener;
}

template<typename T>
ExecutionOrder<T>& AlgoExecutionService<T>::GetData(string _key)
{
	return algoExecutions[_key];
}

template<typename T>
void AlgoExecutionService<T>::OnMessage(ExecutionOrder<T>& _data)
{
	algoExecutions[_data.GetProduct().GetProductId()] = _data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void AlgoExecutionService<T>::AddListener(ServiceListener<ExecutionOrder<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<ExecutionOrder<T>>*>& AlgoExecutionService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
MarketDataListener<T>* AlgoExecutionService<T>::GetListener()
{
	return listener;
}

template<typename T>
void AlgoExecutionService<T>::AlgoExecuteOrder(OrderStacks<T>& orderBook)
{
	T product = orderBook.GetProduct();
	string productId = product.GetProductId();
	string orderId = to_string(numID);
	double price;
	long quantity;

	BidOffer bidOffer = orderBook.GetBestBidOffer();
	Order bestBid = bidOffer.GetBidOrder();
	Order bestOffer = bidOffer.GetOfferOrder();
	double bidPrice = bestBid.GetPrice();
	long bidQuantity = bestBid.GetQuantity();
	double offerPrice = bestOffer.GetPrice();
	long offerQuantity = bestOffer.GetQuantity();

	if (offerPrice - bidPrice <= 1.0/128.0)
	{
		// We all use Market orders
		// We are crossing the spread, so BID will get offer price.
		if (isBuy) {
			ExecutionOrder<T> algoExecution(product, BID, orderId, MARKET, offerPrice, offerQuantity, 0, "NA", false);
			this->OnMessage(algoExecution);
		}
		else {
			ExecutionOrder<T> algoExecution(product, OFFER, orderId, MARKET, bidPrice, bidQuantity, 0, "NA", false);
			this->OnMessage(algoExecution);
		}

		isBuy = !isBuy; // alternate the direction of order
		numID++; // change the next order ID

	}
}


/**
* Algo Execution Service Listener
* Type T is the product type.
*/
template<typename T>
class MarketDataListener : public ServiceListener<OrderStacks<T>>
{

private:

	AlgoExecutionService<T>* service;

public:

	// Connector and Destructor
	MarketDataListener(AlgoExecutionService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(OrderStacks<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(OrderStacks<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(OrderStacks<T>& _data);

};

template<typename T>
MarketDataListener<T>::MarketDataListener(AlgoExecutionService<T>* _service)
{
	service = _service;
}

template<typename T>
void MarketDataListener<T>::ProcessAdd(OrderStacks<T>& _data)
{
	service->AlgoExecuteOrder(_data);
}

template<typename T>
void MarketDataListener<T>::ProcessRemove(OrderStacks<T>& _data) {}

template<typename T>
void MarketDataListener<T>::ProcessUpdate(OrderStacks<T>& _data) {}

#endif
