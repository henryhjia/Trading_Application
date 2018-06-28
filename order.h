#include <iostream>
#include <string>
#include <queue>

using namespace std;

class Order
{
    private:
        unsigned long order_id;
        string side;
        unsigned long quantity;
        double price;

    public:
        Order();
        Order(unsigned long, string, unsigned long, double);

        // operator overloading
        friend ostream& operator<< (ostream& os, Order& );

        // accessors
        unsigned long get_order_id() const { return order_id; }
        string get_side() const { return side;}
        double get_price() const { return price;}
        unsigned long get_quantity() { return quantity;}

};
