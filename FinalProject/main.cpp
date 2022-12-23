/*
*Testing the trading system
*@author: Chaofan Shen
*/

#include <iostream>
#include <string>
#include "soa.hpp"
#include "products.hpp"
#include "algoexecutionservice.hpp"
#include "algostreamingservice.hpp"
#include "bondexecutionservice.hpp"
#include "guiservice.hpp"
#include "historicaldataservice.hpp"
#include "inquiryservice.hpp"
#include "marketdataservice.hpp"
#include "positionservice.hpp"
#include "pricingservice.hpp"
#include "riskservice.hpp"
#include "bondstreamingservice.hpp"
#include "tradebookingservice.hpp"

using namespace std;

int main()
{
	cout << "Start testing trading system." << endl;

	// First, register all the service
	// using Bond productType
	cout << "Start creating Services." << endl;
	pricingService<Bond> pricingservice;
	TradeBookingService<Bond> tradeBookingService;
	PositionService<Bond> positionService;
	RiskService<Bond> riskService;
	marketDataService<Bond> marketdataservice;
	AlgoExecutionService<Bond> algoExecutionService;
	AlgoStreamingService<Bond> algoStreamingService;
	GUIService<Bond> guiService;
	ExecutionService<Bond> executionService;
	StreamingService<Bond> streamingService;
	InquiryService<Bond> inquiryService;
	cout << "Services have been created." << endl;

	// Second, create listeners from HistoricalDataService to record the infomation
	HistoricalDataService<Position<Bond>> historicalPositionService(POSITION);
	HistoricalDataService<PV01<Bond>> historicalRiskService(RISK);
	HistoricalDataService<ExecutionOrder<Bond>> historicalExecutionService(EXECUTION);
	HistoricalDataService<PriceStream<Bond>> historicalStreamingService(STREAMING);
	HistoricalDataService<Inquiry<Bond>> historicalInquiryService(INQUIRY);


	// Then, we add the listeners to the related service
	cout << "Start sending listeners." << endl;
	pricingservice.AddListener(algoStreamingService.GetListener());
	pricingservice.AddListener(guiService.GetListener());
	algoStreamingService.AddListener(streamingService.GetListener());
	streamingService.AddListener(historicalStreamingService.GetListener());
	marketdataservice.AddListener(algoExecutionService.GetListener());
	algoExecutionService.AddListener(executionService.GetListener());
	executionService.AddListener(tradeBookingService.GetListener());
	executionService.AddListener(historicalExecutionService.GetListener());
	tradeBookingService.AddListener(positionService.GetListener());
	positionService.AddListener(riskService.GetListener());
	positionService.AddListener(historicalPositionService.GetListener());
	riskService.AddListener(historicalRiskService.GetListener());
	inquiryService.AddListener(historicalInquiryService.GetListener());
	cout << "Listeners have been sent." << endl;


	// Finally, we use connectors from different services to 
	// load the data
	ifstream prices("prices.txt");
	cout << "Start subcribing prices." << endl;
	pricingservice.GetConnector()->Subscribe(prices);


	ifstream trades("trades.txt");
	cout << "Start subcribing trades." << endl;
	tradeBookingService.GetConnector()->Subscribe(trades);

	ifstream inquiries("inquiries.txt");
	cout << "Start subcribing inquires." << endl;
	inquiryService.GetConnector()->Subscribe(inquiries);	

	//ifstream marketdata("marketdata.txt");
	//cout << "Start subcribing market data." << endl;
	//marketdataservice.GetConnector()->Subscribe(marketdata);

	return 0;
}