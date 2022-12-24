/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */
#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include <algorithm>
#include "soa.hpp"

using namespace std;

// Side for market data
enum PricingSide { BID, OFFER };


/**
 * A market data order with price, quantity, and side.
 */
class Order
{

public:

  // ctor for an order
  Order(double _price, long _quantity, PricingSide _side);
  Order() = default;

  // Get the price on the order
  double GetPrice() const;

  // Get the quantity on the order
  long GetQuantity() const;

  // Get the side on the order
  PricingSide GetSide() const;

private:
  double price;
  long quantity;
  PricingSide side;

};

Order::Order(double _price, long _quantity, PricingSide _side)
{
	price = _price;
	quantity = _quantity;
	side = _side;
}

double Order::GetPrice() const
{
	return price;
}

long Order::GetQuantity() const
{
	return quantity;
}

PricingSide Order::GetSide() const
{
	return side;
}


/**
 * Class representing a bid and offer order
 */
class BidOffer
{

public:

  // ctor for bid/offer
  BidOffer(const Order &_bidOrder, const Order &_offerOrder);
  BidOffer() = default;

  // Get the bid order
  const Order& GetBidOrder() const;

  // Get the offer order
  const Order& GetOfferOrder() const;

private:
  Order bidOrder;
  Order offerOrder;

};

BidOffer::BidOffer(const Order& _bidOrder, const Order& _offerOrder) :
	bidOrder(_bidOrder), offerOrder(_offerOrder)
{}

const Order& BidOffer::GetBidOrder() const
{
	return bidOrder;
}

const Order& BidOffer::GetOfferOrder() const
{
	return offerOrder;
}


/**
 * Order Stack with bid and offer stacks.
 * Type T is the product type.
 */
template<typename T>
class OrderStacks
{

public:

  // ctor for the order book
  OrderStacks(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack);
  OrderStacks() = default;

  // Get the product
  const T& GetProduct() const;

  // Get the bid stack
  vector<Order>& GetBidStack() const;

  // Get the offer stack
  vector<Order>& GetOfferStack() const;

  // Get best bid and offer price
  BidOffer& GetBestBidOffer() const;

private:
  T product;
  vector<Order> bidStack;
  vector<Order> offerStack;

};

template<typename T>
OrderStacks<T>::OrderStacks(const T& _product, const vector<Order>& _bidStack, const vector<Order>& _offerStack) :
	product(_product), bidStack(_bidStack), offerStack(_offerStack)
{
}

template<typename T>
const T& OrderStacks<T>::GetProduct() const
{
	return product;
}

template<typename T>
vector<Order>& OrderStacks<T>::GetBidStack() const
{
	return bidStack;
}

template<typename T>
vector<Order>& OrderStacks<T>::GetOfferStack() const
{
	return offerStack;
}

template<typename T>
BidOffer& OrderStacks<T>::GetBestBidOffer() const
{
	
	Order bestBid(0, 0, BID);
	for (auto p : bidStack) {
		if (p.GetPrice() > bestBid.GetPrice()) bestBid = p;
	}
	Order bestOffer(1000, 0, BID);
	for (auto p : offerStack) {
		if (p.GetPrice() < bestOffer.GetPrice()) bestOffer = p;
	}
	BidOffer newBidOffer(bestBid, bestOffer);

	return newBidOffer;
}


template<typename T>
class marketDataConnector;

/**
 * Market Data Service which provides market data to interested subscribers
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class marketDataService : public Service<string,OrderStacks <T> >
{

public:

	// ctor
	marketDataService();

	// Get data on our service given a key
	OrderStacks<T>& GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(OrderStacks<T>& data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<OrderStacks<T>>* listener);

	// Get all listeners on the Service
	const vector<ServiceListener<OrderStacks<T>>*>& GetListeners() const;

	// Get the connector of the service
	marketDataConnector<T>* GetConnector();

	// Get the best bid/offer order
	BidOffer& GetBestBidOffer(const string &productId);

	// Aggregate the market data at all price points to create a new bid/offer stack
	OrderStacks<T>& AggregateMarketData(const string &productId);

	// dtor
	~marketDataService();

private:

	unordered_map<string, OrderStacks<T>> orderBooks;
	marketDataConnector<T>* connector;
	vector<ServiceListener<OrderStacks<T>>*> listeners;

};


template<typename T>
marketDataService<T>::marketDataService()
{
	orderBooks = unordered_map<string, OrderStacks<T>>();
	connector = new marketDataConnector<T>(this);
	listeners = vector<ServiceListener<OrderStacks<T>>*>();
}

template<typename T>
OrderStacks<T>& marketDataService<T>::GetData(string key)
{
	return orderBooks[key];
}

template<typename T>
void marketDataService<T>::OnMessage(OrderStacks<T>& data)
{
	// add or update a new event
	orderBooks[data.GetProduct().GetProductId()] = data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(data); });
}

template<typename T>
void marketDataService<T>::AddListener(ServiceListener<OrderStacks<T>>* listener)
{
	listeners.push_back(listener);
}

template<typename T>
const vector<ServiceListener<OrderStacks<T>>*>& marketDataService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
marketDataConnector<T>* marketDataService<T>::GetConnector()
{
	return connector;
}

template<typename T>
BidOffer& marketDataService<T>::GetBestBidOffer(const string& productId)
{
	return orderBooks[productId].GetBestBidOffer();
}

template<typename T>
OrderStacks<T>& marketDataService<T>::AggregateMarketData(const string& productId)
{
	T product = orderBooks[productId].GetProduct();
	vector<Order> bidStack = orderBooks[productId].GetBidStack();
	vector<Order> offerStack = orderBooks[productId].GetOfferStack();

	// aggregate the same price
	unordered_map<double, long> tmpBidTable, tmpOfferTable;
	auto aggBid = [&](auto o) {tmpBidTable[o->GetPrice()] += o->GetQuantity(); };
	auto aggOffer = [&](auto o) {tmpOfferTable[o->GetPrice()] += o->GetQuantity(); };
	for_each(bidStack.begin(), bidStack.end(), aggBid);
	for_each(offerStack.begin(), offerStack.end(), aggOffer);

	// convert to new stacks
	vector<Order> newbidStack, newofferStack;
	auto converBid = [&](auto p) {newbidStack.push_back(Order(p->first, p->second, BID)); };
	auto converOffer = [&](auto p) {newofferStack.push_back(Order(p->first, p->second, OFFER)); };
	for_each(tmpBidTable.begin(), tmpBidTable.end(), converBid);
	for_each(tmpOfferTable.begin(), tmpOfferTable.end(), converOffer);
	OrderStacks<T> newOrderStack(product, newbidStack, newofferStack);

	return newOrderStack;
}

template<typename T>
marketDataService<T>::~marketDataService()
{
	delete connector;
}


/**
* Market Data Connector (subscribe-only)
* Type T is the product type.
*/
template<typename T>
class marketDataConnector : public Connector<OrderStacks<T>>
{
public:
	// Ctor
	marketDataConnector(marketDataService<T>* _service);

	// Publish data to the Connector
	void Publish(OrderStacks<T>& data);

	// Subscribe data from the Connector
	void Subscribe(ifstream& data);


private:

	marketDataService<T>* service;
};


template<typename T>
marketDataConnector<T>::marketDataConnector(marketDataService<T>* _service)
{
	service = _service;
}

template<typename T>
void marketDataConnector<T>::Publish(OrderStacks<T>& data) {}

template<typename T>
void marketDataConnector<T>::Subscribe(ifstream& data)
{
	// subcribe the data from files
	string line;
	int num_line = 0; // count the lines already read
	vector<Order> bidStacks, offerStacks;

	// read orders from files
	while (getline(data, line)) {
		stringstream orderInfo(line);
		vector<string> orderData;
		string p;

		while (getline(orderInfo, p, ','))
		{
			orderData.push_back(p);
		}

		// convert string to desired type
		string CUSIP = orderData[0];
		double price = GetNormalPrice(orderData[1]);
		long quantity = stol(orderData[2]);

		if (orderData[3] == "BID")
		{
			bidStacks.push_back(Order(price, quantity, BID));
		}
		else if (orderData[3] == "OFFER")
		{
			offerStacks.push_back(Order(price, quantity, OFFER));
		}

		// decide whether it is an separate update
		num_line++;
		if (num_line == 10) {
			num_line = 0;
			T product = GetProductType(CUSIP);
			OrderStacks<T>  orderBook(product, bidStacks, offerStacks);
			bidStacks = vector<Order>();
			offerStacks = vector<Order>();
			service->OnMessage(orderBook);
		}
	}
}

#endif


