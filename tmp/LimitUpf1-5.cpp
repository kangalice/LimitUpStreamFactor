// 此代码为factor_description_p1中前5个因子的计算代码
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
#include <set>
#include <vector>
#include <cmath>
#include <limits>
#include "utils/rounding.h"

/// @brief 默认日志目录的全局常量
const std::string DEFAULT_LOGS_DIR = "logs";

// 用户自定义结构体，用于存储成交和委托数据
struct MyTrade {
    seqnum_t applseqnum;
    price_t price;
    qty_t qty;
    double amt;
    uint64_t timestamp;
    int side;  // 1=买, 2=卖
    seqnum_t buy_no;
    seqnum_t sell_no;
};

struct MyOrder {
    seqnum_t applseqnum;
    price_t price;
    qty_t qty;
    uint64_t md_time;
};

struct MyTick {
    uint64_t md_time;
    double total_amount;
    uint64_t total_volume;
    uint64_t bid_order_qty;
    uint64_t ask_order_qty;
};

// 因子1的数据结构
struct Factor1Data {
    double sum = 0.0;
    double high_sum = 0.0;
    double threshold = 0.0;
};

// 因子2的数据结构
struct Factor2Data {
    std::set<double> trade_prices;
    std::set<double> high_trade_prices;
    std::set<double> tmp_trade_prices;
    bool first_reach = false;
    int64_t pre_md_time = -1;
    double threshold = 0.0;
};

// 因子3的数据结构
struct Factor3Data {
    int lastFlag = 0;  // 0=未知, 1=买, 2=卖
    uint64_t qty = 0;
    bool isAbove = false;
    uint64_t lastTradeNo = 0;
    int buyIndex = 0;
    std::vector<double> buyTurn;
    double threshold = 0.0;
    MyTrade* last_trade = nullptr;
};

// 因子4的数据结构
struct Factor4Data {
    std::vector<MyTick*> tick_list;
    uint64_t ZT_local_time = 0;
};

// 因子5的数据结构
struct Factor5Data {
    std::vector<MyOrder*> lxjj_order_list;
    std::vector<MyTrade*> fill_list;
    uint64_t ZT_local_time = 0;
};

class LimitUpP1SPI : public MatchEngineSPI {
public:
    /// @brief 初始化函数，会在setAPI之后被调用
    void Init() override {
        logi("[spi] handling code list: {}~{}", code_list_[0], code_list_[code_list_.size() - 1]);
        std::cout << "[spi] handling code list: " << code_list_[0] << "~" << code_list_[code_list_.size() - 1]
                  << std::endl;
        col_size_ = code_list_.size();

        my_sec_idx_.resize(700000, -1);

        for (int i = 0; i < col_size_; i++) {
            my_sec_idx_[code_list_[i]] = i;
            
            // 初始化因子数据结构
            factor1_data_[code_list_[i]] = Factor1Data();
            factor2_data_[code_list_[i]] = Factor2Data();
            factor3_data_[code_list_[i]] = Factor3Data();
            factor4_data_[code_list_[i]] = Factor4Data();
            factor5_data_[code_list_[i]] = Factor5Data();
            
            // 初始化因子3的buyTurn数组
            factor3_data_[code_list_[i]].buyTurn.resize(10, std::numeric_limits<double>::quiet_NaN());
        }

        // 初始化因子值向量
        factor1_values_.resize(col_size_, 0.0);
        factor2_values_.resize(col_size_, 0.0);
        factor3_values_.resize(col_size_, 0.0);
        factor4_values_.resize(col_size_, 0.0);
        factor5_values_.resize(col_size_, 0.0);

        // 通过字段名获取 idx, 名字如有"/"，这里传参直接使用"/"后面的字符串
        up_limit_price_idx_ = api_->getDailyDataIdx("up_limit_price");
        pre_close_idx_ = api_->getDailyDataIdx("act_pre_close_price");
        neg_mkt_idx_ = api_->getDailyDataIdx("neg_market_value");
        free_float_capital_idx_ = api_->getDailyDataIdx("free_float_capital");
        
        // 初始化每个股票的阈值
        for (int i = 0; i < col_size_; i++) {
            int securityid = code_list_[i];
            double pre_close = api_->getDailyData(pre_close_idx_, securityid);
            double up_limit_price = api_->getDailyData(up_limit_price_idx_, securityid);
            
            // 判断是否为注册制（创业板3开头或科创板68开头）
            bool zcz = (securityid >= 300000 && securityid < 400000) || 
                       (securityid >= 680000 && securityid < 690000);
            
            // 因子1和因子2的阈值
            if (zcz) {
                factor1_data_[securityid].threshold = utils::round_half_up(pre_close * 1.12, 4);
                factor2_data_[securityid].threshold = utils::round_half_up(pre_close * 1.12, 4);
            } else {
                factor1_data_[securityid].threshold = utils::round_half_up(pre_close * 1.06, 4);
                factor2_data_[securityid].threshold = utils::round_half_up(pre_close * 1.06, 4);
            }
            
            // 因子3的阈值
            double zcz_val = zcz ? 1.0 : 0.0;
            factor3_data_[securityid].threshold = std::floor(pre_close * 100 * (1.07 + 0.07 * zcz_val) + 0.5) / 100.0;
        }
    }

