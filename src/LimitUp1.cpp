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

// 逐笔因子改写成TimeBar,所有因子按照trade_num进行平均

enum class ActiveType {
    Active,
    Passive,
};

// 用户自定义结构体，引用原始消息的同时拷贝一部分需要改动的字段
struct MyOrder {
    seqnum_t applseqnum;
    price_t price;
    qty_t qty;
    ActiveType activetype = ActiveType::Passive;    // 默认为被动委托，出现主动成交则调整为主动委托
    
};

struct MyTrade {
    seqnum_t applseqnum;
    price_t price;
    qty_t qty;
    bool first_reach = false;
    // double ret;  //相对上笔成交价的收益率
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
            unique_trade_price_[code_list_[i]] = std::set<double>();
            unique_trade_high_price_[code_list_[i]] = std::set<double>();
            my_sec_idx_[code_list_[i]] = i;
        }
        // 如果需要存储大量的数据，建议使用对象池，demo中提供一种对象池的接口，用户可以自行实现更多功能的对象池
        // ObjectPoolST<MyOrder> my_order_pool_ = ObjectPoolST<MyOrder>(10, 1<<23);  // 10 * 1<<23 ≈ 80m 条的数据
        // ObjectPoolST<MyTrade> my_trade_pool_ = ObjectPoolST<MyTrade>(10, 1<<23);

        order_num_.resize(col_size_, 0);
        trade_num_.resize(col_size_, 0);
        BidActiveOrderNum.resize(col_size_, 0);
        AskActiveOrderNum.resize(col_size_, 0);
        AskActiveOrderVolume.resize(col_size_, 0.0);
        BidActiveOrderVolume.resize(col_size_, 0.0);
        BidActiveOrderAmount.resize(col_size_, 0.0);
        AskActiveOrderAmount.resize(col_size_, 0.0);
        trade_amt_B.resize(col_size_, 0.0);
        trade_amt_S.resize(col_size_, 0.0);
        trade_vol_B.resize(col_size_, 0.0);
        trade_vol_S.resize(col_size_, 0.0);
        trade_vol_.resize(col_size_, 0.0);
        trade_amt_.resize(col_size_, 0.0);
        trade_high_price_vol_.resize(col_size_, 0.0);
        first_reach_flag_.resize(col_size_, false); // 必须false
        tamt_ao_add_tvol_precls_ratio.resize(col_size_, 0.0);

        // 通过字段名获取 idx, 名字如有"/"，这里传参直接使用"/"后面的字符串
        up_limit_price_idx_ = api_->getDailyDataIdx("up_limit_price");
        pre_close_idx_ = api_->getDailyDataIdx("act_pre_close_price");
        neg_mkt_idx_ = api_->getDailyDataIdx("neg_market_value");
    }

    /// 状态变量更新函数
    void reset() {
    }

    void reset(int securityid) {
    }

    /// @brief 在当笔盘口更新前，接收逐笔委托
    /// @param order 委托数据
    void onBeforeAddOrder(const UnifiedRecord *order) override {
        if (my_sec_idx_[order->securityid] == -1) {
            return;
        }
        auto &sec_map = ori_order_map[order->securityid];
        int &sec_idx_ = my_sec_idx_[order->securityid];

        if (order->type != Type::Cancel) {
            if (sec_map.find(order->applseqnum) == sec_map.end()) {
                // 如果全为增量写法，则无需写下方的添加到订单簿的代码
                // 建议使用对象池构造
                MyOrder* my_order = new (my_order_pool_.acquire()) MyOrder{order->applseqnum, order->price, order->qty}; 
                sec_map.emplace(order->applseqnum, my_order);

                if (order->side == Side::Buy) {
                    order_num_B[sec_idx_]++;    // 委托笔数只统计原始一次
                } else if (order->side == Side::Sell) {
                    order_num_S[sec_idx_]++;
                }
                // 推荐写法：在逐笔数据更新时以增量的方式计算因子值
                order_num_[my_sec_idx_[order->securityid]]++;
            } else {
                // 处理重复委托数据,上交所可能出现,需要调整委托订单的下单量与下单价，进而调整规模分类
                auto order_it = sec_map.find(order->applseqnum);
                double ori_amt = order_it->second->qty * order_it->second->price;    // 计算原始委托金额
                price_t new_price;                                                   // 新的下单价格
                if (order->side == Side::Sell) {
                    new_price = std::min(order->price, order_it->second->price);
                } else if (order->side == Side::Buy) {
                    new_price = std::max(order->price, order_it->second->price);
                }
                double amt = (order_it->second->qty + order->qty) * new_price;    // 计算更新后的委托金额

                if (order->side == Side::Sell) {    // 卖单
                    if (order_it->second->activetype == ActiveType::Active) {    // 识别新的主动订单
                        AskActiveOrderVolume[sec_idx_] += order->qty;
                        AskActiveOrderAmount[sec_idx_] += amt - ori_amt;
                        
                    }
                }
                else if (order->side == Side::Buy) {    // 买单
                    if (order_it->second->activetype == ActiveType::Active) {    // 识别新的主动订单
                        BidActiveOrderVolume[sec_idx_] += order->qty;
                        BidActiveOrderAmount[sec_idx_] += amt - ori_amt;
                    }
                }

                // 无论何时，更新委托信息
                order_it->second->qty += order->qty;          // 更新委托量
                order_it->second->price = new_price;          // 更新委托价格
                order_it->second->sizetype = new_sizetype;    // 更新委托大小单类型
            }

        } else {
            // 处理撤单数据
        }
    }

    /// @brief 在当笔盘口更新前，接收逐笔成交
    /// @param trade 成交数据
    void onBeforeAddTrade(const UnifiedRecord *trade) override {
        int &sec_idx_ = my_sec_idx_[trade->securityid];
        if (sec_idx_ == -1) {return;}

        // 如果是上交所成交单，且需要保存还原后的逐笔委托，需进行如下代码的还原操作，跳过集合竞价成交单的还原
        if (trade->market == Exchange::SH && trade->side != Side::N) {
            seqnum_t ori_num = trade->side == Side::Buy ? trade->buyno : trade->sellno;
            auto &order_map = ori_order_map[trade->securityid];

            if (order_map.find(ori_num) == order_map.end()) {
                MyOrder *my_order = new (my_order_pool_.acquire()) MyOrder{ori_num, trade->price, trade->qty};
                order_map.emplace(ori_num, my_order);
                if (trade->side == Side::Buy) {
                    order_num_B[sec_idx_]++;    // 可能只出现trade无剩余order，笔数只统计原始一次
                    // order_vol_B[sec_idx_] += order->qty;
                    // order_amt_B[sec_idx_] += order->qty * order->price;
                } else if (trade->side == Side::Sell) {
                    order_num_S[sec_idx_]++;
                    // order_vol_S[sec_idx_] += order->qty;
                    // order_amt_S[sec_idx_] += order->qty * order->price;
                }
            } else {
                // 可能有多笔成交对应一笔主动委托
                auto order_it = order_map.find(ori_num);
                double ori_amt = order_it->second->qty * order_it->second->price;    // 计算原始委托金额
                price_t new_price;                                                   // 新的下单价格
                if (trade->side == Side::Sell) {
                    new_price = std::min(trade->price, order_it->second->price);
                } else if (trade->side == Side::Buy) {
                    new_price = std::max(trade->price, order_it->second->price);
                }
                double amt = (order_it->second->qty + trade->qty) * new_price;    // 计算更新后的委托金额

                if (trade->side == Side::Sell) {    // 卖单
                    if (order_it->second->activetype == ActiveType::Active) {    // 识别新的主动订单
                        AskActiveOrderVolume[sec_idx_] += trade->qty;
                        AskActiveOrderAmount[sec_idx_] += amt - ori_amt;
                }}
                else if (trade->side == Side::Buy) {    // 买单
                    if (order_it->second->activetype == ActiveType::Active) {    // 识别新的主动订单
                        BidActiveOrderVolume[sec_idx_] += trade->qty;
                        BidActiveOrderAmount[sec_idx_] += amt - ori_amt;
                }}

                // 无论何时，更新委托信息
                order_it->second->qty += trade->qty;          // 更新委托量
                order_it->second->price = new_price;          // 更新委托价格
                // order_it->second->sizetype = new_sizetype;    // 更新委托大小单类型
            }
        }

        // 判断数据
        double pre_close_price = api_->getDailyData(pre_close_idx_, trade->securityid);
        double neg_mkt = api_->getDailyData(neg_mkt_idx_, trade->securityid);
        double thresold_price = utils::is_zcz(trade->securityid) ? pre_close_price * 1.12 : pre_close_price * 1.06;
        double rounded_threshold_price = utils::round_half_up(thresold_price,4);
        if not first_reach_flag_[sec_idx_]:  // 首次达到阈值价格
            first_reach_flag_[sec_idx_] = (trade->price >= rounded_threshold_price);
        // 如果全为增量写法，则无需写下方的添加到成交簿的代码
        auto &sec_map = ori_trade_map[trade->securityid];
        auto &sec_order_map = ori_order_map[trade->securityid];

        trade_num_[sec_idx_]++;
        if (sec_map.find(trade->applseqnum) == sec_map.end()) {
            // factor1: accum_prup_qty_ratio
            if (first_reach_flag_[sec_idx_]) {
                trade_high_price_vol_[sec_idx_] += trade->qty;  // 超过阈值价格之后的累计成交量
            }
            trade_vol_[sec_idx_] += trade->qty;  // 总成交量
            trade_amt_[sec_idx_] += trade->qty*trade->price;  // 总成交额
            // factor2: accum_t_prupt_uni_ratio
            auto tp_ = utils::round_half_up(trade->price,4);
            unique_trade_price_[trade->securityid].emplace(tp_);
            if first_reach_flag_[sec_idx_]:  // 超过阈值价格之后的唯一价格集合
                unique_trade_high_price_[trade->securityid].emplace(tp_);

            // factor3: vwap_pre_close_bias 在onFactorOB中计算
            // factor4: sundc_t_tran_16_fix 主动买卖平均量差/总量 
            // 判断是否为主动买入
            bool is_active_buy = ((trade->side == Side::Buy) || (trade->buyno > trade->sellno));
            bool is_active_sell = ((trade->side == Side::Sell) || (trade->sellno > trade->buyno));
            if (is_active_buy) {    // 主动买的统计
                trade_vol_B[sec_idx_] += trade->qty;
                trade_amt_B[sec_idx_] += trade->qty * trade->price;
            } else if (is_active_sell) {    // 主动卖的统计
                trade_vol_S[sec_idx_] += trade->qty;
                trade_amt_S[sec_idx_] += trade->qty * trade->price;
            }
            // 主动买单下单量
            if (trade->side != Side::N) {    // 处理主动交易信息，主要处理深证
                seqnum_t ori_num = trade->side == Side::Buy ? trade->buyno : trade->sellno;
                auto order_it = sec_order_map.find(ori_num);
                if (order_it != sec_order_map.end()) {
                    // 检查订单是否已经在当前窗口内被标记为主动订单
                    // lrh 260106 修改：ontrade识别到order中订单为主动，只对Active量额累加一次，避免之前一笔order多次成交多次累加的情况
                    bool is_first_active = (order_it->second->activetype == ActiveType::Passive);
                    order_it->second->activetype = ActiveType::Active;    // 主动交易方订单标记为主动交易订单
                    
                    if (is_first_active) {
                        // 第一次识别为主动订单，累加整个订单量（因为订单已经在 onBeforeAddOrder 中累加过 AskVolume_）
                        // 主被动行为均以trade为触发次数基准，只在trade定义；
                        double orderamt = order_it->second->qty * order_it->second->price;
                        if (trade->side == Side::Sell) {    // 卖单
                            AskActiveOrderVolume[sec_idx_] += order_it->second->qty;
                            AskActiveOrderAmount[sec_idx_] += orderamt;
                            AskActiveOrderNum[sec_idx_]++;  // 只算初始这一次
                        } else if (trade->side == Side::Buy) {    // 买单
                            BidActiveOrderVolume[sec_idx_] += order_it->second->qty;
                            BidActiveOrderAmount[sec_idx_] += orderamt;
                            BidActiveOrderNum[sec_idx_]++;  // 只算初始这一次
                        }
                    }
                    // 如果订单已经被标记为主动订单，说明已经在第一次成交时累加过整个订单量，后续成交不再累加
                }
            }        
            
            // factor5 成交额与(主动买挂量+成交量)之比相对昨收的偏离
            tamt_ao_add_tvol_precls_ratio[sec_idx_] = trade_amt_[sec_idx_]/(trade_vol_[sec_idx_]+ BidActiveOrderVolume[sec_idx_]) / pre_close_price - 1;
            
            // 
        
        
        }

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
            int row_len = row_length + sec_idx;
            v_shm_[0][row_len] = order_num_[i];
            v_shm_[1][row_len] = trade_num_[i];
            v_shm_[2][row_len] = trade_high_price_vol_[i]/ trade_vol_[i];
            v_shm_[3][row_len] = unique_trade_high_price_[code_list_[i]].size() / unique_trade_price_[code_list_[i]].size();
            v_shm_[4][row_len] = trade_amt_[i]/ trade_vol_[i] / api_->getDailyData(pre_close_idx_, code_list_[i]) - 1;
            v_shm_[5][row_len] = ((trade_vol_B[i]/(BidActiveOrderNum[i]+1))-(trade_vol_S[i]/(AskActiveOrderNum[i]+1)))/trade_vol_[i];
            v_shm_[6][row_len] = tamt_ao_add_tvol_precls_ratio[i];

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
        int row_len = row_length + sec_idx;
        int my_idx = my_sec_idx_[securityid];
        v_shm_[0][row_len] = order_num_[my_idx];
        v_shm_[1][row_len] = trade_num_[my_idx];
        v_shm_[2][row_len] = trade_high_price_vol_[my_idx]/ trade_vol_[my_idx];
        v_shm_[3][row_len] = unique_trade_high_price_[securityid].size() / unique_trade_price_[securityid].size();
        v_shm_[4][row_len] = trade_amt_[my_idx]/ trade_vol_[my_idx] / api_->getDailyData(pre_close_idx_, securityid) - 1;
        v_shm_[5][row_len] = ((trade_vol_B[my_idx]/(BidActiveOrderNum[my_idx]+1))-(trade_vol_S[my_idx]/(AskActiveOrderNum[my_idx]+1)))/trade_vol_[my_idx];
        v_shm_[6][row_len] = tamt_ao_add_tvol_precls_ratio[my_idx];

        this->reset(securityid);
    }

