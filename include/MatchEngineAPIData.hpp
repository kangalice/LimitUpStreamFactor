#pragma once

#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file MatchEngineAPIData.hpp
 * @brief 撮合引擎API数据类型和常量定义。
 *
 * 该文件定义了撮合引擎API中使用的基本数据类型（如价格、数量、序列号），
 * 以及各种全局常量，包括市场数据字段名称、时间参数、共享内存名称等。
 * 这些定义在整个撮合引擎系统中共享。
 */

typedef double price_t, amt_t;    // 价格和金额类型
typedef int64_t seqnum_t;         // 序列号类型
typedef int32_t qty_t;            // 数量类型

static price_t MARKET_ASK_PRICE = 9999999;    // 市场卖方最优价格（极大值）
static price_t MARKET_BID_PRICE = 0;          // 市场买方最优价格（极小值）

static int HEADER_LEN = 5;             // 消息头长度
static int LINE_BUFFER_SIZE = 1024;    // 行缓冲区大小

// IPC资源命名常量
static const std::string PROJ_PREFIX = "fieldengine";           // 项目级IPC资源统一前缀
static const std::string IPC_L2_DATA = "l2";                    // Level 2 行情数据标识
static const std::string IPC_QUEUE_SUFFIX = "queue";            // SpmcQueue资源后缀
static const std::string IPC_TRACKER_SUFFIX = "idx_tracker";    // IndexTracker资源后缀
static const std::string SHM_QUOTE_KLINE = "quotekline";        // 行情K线字段数据标识

// 组合后的共享内存前缀
static const std::string FIELD_OUPUT_SHM_PREFIX = PROJ_PREFIX + "_" + SHM_QUOTE_KLINE;    // 行情字段共享内存前缀

static int OB_DEPTH = 10;    // 订单簿深度 (例如，OB_DEPTH = 10 表示维护买卖双方各10档盘口)

static int INTERVER_MS = 3000;             // 订单簿快照时间间隔（毫秒）
static int OB_START_TIME = 91500000;       // 订单簿快照起始时间（整数表示，例如91500000代表09:15:00）
static int OB_MORNING_LIMIT = 92500000;    // 早上集合竞价限制时间（整数表示）
static int OB_END_TIME = 150000000;        // 订单簿快照结束时间（整数表示）

static std::string CHECK_CAGE_DATE = "2023-06-01";
static double CAGE_HIGH_MUL = 1.02;
static double CAGE_LOW_MUL = 0.98;

static int INF = std::numeric_limits<int>::max();    // 无穷大整数, 用于表示无限大或默认值

/**
 * @brief 市场数据字段名称列表。
 *
 * 这是一个位置敏感的向量，定义了市场数据在共享内存中的存储顺序。
 * 前OB_DEPTH*2个元素是卖方盘口（ask price/volume），
 * 之后OB_DEPTH*2个元素是买方盘口（bid price/volume），
 * 接着是开高低收、成交笔数、成交额、换手率、总买卖量、加权平均价等字段。
 */
static std::vector<std::string> MarketCols = {
    "ask1",  "askv1",    "ask2",        "askv2",      "ask3",        "askv3",     "ask4",  "askv4", "ask5",
    "askv5", "ask6",     "askv6",       "ask7",       "askv7",       "ask8",      "askv8", "ask9",  "askv9",
    "ask10", "askv10",   "bid1",        "bidv1",      "bid2",        "bidv2",     "bid3",  "bidv3", "bid4",
    "bidv4", "bid5",     "bidv5",       "bid6",       "bidv6",       "bid7",      "bidv7", "bid8",  "bidv8",
    "bid9",  "bidv9",    "bid10",       "bidv10",     "open",        "high",      "low",   "close", "tradnum",
    "tradv", "turnover", "totalaskvol", "wavgaskpri", "totalbidvol", "wavgbidpri"};

// trade/order因子，位置敏感，连接在上面的市场字段后面
// static std::vector<std::string> BaseFactors = { };
