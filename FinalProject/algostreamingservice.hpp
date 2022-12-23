/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */

#ifndef ALGO_STREAMING_SERVICE_HPP
#define ALGO_STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "pricingservice.hpp"


/**
 * A price stream order with price and quantity (visible and hidden)
 */
class PriceStreamOrder
{

public:

  // ctor for an order
  PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side);
  PriceStreamOrder() = default;
  ~PriceStreamOrder() {};

  // The side on this order
  PricingSide GetSide() const;

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity on this order
  long GetHiddenQuantity() const;

  // Print the stream
  string print();

private:
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  PricingSide side;

};


PriceStreamOrder::PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side)
{
	price = _price;
	visibleQuantity = _visibleQuantity;
	hiddenQuantity = _hiddenQuantity;
	side = _side;
}

double PriceStreamOrder::GetPrice() const
{
	return price;
}

long PriceStreamOrder::GetVisibleQuantity() const
{
	return visibleQuantity;
}

long PriceStreamOrder::GetHiddenQuantity() const
{
	return hiddenQuantity;
}

PricingSide PriceStreamOrder::GetSide() const
{
	return side;
}

string PriceStreamOrder::print()
{
	stringstream output;

	string orderSide;
	if (side == BID) orderSide = "bid";
	if (side == OFFER) orderSide = "offer";
	output << "Side: " << orderSide << ", ";

	output << "Price: " << GetQuotePrice(price) << ", ";
	output << "Visible quantity: " << to_string(visibleQuantity) << ", ";
	output << "Hidden quantity: " << to_string(hiddenQuantity);
	
	return output.str();
}


/**
 * Price Stream with a two-way market.
 * Type T is the product type.
 */
template<typename T>
class PriceStream
{

public:

	// ctor
	PriceStream(const T& _product, const PriceStreamOrder& _bidOrder, const PriceStreamOrder& _offerOrder);
	PriceStream() = default;
	~PriceStream() {};

	// Get the product
	const T& GetProduct() const;

	// Get the bid order
	const PriceStreamOrder& GetBidOrder() const;

	// Get the offer order
	const PriceStreamOrder& GetOfferOrder() const;

	// Print PriceStream
	string print();

private:
	T product;
	// PriceStream has two members representing the bid and offer.
	PriceStreamOrder bidOrder;
	PriceStreamOrder offerOrder;

};

template<typename T>
PriceStream<T>::PriceStream(const T& _product, const PriceStreamOrder& _bidOrder, const PriceStreamOrder& _offerOrder) :
	product(_product), bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

template<typename T>
const T& PriceStream<T>::GetProduct() const
{
	return product;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetBidOrder() const
{
	return bidOrder;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetOfferOrder() const
{
	return offerOrder;
}

template<typename T>
string PriceStream<T>::print()
{
	stringstream output;
	output << "CUSIP: " << product.GetProductId() << ", ";
	output << bidOrder.print() << ",";
	output << offerOrder.print() << ",";

	return output.str();
}


template<typename T>
class PricingListener;

/**
 * Algo Streaming service 
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class AlgoStreamingService : public Service<string,PriceStream <T> >
{

public:

  // Ctor
  AlgoStreamingService();

  // Dtor
  ~AlgoStreamingService();

  // Get data on our service given a key
  PriceStream<T>& GetData(string _key);

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(PriceStream<T>& _data);

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
  void AddListener(ServiceListener<PriceStream<T>>* _listener);

  // Get all listeners on the Service
  const vector<ServiceListener<PriceStream<T>>*>& GetListeners() const;

  // Get the listener of the service
  ServiceListener<Price<T>>* GetListener();

  // send the bid/offer prices to the BondStreamingService
  void PublishPrice(Price<T>& _price);

private:
	unordered_map<string, PriceStream<T>> algoStreams;
	vector<ServiceListener<PriceStream<T>>*> listeners;
	PricingListener<T>* listener;
	bool isFirst; // alternate visible sizes

};

template<typename T>
AlgoStreamingService<T>::AlgoStreamingService()
{
	algoStreams = unordered_map<string, PriceStream<T>>();
	listeners = vector<ServiceListener<PriceStream<T>>*>();
	listener = new PricingListener<T>(this);
	isFirst = false;
}

template<typename T>
AlgoStreamingService<T>::~AlgoStreamingService() {
	delete listener;
}

template<typename T>
PriceStream<T>& AlgoStreamingService<T>::GetData(string _key)
{
	return algoStreams[_key];
}

template<typename T>
void AlgoStreamingService<T>::OnMessage(PriceStream<T>& _data)
{
	algoStreams[_data.GetProduct().GetProductId()] = _data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void AlgoStreamingService<T>::AddListener(ServiceListener<PriceStream<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<PriceStream<T>>*>& AlgoStreamingService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
ServiceListener<Price<T>>* AlgoStreamingService<T>::GetListener()
{
	return listener;
}

template<typename T>
void AlgoStreamingService<T>::PublishPrice(Price<T>& price)
{
	T product = price.GetProduct();
	string productId = product.GetProductId();

	
	double bidPrice = price.GetMid() - price.GetBidOfferSpread() / 2.0;
	double offerPrice = price.GetMid() + price.GetBidOfferSpread() / 2.0;
	long visibleQuantity = (isFirst + 1) * 10000000;
	long hiddenQuantity = visibleQuantity * 2;
	isFirst = !isFirst;
	
	PriceStreamOrder bidStreamOrder(bidPrice, visibleQuantity, hiddenQuantity, BID);
	PriceStreamOrder offerStreamOrder(offerPrice, visibleQuantity, hiddenQuantity, OFFER);
	PriceStream<T> algoStream(product, bidStreamOrder, offerStreamOrder);

	// update event and pass to all the listeners
	this->OnMessage(algoStream);
}


/**
* PricingListener subscribing data from PricingService and update to AlgoStreamingService.
* Type T is the product type.
*/
template<typename T>
class PricingListener : public ServiceListener<Price<T>>
{

private:

	AlgoStreamingService<T>* service;

public:

	// Connector and Destructor
	PricingListener(AlgoStreamingService<T>* _service);
	~PricingListener() {};

	// Listener callback to process an add event to the Service
	void ProcessAdd(Price<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Price<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Price<T>& _data);

};

template<typename T>
PricingListener<T>::PricingListener(AlgoStreamingService<T>* _service)
{
	service = _service;
}

template<typename T>
void PricingListener<T>::ProcessAdd(Price<T>& _data)
{
	service->PublishPrice(_data);
}

template<typename T>
void PricingListener<T>::ProcessRemove(Price<T>& _data) {}

template<typename T>
void PricingListener<T>::ProcessUpdate(Price<T>& _data) {}

#endif