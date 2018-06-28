#include "order.h"
  
Order::Order()
{

}

Order::Order(unsigned long o_id, string s, unsigned long quan, double pr)
{
    order_id = o_id;
    side = s;
    quantity = quan;
    price = pr;
}

ostream& operator<< (ostream& os, Order& order)
{
    os << order.price<< " ";
    //order.print_queue(os, order.quantity);
    os << order.side << " ";
    os << order.quantity << " ";
    os << endl;

    return os;
}
