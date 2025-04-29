#include "MatchEngineAPI.hpp"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>

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
            ori_order_map[code_list_[i]] = std::unordered_map<seqnum_t, Order *>();
            ori_trade_map[code_list_[i]] = std::unordered_map<seqnum_t, Trade *>();
            my_sec_idx_[code_list_[i]] = i;
        }
        
        order_num_.resize(col_size_, 0);
        trade_num_.resize(col_size_, 0);
    }

    /// 状态变量更新函数
    void reset() {
        std::fill(order_num_.begin(), order_num_.end(), 0.0);
        std::fill(trade_num_.begin(), trade_num_.end(), 0.0);
    }

    /// @brief 在当笔盘口更新前，接收逐笔委托
    /// @param order 委托数据
    void onBeforeAddOrder(Order *order) override {
        if (ori_order_map.find(order->secutiryid) == ori_order_map.end()) {
            return;
        }
        auto &sec_map = ori_order_map[order->secutiryid];

        if (order->ordertype != OrderType::Cancel) {
            if (sec_map.find(order->applseqnum) == sec_map.end()) {
                // 如果全为增量写法，则无需写下方的添加到订单簿的代码
                // sec_map.emplace(order->applseqnum, order);

                // 推荐写法：在逐笔数据更新时以增量的方式计算因子值
                order_num_[my_sec_idx_[order->secutiryid]]++;
            } else {
                // 上交所委托才会出现这种情况
                // auto order_it = sec_map.find(order->applseqnum);
                // order_it->second->orderqty += order->orderqty;
            }

            // 如果要联动盘口数据
            // 获取最新的盘口状态，以对手方第三档数据为例
            // if (order->side == OrderSide::Bid) {
            //     PriceLevel *price_level = api_->getPriceLevel(order->secutiryid, OrderSide::Ask, 2);
            //     // ...
            // } else {
            //     PriceLevel *price_level = api_->getPriceLevel(order->secutiryid, OrderSide::Bid, 2);
            //     // ...
            // }
        } else {
            // 处理撤单数据
        }
    }

    /// @brief 在当笔盘口更新前，接收逐笔成交
    /// @param trade 成交数据
    void onBeforeAddTrade(Trade *trade) override {
        if (ori_trade_map.find(trade->secutiryid) == ori_trade_map.end()) {
            return;
        }
        auto &sec_map = ori_trade_map[trade->secutiryid];

        // 推荐写法：在逐笔数据更新时以增量的方式计算因子值
        trade_num_[my_sec_idx_[trade->secutiryid]]++;

        // 如果全为增量写法，则无需写下方的添加到成交簿的代码
        // if (sec_map.find(trade->applseqnum) == sec_map.end()) {
        //     sec_map.emplace(trade->applseqnum, trade);

        //     // 推荐写法：在逐笔数据更新时以增量的方式计算因子值
        //     trade_num_[my_sec_idx_[trade->secutiryid]]++;
        // }

        // 如果要联动盘口数据
        // 获取最新的盘口状态，以对手方第三档数据为例
        // if (trade->trade_side == TradeSide::Buy) {
        //     PriceLevel *price_level = api_->getPriceLevel(trade->secutiryid, OrderSide::Ask, 2);
        //     // ...
        // } else {
        //     PriceLevel *price_level = api_->getPriceLevel(trade->secutiryid, OrderSide::Bid, 2);
        //     // ...
        // }
    }

    /// @brief 在当笔盘口更新后，接收逐笔委托
    /// @param order 委托数据
    void onAfterAddOrder(Order *order) override {}

    /// @brief 在当笔盘口更新后，接收逐笔成交
    /// @param trade 成交数据
    void onAfterAddTrade(Trade *trade) override {}

    /// @brief 将因子值输出到共享内存中
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

protected:
	std::unordered_map<int, std::unordered_map<seqnum_t, Order *>> ori_order_map;   // 分股票的原始订单委托序列
    std::unordered_map<int, std::unordered_map<seqnum_t, Trade *>> ori_trade_map;   // 分股票的原始订单成交序列
    std::unordered_map<int, int> my_sec_idx_;                                       // 维护一个本spi处理的股票的下标序列

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
    match_api->setParam(MatchParam{std::vector<std::string>{"order_num", "trade_num"}});

    // 启动撮合，进程会阻塞在该函数
    match_api->startMatch(use_date, incre_port);

    // api在结束后会自动保存，清理内存，用户在此处可直接退出
}