
#ifndef EVENTDRIVENSTRATEGY_SSS_AFTER_PBTIMEMAX_7_H
#define EVENTDRIVENSTRATEGY_SSS_AFTER_PBTIMEMAX_7_H

#pragma once

#include "include/factor/base_factor.h"

namespace vendor {
    namespace strategy {
        namespace eventdriven {
            class Sss_after_pbtimemax_7 : public BaseFactor {
            private:
                int zcz;
                double linePrice;
                bool isAbove = false;
                uint64_t lastOpenTime = 0;
                uint64_t lastCloseTime = 0;
                double lastFillPrice = NAN;
                int64_t maxMs = 0;
            public:
                Sss_after_pbtimemax_7(MarketDataManager *manager,
                                         std::unordered_map<std::string, double> *factor_map)
                        : BaseFactor(
                        manager, factor_map) {
                    this->factor_names = std::vector<std::string>{"sss_after_pbtimemax_7"};
                    zcz = marketDataManager->starts_with3 || marketDataManager->starts_with68 ? 1 : 0;
                    linePrice = std::floor(marketDataManager->pre_close * 100 * 
                                                                (1 + 0.01 * 7 * (1 + zcz)) + 0.5) / 100.0;
                    this->update_mode = {1};
                }

                void Update(const Fill *fill) override {
                    if (isAbove) {
                        if (lastFillPrice >= linePrice && fill->price < linePrice) { // open
                            if (lastOpenTime != 0) {
                                maxMs = std::max(maxMs, common_utils::TimeUtil::CalTimeDelta(lastOpenTime, lastCloseTime));
                            }
                            lastOpenTime = fill->timestamp;
                        } else if (lastFillPrice < linePrice && fill->price >= linePrice) { // close
                            lastCloseTime = fill->timestamp;
                        }
                        lastFillPrice = fill->price;
                    }
                    if (!isAbove && fill->price >= linePrice) {
                        isAbove = true;
                        lastFillPrice = fill->price;
                    }
                }

                void Calculate() override {
                    if (lastFillPrice < linePrice && marketDataManager->last_fill->price >= linePrice) { // close
                        lastCloseTime = marketDataManager->last_fill->timestamp;
                    }
                    if (lastOpenTime != 0) {
                        maxMs = std::max(maxMs, common_utils::TimeUtil::CalTimeDelta(lastOpenTime, lastCloseTime));
                    }                    
                    this->UpdateValue(0, std::isnan(maxMs) ? 0 : maxMs);
                }
            };
        }
    }
}
#endif //EVENTDRIVENSTRATEGY_SSS_AFTER_PBTIMEMAX_7_H
    
