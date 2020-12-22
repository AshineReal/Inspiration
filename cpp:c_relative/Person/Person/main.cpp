//
//  main.cpp
//  Person
//
//  Created by Ashine on 2020/12/4.
//

#include <iostream>
#include <string>
#include <vector>//可变大小数组
#include <list>  //双向链表
#include <deque> //双端队列
#include <forward_list> // 单向链表

// 泛型算法
#include <numeric>
#include <algorithm>

#include <functional> // bind函数适配器

#include "Sales_data.hpp"

using std::cout;
using std::endl;
using std::cin;
using std::vector;
using std::list;
using std::string;
using std::deque;
using std::forward_list;

// 使用using指明命名空间,所有在该命名空间中的名字都可以直接使用而不需要再指明一次 例如std::bind,std::find_if
using namespace std;
using namespace std::placeholders; // placeholders也是一个命名空间

// 声明部分:
struct Person;
std::istream &read(std::istream &is,Person &p); // 因为person类内声明并定义的使用istream作为参数的构造函数体中使用了read外部函数,需要提前进行声明才能被编译器识别 ,这里既是声明read外部函数, 然后又因为read函数的参数中有person类类型,需要在read函数调用之前进行声明,所以上面声明了struct person.

// Person类定义部分:
struct Person {
// struct 默认为public
    // 友元函数部分:
    friend std::istream &read(std::istream &,Person &);
    friend std::ostream &print(std::ostream &,const Person &);
    
    // 类内部声明并定义构造函数
    explicit Person(std::istream &is){read(is, *this);}; // 1.使用istream作为参数的构造函数 // 使用explicit修饰符以后 只能显式调用构造函数 而不能通过隐式转化
    Person(const std::string n,const std::string a):name(n),address(a){}; // 2.使用n,a作为参数的构造函数(这里使用了成员的初始化构造)
    Person() = default; // 3.提供了其它初始化函数以后 编译器就不会自动生成默认合成初始化函数了,如需默认初始化函数 则需要显式提供出来
    
private:
    // 成员变量
    std::string name;
    std::string address;
    
public:
    // 成员函数 (函数名后的const修饰函数 表示this是一个指向常量的指针, 该函数不会修改this指针所指对象,另外常量对象,及常量对象的引用或指针都只能调用常量成员函数)
    std::string get_name() const{ return name;};
    std::string get_address() const{return address;};
};

// 接口函数
std::istream &read(std::istream &is,Person &p){ // read的第二个参数是 person的引用
    is >> p.name >> p.address;
    return is;
}

std::ostream &print(std::ostream &os,const Person &p){
    os << p.name << " " << p.address;
    return os;
}

// 在类的外部定义构造函数 (注意在类外需使用Person::作用域运算符指明了这是哪个类的成员 ) ,另外除了函数返回值,从作用域运算符开始处(包括参数列表,都进入了该类的作用域,所以可以正常访问该类的成员)
//Person::Person(std::istream &is){
//    read(is, *this); // 使用 this是指向该对象的隐式指针 通过*this来访问这个对象
//}


///////////////////////////////////////////////////////////////////    Screen   //////////////////////////////////////////////////////////////////////////
class Screen;
class Window_mgr{
public:
    using ScreenIndex = std::vector<Screen>::size_type; //使用类型别名 将一个复杂的类型 通过一简单别名的方式来命名 免去了之后每次都需要敲出完整的类型
    inline void clear(ScreenIndex); // 将指定的Screen置为空白 (显式内联函数声明,其实在类内部<定义>的函数默认也是内联函数不过是隐式的而已,当然也可以在类外部函数的定义处进行内联声明也是可以的)
private:
    std::vector<Screen> screens; // 保存screen集合
};

struct Screen{
    typedef std::string::size_type pos; //类型成员先定义再使用 所以一般放在类开始的地方
    
    friend void Window_mgr::clear(ScreenIndex); //指定Window_mgr的成员函数clear为Screen的友元函数(所以clear就可以正常访问到Screen的私有成员)
    
    Screen(pos ht,pos wd,char c):height(ht),width(wd),contents(ht * wd,c){};
    Screen(pos ht,pos wd):height(ht),width(wd),contents(ht * wd,' '){};
    Screen() = default;
    char get() const {return contents[cursor];}; // 类内隐式的内联
    inline char get(pos r,pos c) const; // 显式的内联
    Screen &move(pos r,pos c); // 之后被设为内联
    
