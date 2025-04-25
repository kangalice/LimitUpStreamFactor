#include "MatchEngineAPIData.hpp"

enum class OrderType {
	Market = 49,
	Limit = 50,
	Cancel = 51,
	U = 85,
};

enum class OrderSide { 
	Bid = 49, 
	Ask = 50 
};

enum class TradeSide { 
	Buy = 'B', 
	Sell = 'S', 
	N = 'N', 
	Both = 'O' 
};

enum class ObMode { 
	Limit = 0,
	Call = 1,
	Copy = 2
};

enum class CallMode {
	Open = 0,
	Close = 1,
	None = 2
};

enum class Exchange {
	SZ = 0,
	SH = 1
};

struct PriceLevel {
    price_t price;
	qty_t qty;
};

struct Order {
    int secutiryid;
    int parse_time;
    seqnum_t applseqnum;
	price_t price;
	qty_t orderqty;
	OrderSide side;
	OrderType ordertype;
};

struct Trade {
    int secutiryid;
    int parse_time;
    seqnum_t applseqnum;
    seqnum_t buyno;
    seqnum_t sellno;
    price_t price;
    qty_t qty;
    TradeSide trade_side;
};

struct MatchParam {
    std::vector<std::string> factor_names;          // 因子名称，位置敏感，与输入ob函数的shms保持顺序一致
    size_t factor_interver_ms = 60000;              // 因子计算间隔时间，单位ms
    size_t factor_ob_start_time = 93000000;         // 因子计算起始时间，格式为%H%M%S%f
    size_t factor_ob_end_time = 150000000;          // 因子计算起始时间，格式为%H%M%S%f

    size_t process_num = 3;                         // 进程数量，可以是1,3,6,9...等进程数量
    std::string recv_market = "sz";                 // 接收的市场类别，仅在process_num=1时生效，可以是sz/sh
};