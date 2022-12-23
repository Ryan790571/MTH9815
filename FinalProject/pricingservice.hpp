/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include <algorithm>
#include "soa.hpp"

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:


  // ctor for a price
	Price() = default;
	Price(T _product, double _mid, double _bidOfferSpread);

  

  // Get the product
  const T& GetProduct() const;
  

  // Get the mid price
  double GetMid() const;

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const;

private:
  T product;
  double mid;
  double bidOfferSpread;

};


template<typename T>
Price<T>::Price(T _product, double _mid, double _bidOfferSpread) :
  product(_product)
{
  mid = _mid;
  bidOfferSpread = _bidOfferSpread;
}

template<typename T>
const T& Price<T>::GetProduct() const
{
  return product;
}

template<typename T>
double Price<T>::GetMid() const
{
  return mid;
}

template<typename T>
double Price<T>::GetBidOfferSpread() const
{
  return bidOfferSpread;
}


template<typename T>
class pricingConnector;

/**
 * Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class pricingService : public Service<string, Price <T> >
{
public:

	// ctor 
	pricingService();

	// Get data on our service given a key
	Price<T>& GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Price<T>& data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<Price<T>>* listener);

	// Get all listeners on the Service.
	const vector< ServiceListener<Price<T>>* >& GetListeners() const ;

	// Get the connector of the service
	pricingConnector<T>* GetConnector();

	// dtor
	~pricingService();

private:
	unordered_map<string, Price<T>> prices;
	vector<ServiceListener<Price<T>>*> listeners;
	pricingConnector<T>* connector;
};

template<typename T>
pricingService<T>::pricingService()
{
	prices = unordered_map<string, Price<T>>();
	listeners = vector<ServiceListener<Price<T>>*>();
	connector = new pricingConnector<T>(this);
}

template<typename T>
pricingService<T>::~pricingService() {
	delete connector;
}

template<typename T>
Price<T>& pricingService<T>::GetData(string _key)
{
	return prices[_key];
}

template<typename T>
void pricingService<T>::OnMessage(Price<T>& data)
{
	prices[data.GetProduct().GetProductId()] = data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(data); });
}

template<typename T>
void pricingService<T>::AddListener(ServiceListener<Price<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<Price<T>>*>& pricingService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
pricingConnector<T>* pricingService<T>::GetConnector()
{
	return connector;
}


/**
* Pricing Connector (subscribe only)
* Type T is the product type.
*/
template<typename T>
class pricingConnector : public Connector<Price<T>>
{

private:

	pricingService<T>* service;

public:

	// Ctor
	pricingConnector(pricingService<T>* _service);

	// Publish data to the Connector
	void Publish(Price<T>& _data);

	// Subscribe data from the Connector
	void Subscribe(ifstream& _data);

};

template<typename T>
pricingConnector<T>::pricingConnector(pricingService<T>* _service)
{
	service = _service;
}

template<typename T>
void pricingConnector<T>::Publish(Price<T>& _data) {}

template<typename T>
void pricingConnector<T>::Subscribe(ifstream& data)
{
	string line;
	while (getline(data, line))
	{
		stringstream priceInfo(line);
		vector<string> priceData;
		string p;

		while (getline(priceInfo, p, ','))
		{
			priceData.push_back(p);
		}

		string productId = priceData[0];
		double mid = GetNormalPrice(priceData[1]);
		double spread = GetNormalPrice(priceData[2]);
		T product = GetProductType(productId);
		Price<T> newPrice(product, mid, spread);
		
		service->OnMessage(newPrice);
	}
}
#endif
