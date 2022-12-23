/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */

#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "algoexecutionservice.hpp"

// Trade sides
enum Side { BUY, SELL };

/**
 * Trade object with a price, side, and quantity on a particular book.
 * Type T is the product type.
 */
template<typename T>
class Trade
{

public:

  // ctor for a trade
  Trade(const T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side);
  Trade() = default;

  // Get the product
  const T& GetProduct() const;

  // Get the trade ID
  const string& GetTradeId() const;

  // Get the mid price
  double GetPrice() const;

  // Get the book
  const string& GetBook() const;

  // Get the quantity
  long GetQuantity() const;

  // Get the side
  Side GetSide() const;

private:
  T product;
  string tradeId;
  double price;
  string book;
  long quantity;
  Side side;

};

template<typename T>
Trade<T>::Trade(const T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side) :
  product(_product)
{
  tradeId = _tradeId;
  price = _price;
  book = _book;
  quantity = _quantity;
  side = _side;
}

template<typename T>
const T& Trade<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& Trade<T>::GetTradeId() const
{
  return tradeId;
}

template<typename T>
double Trade<T>::GetPrice() const
{
  return price;
}

template<typename T>
const string& Trade<T>::GetBook() const
{
  return book;
}

template<typename T>
long Trade<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
Side Trade<T>::GetSide() const
{
  return side;
}


template<typename T>
class TradeBookingConnector;
template<typename T>
class ExecutionListener;


/**
 * Trade Booking Service to book trades to a particular book.
 * Keyed on trade id.
 * Type T is the product type.
 */
template<typename T>
class TradeBookingService : public Service<string, Trade <T> >
{

public:

	// Ctor
	TradeBookingService();

	// Get data on our service given a key
	Trade<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Trade<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<Trade<T>>* _listener);

	// Get the listener of the service
	ExecutionListener<T>* GetListener();

	// Get all listeners on the Service
	const vector<ServiceListener<Trade<T>>*>& GetListeners() const;

	// Get the connector of the service
	TradeBookingConnector<T>* GetConnector();

	// add the trade
	void AddTrade(Trade<T>& trade);

	// Dtor
	~TradeBookingService();
	

private:

	unordered_map<string, Trade<T>> trades;
	vector<ServiceListener<Trade<T>>*> listeners;
	TradeBookingConnector<T>* connector;
	ExecutionListener<T>* listener;


};

template<typename T>
TradeBookingService<T>::TradeBookingService()
{
	trades = unordered_map<string, Trade<T>>();
	listeners = vector<ServiceListener<Trade<T>>*>();
	connector = new TradeBookingConnector<T>(this);
	listener = new ExecutionListener<T>(this);
}

template<typename T>
TradeBookingService<T>::~TradeBookingService() {
	delete connector;
	delete listener;
}

template<typename T>
Trade<T>& TradeBookingService<T>::GetData(string _key)
{
	return trades[_key];
}

template<typename T>
void TradeBookingService<T>::OnMessage(Trade<T>& _data)
{
	trades[_data.GetTradeId()] = _data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void TradeBookingService<T>::AddListener(ServiceListener<Trade<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<Trade<T>>*>& TradeBookingService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
ExecutionListener<T>* TradeBookingService<T>::GetListener()
{
	return listener;
}

template<typename T>
TradeBookingConnector<T>* TradeBookingService<T>::GetConnector()
{
	return connector;
}

template<typename T>
void TradeBookingService<T>::AddTrade(Trade<T> &trade)
{
	this->OnMessage(trade);
}


/**
* Trade Booking Connector (Subscribe only)
* Type T is the product type.
*/
template<typename T>
class TradeBookingConnector : public Connector<Trade<T>>
{
private:

	TradeBookingService<T>* service;

public:

	// Ctor
	TradeBookingConnector(TradeBookingService<T>* _service);

	// Publish data to the Connector
	void Publish(Trade<T>& _data);

	// Subscribe data from the Connector
	void Subscribe(ifstream& _data);

};

template<typename T>
TradeBookingConnector<T>::TradeBookingConnector(TradeBookingService<T>* _service)
{
	service = _service;
}

template<typename T>
void TradeBookingConnector<T>::Publish(Trade<T>& _data) {}

template<typename T>
void TradeBookingConnector<T>::Subscribe(ifstream& _data)
{
	string line;
	while (getline(_data, line))
	{
		stringstream tradeInfo(line);
		vector<string> tradeData;
		string p;
		
		while (getline(tradeInfo, p, ','))
		{
			tradeData.push_back(p);
		}

		string productId = tradeData[0];
		T product = GetProductType(productId);
		string tradeId = tradeData[1];
		double price = GetNormalPrice(tradeData[2]);
		string book = tradeData[3];
		long quantity = stol(tradeData[4]);
		
		
		Side side;
		if (tradeData[5] == "BUY") side = BUY;
		if (tradeData[5] == "SELL") side = SELL;
		Trade<T> trade(product, tradeId, price, book, quantity, side);

		service->OnMessage(trade);
	}
}

/**
* Trade Booking Service Listener
* Type T is the product type.
*/
template<typename T>
class ExecutionListener : public ServiceListener<ExecutionOrder<T>>
{

private:

	TradeBookingService<T>* service;
	long num; // decide the book traded

public:

	// Connector and Destructor
	ExecutionListener(TradeBookingService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(ExecutionOrder<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(ExecutionOrder<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(ExecutionOrder<T>& _data);

};

template<typename T>
ExecutionListener<T>::ExecutionListener(TradeBookingService<T>* _service)
{
	service = _service;
	num = 0;
}

template<typename T>
void ExecutionListener<T>::ProcessAdd(ExecutionOrder<T>& _data)
{
	T product = _data.GetProduct();
	PricingSide pricingSide = _data.GetPricingSide();
	string tradeId = "TRADE-EXECUTE-" + _data.GetOrderId();
	double price = _data.GetPrice();
	long visibleQuantity = _data.GetVisibleQuantity();
	long hiddenQuantity = _data.GetHiddenQuantity();
	long quantity = visibleQuantity + hiddenQuantity; // Market orders

	Side side;
	if (pricingSide == BID) side = BUY;
	if (pricingSide == OFFER) side = SELL;

	
	string book;
	if (num % 3 == 0) book = "TRSY1";
	if (num % 3 == 1) book = "TRSY2";
	if (num % 3 == 2) book = "TRSY3";


	Trade<T> trade(product, tradeId, price, book, quantity, side);
	service->OnMessage(trade);
	service->AddTrade(trade);
}

template<typename T>
void ExecutionListener<T>::ProcessRemove(ExecutionOrder<T>& _data) {}

template<typename T>
void ExecutionListener<T>::ProcessUpdate(ExecutionOrder<T>& _data) {}

#endif
