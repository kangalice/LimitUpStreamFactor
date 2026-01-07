#pragma once

#include "include/factor/base_factor.h"

using namespace vendor::strategy::common_utils;

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class Yzhan_hf_ak4_40 : public BaseFactor {
            public:
                Yzhan_hf_ak4_40(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map) {
                    this->factor_names = std::vector < std::string > {"yzhan_hf_ak4_40"};
                }

                void Calculate() override {
                    auto &lxjj_fill_list = marketDataManager->lxjj_fill_list;
                    if (lxjj_fill_list.empty()) {
                        this->UpdateValue(0, 0);
                        return;
                    }

                    double trade_qty_sum = marketDataManager->lxjj_total_qty;
                    double trade_money_sum = marketDataManager->lxjj_total_amt;

                    SimpleRegression regression98;
                    SimpleRegression regression96;

                    for (int64_t i = lxjj_fill_list.size() - 1; i >= 0; --i) {
                        auto &fill = lxjj_fill_list[i];
                        if (fill->cum_trade_money / trade_money_sum > 0.98) {
                            regression98.addData(fill->qty / trade_qty_sum, fill->amt / trade_money_sum);
                            regression96.addData(fill->qty / trade_qty_sum, fill->amt / trade_money_sum);
                        } else if (fill->cum_trade_money / trade_money_sum > 0.96) {
                            regression96.addData(fill->qty / trade_qty_sum, fill->amt / trade_money_sum);
                        } else {
                            break;
                        }
                    }

                    double correlation98 = regression98.getR();
                    double laValue = std::isnan(correlation98) ? 0 : correlation98;

                    double correlation96 = regression96.getR();
                    double lbValue = std::isnan(correlation96) ? 0 : correlation96;

                    double factorValue = laValue - lbValue;
                    this->UpdateValue(0, std::isnan(factorValue) ? 0 : factorValue);
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor