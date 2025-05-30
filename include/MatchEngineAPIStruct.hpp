#pragma once
#include "MatchEngineAPIData.hpp"

enum class ObMode: uint8_t { 
	Limit = 0,
	Call = 1,
	Copy = 2
};

enum class CallMode: uint8_t {
	Open = 0,
	Close = 1,
	None = 2
};

struct PriceLevel {
    price_t price;
	qty_t qty;
};

enum class Exchange: uint8_t {
	SZ = 0,                         // 深圳
	SH = 1                          // 上海
};

enum class Side: uint8_t { 
	Buy = 0,                        // 主动买
	Sell = 1,                       // 主动卖
	N = 2,                          // 未知，集合竞价成交单
	Both = 3                        // 测试用
};

enum class Type: uint8_t { 
	Limit = 0,                      // 限价单
    Market = 1,                     // 市价单
    Best = 2,                       // 本方最优
	Cancel = 3,                     // 撤单
    Trade = 4,                      // 成交
	Status = 5,                     // 产品状态
    End = 6                         // 结束标识符
};

#pragma pack(push, 1)
struct UnifiedRecord {
    Type type;                      // 类型
    Exchange market;                // 市场
    int32_t channel;                // 行情通道
    int32_t securityid;             // 股票代码
    seqnum_t applseqnum;            // 订单号
    int32_t parse_time;             // 时间，格式为HHMMSSsss
    seqnum_t buyno = 0;             // 买方订单号，当type=Trade/Cancel时有效
    seqnum_t sellno = 0;            // 卖方订单号，当type=Trade/Cancel时有效
    price_t price;                  // 价格
    qty_t qty;                      // 成交量
    Side side;                      // 买卖方向
    seqnum_t biz_index;             // 业务序号
};
#pragma pack(pop)

struct MatchParam {
    std::vector<std::string> factor_names;              // 因子名称，位置敏感，与输入ob函数的shms保持顺序一致
    int factor_interver_ms = 60000;                     // 因子计算间隔时间，单位ms
    int factor_ob_start_time = 93000000;                // 因子计算起始时间，格式为%H%M%S%f
    int factor_ob_end_time = 150000000;                 // 因子计算起始时间，格式为%H%M%S%f

    size_t process_num = 3;                             // 进程数量，可以是1,3,6,9...等进程数量
    std::string recv_market = "sz";                     // 接收的市场类别，仅在process_num=1时生效，可以是sz/sh

    std::vector<std::string> save_names = {"factor"};   // 保存的数据名称，支持"kline","factor"，默认"factor"
    size_t log_level = 1;                               // 日志等级
};
