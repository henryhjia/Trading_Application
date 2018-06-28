#include <sstream>
#include <iostream>
#include "feed_handler.h"

// static member initialization
unsigned long FeedHandler::pre_trade_quantity = 0;
double FeedHandler::pre_trade_price = 0.0;

FeedHandler::FeedHandler()
{
    highest_buy = 0;
    lowest_sell = 0;
    log_flag = false;

    init_error_key();
}

FeedHandler::FeedHandler(bool flag)
{
    highest_buy = 0;
    lowest_sell = 0;
    log_flag = flag;

    init_error_key();
}

void FeedHandler::init_error_key()
{
    error_key[0] = "corrupted message";
    error_key[1] = "duplicated order id";
    error_key[2] = "trade with no corresponding order";
    error_key[3] = "remove with no corresponding order";
    error_key[4] = "no trade";
    error_key[5] = "invalid orderid,quantity,price";
    error_key[6] = "other";

    for(int i=0; i<key_size;i++) error_map[error_key[i]] = 0;
}

void FeedHandler::calculate_midquotes(string side, double price)
{
    if(lowest_sell == 0 && side == "S") {
        lowest_sell = price;
    }

    if(highest_buy == 0 && side == "B") {
        highest_buy = price;
    }

    if(price < lowest_sell && side == "S") {
        lowest_sell = price;
    }

    if(price > highest_buy && side == "B") {
        highest_buy = price;
    }

    if(orderid_map.size() == 1) {
        cout << "Midquotes: NAN" << endl;
    }
    else if(highest_buy !=0 && lowest_sell !=0) {
        cout << "Midquotes: " << (lowest_sell+highest_buy)/2.0 << endl;
    }
}

void FeedHandler::add_order(unsigned long order_id, string side, unsigned long quantity, double price)
{
    if(log_flag)
        cout << "add order: " << order_id << " " << price << " " << side << " " << quantity << endl;

    // add to orderid_map
    Order order(order_id, side, quantity, price);
    orderid_map[order_id] = order;

    // add to price_sell_map
    if(side == "S") {
        map<double, queue<Order> >::iterator it;
        queue<Order> myq;

        it = price_sell_map.find(price);
        if(it != price_sell_map.end()) {
            // found it, push order into the myq
            myq = it->second;
            myq.push(order);
            price_sell_map[price] = myq;
        }
        else {
            // it is the first order, just push it in
            myq.push(order);
            price_sell_map[price] = myq;
        }
    }
    else if (side == "B") {
        // add to price_buy_map
        map<double, queue<Order> >::iterator it;
        queue<Order> myq;

        it = price_buy_map.find(price);
        if(it != price_buy_map.end()) {
            // found it, push the order into the myq
            myq = it->second;
            myq.push(order);
            price_buy_map[price] = myq;
        }
        else {
            // it is the first order, just push it in
            myq.push(order);
            price_buy_map[price] = myq;
        }
    }
}

bool FeedHandler::remove_price_queue(queue<Order>& q, unsigned long order_id)
{
    bool ret=false;
    queue<Order> tmp;
    Order tmp_odr;

    while(!q.empty()) {
        tmp_odr = q.front();
        if(tmp_odr.get_order_id() == order_id) {
            ret = true;
        }
        else {
            tmp.push(tmp_odr);
        }
        q.pop();
    }

    while(!tmp.empty()) {
        q.push(tmp.front());
        tmp.pop();
    }

    return ret;
}

void FeedHandler::remove_order(unsigned long order_id, string side)
{
    if(log_flag)
        cout << "remove order:" << order_id << endl;

    // remove from orderid_map
    map<unsigned long, Order>::iterator it;
    it = orderid_map.find(order_id);
    
    if(it != orderid_map.end()) {
        orderid_map.erase(it);
    }

    // remove from price_sell_map
    if(side == "S") {
        map<double, queue<Order> >::iterator itr;
        itr = price_sell_map.begin();

        while(itr != price_sell_map.end()) {
            if(remove_price_queue(itr->second, order_id)) {
                // if q size is 0, then erase this item
                if((itr->second).size() == 0) {
                    price_sell_map.erase(itr);
                }
                return;
            }
            itr++;
        }
    }
    else if(side == "B") {
        // remove from price_buy_map
        map<double, queue<Order> >::iterator itr;
        itr = price_buy_map.begin();

        while(itr != price_buy_map.end()) {
            if(remove_price_queue(itr->second, order_id)) {
                // if q size is 0, then erase this item
                if((itr->second).size() == 0) {
                    price_buy_map.erase(itr);
                }
                return;
            }
            itr++;
        }
    }
}

void FeedHandler::modify_order(unsigned long order_id, string side, unsigned long quantity, double price)
{
    if(log_flag)
        cout << "modify order:" << order_id << " " << price << " " << side << " " << quantity << endl;

    // find the order based on the orider_id
    map<unsigned long, Order>::iterator it;
    it = orderid_map.find(order_id);

    // found it, remove the old one, add new one back on
    if(it != orderid_map.end()) {
        remove_order(order_id, side);
        add_order(order_id, side, quantity, price);
    }
}

// check if a string has all digit.  Empty string will return
// false
bool FeedHandler::are_digits(string str)
{
    int l = str.length();

    if(l==0) return false;

    for (int i = 0; i < l; i++)
    {
        if (str.at(i) < '0' || str.at(i) > '9')
            return false;
    }

    return true;
}

