//
// Created by appadmin on 2024/1/24.
//

#ifndef EVENTDRIVEN_STRATEGY_CPP_BACKTEST_WJ_TTICK_20_BSV4_H
#define EVENTDRIVEN_STRATEGY_CPP_BACKTEST_WJ_TTICK_20_BSV4_H

#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class Wj_TTick_20_bsv4 : public BaseFactor {

            public:
                Wj_TTick_20_bsv4(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map) {
                    this->factor_names = std::vector < std::string > {"wj_TTick_20_bsv4"};
                }

                void Calculate() override {
                    const auto &quote_list = marketDataManager->quote_list;
                    int start_index = 0;
                    for (; start_index < quote_list.size(); ++start_index) {
                        if (quote_list[start_index]->md_time >= 9'30'00'000) {
                            break;
                        }
                    }
                    auto trunc_time = common_utils::TimeUtil::AddMilliseconds(marketDataManager->ZT_local_time, -marketDataManager->params->tickTimeLag);
                    auto trunc_size = quote_list.size(); // 向前截取的之后的quote_list的size
                    for (; trunc_size > 0; --trunc_size) {
                        if (quote_list[trunc_size - 1]->md_time <= trunc_time) {
                            break;
                        }
                    }

                    if (trunc_size <= start_index) {
                        this->UpdateValue(0, 0);
                        return;
                    }

                    auto start_time = marketDataManager->quote_list[trunc_size - 1]->md_time;
                    if (start_time <= 9'31'00'000) {
                        this->UpdateValue(0, 0);
                        return;
                    }
                    auto end_time = start_time;
                    int sec_delta = 0;
                    if (start_time <= 9'35'00'000) {
                        sec_delta = 40;
                    } else if (start_time <= 10'00'00'000) {
                        sec_delta = 240;
                    } else {
                        sec_delta = 1200;
                    }
                    start_time = std::max(common_utils::TimeUtil::FunGetTime(end_time, -sec_delta), (LocalTime)93000000);
                    std::vector<double> ret_pct_o;
                    ret_pct_o.reserve(trunc_size);
                    double ret_pct_o_sum = 0;
                    int ret_pct_o_cnt = 0;
                    for (int i = start_index; i < trunc_size; ++i) {
                        auto *tick = quote_list[i];
                        if (tick->md_time <= start_time) {
                            continue;
                        }
                        ret_pct_o.push_back((double)(tick->ask_order_qty - tick->total_volume + 1) / (double)(tick->total_volume + 1));
                        ret_pct_o_sum += ret_pct_o.back();
                        ++ret_pct_o_cnt;
                    }
                    auto mean = ret_pct_o_sum / (double)ret_pct_o_cnt;
                    auto std = common_utils::MathUtil::calcStd(ret_pct_o, true, false, true, mean);
                    auto value = (mean + 1e-4) / (std + 1e-4);
                    this->UpdateValue(0, (std::isnan(value) || std::isinf(value)) ? 0 : value);
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor

#endif //EVENTDRIVEN_STRATEGY_CPP_BACKTEST_WJ_TTICK_20_BSV4_H
