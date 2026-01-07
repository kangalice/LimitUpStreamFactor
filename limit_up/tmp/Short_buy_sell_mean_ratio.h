#ifndef EVENTDRIVENSTRATEGY_SHORT_BUY_SELL_MEAN_RATIO_H
#define EVENTDRIVENSTRATEGY_SHORT_BUY_SELL_MEAN_RATIO_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class Short_buy_sell_mean_ratio : public BaseFactor {
            public:
                Short_buy_sell_mean_ratio(MarketDataManager *manager,
                                          std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(
                        manager, factor_map) {
                    this->factor_names = std::vector<std::string>{"Short_buy_sell_mean_ratio"};
                }

                void Calculate() override {
                    LocalTime ztTime = this->marketDataManager->GetZTLocalTime();
                    LocalTime last5StartTime = common_utils::TimeUtil::AddMilliseconds(ztTime, -5000);

                    std::unordered_map<uint64_t, MarketOrder> &lxjjTradeBuyMap = this->marketDataManager->lxjj_trade_buy_map;
                    std::unordered_map<uint64_t, MarketOrder> &lxjjTradeSellMap = this->marketDataManager->lxjj_trade_sell_map;
                    if (lxjjTradeBuyMap.empty() || lxjjTradeSellMap.empty()) {
                        this->UpdateValue(0, 0);
                        return;
                    }

                    uint64_t buySum = 0;
                    uint64_t buyCnt = 0;
                    std::set<uint64_t> buySet;
                    double sellSum = 0;
                    int sellCnt = 0;
                    std::set<uint64_t> sellSet;
                    std::vector<Fill *> &last_5sec_fill_list = this->marketDataManager->last_5sec_fill_list;
                    for (auto &fill: last_5sec_fill_list) {
                        uint64_t buyNo = fill->buy_no;
                        MarketOrder &buyOrder = lxjjTradeBuyMap.find(buyNo)->second;
                        if (buyOrder.first_fill_time >= last5StartTime && buySet.count(buyNo) == 0) {
                            buySum += buyOrder.qty;
                            buyCnt++;
                            buySet.emplace(buyNo);
                        }

                        uint64_t sellNo = fill->sell_no;
                        MarketOrder sellOrder = lxjjTradeSellMap.find(sellNo)->second;
                        if (sellOrder.first_fill_time >= last5StartTime && sellSet.count(sellNo) == 0) {
                            sellSum += sellOrder.qty;
                            sellCnt++;
                            sellSet.emplace(sellNo);
                        }
                    }

                    double val = buySum / (buyCnt * 1.0) / (sellSum / sellCnt);
                    if (!isnan(val) && val != 0) {
                        val = std::log(val);
                    } else {
                        val = 0;
                    }

                    this->UpdateValue(0, val);
                }
            };
        }
    }
}
#endif //EVENTDRIVENSTRATEGY_SHORT_BUY_SELL_MEAN_RATIO_H
