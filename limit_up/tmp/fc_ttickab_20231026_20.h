//
// Created by appadmin on 2024/1/24.
//

#ifndef EVENTDRIVEN_STRATEGY_CPP_BACKTEST_FC_TTICKAB_20231026_20_H
#define EVENTDRIVEN_STRATEGY_CPP_BACKTEST_FC_TTICKAB_20231026_20_H

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class fc_ttickab_20231026_20 : public BaseFactor {
private:
    std::map<uint64_t, double> minPrice;

public:
    fc_ttickab_20231026_20(MarketDataManager *manager,
                           std::unordered_map<std::string, double> *factor_map)
            : BaseFactor(manager, factor_map) {
        this->factor_names = std::vector<std::string>{"fc_ttickab_20231026_20"};
    }

    void Calculate() override {
        double preClose = marketDataManager->pre_close;
        bool flag = marketDataManager->starts_with3 || marketDataManager->starts_with68;

        LocalTime ztTime = marketDataManager->ZT_local_time;
        LocalTime startTime = 93000000;

        LocalTime tickEndTime = std::max(TimeUtil::AddMilliseconds(ztTime, -marketDataManager->params->tickTimeLag), startTime); //获取ZTTime - lagtime
        std::vector<Tick *> &tickList = this->marketDataManager->quote_list;

        for (int i = tickList.size() - 1; i >= 0; --i) {
            if (tickList[i]->md_time <= tickEndTime) {
                startTime = std::max(common_utils::TimeUtil::FunGetTime(tickList[i]->md_time, -600), startTime);
                tickEndTime = tickList[i]->md_time;
                break;
            }
        }

        std::map<int, std::pair<double, int>> group_res;
        for (int i = tickList.size() - 1; i >= 0; --i) {
            if (tickList[i]->md_time > startTime) {
                if (tickList[i]->md_time <= tickEndTime) {
                    double pct = tickList[i]->ask_price[4] / preClose - 1;
                    int sec10 = tickList[i]->md_time / 10000;
                    if (flag) {
                        pct /= 2;
                    }
                    if (group_res.find(sec10) != group_res.end()) {
                        group_res[sec10].first += pct;
                        ++group_res[sec10].second;
                    } else {
                        group_res[sec10].first = pct;
                        group_res[sec10].second = 1;
                    }
                }
            } else {
                break;
            }
        }

        std::vector<double> lxjj_ret_list;
        lxjj_ret_list.reserve(512);
        for (auto it = group_res.begin(); it != group_res.end(); ++it) {
            lxjj_ret_list.emplace_back(it->second.first / (double) it->second.second);
        }

        double mean = common_utils::MathUtil::calcMean(lxjj_ret_list);
        double factorValue = mean / (common_utils::MathUtil::calcStd(lxjj_ret_list, true, false, false, mean) + 0.001);
        factorValue = isnan(factorValue) ? 0 : factorValue;

        this->UpdateValue(0, factorValue);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor



#endif //EVENTDRIVEN_STRATEGY_CPP_BACKTEST_FC_TTICKAB_20231026_20_H
