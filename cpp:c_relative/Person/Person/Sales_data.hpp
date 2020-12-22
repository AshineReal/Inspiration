//
//  Sales_data.hpp
//  Person
//
//  Created by Ashine on 2020/12/4.
//

#ifndef Sales_data_hpp
#define Sales_data_hpp

#include <string>
#include <iostream>

/*
 1.首先明确私有成员变量bookNo、units_sold、revenue
 2.然后是构造函数的声明(这里使用了一个三参数构造函数,然后其它构造函数是委托构造函数)
 3.然后是类的一些功能声明(public有isbn(),combine() / private有avg_price())
 4.接下来是接口函数声明(非成员函数 / 2、3点是成员函数) read()、print()、add()
 5.然后因为接口函数需要使用到类的私有成员 需要在类中将其进行友元声明
 */

class Sales_data
{
    // 一般在类开始或者结束定义前集中声明友元
    friend std::istream &read(std::istream &is,Sales_data &item);
    friend std::ostream &print(std::ostream &os,const Sales_data &item);
    friend Sales_data add(const Sales_data &lhs,const Sales_data &rhs);
    
public:
    // 使用三参数初始化列表的构造函数
    Sales_data(const std::string &bn,unsigned n,double p):bookNo(bn),units_sold(n),revenue(n*p)
    {
        std::cout << "Sales_data(const std::string&, unsigned, double)" << std::endl;
    };
    
    // 使用委托构造函数 (委托的上面那个三参数的构造函数)
    Sales_data() : Sales_data(" ",0,0.0f)
    {
        std::cout << "Sales_data()" << std::endl;
    };
    
    // 同样使用的最上面那个三参数构造函数(不过这个构造函数提供了一个string对象作为参数)
    Sales_data(const std::string &s) : Sales_data(s,0,0.0f)
    {
        std::cout << "Sales_data(const std::string&)" << std::endl;
    };
    
    
    // 使用istream作为参数的构造函数
    Sales_data(std::istream &is);
    
    
    std::string const& isbn() const { return bookNo; };
    Sales_data& combine(const Sales_data&);
    
private:
    // 内联函数(这里是显式内联,在类外进行定义,也可以隐式内联直接在类内完成声明定义)
    inline double avg_price() const;// 获取平均价格

private:
    std::string bookNo;               // 书号
    unsigned units_sold = 0;          // 售出量
    double revenue = 0.0;             // 收入
};

// 这里进行成员函数的定义 (因为是内联的函数,需要在头文件中进行定义,编译器在使用到该函数的地方才有依据进行替换)
inline double Sales_data::avg_price() const
{
    return units_sold ? revenue / units_sold : 0;
}

// 接口函数声明部分

// 使用istream和item引用作为参数 对这个item的成员进行赋值 (因为需要访问到私有成员,所以需要在类内部进行友元声明)
std::istream &read(std::istream &is,Sales_data &item);

// 使用ostream和item引用作为参数 对这个item的成员进行打印 (同上)
std::ostream &print(std::ostream &os,const Sales_data &item);

// 使用两个Sales_data类型的参数 对这两个对象的成员进行相加 赋值给新的Sales_data类型成员 然后将这个新的Sales_data返回
Sales_data add(const Sales_data &lhs,const Sales_data &rhs);

/*
 补充友元函数虽然在类中进行了声明, 但是对于接口函数还是要在类外进行声明(友元仅仅只是影响访问控制的权限,它本身并非普通意义上的声明),记住这是最好的实践
 */


#endif /* Sales_data_hpp */
