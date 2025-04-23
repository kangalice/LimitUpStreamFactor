#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>

typedef double price_t, amt_t;
typedef int64_t seqnum_t, qty_t;

static price_t MARKET_ASK_PRICE = 9999999;
static price_t MARKET_BID_PRICE = 0;

static int HEADER_LEN = 5;
static int LINE_BUFFER_SIZE = 1024;
static std::string END_MSG = "xxEND";
static std::string SHM_HEADER = "quote_kline";

static int OB_DEPTH = 10;
static int INTERVER_MS = 3000;
static int OB_START_TIME = 91500000;
static int OB_MORNING_LIMIT = 92500000;
static int OB_END_TIME = 150000000;
static std::string CHECK_CAGE_DATE = "2023-06-01";
static double CAGE_HIGH_MUL = 1.02;
static double CAGE_LOW_MUL = 0.98;

static int INF = std::numeric_limits<int>::max();

// 位置敏感，前OB_DEPTH*2为ASK，之后OB_DEPTH*2为BID，然后为ohlc/tradn/tradv/turnover/totalbidvol/wavgbidpri/totalaskvol/wavgaskpri
static std::vector<std::string> MarketCols = {
    "ask1",        "askv1",     "ask2",    "askv2", "ask3",     "askv3",       "ask4",
    "askv4",       "ask5",      "askv5",   "ask6",  "askv6",    "ask7",        "askv7",
    "ask8",        "askv8",     "ask9",    "askv9", "ask10",    "askv10",      "bid1",
    "bidv1",       "bid2",      "bidv2",   "bid3",  "bidv3",    "bid4",        "bidv4",
    "bid5",        "bidv5",     "bid6",    "bidv6", "bid7",     "bidv7",       "bid8",
    "bidv8",       "bid9",      "bidv9",   "bid10", "bidv10",   "open",        "high",
    "low",         "close",     "tradnum", "tradv", "turnover", "totalaskvol", "wavgaskpri", 
	"totalbidvol", "wavgbidpri"
	};

// trade/order因子，位置敏感，连接在上面的市场字段后面
// static std::vector<std::string> BaseFactors = { };