    /// 状态变量更新函数
    void reset() {
        std::fill(factor1_values_.begin(), factor1_values_.end(), 0.0);
        std::fill(factor2_values_.begin(), factor2_values_.end(), 0.0);
        std::fill(factor3_values_.begin(), factor3_values_.end(), 0.0);
        std::fill(factor4_values_.begin(), factor4_values_.end(), 0.0);
        std::fill(factor5_values_.begin(), factor5_values_.end(), 0.0);
        
        // 重置因子1和因子2的累加变量
        for (auto& pair : factor1_data_) {
            pair.second.sum = 0.0;
            pair.second.high_sum = 0.0;
        }
        for (auto& pair : factor2_data_) {
            pair.second.trade_prices.clear();
            pair.second.high_trade_prices.clear();
            pair.second.tmp_trade_prices.clear();
            pair.second.first_reach = false;
            pair.second.pre_md_time = -1;
        }
        for (auto& pair : factor3_data_) {
            pair.second.lastFlag = 0;
            pair.second.qty = 0;
            pair.second.isAbove = false;
            pair.second.lastTradeNo = 0;
            pair.second.buyIndex = 0;
            std::fill(pair.second.buyTurn.begin(), pair.second.buyTurn.end(), 
                     std::numeric_limits<double>::quiet_NaN());
            pair.second.last_trade = nullptr;
        }
    }

    void reset(int securityid) {
        int idx = my_sec_idx_[securityid];
        if (idx == -1) return;
        
        factor1_values_[idx] = 0.0;
        factor2_values_[idx] = 0.0;
        factor3_values_[idx] = 0.0;
        factor4_values_[idx] = 0.0;
        factor5_values_[idx] = 0.0;
        
        if (factor1_data_.find(securityid) != factor1_data_.end()) {
            factor1_data_[securityid].sum = 0.0;
            factor1_data_[securityid].high_sum = 0.0;
        }
        if (factor2_data_.find(securityid) != factor2_data_.end()) {
            factor2_data_[securityid].trade_prices.clear();
            factor2_data_[securityid].high_trade_prices.clear();
            factor2_data_[securityid].tmp_trade_prices.clear();
            factor2_data_[securityid].first_reach = false;
            factor2_data_[securityid].pre_md_time = -1;
        }
        if (factor3_data_.find(securityid) != factor3_data_.end()) {
            factor3_data_[securityid].lastFlag = 0;
            factor3_data_[securityid].qty = 0;
            factor3_data_[securityid].isAbove = false;
            factor3_data_[securityid].lastTradeNo = 0;
            factor3_data_[securityid].buyIndex = 0;
            std::fill(factor3_data_[securityid].buyTurn.begin(), 
                     factor3_data_[securityid].buyTurn.end(), 
                     std::numeric_limits<double>::quiet_NaN());
            factor3_data_[securityid].last_trade = nullptr;
        }
    }

    /// @brief 在当笔盘口更新前，接收逐笔委托
    /// @param order 委托数据
    void onBeforeAddOrder(const UnifiedRecord *order) override {
        if (my_sec_idx_[order->securityid] == -1) {
            return;
        }
        
        // 因子5：维护连续竞价委托列表
        if (order->type != Type::Cancel && order->parse_time >= 93000000) {
            auto& order_list = factor5_data_[order->securityid].lxjj_order_list;
            MyOrder* my_order = new MyOrder{order->applseqnum, order->price, order->qty, 
                                           static_cast<uint64_t>(order->parse_time)};
            order_list.push_back(my_order);
        }
    }

    /// @brief 在当笔盘口更新前，接收逐笔成交
    /// @param trade 成交数据
    void onBeforeAddTrade(const UnifiedRecord *trade) override {
        if (my_sec_idx_[trade->securityid] == -1) {
            return;
        }
        
        int idx = my_sec_idx_[trade->securityid];
        int securityid = trade->securityid;
        
        // 计算成交金额
        double amt = static_cast<double>(trade->price) * static_cast<double>(trade->qty);
        
        // 创建MyTrade对象
        MyTrade* my_trade = new MyTrade{
            trade->applseqnum,
            trade->price,
            trade->qty,
            amt,
            static_cast<uint64_t>(trade->parse_time),
            (trade->side == Side::Buy) ? 1 : 2,
            trade->buyno,
            trade->sellno
        };
        
        // 因子1：xly_t_prup_qty_ratio - 涨停价及以上成交量占比
        auto& f1 = factor1_data_[securityid];
        f1.sum += trade->qty;
        if (trade->price >= f1.threshold) {
            f1.high_sum += trade->qty;
        }
        
        // 因子2：xly_t_prupt_uni_ratio - 涨停触发价位上独立价档占比
        auto& f2 = factor2_data_[securityid];
        double price_rounded = utils::round_half_up(trade->price, 4);
        f2.trade_prices.emplace(price_rounded);
        
        if (!f2.first_reach) {
            if (my_trade->timestamp > static_cast<uint64_t>(f2.pre_md_time)) {
                f2.tmp_trade_prices.clear();
                f2.tmp_trade_prices.emplace(price_rounded);
            } else {
                f2.tmp_trade_prices.emplace(price_rounded);
            }
            
            if (price_rounded >= f2.threshold) {
                f2.first_reach = true;
                f2.high_trade_prices.insert(f2.tmp_trade_prices.begin(), f2.tmp_trade_prices.end());
            }
            f2.pre_md_time = my_trade->timestamp;
        } else {
            if (price_rounded >= f2.threshold) {
                f2.high_trade_prices.emplace(price_rounded);
            }
        }
        
        // 因子3：sss_turnsum_b10_p7 - 突破阈值后的买单"冲击量"近10段合计/流通股本
        auto& f3 = factor3_data_[securityid];
        if (f3.isAbove || trade->price >= f3.threshold) {
            f3.isAbove = true;
            if (f3.lastFlag == 1) {  // 上一笔是买单
                uint64_t tradeNo = (my_trade->side == 1) ? my_trade->buy_no : my_trade->sell_no;
                if (tradeNo == f3.lastTradeNo) {
                    f3.qty += trade->qty;
                } else {
                    f3.buyTurn[f3.buyIndex] = static_cast<double>(f3.qty);
                    f3.buyIndex = (f3.buyIndex != 9) ? f3.buyIndex + 1 : 0;
                    f3.lastTradeNo = tradeNo;
                    f3.qty = trade->qty;
                }
            } else {
                f3.qty = trade->qty;
                f3.lastTradeNo = (my_trade->side == 1) ? my_trade->buy_no : my_trade->sell_no;
            }
            f3.lastFlag = my_trade->side;
        }
        f3.last_trade = my_trade;
        
        // 因子5：维护成交列表
        factor5_data_[securityid].fill_list.push_back(my_trade);
        
        // 因子4和因子5：更新ZT_local_time（当价格达到涨停价时）
        double up_limit_price = api_->getDailyData(up_limit_price_idx_, securityid);
        if (trade->price >= up_limit_price && factor4_data_[securityid].ZT_local_time == 0) {
            factor4_data_[securityid].ZT_local_time = my_trade->timestamp;
            factor5_data_[securityid].ZT_local_time = my_trade->timestamp;
        }
    }

    /// @brief 在当笔盘口更新后，接收逐笔委托
    /// @param order 委托数据
    void onAfterAddOrder(const UnifiedRecord *order) override {
        // 因子4：从3sKline获取tick数据（在onFactorOB中计算）
    }

