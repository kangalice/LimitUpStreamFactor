
#ifndef EVENTDRIVENSTRATEGY_SSS_1S_VOLSBO_NUM_BSDIFF_H
#define EVENTDRIVENSTRATEGY_SSS_1S_VOLSBO_NUM_BSDIFF_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
namespace strategy {
namespace eventdriven {
class Sss_1s_volsbo_num_bsdiff : public BaseFactor
{
    struct md_time_struct
    {
        double volume = 0;
        double amount = 0;
        double actbuy_vol = 0;
        double actbuy_ratio = 0;
    };
private:
    std::map<int, md_time_struct> mdtime_map;

public:
    Sss_1s_volsbo_num_bsdiff(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map)
        : BaseFactor(manager, factor_map)
    {
        this->factor_names = std::vector<std::string>{"sss_1s_volsbo_num_bsdiff"};
        this->update_mode = {1};
    }

    void Update(const Fill *fill) {
        int md_min = fill->timestamp / 1000;
        auto ptr1 = mdtime_map.find(md_min);
        if (ptr1 != mdtime_map.end())
        {
            ptr1->second.volume += fill->qty;
            ptr1->second.amount += fill->amt;
            if (fill->buy_no > fill->sell_no)
            {
                ptr1->second.actbuy_vol += fill->qty * 1.0;
            }
        }
        else
        {
            struct md_time_struct tmp;
            tmp.volume = fill->qty;
            tmp.amount = fill->amt;
            tmp.actbuy_ratio = 0;

            if (fill->buy_no > fill->sell_no)
            {
                tmp.actbuy_vol = fill->qty * 1.0;
            }
            else
            {
                tmp.actbuy_vol = 0;
            }
            mdtime_map.insert(std::make_pair(md_min, tmp));
        }
    }



    void Calculate() override
    {
        double factorVal = 0;
        auto &trade_list = this->marketDataManager->lxjj_fill_list;
        // tuple : 1.volume 2.amount 3.actbuy_vol

        if (mdtime_map.size() > 2)
        {
            std::vector<double> value;
            std::vector<double> act_buy_ratio;

            auto it = mdtime_map.begin();
            while (it != mdtime_map.end())
            {
                value.emplace_back(common_utils::MathUtil::RoundHalfUp(it->second.volume, 2));
                it->second.actbuy_ratio = common_utils::MathUtil::RoundHalfUp(it->second.actbuy_vol / it->second.volume, 4);
                it++;
            }

            double value_mean = common_utils::MathUtil::calcMean(value);
            double value_std = common_utils::MathUtil::calcNaNStd(value, true);
            double thred = common_utils::MathUtil::RoundHalfUp(value_mean + value_std, 4);

            auto ptr1 = mdtime_map.begin();
            int prev_above = NAN;
            int cum_count = 0;
            std::map<int, md_time_struct> cumsum_map;

            while (ptr1 != mdtime_map.end())
            {
                if (ptr1->second.volume >= thred)
                {
                    if (prev_above != 1)
                    {
                        cum_count++;
                        prev_above = 1;
                        struct md_time_struct tmp;
                        tmp.amount = ptr1->second.amount;
                        tmp.volume = ptr1->second.volume;
                        tmp.actbuy_vol = ptr1->second.actbuy_vol;
                        cumsum_map.emplace(cum_count, tmp);
                    }
                    else
                    {
                        auto cur_struct = &(cumsum_map.find(cum_count)->second);
                        cur_struct->volume += ptr1->second.volume;
                        cur_struct->amount += ptr1->second.amount;
                        cur_struct->actbuy_vol += ptr1->second.actbuy_vol;
                    }
                }
                else
                {
                    prev_above = 0;
                }

                ptr1++;
            }

            double above_buy_counter = 0;
            double above_sell_counter = 0;
            size_t md_min_size = mdtime_map.size();

            for (auto &it : cumsum_map)
            {
                auto ratio = it.second.actbuy_vol / it.second.volume;
                if (ratio > 0.5)
                {
                    ++above_buy_counter;
                }
                else if (ratio < 0.5)
                {
                    ++above_sell_counter;
                }
            }
            factorVal = above_buy_counter / md_min_size - above_sell_counter / md_min_size;
        }

        this->UpdateValue(0, isnan(factorVal) ? 0 : factorVal);
    }
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_SSS_1S_VOLSBO_NUM_BSDIFF_H