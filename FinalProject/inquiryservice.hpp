/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 *
 * @author Breman Thuraisingham
 * @author Chaofan Shen
 */
#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include <algorithm>
#include "soa.hpp"
#include "tradebookingservice.hpp"

// Various inqyury states
enum InquiryState { RECEIVED, QUOTED, DONE, REJECTED, CUSTOMER_REJECTED };

/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template<typename T>
class Inquiry
{

public:
  // ctor for an inquiry
	Inquiry() = default;
	Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state);
  

  // Get the inquiry ID
  const string& GetInquiryId() const;

  // Get the product
  const T& GetProduct() const;

  // Get the side on the inquiry
  Side GetSide() const;

  // Get the quantity that the client is inquiring for
  long GetQuantity() const;

  // Get the price that we have responded back with
  double GetPrice() const;

  // Get the current state on the inquiry
  InquiryState GetState() const;

  // Set the price that we have responded back with
  double SetPrice(double _price);

  // Set the current state on the inquiry
  void SetState(InquiryState _state);

  // Print inquiry
  string print();

private:
  string inquiryId;
  T product;
  Side side;
  long quantity;
  double price;
  InquiryState state;

};


template<typename T>
Inquiry<T>::Inquiry(string _inquiryId, const T& _product, Side _side, long _quantity, double _price, InquiryState _state) :
	product(_product)
{
	inquiryId = _inquiryId;
	side = _side;
	quantity = _quantity;
	price = _price;
	state = _state;
}

template<typename T>
const string& Inquiry<T>::GetInquiryId() const
{
	return inquiryId;
}

template<typename T>
const T& Inquiry<T>::GetProduct() const
{
	return product;
}

template<typename T>
Side Inquiry<T>::GetSide() const
{
	return side;
}

template<typename T>
long Inquiry<T>::GetQuantity() const
{
	return quantity;
}

template<typename T>
double Inquiry<T>::GetPrice() const
{
	return price;
}

template<typename T>
InquiryState Inquiry<T>::GetState() const
{
	return state;
}

template<typename T>
double Inquiry<T>::SetPrice(double _price)
{
	price = _price;
}

template<typename T>
void Inquiry<T>::SetState(InquiryState _state)
{
	state = _state;
}

template<typename T>
string Inquiry<T>::print()
{
	stringstream output;
	output << "Inquiry ID: " << inquiryId << ", ";

	string orderSide;
	if (side == BID) orderSide = "bid";
	if (side == OFFER) orderSide = "offer";
	output << "Side: " << orderSide << ", ";
	output << "Price: " << GetQuotePrice(price) << ", ";
	output << "Quantity: " << to_string(quantity) << ", ";

	string tmps;
	if (state == RECEIVED) tmps = "RECEIVED";
	if (state == QUOTED) tmps = "QUOTED";
	if (state == DONE) tmps = "DONE";
	if (state == REJECTED) tmps = "REJECTED";
	if (state == CUSTOMER_REJECTED) tmps = "CUSTOMER_REJECTED";
	output << "State: " << tmps;

	return output.str();
}

template<typename T>
class InquiryConnector;

/**
 * Service for customer inquirry objects.
 * Keyed on inquiry identifier 
 * Type T is the product type.
 */
template<typename T>
class InquiryService : public Service<string,Inquiry <T> >
{

public:

	// Ctor
	InquiryService();

	// Dtor
	~InquiryService();

	// Get data on our service given a key
	Inquiry<T>& GetData(string _key);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Inquiry<T>& _data);

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<Inquiry<T>>* _listener);

	// Get all listeners on the Service
	const vector<ServiceListener<Inquiry<T>>*>& GetListeners() const;

	// Get the connector of the service
	InquiryConnector<T>* GetConnector();

	// Send a quote back to the client
	void SendQuote(const string& _inquiryId, double _price);

	// Reject an inquiry from the client
	void RejectInquiry(const string& _inquiryId);

private:

	unordered_map<string, Inquiry<T>> inquiries;
	vector<ServiceListener<Inquiry<T>>*> listeners;
	InquiryConnector<T>* connector;
};