    /// @brief 在当笔盘口更新后，接收逐笔成交
    /// @param trade 成交数据
    void onAfterAddTrade(const UnifiedRecord *trade) override {
        // 因子4：从3sKline获取tick数据（在onFactorOB中计算）
    }

    /// @brief 计算因子值
    void calculateFactors(int securityid) {
        int idx = my_sec_idx_[securityid];
        if (idx == -1) return;
        
        // 因子1：xly_t_prup_qty_ratio
        auto& f1 = factor1_data_[securityid];
        if (f1.sum != 0) {
            factor1_values_[idx] = f1.high_sum / f1.sum;
        } else {
            factor1_values_[idx] = 0.0;
        }
        
        // 因子2：xly_t_prupt_uni_ratio
        auto& f2 = factor2_data_[securityid];
        if (!f2.trade_prices.empty()) {
            factor2_values_[idx] = static_cast<double>(f2.high_trade_prices.size()) / 
                                  static_cast<double>(f2.trade_prices.size());
        } else {
            factor2_values_[idx] = 0.0;
        }
        
        // 因子3：sss_turnsum_b10_p7
        auto& f3 = factor3_data_[securityid];
        std::vector<double> buyTurnCalc(f3.buyTurn.begin(), f3.buyTurn.end());
        if (f3.last_trade && f3.last_trade->side == 1) {
            buyTurnCalc[f3.buyIndex] = static_cast<double>(f3.qty);
        }
        
        double sum = 0.0;
        for (double val : buyTurnCalc) {
            if (!std::isnan(val)) {
                sum += val;
            }
        }
        
        double free_float_capital = api_->getDailyData(free_float_capital_idx_, securityid);
        if (free_float_capital > 0) {
            factor3_values_[idx] = sum * 100.0 / free_float_capital;
            if (std::isnan(factor3_values_[idx]) || std::isinf(factor3_values_[idx])) {
                factor3_values_[idx] = 0.0;
            }
        } else {
            factor3_values_[idx] = 0.0;
        }
        
        // 因子4：zwh_20230831_005 - 成交额与(买挂量+成交量)之比相对昨收的偏离
        // 注意：在MatchEngineSPI架构中，我们无法直接访问quote_list和tickTimeLag参数
        // 这里使用3sKline数据来近似计算，使用ZT_local_time作为时间基准
        auto& f4 = factor4_data_[securityid];
        double pre_close = api_->getDailyData(pre_close_idx_, securityid);
        bool zcz = (securityid >= 300000 && securityid < 400000) || 
                   (securityid >= 680000 && securityid < 690000);
        
        // 尝试从3sKline获取最新数据
        // 使用最新的trade记录来获取3sKline数据
        // 注意：由于无法获取tickTimeLag，我们使用ZT_local_time作为时间基准
        if (f4.ZT_local_time > 0 && f4.ZT_local_time >= 93000000) {
            UnifiedRecord dummy_record;
            dummy_record.securityid = securityid;
            dummy_record.parse_time = static_cast<int32_t>(f4.ZT_local_time);
            dummy_record.market = (securityid >= 600000) ? Exchange::SH : Exchange::SZ;
            
            // 从3sKline获取数据，字段名参考MarketCols
            // 尝试获取时间点之前的数据（通过时间查询）
            int end_time = static_cast<int>(f4.ZT_local_time);
            double turnover = api_->get3sKlineByTime(&dummy_record, "turnover", end_time);  // 成交金额
            double tradv = api_->get3sKlineByTime(&dummy_record, "tradv", end_time);  // 成交量
            double totalbidvol = api_->get3sKlineByTime(&dummy_record, "totalbidvol", end_time);  // 买盘挂单总量
            
            // 如果按时间查询失败，尝试使用最新值
            if (std::isnan(turnover) || std::isnan(tradv) || std::isnan(totalbidvol)) {
                turnover = api_->get3sKlineLatest(&dummy_record, "turnover");
                tradv = api_->get3sKlineLatest(&dummy_record, "tradv");
                totalbidvol = api_->get3sKlineLatest(&dummy_record, "totalbidvol");
            }
            
            if (!std::isnan(turnover) && !std::isnan(tradv) && !std::isnan(totalbidvol) &&
                pre_close > 0 && (totalbidvol + tradv) > 0) {
                double res = turnover / (totalbidvol + tradv);
                factor4_values_[idx] = res / pre_close - 1.0;
                if (zcz) {
                    factor4_values_[idx] = factor4_values_[idx] / 2.0;
                }
            } else {
                factor4_values_[idx] = 0.0;
            }
        } else {
            factor4_values_[idx] = 0.0;
        }
        
        // 因子5：sss_to_pricediffstd_all_s60 - 近60秒委托价相对成交价的加权标准差
        auto& f5 = factor5_data_[securityid];
        if (f5.ZT_local_time == 0) {
            factor5_values_[idx] = 0.0;
            return;
        }
        
        // 计算时间阈值
        int64_t time_threshold = std::max(static_cast<int64_t>(f5.ZT_local_time) - 60000, 93000000LL);
        
        double pre_close_f5 = api_->getDailyData(pre_close_idx_, securityid);
        bool zcz_f5 = (securityid >= 300000 && securityid < 400000) || 
                      (securityid >= 680000 && securityid < 690000);
        
        uint64_t curr_timestamp = std::numeric_limits<uint64_t>::max();
        double trade_price = 0.0;
        int64_t order_qty_sum = 0;
        double pricediff_times_order_qty_sum = 0.0;
        std::vector<double> pricediff_list;
        std::vector<uint64_t> order_qty_list;
        
        // 从后向前遍历委托列表
        int j = static_cast<int>(f5.fill_list.size()) - 1;
        for (int i = static_cast<int>(f5.lxjj_order_list.size()) - 1; i >= 0; --i) {
            MyOrder* order = f5.lxjj_order_list[i];
            if (order->md_time < static_cast<uint64_t>(time_threshold)) {
                break;
            }
            
            if (order->md_time <= curr_timestamp) {
                // 需要重新计算成交价
                while (j >= 0 && f5.fill_list[j]->timestamp >= order->md_time) {
                    --j;
                }
                if (j < 0) {
                    trade_price = pre_close_f5;
                } else {
                    uint64_t trade_qty_sum = 0;
                    double trade_money_sum = 0.0;
                    curr_timestamp = f5.fill_list[j]->timestamp;
                    while (j >= 0 && f5.fill_list[j]->timestamp == curr_timestamp) {
                        trade_qty_sum += f5.fill_list[j]->qty;
                        trade_money_sum += f5.fill_list[j]->amt;
                        --j;
                    }
                    if (trade_qty_sum > 0) {
                        trade_price = trade_money_sum / static_cast<double>(trade_qty_sum);
                    } else {
                        trade_price = pre_close_f5;
                    }
                }
            }
            
            // 计算价差（百分比）
            double pricediff = (order->price - trade_price) / pre_close_f5 * 100.0;
            if (zcz_f5) {
                pricediff = pricediff / 2.0;
            }
            pricediff_list.push_back(pricediff);
            order_qty_list.push_back(order->qty);
            pricediff_times_order_qty_sum += pricediff * static_cast<double>(order->qty);
            order_qty_sum += order->qty;
        }
        
        if (order_qty_sum == 0) {
            factor5_values_[idx] = 0.0;
            return;
        }
        
        // 计算加权平均价差
        double vwap = pricediff_times_order_qty_sum / static_cast<double>(order_qty_sum);
        
        // 计算加权方差
        double numerator = 0.0;
        for (size_t k = 0; k < pricediff_list.size(); ++k) {
            double diff = pricediff_list[k] - vwap;
            numerator += diff * diff * static_cast<double>(order_qty_list[k]);
        }
        
        // 计算标准差
        double value = std::sqrt(numerator / static_cast<double>(order_qty_sum));
        if (std::isinf(value) || std::isnan(value)) {
            factor5_values_[idx] = 0.0;
        } else {
            factor5_values_[idx] = value;
        }
    }