void FeedHandler::processMessage(const string& line)
{
    string word="";
    int index=0;
    string action="";
    unsigned long order_id=0;
    unsigned long quantity=0;
    string side="";
    double price=0.0;
    map<unsigned long, Order>::iterator it;

    try {
        // parsing parameters
        stringstream ss(line);
        while(getline(ss,word,',')) {
            switch(index) {
            case 0:
                action = word;
                if(action != "A" && action != "X" &&
                   action != "M" && action != "T") {
                        update_error_map(error_key[0]);
                }
                break;
            case 1:
                // check if the string contains all digit
                if(!are_digits(word)) {
                    update_error_map(error_key[0]);
                }

                if(action == "T") {
                    // for trade messages
                    quantity = stol(word);
                }
                else {
                    // for order messages (add, remove, modify)
                    order_id = stol(word);

                    // check duplicate for add order
                    if(action == "A") {
                        it = orderid_map.begin();
                        it = orderid_map.find(order_id);
                        if(it != orderid_map.end()) {
                            update_error_map(error_key[1]);
                            it++;
                        }
                    }

                    // check if record exists for remove
                    if(action == "X" || action == "M") {
                        it = orderid_map.begin();
                        it = orderid_map.find(order_id);
                        if(it == orderid_map.end()) {
                            update_error_map(error_key[3]);
                            it++;
                        }
                    }
                }
                break;
            case 2:
                if(action == "T")  {
                    // for trade messages
                    // check if the string contains all digit
                    if(!are_digits(word)) {
                        update_error_map(error_key[0]);
                    }
                    price = stod(word);
                }
                else {
                    // for order messages
                    if(word != "B" && word != "S") {
                        update_error_map(error_key[0]);
                    }
                    side = word;
                }
                break;
            case 3:
                // order messages: check if the string contains all digit
                if(!are_digits(word)) {
                    update_error_map(error_key[0]);
                }
                quantity = stol(word);
                break;
            case 4:
                // order messges: check if the string contains all digit
                if(!are_digits(word)) {
                    update_error_map(error_key[0]);
                }
                price = stod(word);
                break;
            default:
                break;
            }
            index++;
        }

        // calculate midquotes(average of best buy(highest) and best sell(lowest))
        calculate_midquotes(side,price);

        // process orders based on action
        if(action == "A") {
            if(price ==0) {
                update_error_map(error_key[5]);
            }
            add_order(order_id, side, quantity, price);
        }
        else if(action == "X") {
            if(order_id == 0) {
                update_error_map(error_key[3]);
            }
            if(quantity == 0 ) {
                update_error_map(error_key[5]);
            }
            if(price == 0.0 ) {
                update_error_map(error_key[5]);
            }
            remove_order(order_id, side);
        }
        else if(action == "M") {
            modify_order(order_id, side, quantity, price);
        }
        else if(action == "T") {
            if(price == 0) {
                update_error_map(error_key[5]);
            }
            if(quantity == 0) {
                update_error_map(error_key[5]);
            }

            unsigned long quan = 0;

            if(pre_trade_price == price) {
                quan = pre_trade_quantity + quantity;
            }
            else {
                quan = quantity;
                pre_trade_quantity = quantity;
                pre_trade_price = price;
            }
            
            // Total quantity traded at the most recent trade price
            cout << "Total quantity traded: "
                 << quan << "@" << price << endl;
        }
    }
    catch(std::invalid_argument& e) {
        cout << e.what() << endl;
    }
    catch (...) {
        cout << "unspecified error catched" << endl;
    }
}

void FeedHandler::print_queue(ostream& os, queue<Order>& q)
{
    Order order;
    while(!q.empty()) {
        order = q.front();
        os << order.get_side() << " " << order.get_quantity() << " ";
        q.pop();
    }
}

void FeedHandler::printCurrentOrderBook(ostream& os)
{
    if(log_flag) {
        map<unsigned long, Order>::iterator itr;
        itr = orderid_map.begin();
        os << "orderid_map:" << endl;
        while(itr != orderid_map.end()) {
            os << "Book order: " << itr->first << " " << itr->second;
            itr++;
        }
    }

    map<double, queue<Order> >::reverse_iterator it;
    queue<Order> tmp_q;

    os << "Book order: price orders" << endl;

    // print sell order book
    it = price_sell_map.rbegin();
    while(it != price_sell_map.rend()) {
        // make a copy of queue it->second for print
        tmp_q = (it->second);
        os << "Book order: " << it->first << " ";
        print_queue(cout, tmp_q);
        os << endl;
        it++;
    }

    //print buy order book
    it = price_buy_map.rbegin();
    while(it != price_buy_map.rend()) {
        // make a copy of queue it->second for print
        tmp_q = (it->second);
        os << "Book order: " << it->first << " ";
        print_queue(cout, tmp_q);
        os << endl;
        it++;
    }
}

void FeedHandler::printErrorSummary(ostream& os)
{

    os << endl;
    os << "Error summary:" << endl;

    string key="";
    unsigned int ct=0;
    unsigned int total_ct = 0;

    map<string, unsigned>::iterator it;
    it = error_map.begin();
    while(it != error_map.end()) {
        if(it->second !=0) ct++;
        it++;
    }

    if(ct ==0) { cout << "No error found" << endl; }
    else {
        ct = 0;
        for(int i=0; i<key_size; i++) {
            key = error_key[i];
            ct = error_map[key];
            total_ct += ct;
            if(ct != 0)
                cout << key << ": " << error_map[key] << endl;
        }
        cout << "Total error count: " << total_ct << endl;
    }
}

void FeedHandler::update_error_map(string key) {
    unsigned int ct = error_map[key];
    error_map[key] = ++ct;
    throw std::invalid_argument("Exception:" + key);
}

       

                   
