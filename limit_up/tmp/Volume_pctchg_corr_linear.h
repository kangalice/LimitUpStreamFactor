//
// Created by zhangtian on 2023/2/20.
//

#pragma once

#include <climits>
#include <optional>
#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Volume_pctchg_corr_linear : public BaseFactor
{ // 突破前1分钟，卖单成交时长成交金额加权均值
public:
    Volume_pctchg_corr_linear(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
        , regression()
        , index(0)
        , totalQtyMultiRetSum(0)
    {
        this->factor_names =
            std::vector<std::string>{"Volume_pctchg_corr_linear", "Relative_total_strength", "Relative_total_strength_linear_0085"};
        this->update_mode = {1};
    }

    void Calculate() override
    {
        double volumePctchgCorrLinear = 1;
        double relativeTotalStrength = 0;
        double relativeTotalStrengthLinear = 5;

        const std::vector<Fill *> &fillList = marketDataManager->lxjj_fill_list;
        if (!fillList.empty())
        {
            double preClose = marketDataManager->pre_close;

            const std::vector<TimeInfo> &timeInfoList = marketDataManager->lxjj_time_info_list;
            double lastPrice = index == 0 ? marketDataManager->jhjj_price : timeInfoList[index - 1].price;
            double ret = (timeInfoList[index].price - lastPrice) / preClose;
            double qty = timeInfoList[index].qty;
            common_utils::SimpleRegression regressionNow;
            regressionNow.append(regression);
            regressionNow.addData(qty, ret);
            double totalQtyMultiRetSumNow = totalQtyMultiRetSum + qty * 100 * ret;
            volumePctchgCorrLinear = abs(0.2 - regressionNow.getR());
            relativeTotalStrength = totalQtyMultiRetSumNow / marketDataManager->total_qty;
            relativeTotalStrengthLinear = abs(0.085 - relativeTotalStrength);
        }

        UpdateValue(0, isnan(volumePctchgCorrLinear) != 0 ? 1 : volumePctchgCorrLinear);
        UpdateValue(1, isnan(relativeTotalStrength) != 0 ? 0 : relativeTotalStrength);
        UpdateValue(2, isnan(relativeTotalStrengthLinear) != 0 ? 5 : relativeTotalStrengthLinear);
    }

    void Update(const Fill *fill) override
    {
        const std::vector<TimeInfo> &timeInfoList = marketDataManager->lxjj_time_info_list;
        if (timeInfoList.size() > index + 1)
        {
            this->updateHelp();
            index++;
        }
    }

private:
    void updateHelp()
    {
        double preClose = marketDataManager->pre_close;
        const std::vector<TimeInfo> &timeInfo = marketDataManager->lxjj_time_info_list;
        double lastPrice = index == 0 ? marketDataManager->jhjj_price : timeInfo[index - 1].price;
        double ret = (timeInfo[index].price - lastPrice) / preClose;
        double qty = timeInfo[index].qty;
        regression.addData(qty, ret);
        totalQtyMultiRetSum += qty * 100 * ret;
    }

    int index;
    double totalQtyMultiRetSum;
    common_utils::SimpleRegression regression;
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor