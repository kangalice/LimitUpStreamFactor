#pragma once

#include "MatchEngineAPIStruct.hpp"

class MatchEngine;

class MatchEngineSPI {
public:
    virtual ~MatchEngineSPI() = default;

    /// @brief 初始化函数，会在setAPI之后被调用
    virtual void Init() {}

    /// @brief 在当笔盘口更新前，接收逐笔委托
    /// @param order 委托数据
    virtual void onBeforeAddOrder(const UnifiedRecord *order) {}

    /// @brief 在当笔盘口更新前，接收逐笔成交
    /// @param trade 成交数据
    virtual void onBeforeAddTrade(const UnifiedRecord *trade) {}

    /// @brief 在当笔盘口更新后，接收逐笔委托
    /// @param order 委托数据
    virtual void onAfterAddOrder(const UnifiedRecord *order) {}

    /// @brief 在当笔盘口更新后，接收逐笔成交
    /// @param trade 成交数据
    virtual void onAfterAddTrade(const UnifiedRecord *trade) {}

    /// @brief 将本对象所维护的所有股票的因子值输出到共享内存中，该函数必须实现
    /// @param factor_ob_idx 因子ob的次数索引，也等于其在共享内存中应写入的行号
    /// @param row_length 当前行数据起始点
    virtual void onFactorOB(int factor_ob_idx, int row_length) {}

    /// @brief 将函数指定的股票的因子值输出到共享内存中，该函数必须实现
    /// @param factor_ob_idx 因子ob的次数索引，也等于其在共享内存中应写入的行号
    /// @param row_length 当前行数据起始点
    /// @param securityid 写出数据的股票代码
    virtual void onFactorOB(int factor_ob_idx, int row_length, int securityid) {}

    /// @brief API注入函数，用户无需关心
    /// @param api 
    void setAPI(
        MatchEngine* api, 
        std::vector<double *> v_shm, 
        std::unordered_map<int, int> sec_idx_dict,
        std::vector<int> code_list,
        int process_id
        ) {
        api_ = api;
        v_shm_ = v_shm;
        sec_idx_dict_ = sec_idx_dict;
        code_list_ = code_list;
        process_id_ = process_id;
    }

protected:
    MatchEngine *api_ = nullptr;                    // 支持在回调函数中调用API的接口函数，具体见MatchEngine函数定义
    std::vector<double *> v_shm_;                   // 按照API中setParam函数输入的factor_names顺序排列的共享内存指针
    std::unordered_map<int, int> sec_idx_dict_;     // 股票列索引字典，为当天所有股票对应的共享内存列索引
    std::vector<int> code_list_;                    // 该个spi需要处理的股票代码
    int process_id_;                                // 进程id
};


class MatchEngineAPI {
public:
    /// @brief 创建API
    /// @return 创建出的API接口对象
    static MatchEngineAPI *createMatchAPI();
    
    /// @brief 注册回调接口
    /// @param spi 回调接口指针
    virtual void registerSPI(MatchEngineSPI *spi) = 0;

    /// @brief 设置参数，具体参数设置请查看结构体定义
    /// @param param 
    virtual void setParam(MatchParam param) = 0;

    /// @brief 开始匹配
    /// @param date 日期，需与回放日期一致，格式%Y-%m-%d，默认运行当天
    /// @param incre_port 增量端口，多开时使用，默认为0
    /// @return 0:表示接口调用正常，其他值表示接口调用异常 
    virtual int startMatch(std::string date = "", size_t incre_port = 0) = 0;
};


class MatchEngine {
public:
    /// @brief 获取指定股票的指定盘口的数据
    /// @param securityid 股票代码
    /// @param side 盘口方向
    /// @param n 盘口层数
    /// @return PriceLevel
    virtual PriceLevel *getPriceLevel(int securityid, Side side, size_t n) = 0;
};
