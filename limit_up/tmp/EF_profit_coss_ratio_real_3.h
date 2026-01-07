//
// Created by appadmin on 2023/3/7.
//

#ifndef EVENTDRIVENSTRATEGY_EF_PROFIT_COSS_RATIO_REAL_3_H
#define EVENTDRIVENSTRATEGY_EF_PROFIT_COSS_RATIO_REAL_3_H

#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class EF_profit_coss_ratio_real_3 : public BaseFactor {
            private:
                std::vector<std::string> filter_shape4_stocks;
                std::vector<std::string> filter_shape2_stocks;

            public:
                EF_profit_coss_ratio_real_3(MarketDataManager *manager,
                                            std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(manager, factor_map) {
                    this->factor_names = std::vector < std::string > {"EF_profit_coss_ratio_real_3"};
                    auto &shape4_stocks = marketDataManager->params->filterShape4StockList;
                    for (int i = 0; i < shape4_stocks.size(); ++i) {
                        filter_shape4_stocks.emplace_back(shape4_stocks[i].symbol.substr(0, 6));
                    }

                    auto &shape2_stocks = marketDataManager->params->filterShape2StockList;
                    for (int i = 0; i < shape2_stocks.size(); ++i) {
                        filter_shape2_stocks.emplace_back(shape2_stocks[i].symbol.substr(0, 6));
                    }
                }

                void Calculate() override {
                    double shape4_last2_days_sum = marketDataManager->params->shape4Last2DaysSum;
                    int shape4_last_3days_count = marketDataManager->params->shape4Last3DaysCount;

                    for (auto &symbol: filter_shape4_stocks) {
                        double it = marketDataManager->GetOpenToPreHighRatio(symbol);
                        if (isnan(it)) {
                            --shape4_last_3days_count;
                        } else {
                            shape4_last2_days_sum += it;
                        }
                    }

                    double shape2_last_2days_sum = marketDataManager->params->shape2Last2DaysSum;
                    int shape2_last_3days_count = marketDataManager->params->shape2Last3DaysCount;
                    for (auto &symbol: filter_shape2_stocks) {
                        double it = marketDataManager->GetOpenToPreHighRatio(symbol);
                        if (isnan(it)) {
                            --shape2_last_3days_count;
                        } else {
                            shape2_last_2days_sum += it;
                        }
                    }

                    double factor = 0;
                    if (shape2_last_2days_sum != 0 && shape4_last_3days_count != 0) {
                        factor = -shape4_last2_days_sum * shape2_last_3days_count / shape4_last_3days_count /
                                 shape2_last_2days_sum;
                    }

                    this->UpdateValue(0, factor);
                }
            };
        } // namespace eventdriven
    } // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_EF_PROFIT_COSS_RATIO_REAL_3_H