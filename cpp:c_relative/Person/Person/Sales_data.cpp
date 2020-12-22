//
//  Sales_data.cpp
//  Person
//
//  Created by Ashine on 2020/12/4.
//

#include "Sales_data.hpp"

#include <stdio.h>
#include <string>
#include <iostream>

Sales_data::Sales_data(std::istream &is) : Sales_data() // 这里的委托默认初始化已经初始化了this
{
    std::cout << "Sales_data(istream &is)" << std::endl;
    read(is, *this); // 这里就可以使用this了
}


Sales_data& Sales_data::combine(const Sales_data& rhs)
{
    units_sold += rhs.units_sold;
    revenue += rhs.revenue;
    return *this;
}

std::istream &read(std::istream &is, Sales_data &item)
{
    double price = 0;
    is >> item.bookNo >> item.units_sold >> price;
    item.revenue = price * item.units_sold;
    return is;
}

std::ostream &print(std::ostream &os, const Sales_data &item)
{
    os << item.isbn() << " " << item.units_sold << " " << item.revenue;
    return os;
}

Sales_data add(const Sales_data &lhs, const Sales_data &rhs)
{
    Sales_data sum = lhs;
    sum.combine(rhs);
    return sum;
}
