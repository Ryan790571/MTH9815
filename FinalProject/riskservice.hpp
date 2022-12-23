/**
 * riskservice.hpp
 * Defines the data types and Service for fixed income risk.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "soa.hpp"
#include "positionservice.hpp"


/**
 * PV01 risk.
 * Type T is the product type.
 */
template<typename T>
class PV01
{

public:

  // ctor for a PV01 value
  PV01(const T &_product, double _pv01, long _quantity);
  PV01() = default;

  // Get the product on this PV01 value
  const T& GetProduct() const;

  // Get the PV01 value
  double GetPV01() const;

  // Get the quantity that this risk value is associated with
  long GetQuantity() const;

  // Set the quantity that this risk value is associated with
  void SetQuantity(long _quantity);

  // Print risk
  string print();

private:
  T product;
  double pv01;
  long quantity;

};


template<typename T>
PV01<T>::PV01(const T& _product, double _pv01, long _quantity) :
	product(_product)
{
	pv01 = _pv01;
	quantity = _quantity;
}

template<typename T>
const T& PV01<T>::GetProduct() const
{
	return product;
}

template<typename T>
double PV01<T>::GetPV01() const
{
	return pv01;
}

template<typename T>
long PV01<T>::GetQuantity() const
{
	return quantity;
}

template<typename T>
void PV01<T>::SetQuantity(long _quantity)
{
	quantity = _quantity;
}

template<typename T>
string PV01<T>::print()
{
	stringstream output;
	output << "CUSIP: " << product.GetProductId() << ", ";
	output << "PV01: " << to_string(pv01) << ", ";
	output << "Quantity: " << to_string(quantity);

	return output.str();
}

/**
 * A bucket sector to bucket a group of securities.
 * We can then aggregate bucketed risk to this bucket.
 * Type T is the product type.
 */
template<typename T>
class BucketedSector
{

public:

  // ctor for a bucket sector
  BucketedSector(const vector<T> &_products, string _name);

  // Get the products associated with this bucket
  const vector<T>& GetProducts() const;

  // Get the name of the bucket
  const string& GetName() const;

private:
  vector<T> products;
  string name;

};

template<typename T>
BucketedSector<T>::BucketedSector(const vector<T>& _products, string _name) :
	products(_products)
{
	name = _name;
}

template<typename T>
const vector<T>& BucketedSector<T>::GetProducts() const
{
	return products;
}

template<typename T>
const string& BucketedSector<T>::GetName() const
{
	return name;
}


// Listener obtaining updates from Position service
template<typename T>
class PositionListener;

/**
 * Risk Service to vend out risk for a particular security and across a risk bucketed sector.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class RiskService : public Service<string,PV01 <T> >
{

public:

	// Ctor
	RiskService();

	// Dtor
	~RiskService();

	// Get data on our service given a key
	PV01<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(PV01<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<PV01<T>>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<PV01<T>>*>& GetListeners() const;

	// Get the listener of the service
	PositionListener<T>* GetListener();

	// Add a position that the service will risk
	void AddPosition(Position<T>& _position);

	// Get the bucketed risk for the bucket sector
	const PV01<BucketedSector<T>>& GetBucketedRisk(const BucketedSector<T>& _sector) const;

private:

	unordered_map<string, PV01<T>> pvs;
	vector<ServiceListener<PV01<T>>*> listeners;
	PositionListener<T>* listener;
};

template<typename T>
RiskService<T>::RiskService()
{
	pvs = unordered_map<string, PV01<T>>();
	listeners = vector<ServiceListener<PV01<T>>*>();
	listener = new PositionListener<T>(this);
}

template<typename T>
RiskService<T>::~RiskService() {
	delete listener;
}

template<typename T>
PV01<T>& RiskService<T>::GetData(string _key)
{
	return pvs[_key];
}

template<typename T>
void RiskService<T>::OnMessage(PV01<T>& _data)
{
	pvs[_data.GetProduct().GetProductId()] = _data;

	// invoke all the listeners
	for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
}

template<typename T>
void RiskService<T>::AddListener(ServiceListener<PV01<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<PV01<T>>*>& RiskService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
PositionListener<T>* RiskService<T>::GetListener()
{
	return listener;
}

template<typename T>
void RiskService<T>::AddPosition(Position<T>& position)
{
	T product = position.GetProduct();
	string productId = product.GetProductId();
	double pv01value = GetPV01(productId);
	long quantity = position.GetAggregatePosition();
	PV01<T> pv01(product, pv01value, quantity);
	this->OnMessage(pv01);
}

template<typename T>
const PV01<BucketedSector<T>>& RiskService<T>::GetBucketedRisk(const BucketedSector<T>& _sector) const
{
	BucketedSector<T> sector = _sector;
	double pv01 = 0;
	long quantity = 1;

	vector<T>& products = _sector.GetProducts();
	for (auto& p : products)
	{
		string pId = p.GetProductId();
		pv01 += pvs[pId].GetPV01() * pvs[pId].GetQuantity();
	}

	return PV01<BucketedSector<T>>(products, pv01, quantity);
}


/**
* Risk Service Listener subscribing data from Position Service
* Type T is the product type.
*/
template<typename T>
class PositionListener : public ServiceListener<Position<T>>
{

private:

	RiskService<T>* service;

public:

	// Ctor
	PositionListener(RiskService<T>* _service);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Position<T>& _data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Position<T>& _data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Position<T>& _data);

};

template<typename T>
PositionListener<T>::PositionListener(RiskService<T>* _service)
{
	service = _service;
}


template<typename T>
void PositionListener<T>::ProcessAdd(Position<T>& _data)
{
	service->AddPosition(_data);
}

template<typename T>
void PositionListener<T>::ProcessRemove(Position<T>& _data) {}

template<typename T>
void PositionListener<T>::ProcessUpdate(Position<T>& _data) {}

#endif
