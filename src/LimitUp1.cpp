// 此代码为Basic相关因子的计算代码，仅作为demo中代码约定格式举例
#include "MatchEngineAPI.hpp"
#include "ObjectPool.hpp"
#include <algorithm>
#include <bitset>
#include <common_utils/Utils.hpp>
#include <ctime>
#include <cxxopts.hpp>
#include <filesystem>
#include <fmtlog/fmtlog.h>
#include <iomanip>
#include <iostream>
#include "utils/rounding.h"
/// @brief 默认日志目录的全局常量
const std::string DEFAULT_LOGS_DIR = "logs";

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

class DemoMatchEngineSPI : public MatchEngineSPI {
public:
    /// @brief 初始化函数，会在setAPI之后被调用
    void Init() override {
        logi("[spi] handling code list: {}~{}", code_list_[0], code_list_[code_list_.size() - 1]);
        std::cout << "[spi] handling code list: " << code_list_[0] << "~" << code_list_[code_list_.size() - 1]
                  << std::endl;
        col_size_ = code_list_.size();

        my_sec_idx_.resize(700000, -1);

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
        up_limit_order_vol_.resize(col_size_, 0);

        // 通过字段名获取 idx, 名字如有"/"，这里传参直接使用"/"后面的字符串
        up_limit_price_idx_ = api_->getDailyDataIdx("up_limit_price");
    }

    /// 状态变量更新函数
    void reset() {
        std::fill(order_num_.begin(), order_num_.end(), 0.0);
        std::fill(trade_num_.begin(), trade_num_.end(), 0.0);
        std::fill(up_limit_order_vol_.begin(), up_limit_order_vol_.end(), 0.0);
    }

    void reset(int securityid) {
        order_num_[my_sec_idx_[securityid]] = 0.0;
        trade_num_[my_sec_idx_[securityid]] = 0.0;
        up_limit_order_vol_[my_sec_idx_[securityid]] = 0.0;
    }