    Screen &set(char); //设置光标所在位置的字符  (注意这里返回的是screen引用,是调用者本身,而不是调用者的一个副本对象,下面的示例中展示了可以使用此方式进行的链式调用)
    Screen &set(pos,pos,char); //设置任一给定位置的字符
    
    // 根据调用对象是否是const 重载了display函数
    Screen &display(std::ostream &os){do_display(os);return *this;}
    const Screen &display(std::ostream &os) const{do_display(os);return *this;}
    
private:
    pos cursor = 0; //光标默认类内初始化为0位置
    pos height = 0, width = 0;
    std::string contents;
    
    void do_display(std::ostream &os) const {os << contents;};
};

inline Screen &Screen::move(pos r, pos c){ // 类外部定义处定义为内联
    pos row = r * width;
    cursor = row + c;
    return *this;
}

char Screen::get(pos r, pos c) const{
    pos row = r * width;
    return contents[row + c];
}

Screen &Screen::set(char c){
    contents[cursor] = c;
    return *this;
}

Screen &Screen::set(pos r, pos col, char ch){
    contents[r * width + col] = ch;
    return *this;
}


//这里定义Window_mgr的成员函数clear(注意这里必须在Screen定义了clear为友元函数以后才能访问Screen的私有成员变量width、height、contents)
void Window_mgr::clear(ScreenIndex index){
    Screen &s = screens[index];
    s.contents = std::string(s.width * s.height,' ');
}

//关于友元的总结:需理解友元声明的作用仅仅是影响访问权限,它本身并非普通意义上的声明!(也就是说声明了友元函数以后还是需要单独去声明这个函数的!!虽然有些编译器不强制执行友元函数的单独声明)(所以看到友元的时候最好仅仅把它当成仅影响了访问权,其它并无声明意义)

bool find(vector<int>::const_iterator begin, vector<int>::const_iterator end, int i)
{
    while (begin != end)
    {
        if (*begin == i){
            return true;
        }
        ++begin;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////  Sort function ///////////////////////////////////////////////////////////////////

// insert sort

// bubble sort
void bubbleSort(vector<int> &ivec){

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void forward_insert(forward_list<string> &sfl,string find,string ins);

void replace_str_by_iterator(string &s,const string &oldVal,const string &newVal);
void replace_str_by_index(string &s,const string &oldVal,const string &newVal);
//void replace_str_by_iterator_replace(string &s,const string &oldVal,const string &newVal);

bool check_size(const string &s, string::size_type sz);

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    
    //////////////////////////////////////////////////////////////// 使用person示例 ////////////////////////////////////////////////////////////////
    
//    // 1.使用istream
//    Person p1 = Person(std::cin);
//    print(std::cout, p1);
//
//    // 2.使用n,a参数初始化person对象
//    Person p2 = Person("ashine", "china");
//    print(std::cout, p2);
//
//    // 3.使用默认构造函数初始化person对象
//    Person p3;
//    read(std::cin, p3);
//    print(std::cout, p3);
    
    
    //////////////////////////////////////////////////////////////// 使用screen示例 ////////////////////////////////////////////////////////////////
//    Screen myscreen = Screen(5, 5 ,'c');
//    myscreen.move(4, 4).set('#'); // 注意这里因为move 返回的是调用对象本身的引用(引用是左值) 故这里可以链式调用 将操作连到一起 等同于下面的语句展示的分布操作
    
//    myscreen.move(4, 0);
//    myscreen.set('#');
    
    // 根据调用对象是否是const,决定了应该调用哪个display的版本.
//    myscreen.display(std::cout); // 这里是调用非常量的版本
//    std::cout << std::endl;
//
//    const Screen blank(5,3,'a');
//    blank.display(std::cout); // 这里是调用常量版本
    
//    测试
//    Screen myScreen(5,5,'X');
//    myScreen.move(4, 0).set('#').display(std::cout);
//    std::cout << '\n';
//
//    myScreen.display(std::cout);
//    std::cout << '\n';

//    std::cout << std::endl;
    
    
//    cout << "1. default way: " << endl;
//    cout << "----------------" << endl;
//    Sales_data s1;
//
//    cout << "\n2. use std::string as parameter: " << endl;
//    cout << "----------------" << endl;
//    Sales_data s2("CPP-Primer-5th");
//
//    cout << "\n3. complete parameters: " << endl;
//    cout << "----------------" << endl;
//    Sales_data s3("CPP-Primer-5th", 3, 25.8);
//
//    cout << "\n4. use istream as parameter: " << endl;
//    cout << "----------------" << endl;
//    Sales_data s4(std::cin);
    
    
    //////////////////////////////////////////////////////////////// 顺序容器 ////////////////////////////////////////////////////////////////
//    vector<int> ivec{1,2,3,4,5,6,7,8,9,10};
//    vector<int>::const_iterator beg = ivec.begin();
//    vector<int>::const_iterator end = ivec.end();
//
//    auto it1 = ivec.cbegin();
//    auto it2 = ivec.cend();
//
//    bool f = find(it1, it2, 1); //这里find输入参数为const 所以必须明确auto的类型为const 所以使用cbegin() 或者直接显式指定it1的类型为vector<int>::const_iterator (见beg)
//    if (f == true) {
//        cout << " f i n d" << endl;
//    }
//    else{
//        cout << "n o f i n d" << endl;
//    }
//    cout << "===========" << endl;
//    while (beg != end) {
//        cout << *beg << endl;
//        ++beg;
//    }
//
//
//    list<int> il(5,5);
//    vector<double> dv(il.begin(),il.end()); // 使用容器初始化另一个容器 容器类型不同,元素不同时 则选择使用迭代器的方式(只要元素能相容)
//
//    vector<int> dv0(5,5);
//    vector<double> dv1(dv0.begin(),dv0.end()); // 初始化方式
//
//    list<const char *> cl(5,"Hiya");
//    vector<string> svec ; // 默认初始化
//    svec.assign(cl.begin(), cl.end()); // 赋值
//    svec.insert(svec.begin(), "begin=");
//
//
//    for (auto s : svec) {
//        cout << s << " ";
//    }
//    cout << endl;
//
//
//    list<int> ilist{1,2,3,4,5,6,7,8,9};
//    deque<int> odd,even;
//
//    // 1使用迭代器遍历容器
////    for (std::list<int>::const_iterator ilist_iter = ilist.cbegin(); ilist_iter != ilist.cend(); ++ilist_iter) {
////        if ((*ilist_iter) % 2 == 0) {
////            even.push_back(*ilist_iter);
////        }
////        else{
////            odd.push_back(*ilist_iter);
////        }
////    }
//
//
//    // 2.使用范围for
//    for (auto i : ilist) {
//        (i & 0x1 ? odd : even).push_back(i);
//    }
//
//    for (auto i : odd) {
//        cout << i << " ";
//    }
//    cout << endl;
//
//    for (auto i : even) {
//        cout << i << " ";
//    }
//    cout << endl;
    
    
    
    int ia[] = {0,1,1,2,3,5,8,13,21,55,89};
    
    vector<int> iavec(std::begin(ia),std::end(ia));
    list<int> iali(std::begin(ia),std::end(ia));

    for (auto it = iavec.begin(); it != iavec.end(); ++it) {
        if (((*it) & 0x1) == 0) {
            iavec.erase(it);
        }
    }

    for (auto it = iali.begin(); it != iali.end(); ++it) {
        if ((*it) & 0x1) {
            iali.erase(it);
        }
    }


    for (auto i : iavec) {
        cout << i << " ";
    }
    cout << endl;

    for (auto i : iali) {
        cout << i << " ";
    }
    cout << endl;
    
    
    // 使用forward_list 的特殊erase删除操作 ,因为单向链表的特殊性 删除一个元素会改变它的前驱的后继,由于链表单向没办法访问到它的前驱,但能删除指定元素之后的那个元素(也就是指定元素的后继节点),为了能访问后继节点的值和当前节点的值,一般使用两个迭代器 1.指向待删除元素的迭代器 2.指向待删除元素前驱的那个迭代器.(也就是说 一个迭代器方便给出元素值,另一个迭代器来进行操作这个元素值)
    
    
//    forward_list<string> fsl;
//    forward_list<int> fil;
//    forward_list<<#class _Tp#>>
    
    forward_list<int> fia(ia,std::end(ia));
    auto prev = fia.before_begin();
    auto curr = fia.begin();
    while (curr != fia.end()) {
        if ((*curr) % 2) { // odd
            curr = fia.erase_after(prev); // erase_after返回指向被删除元素的后继元素的迭代器 , 这里作用是更新curr的值使之往下一元素移动
        }
        else{ // even
            prev = curr; // 更新指向前驱节点的迭代器
            ++curr; // 再更新指向当前节点的迭代器 使之往下一元素移动
        }
    }
    
    for (auto i : fia) {
        cout << i << " ";
    }
    cout << endl;
    
    
    forward_list<string> sfl{"ashine","china","chengdu","gamer"};
    forward_insert(sfl, "ashine", "a");
    
    for (auto s : sfl) {
        cout << s << " ";
    }
    cout << endl;
    
    
    string s1("ashin is a china chengdu gamer.");
    
    replace_str_by_iterator(s1, "chengdu", "cd");
    cout << s1 << endl;
    replace_str_by_index(s1, "gamer", "movier");
    cout << s1 << endl;
    
    
    vector<int> acc{1,2,3,4,5,6,7,8,9};
    cout << std::accumulate(acc.begin(), acc.end(), 0) << endl;
    
    
    int test_lambda_int = 5;
    auto lambda = [](const int lhs,const int rhs){return lhs + rhs;}; // 捕获列表为空
    auto lambda2 = [test_lambda_int](const int lhs){return test_lambda_int + lhs;}; //值捕获
    
    lambda(1, 2);
    lambda2(3);
    
    auto lambda3 = [test_lambda_int]()mutable{test_lambda_int+=1;};
    lambda3();
    cout << test_lambda_int << endl;
    
    
    auto lamdda3 = []{}; // 最简单的lambda表达式
    
    
    // 使用标准库bind函数
    auto check6 = std::bind(check_size, std::placeholders::_1,6); // 名字_1定义在命名空间placeholders中,而这个命名空间本身定义在std命名空间中
    auto check5 = std::bind(check_size, std::placeholders::_1,5);
    
    string s = "Hello";
    bool b1 = check6(s);
    
    vector<string> words{"Hello","World","The","End"};
    auto wc = std::find_if(words.begin(), words.end(), check5); // 使用参数绑定的方式传递函数作为谓词,而不是使用lambda
    
//    back_inserter(fia);
//    push_heap(fia, <#_RandomAccessIterator __last#>, <#_Compare __comp#>)
    
    return 0;
}

bool check_size(const string &s, string::size_type sz){
    return s.size() >= sz;
}

void forward_insert(forward_list<string> &sfl,string find,string ins){
    auto prev = sfl.before_begin();
    auto curr = sfl.begin();
    while (curr != sfl.end()) {
        if ((*curr) == find) {
            sfl.insert_after(curr, ins);
            return;
        }
        prev = curr;
        ++curr;
    }
    sfl.insert_after(prev, ins);
}


// 使用迭代器并配合使用erase和insert版本的字符串替换算法
void replace_str_by_iterator(string &s,const string &oldVal,const string &newVal){
    auto iterator = s.begin();
    while (iterator != s.end() - newVal.size()) {
        if (oldVal == string(iterator, iterator + oldVal.size())) { // 通过迭代器找到oldval对应的位置
            iterator = s.erase(iterator, iterator + oldVal.size()); // 抹除从迭代器开始oldval.size长度的元素(就是抹掉和oldval一样的元素),并更新迭代器的值 现在迭代器指向了抹除元素之后的那个元素位置
            iterator = s.insert(iterator, newVal.begin(), newVal.end()); //在迭代器位置之前插入newval的值,然后更新了迭代器的值,现在迭代器指向新插入的第一个元素 由于插入/删除可能会使迭代器失效(对于vector,string,deque来说) 需要更新迭代器的值
            iterator += newVal.size(); // 因为上面的更新 现在iterator指向了第一个插入的元素,然后这个语句跳过了新插入的元素们
        }
        else{
            ++iterator;
        }
    }
}

// 使用下标和replace版本的字符串替换算法
void replace_str_by_index(string &s,const string &oldVal,const string &newVal){
    for (string::size_type pos = 0; pos < s.size();/* 这里不使用自增运算是因为replace和普通的遍历自增改变的pos的步长不一致*/ ) {
        if (oldVal == string(s , pos , oldVal.size())) {
            s.replace(pos, oldVal.size(), newVal);
            pos += newVal.size(); // 当有replace发生时,pos增长的步长为newVal的长度
        }
        else{
            ++pos; // 自增遍历时步长为固定值1
        }
    }
}



//void replace_str_by_iterator_replace(string &s,const string &oldVal,const string &newVal){
//    auto iterator = s.begin();
//    while (iterator != s.end() - newVal.size()) {
//        if (oldVal == string(iterator, iterator + oldVal.size())) {
//            s.replace(iterator, iterator + oldVal.size(), newVal); // replace以后迭代器没有更新 这是个错误的方式
//            iterator += oldVal.size();
//        }
//        else{
//            ++iterator;
//        }
//    }
//}


void find_num_by_first_of(const string &s,const string &nums){
    string::size_type pos = 0;
    while ((pos = s.find_first_of(nums,pos))) {
        cout << s[pos] << " ";
        ++pos;
    }
}


