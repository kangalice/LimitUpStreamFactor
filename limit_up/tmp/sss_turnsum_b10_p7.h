//
// Created by appadmin on 2024/3/18.
//
#include <vector>

#ifndef EVENTDRIVENSTRATEGY_SSS_TURNSUM_B10_P7_H
#define EVENTDRIVENSTRATEGY_SSS_TURNSUM_B10_P7_H
namespace vendor {
namespace strategy {
namespace eventdriven {
class sss_turnsum_b10_p7 : public BaseFactor
{
private:
    int lastFlag = 0;
    uint64_t qty = 0;
    double price = 0;
    bool isAbove = false;
    uint64_t lastTradeNo = 0;
    int buyIndex = 0;
    std::vector<double> buyTurn{NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};

public:
    sss_turnsum_b10_p7(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        double zcz = marketDataManager->zcz ? 1 : 0;
        price = std::floor(marketDataManager->pre_close * 100 * (1.07 + 0.07 * zcz) + 0.5) / 100;
        this->factor_names = std::vector<std::string>{"sss_turnsum_b10_p7"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) override
    {
        if (isAbove || fill->price >= price)
        {
            if (lastFlag == 1)
            {
                uint64_t tradeNo = fill->side == 1 ? fill->buy_no : fill->sell_no;
                if (tradeNo == lastTradeNo)
                {
                    qty += fill->qty;
                }
                else
                {
                    buyTurn[buyIndex] = qty;
                    buyIndex = buyIndex != 9 ? buyIndex + 1 : 0;
                    lastTradeNo = tradeNo;
                    qty = fill->qty;
                }
            }
            else
            {
                qty = fill->qty;
                lastTradeNo = fill->side == 1 ? fill->buy_no : fill->sell_no;
            }
            lastFlag = fill->side;
            isAbove = true;
        }
    }

    void Calculate() override
    {
        std::vector<double> buyTurnCalc(buyTurn.begin(), buyTurn.end());
        if (marketDataManager->last_fill->side == 1)
        {
            buyTurnCalc[buyIndex] = qty;
        }
        double value = common_utils::MathUtil::calcSum(buyTurnCalc, false, true) * 100 / marketDataManager->free_float_capital;
        this->UpdateValue(0, std::isnan(value) ? 0 : value);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_SSS_TURNSUM_B10_P7_H