    /// @brief 在当笔盘口更新前，接收逐笔委托
    /// @param order 委托数据
    void onBeforeAddOrder(const UnifiedRecord *order) override {
        // if (ori_order_map.find(order->securityid) == ori_order_map.end()) {
        //     return;
        // }
        if (my_sec_idx_[order->securityid] == -1) {
            return;
        }
        auto &sec_map = ori_order_map[order->securityid];

        if (order->type != Type::Cancel) {
            if (sec_map.find(order->applseqnum) == sec_map.end()) {
                // 如果全为增量写法，则无需写下方的添加到订单簿的代码
                // 建议使用对象池构造
                // MyOrder* my_order = new (my_order_pool_.acquire()) MyOrder{order->applseqnum, order->price,
                // order->qty}; sec_map.emplace(order->applseqnum, my_order);

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

            // // 如果要获取历史3s盘口数据示例：
            // double ask1_latest = api_->get3sKlineLatest(order, "ask1");       // 获取该股票最近一次
            // double ask1_by_idx = api_->get3sKlineByIdx(order, "ask1", 97);    // 按该股票OB次数索引获取
            // double ask1_by_time = api_->get3sKlineByTime(order, "ask1", 93015000);    // 按时间获取

            // // 获取日频静态数据示例：需先在 MatchParam 中正确配置字段的加载
            // // double adj_factor = api_->getDailyData(accum_adj_factor_idx_, order->securityid);
            double up_limit_price = api_->getDailyData(up_limit_price_idx_, order->securityid);
            if (order->price >= up_limit_price) {
                up_limit_order_vol_[my_sec_idx_[order->securityid]] += order->qty;
            }
        } else {
            // 处理撤单数据
        }
    }

    /// @brief 在当笔盘口更新前，接收逐笔成交
    /// @param trade 成交数据
    void onBeforeAddTrade(const UnifiedRecord *trade) override {
        // if (ori_trade_map.find(trade->securityid) == ori_trade_map.end()) {
        //     return;
        // }
        if (my_sec_idx_[trade->securityid] == -1) {
            return;
        }

        // 推荐写法：在逐笔数据更新时以增量的方式计算因子值
        trade_num_[my_sec_idx_[trade->securityid]]++;

        // 如果是上交所成交单，且需要保存还原后的逐笔委托，需进行如下代码的还原操作，跳过集合竞价成交单的还原
        if (trade->market == Exchange::SH && trade->side != Side::N) {
            seqnum_t ori_num = trade->side == Side::Buy ? trade->buyno : trade->sellno;
            auto &order_map = ori_order_map[trade->securityid];

            if (order_map.find(ori_num) == order_map.end()) {
                MyOrder *my_order = new (my_order_pool_.acquire()) MyOrder{ori_num, trade->price, trade->qty};
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
        std::cout << "[factor ob " << process_id_ << "] ob idx: " << factor_ob_idx << std::endl;
        logi("[factor ob {}] ob idx: {}", process_id_, factor_ob_idx);
        int sec_idx;
        // 按在Params.factor_names中确定好的因子顺序，逐股票写出因子值，同时注意只写出本进程对应的股票的数据
        for (int i = 0; i < col_size_; i++) {
            sec_idx = sec_idx_vec_[code_list_[i]];
            v_shm_[0][row_length + sec_idx] = order_num_[i];
            v_shm_[1][row_length + sec_idx] = trade_num_[i];
            v_shm_[2][row_length + sec_idx] = up_limit_order_vol_[i];
        }
        this->reset();
    }

    /// @brief 将函数指定的股票的因子值输出到共享内存中
    /// @param factor_ob_idx 因子ob的次数索引，也等于其在共享内存中应写入的行号
    /// @param row_length 当前行数据起始点
    /// @param securityid 写出数据的股票代码
    void onFactorOB(int factor_ob_idx, int row_length, int securityid) override {
        int sec_idx = sec_idx_vec_[securityid];
        if (factor_ob_idx >= max_factor_ob_idx_) {
            std::cout << "[factor ob " << process_id_ << "] ob idx: " << factor_ob_idx << std::endl;
            logi("[factor ob {}] ob idx: {}, row_length: {}, sec_idx: {}",
                 process_id_,
                 factor_ob_idx,
                 row_length,
                 sec_idx);
            max_factor_ob_idx_++;
        }
        v_shm_[0][row_length + sec_idx] = order_num_[my_sec_idx_[securityid]];
        v_shm_[1][row_length + sec_idx] = trade_num_[my_sec_idx_[securityid]];
        v_shm_[2][row_length + sec_idx] = up_limit_order_vol_[my_sec_idx_[securityid]];
        this->reset(securityid);
    }

protected:
    ObjectPoolST<MyOrder> my_order_pool_;    // 委托对象池
    ObjectPoolST<MyTrade> my_trade_pool_;    // 成交对象池

    std::unordered_map<int, std::unordered_map<seqnum_t, MyOrder *>> ori_order_map;    // 分股票的原始订单委托序列
    std::unordered_map<int, std::unordered_map<seqnum_t, MyTrade *>> ori_trade_map;    // 分股票的原始订单成交序列
    // std::unordered_map<int, int> my_sec_idx_;    // 维护一个本spi处理的股票的下标序列
    std::vector<int> my_sec_idx_;    // 数组形式的本spi处理的股票的下标序列

    std::vector<double> order_num_;             // 要写的因子1：委托笔数
    std::vector<double> trade_num_;             // 要写的因子2：成交笔数
    std::vector<double> up_limit_order_vol_;    // 要写的因子3：涨停价委托量

    size_t col_size_;            // 列的长度，即本spi处理的股票总数
    size_t aligned_col_size_;    // 实际列长度，为SIMD对齐准备的股票个数

    int max_factor_ob_idx_ = 0;    // 当前最大的factor_ob_idx

    // 用户定义的静态数据字段索引
    size_t up_limit_price_idx_;
};

int main(int argc, char *argv[]) {
    // 接收处理命令行参数
    cxxopts::Options options(argv[0],
                             "Basic Factor Generator Demo\n"
                             "此程序用于生成基本的交易因子数据。\n\n"
                             "用法示例：\n"
                             "  ./demo -d 2023-01-01 -i 0 -c 3\n"
                             "  ./demo 2023-01-01 0 3\n"
                             "  ./demo \n");
    auto opts_builder = options.add_options();
    opts_builder("d,date", "指定数据处理的日期...", cxxopts::value<std::string>()->default_value(""));
    opts_builder("i,incre_port", "指定增量端口...", cxxopts::value<int>()->default_value("0"));
    opts_builder("p,pnum", "指定工作进程数量...", cxxopts::value<int>()->default_value("3"));
    opts_builder("skip_unlink", "跳过共享内存的自动清理（调试用）...", cxxopts::value<bool>()->default_value("false"));
    opts_builder(
        "check_error", "是否要进行spi错误的检查（算历史用）...", cxxopts::value<bool>()->default_value("false"));
    opts_builder("h,help", "打印此帮助信息...");

    options.parse_positional({"date", "incre_port", "pnum"});

    // 解析命令行参数
    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const cxxopts::exceptions::exception &e) {
        loge("解析选项错误: {}", e.what());
        std::cerr << options.help() << std::endl;
        return 1;
    }
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // 提取已经解析好的命令行参数
    std::string date_input = result["date"].as<std::string>();
    int incre_port = result["incre_port"].as<int>();
    int process_num = result["pnum"].as<int>();
    bool skip_unlink = result["skip_unlink"].as<bool>();
    bool check_error = result["check_error"].as<bool>();

    std::string use_date = get_date_string(date_input);

    /*********************
     * api的初始化分为以下三步：
     * 1、创建一个接收回报的类，该类继承自MatchEngineSPI，用来接收撮合引擎过程中接收到的逐笔委托和成交
     * 2、调用createMatchAPI接口，创建一个API对象，该对象用来启动撮合和和查询盘口
     * 3、调用registerSpi接口，将回报接收类对象传进请求类对象中
     *************************/
    MatchEngineAPI *match_api = MatchEngineAPI::createMatchAPI();
    auto match_spi = new DemoMatchEngineSPI();
    match_api->registerSPI(match_spi);

    // 设置参数，参数含义见MatchParam声明
    MatchParam param;
    param.factor_names = std::vector<std::string>{"order_num", "trade_num", "up_limit_order_vol","xly_t_prup_qty_ratio"};
    param.incre_port = incre_port;
    param.process_num = process_num;
    param.skip_unlink = skip_unlink;
    param.log_level = 1;
    param.check_error = check_error;
    param.factor_interver_ms = 60000;
    param.bind_cpu_start_id = -1;

    // // 使用 "parquet" 模式读取静态数据
    // param.offline_mode = "parquet";
    // param.path_parquet = "/mnt/public_shared_files/temp_daily_use_data";
    // param.data_keys = {"accum_adj_factor_2",    //
    //                    "down_limit_price",      //
    //                    "pre_close_price",       //
    //                    "static_data",           //
    //                    "up_limit_price"};       //
    // param.auto_sim_date = true;

    // 或使用 "hdfdata" 模式读取静态数据
    param.offline_mode = "hdfdata";
    param.data_keys = {"basic/up_limit_price","basic/act_pre_close_price","basic/neg_market_value"};

    match_api->setParam(param);

    // 启动撮合，进程会阻塞在该函数
    match_api->startMatch(use_date);

    // 用户可调用saveData函数来保存数据
    // match_api->saveData(param.factor_names, "Test1", "fileSystem", "1min,stock");
    // 无文件系统保存权限的用户可以保存为parquet文件
    match_api->saveData(param.factor_names, "Test1", "parquet");

    // 结束后建议手动调用close函数
    match_api->close();
    return 0;
}