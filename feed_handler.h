#include <string>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <queue>
#include <vector>
#include <utility>
#include "order.h"
#include "trade.h"

using namespace std;

class FeedHandler
{

    private:
        // map with order_id as key
        map<unsigned long, Order> orderid_map;

        // map with price as key for buy order
        map<double, queue<Order> > price_buy_map;

        // map with price as key for sell order
        map<double, queue<Order> > price_sell_map;

        // map for trade
        map<double, unsigned long> trade_map;

        // midquotes
        unsigned long highest_buy;
        unsigned long lowest_sell;
        
        // trade parameters
        static unsigned long pre_trade_quantity;
        static double pre_trade_price;

        // error string key
        static const unsigned int key_size = 7;

        // error_key[0] = "corrupted message";
        // error_key[1] = "duplicated order id";
        // error_key[2] = "trade with no corresponding order";
        // error_key[3] = "remove with no corresponding order";
        // error_key[4] = "no trade";
        // error_key[5] = "invalid orderid,quantity,price";
        // error_key[6] = "other";
        string error_key[key_size];

        // error message counter
        map<string, unsigned int> error_map;

        // log flag
        bool log_flag;

        // init error key
        void init_error_key();

        // operation on orders
        void add_order(unsigned long order_id, string side, unsigned long quantity, double price);
        void remove_order(unsigned long order_id, string side);
        void modify_order(unsigned long order_id, string side, unsigned long quantity, double price);
        
        // helper functions
        // remove an order object from the queue associated with key price
        bool remove_price_queue(queue<Order>& q, unsigned long orderid);

        // print all element in a queue (copy the queue before using this function)
        void print_queue(ostream& os, queue<Order>& q);

        // calculate midquotes
        void calculate_midquotes(string side, double price);

        // update error map
        void update_error_map(string key);

        // are string digits
        bool are_digits(string);

    public:
        FeedHandler();
        FeedHandler(bool flag);
        void processMessage(const string& line);
        void printCurrentOrderBook(ostream& os);
        void printErrorSummary(ostream& os);

};        