template<typename T>
InquiryService<T>::InquiryService()
{
	inquiries = unordered_map<string, Inquiry<T>>();
	listeners = vector<ServiceListener<Inquiry<T>>*>();
	connector = new InquiryConnector<T>(this);
}

template<typename T>
InquiryService<T>::~InquiryService() {
	delete connector;
}

template<typename T>
Inquiry<T>& InquiryService<T>::GetData(string _key)
{
	return inquiries[_key];
}

template<typename T>
void InquiryService<T>::OnMessage(Inquiry<T>& _data)
{
	InquiryState state = _data.GetState();
	if (state == RECEIVED) {
		// send back the quote
		inquiries[_data.GetInquiryId()] = _data;
		connector->Publish(_data);

		// invoke all the listeners
		for_each(listeners.begin(), listeners.end(), [&](auto& l) {l->ProcessAdd(_data); });
	}
	if (state == QUOTED) {
		// When quoted, set state as DONE
		_data.SetState(DONE);
		inquiries[_data.GetInquiryId()] = _data;
	}
}

template<typename T>
void InquiryService<T>::AddListener(ServiceListener<Inquiry<T>>* _listener)
{
	listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<Inquiry<T>>*>& InquiryService<T>::GetListeners() const
{
	return listeners;
}

template<typename T>
InquiryConnector<T>* InquiryService<T>::GetConnector()
{
	return connector;
}

template<typename T>
void InquiryService<T>::SendQuote(const string& _inquiryId, double _price)
{
	Inquiry<T>& inquiry = inquiries[_inquiryId];
	InquiryState state = inquiry.GetState();
	if (state == RECEIVED) {
		inquiry.SetPrice(_price);
		this->OnMessage(inquiry);
	}
}

template<typename T>
void InquiryService<T>::RejectInquiry(const string& _inquiryId)
{
	Inquiry<T>& inquiry = inquiries[_inquiryId];
	inquiry.SetState(REJECTED);
}


/**
* Inquiry Connector (Subscribe and publish)
* Type T is the product type.
*/
template<typename T>
class InquiryConnector : public Connector<Inquiry<T>>
{

private:

	InquiryService<T>* service;

public:

	// Connector and Destructor
	InquiryConnector(InquiryService<T>* _service);

	// Publish data to the Connector
	void Publish(Inquiry<T>& _data);

	// Subscribe data from the Connector
	void Subscribe(ifstream& _data);
};

template<typename T>
InquiryConnector<T>::InquiryConnector(InquiryService<T>* _service)
{
	service = _service;
}


template<typename T>
void InquiryConnector<T>::Publish(Inquiry<T>& _data)
{
	_data.SetState(QUOTED); // set the new state
	service->OnMessage(_data); // send back to service
}

template<typename T>
void InquiryConnector<T>::Subscribe(ifstream& _data)
{
	string line;
	while (getline(_data, line))
	{
		stringstream inquiryInfo(line);
		string p;
		vector<string> inquiryData;
		while (getline(inquiryInfo, p, ','))
		{
			inquiryData.push_back(p);
		}

		string inquiryId = inquiryData[0];
		string productId = inquiryData[1];
		long quantity = stol(inquiryData[3]);
		double price = GetNormalPrice(inquiryData[4]);

		InquiryState state;
		if (inquiryData[5] == "RECEIVED") state = RECEIVED;
		else if (inquiryData[5] == "QUOTED") state = QUOTED;
		else if (inquiryData[5] == "DONE") state = DONE;
		else if (inquiryData[5] == "REJECTED") state = REJECTED;
		else if (inquiryData[5] == "CUSTOMER_REJECTED") state = CUSTOMER_REJECTED;

		T product = GetProductType(productId);

		Side side;
		if (inquiryData[2] == "BUY") side = BUY;
		if (inquiryData[2] == "SELL") side = SELL;
		Inquiry<T> inquiry(inquiryId, product, side, quantity, price, state);

		service->OnMessage(inquiry);
	}
}

#endif