protected:
    ObjectPoolST<MyOrder> my_order_pool_;    // 委托对象池
    ObjectPoolST<MyTrade> my_trade_pool_;    // 成交对象池

    std::unordered_map<int, std::unordered_map<seqnum_t, MyOrder *>> ori_order_map;    // 分股票的原始订单委托序列
    std::unordered_map<int, std::unordered_map<seqnum_t, MyTrade *>> ori_trade_map;    // 分股票的原始订单成交序列
    // std::unordered_map<int, int> my_sec_idx_;    // 维护一个本spi处理的股票的下标序列
    std::vector<int> my_sec_idx_;    // 数组形式的本spi处理的股票的下标序列

    std::vector<double> order_num_;             // 累计委托笔数
    std::vector<double> trade_num_;             // 累计成交笔数

    std::unordered_map<int, std::set<double>> unique_trade_price_; // 价格档位 不reset
    std::unordered_map<int, std::set<double>> unique_trade_high_price_; // 高于阈值价格档位 不reset


    std::vector<double> BidActiveOrderNum; //主动买单委托单独立数量  不reset
    std::vector<double> AskActiveOrderNum; // 主动卖单委托单独立数量  不reset
    std::vector<double> AskActiveOrderVolume; // 主动卖单委托量  不reset
    std::vector<double> BidActiveOrderVolume; // 主动买单委托量  不reset
    std::vector<double> BidActiveOrderAmount; // 主动买单委托金额  不reset
    std::vector<double> AskActiveOrderAmount; // 主动卖单委托金额  不reset
    std::vector<double> trade_amt_B;  // 主动买入累计成交额  不reset
    std::vector<double> trade_amt_S;  // 主动卖出累计成交额  不reset
    std::vector<double> trade_vol_B;  //主动买入累计成交量  不reset
    std::vector<double> trade_vol_S;  // 主动卖出累计成交量  不reset


    std::vector<double> trade_vol_;  // 累计总成交量 不reset
    std::vector<double> trade_amt_;  // 累计总成交额 不reset

    std::vector<bool> first_reach_flag_; // 是否首次达到阈值价格 不reset
    std::vector<double> trade_high_price_vol_;  // 累计高位价格及以上成交量 不reset
    std::vector<double> tamt_ao_add_tvol_precls_ratio; // 成交额与(主动买挂量+成交量)之比相对昨收的偏离

    size_t col_size_;            // 列的长度，即本spi处理的股票总数
    size_t aligned_col_size_;    // 实际列长度，为SIMD对齐准备的股票个数

    int max_factor_ob_idx_ = 0;    // 当前最大的factor_ob_idx

    // 用户定义的静态数据字段索引
    size_t up_limit_price_idx_;  // 
    size_t pre_close_idx_;
    size_t neg_mkt_idx_;
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
    param.factor_names = std::vector<std::string>{"order_num", "trade_num","accum_prup_qty_ratio","accum_t_prupt_uni_ratio","vwap_pre_close_bias",
    "sundc_t_tran_16_fix","tamt_ao_add_tvol_precls_ratio"};
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