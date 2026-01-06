#pragma once
#include "MatchEngineAPIData.hpp"
#include <cstdint>

/**
 * @file MatchEngineAPIStruct.hpp
 * @brief 撮合引擎API结构体和枚举类型定义。
 *
 * 该文件定义了撮合引擎API中使用的各种枚举类型和数据结构，
 * 包括订单簿模式、集合竞价模式、价格层级、交易所、买卖方向、
 * 消息类型、统一记录格式以及撮合参数等。
 */

/**
 * @enum ObMode
 * @brief 订单簿快照模式。
 *
 * 定义了在不同市场阶段生成订单簿快照的方式。
 */
enum class ObMode : uint8_t {
    Limit = 0,
    Call = 1,
    Copy = 2
};

/**
 * @enum CallMode
 * @brief 集合竞价模式。
 *
 * 定义了集合竞价的具体类型。
 */
enum class CallMode : uint8_t {
    Open = 0,
    Close = 1,
    None = 2
};

/**
 * @struct PriceLevel
 * @brief 价格档位结构体。
 *
 * 表示订单簿中某一价格层级的价格和数量信息。
 */
struct PriceLevel {
    price_t price;
    qty_t qty;
};

/**
 * @enum Exchange
 * @brief 交易所类型。
 */
enum class Exchange : uint8_t {
    SZ = 0,    // 深圳
    SH = 1     // 上海
};

/**
 * @enum Side
 * @brief 买卖方向。
 *
 * 定义了订单和成交的主动方向。
 */
enum class Side : uint8_t {
    Buy = 0,     // 主动买
    Sell = 1,    // 主动卖
    N = 2,       // 未知，集合竞价成交单
    Both = 3     // 测试用
};

/**
 * @enum Type
 * @brief 市场数据消息类型。
 *
 * 定义了撮合引擎处理的各种市场数据消息的类型。
 */
enum class Type : uint8_t {
    Limit = 0,     // 限价单
    Market = 1,    // 市价单
    Best = 2,      // 本方最优
    Cancel = 3,    // 撤单
    Trade = 4,     // 成交
    Status = 5,    // 产品状态
    End = 6,       // 结束标识符
    CallOB = 7     // 9:25:03~9:30:00 OB提示符
};

enum class TradeStatus : uint8_t {
    Susp = 0,     // 停牌
    Start = 1,    // 启动
    Ocall = 2,    // 开盘集合竞价
    Trade = 3,    // 连续竞价
    Ccall = 4,    // 收盘集合竞价
    Close = 5,    // 闭市
    Endtr = 6,    // 交易结束
    Break = 7,    // 休市
};

struct alignas(64) UnifiedRecord {
    Type type;              // 类型
    Exchange market;        // 市场
    int32_t channel;        // 行情通道
    int32_t securityid;     // 股票代码
    seqnum_t applseqnum;    // 订单号
    int32_t parse_time;     // 时间，格式为HHMMSSsss
    seqnum_t buyno = 0;     // 买方订单号，当type=Trade/Cancel时有效
    seqnum_t sellno = 0;    // 卖方订单号，当type=Trade/Cancel时有效
    price_t price;          // 价格
    qty_t qty;              // 成交量
    Side side;              // 买卖方向
    seqnum_t biz_index;     // 业务序号
};

struct alignas(64) MarketRecord {
    int32_t updatetime;          // 数据生成时间
    int32_t securityid;          // 股票代码
    TradeStatus trade_status;    // 交易代码状态
    qty_t tradnum;               // 成交笔数
    qty_t tradv;                 // 成交量
    amt_t turnover;              // 成交金额
    price_t lastp;               // 最新价
    qty_t totalaskvol;           // 委托卖出总量
    price_t wavgaskpri;          // 加权平均卖价
    qty_t totalbidvol;           // 委托买入总量
    price_t wavgbidpri;          // 加权平均买价
    price_t ask[10];             // 卖方盘口价
    price_t askv[10];            // 卖方盘口量
    price_t bid[10];             // 买方盘口价
    price_t bidv[10];            // 买方盘口量
};

/**
 * @struct MatchParam
 * @brief 撮合引擎参数结构体。
 *
 * 定义了启动撮合引擎所需的所有配置参数，包括因子计算、进程管理等。
 */
struct MatchParam {
    std::vector<std::string> factor_names;    // 因子名称，位置敏感，与输入ob函数的shms保持顺序一致
    int factor_interver_ms = 60000;           // 因子计算间隔时间，单位ms
    int factor_ob_start_time = 93000000;      // 因子计算起始时间，格式为%H%M%S%f
    int factor_ob_end_time = 150000000;       // 因子计算起始时间，格式为%H%M%S%f
    int incre_port = 0;                       // 增量端口，用于多开进程
    size_t process_num = 3;                   // 进程数量，可以是1,3,6,9...等进程数量
    std::string recv_market = "sz";           // 接收的市场类别，仅在process_num=1时生效，可以是sz/sh
    bool skip_unlink = false;                 // 是否跳过共享内存的自动清理，默认false
    bool check_error = false;                 // 是否要进行spi错误的检查，如果需要跑上交所21年以前的历史数据且字段逻辑
                                              // 必须要数据保序，则开启该选项。该选项会较大的减慢代码运行速度。
    size_t log_level = 1;                     // 日志等级
    size_t bind_cpu_start_id =
        -1;    // 如果该值为-1，则不进行绑核；如果>=0，则从对应的cpu_id开始，顺序绑process_num个核心

    // 静态数据参数
    std::string offline_mode = "hdfdata";    // 外部日频历史数据输入模式，支持parquet, hdfdata，
    std::string path_parquet =
        "/mnt/public_shared_files/temp_daily_use_data";    // parquet模式下的文件夹路径，需要满足{path_parquet}/{date}/{data_keys}.parquet的存储结构
    std::vector<std::string> data_keys;    // 数据字段名列表，parquet模式下为文件名，hdfdata模式下为因子的完整名称
    bool auto_sim_date = true;             // 是否自动在路径中简化日期输入，统一简化为YYYYmmdd
};
