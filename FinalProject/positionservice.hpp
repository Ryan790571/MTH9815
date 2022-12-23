/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <string>
#include <unordered_map>
#include "soa.hpp"
#include "tradebookingservice.hpp"

using namespace std;

/**
 * Position class in a particular book.
 * Type T is the product type.
 */
template<typename T>
class Position
{

public:

  // ctor for a position
  Position(const T &_product);
  Position() = default;

  // Get the product
  const T& GetProduct() const;

  // Get the position quantity
  long GetPosition(string &book);

  // Set the position quantity
  void AddPosition(string& _book, long _position);

  // Get the aggregate position
  long GetAggregatePosition();

  // Print the position
  string print();

private:
  T product;
  unordered_map<string, long> positions;

};

template<typename T>
Position<T>::Position(const T& _product): product(_product) {
}

template<typename T>
const T& Position<T>::GetProduct() const
{
	return product;
}

template<typename T>
long Position<T>::GetPosition(string& book)
{
	return positions[book];
}

template<typename T>
void Position<T>::AddPosition(string& _book, long _position)
{
	positions[_book] += _position;
}

template<typename T>
long Position<T>::GetAggregatePosition()
{
	long aggPosition = 0;
	for (auto& p : positions)
	{
		aggPosition += p.second;
	}
	return aggPosition;
}

template<typename T>
string Position<T>::print()
{
	stringstream output;
	output << "CUSIP: " << product.GetProductId() << ", ";

	for (auto p : positions) {
		output << p.first << ": " << p.second << ", ";
	}

	// Also print aggregate position
	output << "Aggregate: " << to_string(this->GetAggregatePosition());

	return output.str();
}

template<typename T>
class TradeBookingListener;

/**
 * Position Service to manage positions across multiple books and bonds.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PositionService : public Service<string,Position <T> >
{

public:

	// Ctor
	PositionService();

	// Get data on our service given a key
	Position<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Position<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<Position<T>>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<Position<T>>*>& GetListeners() const;

	// Get the listener of the service
	TradeBookingListener<T>* GetListener();

	// Add a trade to the service
	virtual void AddTrade(const Trade<T>& _trade);

	// Dtor
	~PositionService();


private:

	unordered_map<string, Position<T>> positions;
	vector<ServiceListener<Position<T>>*> listeners;
	TradeBookingListener<T>* listener;
};

template<typename T>
PositionService<T>::PositionService()
{
	positions = unordered_map<string, Position<T>>();
	listeners = vector<ServiceListener<Position<T>>*>();
	listener = new TradeBookingListener<T>(this);
}

template<typename T>
PositionService<T>::~PositionService() {
	delete listener;
}

template<typename T>
Position<T>& PositionService<T>::GetData(string _key)
{
	return positions[_key];
}

template<typename T>
void PositionService<T>::OnMessage(Position<T>& _data)
{
	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void PositionService<T>::AddListener(ServiceListener<Position<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
TradeBookingListener<T>* PositionService<T>::GetListener()
{
	return listener;
}

template<typename T>
const vector<ServiceListener<Position<T>>*>& PositionService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
void PositionService<T>::AddTrade(const Trade<T>& trade)
{
	string productId = trade.GetProduct().GetProductId();
	double price = trade.GetPrice();
	string book = trade.GetBook();
	long quantity = trade.GetQuantity();
	Side side = trade.GetSide();
	if (side == SELL) quantity = -quantity;
	
	// See if already record the trade
	if (positions.find(productId) == positions.end())
	{
		positions[productId] = Position<T>(trade.GetProduct());
		positions[productId].AddPosition(book, quantity);
	}
	else
	{
		positions[productId].AddPosition(book, quantity);
	}

	this->OnMessage(positions[productId]);
}


/**
* Position Service Listener subscribing data from Trading Booking Service
* Type T is the product type.
*/
template<typename T>
class TradeBookingListener : public ServiceListener<Trade<T>>
{

private:

	PositionService<T>* service;

public:

	// Connector and Destructor
	TradeBookingListener(PositionService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Trade<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Trade<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Trade<T>& _data);

};

template<typename T>
TradeBookingListener<T>::TradeBookingListener(PositionService<T>* _service)
{
	service = _service;
}

template<typename T>
void TradeBookingListener<T>::ProcessAdd(Trade<T>& _data)
{
	service->AddTrade(_data);
}

template<typename T>
void TradeBookingListener<T>::ProcessRemove(Trade<T>& _data) {}

template<typename T>
void TradeBookingListener<T>::ProcessUpdate(Trade<T>& _data) {}


#endif