    /// @brief 将本对象所维护的所有股票的因子值输出到共享内存中
    /// @param factor_ob_idx 因子ob的次数索引，也等于其在共享内存中应写入的行号
    /// @param row_length 当前行数据起始点
    void onFactorOB(int factor_ob_idx, int row_length) override {
        std::cout << "[factor ob " << process_id_ << "] ob idx: " << factor_ob_idx << std::endl;
        logi("[factor ob {}] ob idx: {}", process_id_, factor_ob_idx);
        int sec_idx;
        
        // 先计算所有股票的因子值
        for (int i = 0; i < col_size_; i++) {
            calculateFactors(code_list_[i]);
        }
        
        // 按在Params.factor_names中确定好的因子顺序，逐股票写出因子值
        for (int i = 0; i < col_size_; i++) {
            sec_idx = sec_idx_vec_[code_list_[i]];
            v_shm_[0][row_length + sec_idx] = factor1_values_[i];
            v_shm_[1][row_length + sec_idx] = factor2_values_[i];
            v_shm_[2][row_length + sec_idx] = factor3_values_[i];
            v_shm_[3][row_length + sec_idx] = factor4_values_[i];
            v_shm_[4][row_length + sec_idx] = factor5_values_[i];
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
        
        // 计算该股票的因子值
        calculateFactors(securityid);
        
        int idx = my_sec_idx_[securityid];
        v_shm_[0][row_length + sec_idx] = factor1_values_[idx];
        v_shm_[1][row_length + sec_idx] = factor2_values_[idx];
        v_shm_[2][row_length + sec_idx] = factor3_values_[idx];
        v_shm_[3][row_length + sec_idx] = factor4_values_[idx];
        v_shm_[4][row_length + sec_idx] = factor5_values_[idx];
        
        this->reset(securityid);
    }

protected:
    std::vector<int> my_sec_idx_;    // 数组形式的本spi处理的股票的下标序列
    
    // 因子数据结构
    std::unordered_map<int, Factor1Data> factor1_data_;
    std::unordered_map<int, Factor2Data> factor2_data_;
    std::unordered_map<int, Factor3Data> factor3_data_;
    std::unordered_map<int, Factor4Data> factor4_data_;
    std::unordered_map<int, Factor5Data> factor5_data_;
    
    // 因子值向量
    std::vector<double> factor1_values_;  // xly_t_prup_qty_ratio
    std::vector<double> factor2_values_;  // xly_t_prupt_uni_ratio
    std::vector<double> factor3_values_;  // sss_turnsum_b10_p7
    std::vector<double> factor4_values_;  // zwh_20230831_005
    std::vector<double> factor5_values_;  // sss_to_pricediffstd_all_s60
    
    size_t col_size_;            // 列的长度，即本spi处理的股票总数
    int max_factor_ob_idx_ = 0;    // 当前最大的factor_ob_idx

    // 用户定义的静态数据字段索引
    size_t up_limit_price_idx_;
    size_t pre_close_idx_;
    size_t neg_mkt_idx_;
    size_t free_float_capital_idx_;
};

int main(int argc, char *argv[]) {
    // 接收处理命令行参数
    cxxopts::Options options(argv[0],
                             "LimitUp Factor P1 Generator\n"
                             "此程序用于生成打板因子数据（前5个因子）。\n\n"
                             "用法示例：\n"
                             "  ./limitupf1-5 -d 2023-01-01 -i 0 -p 3\n"
                             "  ./limitupf1-5 2023-01-01 0 3\n"
                             "  ./limitupf1-5 \n");
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
    auto match_spi = new LimitUpP1SPI();
    match_api->registerSPI(match_spi);

    // 设置参数，参数含义见MatchParam声明
    MatchParam param;
    param.factor_names = std::vector<std::string>{
        "xly_t_prup_qty_ratio",
        "xly_t_prupt_uni_ratio",
        "sss_turnsum_b10_p7",
        "zwh_20230831_005",
        "sss_to_pricediffstd_all_s60"
    };
    param.incre_port = incre_port;
    param.process_num = process_num;
    param.skip_unlink = skip_unlink;
    param.log_level = 1;
    param.check_error = check_error;
    param.factor_interver_ms = 60000;
    param.bind_cpu_start_id = -1;

    // 使用 "hdfdata" 模式读取静态数据
    param.offline_mode = "hdfdata";
    param.data_keys = {
        "basic/up_limit_price",
        "basic/act_pre_close_price",
        "basic/neg_market_value",
        "basic/free_float_capital"
    };

    match_api->setParam(param);

    // 启动撮合，进程会阻塞在该函数
    match_api->startMatch(use_date);

    // 用户可调用saveData函数来保存数据
    match_api->saveData(param.factor_names, "LimitUpP1", "parquet");

    // 结束后建议手动调用close函数
    match_api->close();
    return 0;
}
