//
// Created by appadmin on 2023/1/16.
//

#ifndef EVENTDRIVENSTRATEGY_BASE_FACTOR_H
#define EVENTDRIVENSTRATEGY_BASE_FACTOR_H

#include "unordered_set"

#include "include/common/marketdata_manager.h"
#include "include/scaler/base_scaler.h"

#include "thirdparty/common_utils/include/big_dec.h"
#include "thirdparty/common_utils/include/math_util.h"
#include "thirdparty/common_utils/include/ordering.h"
#include "thirdparty/common_utils/include/simple_regression.h"
#include "thirdparty/common_utils/include/standard_deviation.h"
#include "thirdparty/common_utils/include/time_util.h"
#include "thirdparty/common_utils/include/topk.h"
#include <cfloat>

using namespace vendor::strategy::common_utils;

namespace vendor {
namespace strategy {
namespace eventdriven {
struct DataPair
{
    double left;
    double right;
};

class BaseFactor
{
private:
    std::unordered_map<std::string, double> *factor_value_map;
    std::unordered_set<BaseScaler *> scaler_set;
    std::string class_name = "BaseFactor";

protected:
    MarketDataManager *marketDataManager;
    std::vector<std::string> factor_names;
    std::vector<int> update_mode = {};

    void UpdateValue(int index, double value);

public:
    BaseFactor(MarketDataManager *manager, std::unordered_map<std::string, double> *factor_map);

    virtual void Update(const Trade *trade){};

    virtual void Update(const Fill *fill){};

    virtual void Update(const Cancel *cancelOrder){};

    virtual void Update(const TOrder *tOrder){};

    std::vector<std::string> *GetFactorNames();

    virtual void Calculate() = 0;

    const std::vector<int> &GetUpdateMode() const;

    void RegisterScaler(BaseScaler *scaler_ptr);

    void SetClassName(const std::string &name)
    {
        this->class_name = name;
    };

    std::string GetClassName() const
    {
        return this->class_name;
    };
};
} // namespace eventdriven
} // namespace strategy
} // namespace vendor
#endif // EVENTDRIVENSTRATEGY_BASE_FACTOR_H
