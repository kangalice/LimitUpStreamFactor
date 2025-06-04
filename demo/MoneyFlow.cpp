// 此代码并非MoneyFlow相关因子的计算代码，仅作为demo中代码约定格式举例
#include "MatchEngineAPI.hpp"
#include "ObjectPool.hpp"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>

// 用户自定义结构体，引用原始消息的同时拷贝一部分需要改动的字段
struct MyOrder {
    seqnum_t applseqnum;
    price_t price;
    qty_t qty;
};

struct MyTrade {
    seqnum_t applseqnum;
    price_t price;
    qty_t qty;
};

std::string get_date_string(const std::string& dateInput = "") {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);

    if (!dateInput.empty()) {
        std::istringstream ss(dateInput);
        ss >> std::get_time(tm, "%Y-%m-%d");
    }

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

class DemoMatchEngineSPI : public MatchEngineSPI {
public:
    /// @brief 初始化函数，会在setAPI之后被调用
    void Init() override {
        std::cout << "[spi] handling code list: " << code_list_[0] << "~" << code_list_[code_list_.size()-1] << std::endl;
        col_size_ = code_list_.size();

        for (int i = 0; i < col_size_; i++) {
            ori_order_map[code_list_[i]] = std::unordered_map<seqnum_t, MyOrder *>();
            ori_trade_map[code_list_[i]] = std::unordered_map<seqnum_t, MyTrade *>();
            my_sec_idx_[code_list_[i]] = i;
        }
        // 如果需要存储大量的数据，建议使用对象池，demo中提供一种对象池的接口，用户可以自行实现更多功能的对象池
        // ObjectPoolST<MyOrder> my_order_pool_ = ObjectPoolST<MyOrder>(10, 1<<23);  // 10 * 1<<23 ≈ 80m 条的数据
        // ObjectPoolST<MyTrade> my_trade_pool_ = ObjectPoolST<MyTrade>(10, 1<<23);
        
        order_num_.resize(col_size_, 0);
        trade_num_.resize(col_size_, 0);
    }

    /// 状态变量更新函数
    void reset() {
        std::fill(order_num_.begin(), order_num_.end(), 0.0);
        std::fill(trade_num_.begin(), trade_num_.end(), 0.0);
    }

    void reset(int securityid) {
        order_num_[my_sec_idx_[securityid]] = 0.0;
        trade_num_[my_sec_idx_[securityid]] = 0.0;
    }

    /// @brief 在当笔盘口更新前，接收逐笔委托
    /// @param order 委托数据
    void onBeforeAddOrder(const UnifiedRecord *order) override {
        if (ori_order_map.find(order->securityid) == ori_order_map.end()) {
            return;
        }
        auto &sec_map = ori_order_map[order->securityid];
        
        if (order->type != Type::Cancel) {
            if (sec_map.find(order->applseqnum) == sec_map.end()) {
                // 如果全为增量写法，则无需写下方的添加到订单簿的代码
                // 建议使用对象池构造
                // MyOrder* my_order = new (my_order_pool_.acquire()) MyOrder{order->applseqnum, order->price, order->qty};
                // sec_map.emplace(order->applseqnum, my_order);

                // 推荐写法：在逐笔数据更新时以增量的方式计算因子值
                order_num_[my_sec_idx_[order->securityid]]++;
            } else {
                // 上交所委托才会出现这种情况，还原逐笔委托和价格和量
                // auto my_order_it = sec_map.find(order->applseqnum);
                // my_order_it->second->qty += order->qty;
                // my_order_it->second->price = order->price;
            }

            // 如果要联动盘口数据
            // 获取最新的盘口状态，以对手方第三档数据为例
            // if (order->side == OrderSide::Bid) {
            //     PriceLevel *price_level = api_->getPriceLevel(order->securityid, OrderSide::Ask, 2);
            //     // ...
            // } else {
            //     PriceLevel *price_level = api_->getPriceLevel(order->securityid, OrderSide::Bid, 2);
            //     // ...
            // }
        } else {
            // 处理撤单数据
        }
    }

    /// @brief 在当笔盘口更新前，接收逐笔成交
    /// @param trade 成交数据
    void onBeforeAddTrade(const UnifiedRecord *trade) override {
        if (ori_trade_map.find(trade->securityid) == ori_trade_map.end()) {
            return;
        }

        // 推荐写法：在逐笔数据更新时以增量的方式计算因子值
        trade_num_[my_sec_idx_[trade->securityid]]++;

        // 如果是上交所成交单，且需要保存还原后的逐笔委托，需进行如下代码的还原操作
        if (trade->market == Exchange::SH) {
            seqnum_t ori_num = trade->side == Side::Buy ? trade->buyno : trade->sellno;
            auto &order_map = ori_order_map[trade->securityid];

            if (order_map.find(ori_num) == order_map.end()) {
                MyOrder* my_order = new (my_order_pool_.acquire()) MyOrder{ori_num, trade->price, trade->qty};
                order_map.emplace(ori_num, my_order);
            } else {
                // 可能有多笔成交对应一笔主动委托
                auto my_order_it = order_map.find(ori_num);
                my_order_it->second->qty += trade->qty;
                my_order_it->second->price = trade->price;
            }
        }

        // 如果全为增量写法，则无需写下方的添加到成交簿的代码
        // auto &sec_map = ori_trade_map[trade->securityid];
        // if (sec_map.find(trade->applseqnum) == sec_map.end()) {
        //     // 建议使用对象池构造
        //     MyTrade* my_trade = new (my_trade_pool_.acquire()) MyTrade{trade->applseqnum, trade->price, trade->qty};
        //     sec_map.emplace(trade->applseqnum, my_trade);
        // }

        // 如果要联动盘口数据
        // 获取最新的盘口状态，以对手方第三档数据为例
        // if (trade->trade_side == TradeSide::Buy) {
        //     PriceLevel *price_level = api_->getPriceLevel(trade->securityid, OrderSide::Ask, 2);
        //     // ...
        // } else {
        //     PriceLevel *price_level = api_->getPriceLevel(trade->securityid, OrderSide::Bid, 2);
        //     // ...
        // }
    }

    /// @brief 在当笔盘口更新后，接收逐笔委托
    /// @param order 委托数据
    void onAfterAddOrder(const UnifiedRecord *order) override {}

    /// @brief 在当笔盘口更新后，接收逐笔成交
    /// @param trade 成交数据
    void onAfterAddTrade(const UnifiedRecord *trade) override {}

    /// @brief 将本对象所维护的所有股票的因子值输出到共享内存中
    /// @param factor_ob_idx 因子ob的次数索引，也等于其在共享内存中应写入的行号
    /// @param row_length 当前行数据起始点
    void onFactorOB(int factor_ob_idx, int row_length) override {
        std::cout << "[factor ob "<< process_id_ << "] ob idx: " << factor_ob_idx << std::endl;
        int sec_idx;
        // 按在Params.factor_names中确定好的因子顺序，逐股票写出因子值，同时注意只写出本进程对应的股票的数据
        for (int i = 0; i < col_size_; i++) {
            sec_idx = sec_idx_dict_[code_list_[i]];
            v_shm_[0][row_length+sec_idx] = order_num_[i];
            v_shm_[1][row_length+sec_idx] = trade_num_[i];
        }
        this->reset();
    }

    /// @brief 将函数指定的股票的因子值输出到共享内存中
    /// @param factor_ob_idx 因子ob的次数索引，也等于其在共享内存中应写入的行号
    /// @param row_length 当前行数据起始点
    /// @param securityid 写出数据的股票代码
    void onFactorOB(int factor_ob_idx, int row_length, int securityid) override {
        int sec_idx = sec_idx_dict_[securityid];
        v_shm_[0][row_length+sec_idx] = order_num_[my_sec_idx_[securityid]];
        v_shm_[1][row_length+sec_idx] = trade_num_[my_sec_idx_[securityid]];
        this->reset(securityid);
    }

protected:
    ObjectPoolST<MyOrder> my_order_pool_;    // 委托对象池
    ObjectPoolST<MyTrade> my_trade_pool_;    // 成交对象池

	std::unordered_map<int, std::unordered_map<seqnum_t, MyOrder *>> ori_order_map;     // 分股票的原始订单委托序列
    std::unordered_map<int, std::unordered_map<seqnum_t, MyTrade *>> ori_trade_map;     // 分股票的原始订单成交序列
    std::unordered_map<int, int> my_sec_idx_;                                           // 维护一个本spi处理的股票的下标序列

	std::vector<double> order_num_;     // 要写的因子1：委托笔数
	std::vector<double> trade_num_;     // 要写的因子2：成交笔数
    size_t col_size_;                   // 列的长度，即本spi处理的股票总数
};

int main(int argc, char* argv[]) {
    // 调用时输入日期，否则默认运行当天
    std::string date_input = "";
    if (argc > 1) {
        date_input = argv[1];
    }
    std::string use_date = get_date_string(date_input);
    std::string sim_date;
    std::remove_copy(use_date.begin(), use_date.end(), std::back_inserter(sim_date), '-');

    // 调用时输入端口增量
    int incre_port = 0;
    if (argc > 2) {
        incre_port = std::stoi(argv[2]);
    }

    /*********************
     * api的初始化分为以下三步：
     * 1、创建一个接收回报的类，该类继承自MatchEngineSPI，用来接收撮合引擎过程中接收到的逐笔委托和成交
     * 2、调用createMatchAPI接口，创建一个API对象，该对象用来启动撮合和和查询盘口
     * 3、调用registerSpi接口，将回报接收类对象传进请求类对象中
    *************************/
    MatchEngineAPI *match_api = MatchEngineAPI::createMatchAPI();
    auto match_spi = new DemoMatchEngineSPI();
    match_api->registerSPI(match_spi);

    MatchParam param;
    param.factor_names = std::vector<std::string>{"order_num", "trade_num"};
    param.process_num = 3;
    param.recv_market = "sz";
    match_api->setParam(param);

    // 启动撮合，进程会阻塞在该函数
    match_api->startMatch(use_date, incre_port);

    // api在结束后会自动保存，清理内存，用户在此处可直接退出
}