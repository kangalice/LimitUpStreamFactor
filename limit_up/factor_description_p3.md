# 打板因子算法文档 - 第三部分（因子21-30）

## 文档说明

本文档详细描述打板因子的计算逻辑，包括每个因子的具体计算步骤和依赖的逐笔成交数据字段。本部分涵盖因子21-30。

---

## 21. Short_buy_sell_mean_ratio - 涨停前5秒买卖平均单量对数比

### 因子说明
以ZT_time为基准回溯5秒，按买/卖编号在trade_map去重，统计首笔时间≥窗口起点的订单数量和笔数，求均量比后取ln。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 获取涨停时间：`ztTime = GetZTLocalTime()`
2. 计算窗口起点：`last5StartTime = AddMilliseconds(ztTime, -5000)`
3. 获取交易映射：
   - `lxjjTradeBuyMap = lxjj_trade_buy_map`
   - `lxjjTradeSellMap = lxjj_trade_sell_map`
4. 若映射为空：返回0
5. 初始化累加变量：
   - `buySum = 0`，`buyCnt = 0`，`buySet`：买方集合
   - `sellSum = 0`，`sellCnt = 0`，`sellSet`：卖方集合
6. 遍历`last_5sec_fill_list`：
   - 对每个`fill`：
     - 获取买单编号：`buyNo = fill->buy_no`
     - 从`lxjjTradeBuyMap`中查找订单
     - 若`buyOrder.first_fill_time >= last5StartTime && buyNo不在buySet中`：
       - `buySum += buyOrder.qty`
       - `buyCnt++`
       - `buySet.emplace(buyNo)`
     - 获取卖单编号：`sellNo = fill->sell_no`
     - 从`lxjjTradeSellMap`中查找订单
     - 若`sellOrder.first_fill_time >= last5StartTime && sellNo不在sellSet中`：
       - `sellSum += sellOrder.qty`
       - `sellCnt++`
       - `sellSet.emplace(sellNo)`
7. 计算均量比：`val = (buySum / buyCnt) / (sellSum / sellCnt)`
8. 若`val != 0 && !isnan(val)`：`factor_value = log(val)`
9. 否则：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| GetZTLocalTime | function | MarketDataManager | 获取涨停触发本地时间 |
| last_5sec_fill_list | vector<Fill*> | MarketDataManager | 近5秒成交列表 |
| lxjj_trade_buy_map | unordered_map<uint64_t, MarketOrder> | MarketDataManager | 连续竞价买单映射 |
| lxjj_trade_sell_map | unordered_map<uint64_t, MarketOrder> | MarketDataManager | 连续竞价卖单映射 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| MarketOrder.first_fill_time | uint64_t | MarketOrder结构 | 订单首笔成交时间 |
| MarketOrder.qty | uint64_t | MarketOrder结构 | 订单总成交量 |

### 代码实现位置
`因子/Short_buy_sell_mean_ratio.h`
---

## 22. skk_order_bs_ratio_mean - 近30个时间片卖单均量/买单均量比

### 因子说明
找到最近order_type=='2'委托时间，回溯30秒(不早于9:30)，累加区间内买/卖委托量与笔数，输出均买量/均卖量，结果截顶100，异常值置2.2。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 默认值：`nan_val = 2.2`
2. 若`lxjj_order_list`为空：返回`nan_val`
3. 从后向前找到最近`order_type == '2'`的委托时间：`end_time`
4. 计算窗口起点：`start_time = max(FunGetTime(end_time, -30), 93000000)`
5. 初始化累加变量：
   - `cnt[2] = {0, 0}`：买/卖委托笔数
   - `qty_sum[2] = {0, 0}`：买/卖委托量总和
6. 从后向前遍历委托列表：
   - 若`order->md_time < start_time`：跳出
   - 若`order->order_type != '2'`：跳过
   - 确定方向：`side = order->side - '1'`（0=买，1=卖）
   - `qty_sum[side] += order->order_qty`
   - `cnt[side]++`
7. 计算均量：
   - `amt1 = qty_sum[0] / cnt[0]`（买单均量）
   - `amt2 = qty_sum[1] / cnt[1]`（卖单均量）
8. 计算比值：`value = min(amt1 / amt2, 100.0)`
9. 若为Inf或NaN：`value = nan_val`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_order_list | vector<TOrder*> | MarketDataManager | 连续竞价委托列表 |
| order->order_type | char | TOrder结构 | 委托类型（'2'表示某种类型） |
| order->side | char | TOrder结构 | 买卖方向（'1'=买，'2'=卖） |
| order->order_qty | uint64_t | TOrder结构 | 委托数量 |
| order->md_time | uint64_t | TOrder结构 | 委托时间戳 |

### 代码实现位置
`因子/Skk_order_bs_ratio_mean.h`

---

## 23. sss_1s_volsbo_num_bsdiff - 高量秒级主动买卖次数差

### 因子说明
按秒聚合成交量与金额，标记buy_no>sell_no的量为主动买；以均值+标准差为阈值合并连续高量秒段，比较主动买比例>0.5与<0.5的段数，占比相减后输出。

### 计算逻辑

#### 初始化阶段
1. `mdtime_map`：按秒（timestamp/1000）聚合的数据结构
   - `volume`：成交量
   - `amount`：成交额
   - `actbuy_vol`：主动买成交量
   - `actbuy_ratio`：主动买比例

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 计算秒级时间：`md_min = fill->timestamp / 1000`
2. 查找或创建该秒的数据结构
3. 累加：`volume += fill->qty`，`amount += fill->amt`
4. 若`fill->buy_no > fill->sell_no`（主动买）：
   - `actbuy_vol += fill->qty`

#### 计算阶段（Calculate方法）
1. 若`mdtime_map.size() <= 2`：返回0
2. 计算每秒的主动买比例：`actbuy_ratio = actbuy_vol / volume`
3. 计算成交量均值和标准差：`value_mean`，`value_std`
4. 计算阈值：`thred = value_mean + value_std`
5. 合并连续高量秒段：
   - 遍历每秒数据
   - 若`volume >= thred`：
     - 若上一秒不是高量：创建新段
     - 否则：累加到当前段
   - 否则：标记为非高量
6. 统计各段的主动买比例：
   - 若`ratio > 0.5`：`above_buy_counter++`
   - 若`ratio < 0.5`：`above_sell_counter++`
7. 计算因子值：`factorVal = above_buy_counter / md_min_size - above_sell_counter / md_min_size`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->amt | double | Fill结构 | 成交金额 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |

### 代码实现位置
`因子/Sss_1s_volsbo_num_bsdiff.h`

---

## 24. sss_4wbigamt1_diff_open - 早盘大额买卖净额

### 因子说明
在时间93000000-100000000内累计每个买/卖编号成交额；若编号在早盘过滤集合中且累计金额>4万，则并入总买/卖额，最终输出totalBuySum-totalSellSum。

### 计算逻辑

#### 初始化阶段
1. `totalBuySum = 0`：总买额
2. `totalSellSum = 0`：总卖额
3. `buyNoAmtMap`：买单编号到累计金额的映射
4. `sellNoAmtMap`：卖单编号到累计金额的映射
5. `filterBuyNoSet`：早盘（9:30-10:00）出现的买单编号集合
6. `filterSellNoSet`：早盘出现的卖单编号集合
7. `buyNoSet`：已计入总买额的编号集合
8. `sellNoSet`：已计入总卖额的编号集合

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 获取编号：`buyNo = fill->buy_no`，`sellNo = fill->sell_no`
2. 若在早盘时间（`fill->timestamp > 93000000 && fill->timestamp < 100000000`）：
   - `filterBuyNoSet.emplace(buyNo)`
   - `filterSellNoSet.emplace(sellNo)`
3. 处理买单：
   - 若编号已在`buyNoSet`中：`totalBuySum += fill->amt`
   - 否则：
     - 更新`buyNoAmtMap[buyNo] += fill->amt`
     - 若`buyNo在filterBuyNoSet中 && buyNoAmtMap[buyNo] > 40000`：
       - `totalBuySum += buyNoAmtMap[buyNo]`
       - `buyNoSet.emplace(buyNo)`
4. 处理卖单（类似买单逻辑）

#### 计算阶段（Calculate方法）
1. `factor_value = totalBuySum - totalSellSum`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| fill->amt | double | Fill结构 | 成交金额 |

### 代码实现位置
`因子/Sss_4wbigamt1_diff_open.h`

---

## 25. sss_after_pbtimemax_8 - 突破8%线后回踩停留时间最大值

### 因子说明
与`sss_after_pbtimemax_7`相同，但阈值改为8%。

### 计算逻辑
与`sss_after_pbtimemax_7`相同，仅阈值计算不同：
- `linePrice = floor(pre_close × 100 × (1 + 0.01 × 8 × (1 + zcz)) + 0.5) / 100.0`

### 依赖字段
与`sss_after_pbtimemax_7`相同。

### 代码实现位置
`因子/Sss_after_pbtimemax_8.h`

---

## 26. sss_breaknum_diff - 多价成交买卖编号差

### 因子说明
对买单(buy_no>sell_no)与卖单分别记录首个成交价，若同编号后续出现不同价则加入集合，因子为买集合数减卖集合数。

### 计算逻辑

#### 初始化阶段
1. `actBuyMap`：主动买单编号到首个成交价的映射
2. `actSellMap`：主动卖单编号到首个成交价的映射
3. `actBuySetWithNuniquePrice`：主动买单中多价成交的编号集合
4. `actSellSetWithNuniquePrice`：主动卖单中多价成交的编号集合

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 获取编号和价格：`buyNo = fill->buy_no`，`sellNo = fill->sell_no`，`price = fill->price`
2. 若为主动买（`buyNo > sellNo`）：
   - 若编号不在`actBuySetWithNuniquePrice`中：
     - 查找`actBuyMap`中是否存在该编号
     - 若不存在：记录首个成交价
     - 若存在且价格不同：加入`actBuySetWithNuniquePrice`
3. 若为主动卖（`buyNo <= sellNo`）：
   - 类似处理卖单逻辑

#### 计算阶段（Calculate方法）
1. `factor_value = actBuySetWithNuniquePrice.size() - actSellSetWithNuniquePrice.size()`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| fill->price | double | Fill结构 | 成交价格 |

### 代码实现位置
`因子/Sss_breaknum_diff.h`

---

## 27. sss_tk_slope_sell_t20_max - 最近20笔盘口卖盘倾斜度最大值

### 因子说明
截断时间前倒序取最多20条tick，计算(1-ask_qty0/总卖量)/(最远非零卖价-最优价)，记录最大值。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 计算截断时间：`endTime = AddMilliseconds(ZT_local_time, -tickTimeLag)`
2. 初始化：`counter = 20`，`slope_sell = 0`
3. 从后向前遍历tick列表：
   - 若`counter <= 0`：跳出
   - 若`tick->md_time <= endTime && tick->last_px > 0`：
     - 计算总卖量：`sell_order_sum = Σ(ask_qty[0..9])`
     - 找到最远非零卖价：`fprice = 最后一个非零ask_price[i]`
     - 计算倾斜度：`tmp = (1 - ask_qty[0] / sell_order_sum) / (fprice - ask_price[0])`
     - 若不为NaN：`slope_sell = max(slope_sell, tmp)`
     - `counter--`
4. 返回`slope_sell`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->ask_qty[0..9] | uint64_t | Tick结构 | 卖盘第1-10档挂单数量 |
| tick->ask_price[0..9] | double | Tick结构 | 卖盘第1-10档价格 |
| tick->last_px | double | Tick结构 | 最新成交价 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |

### 代码实现位置
`因子/Sss_tk_slope_sell_t20_max.h`

---

## 28. sss_tk_spctdiff_all_half - 卖均价偏离后半段vs总体

### 因子说明
截取9:30至截断时刻的tick，计算每点(ask_avg_px-last_px)/pre_close(注册制再除以2)，求全部均值及后半段均值，输出ln(halfMean/mean)。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 计算截断时间：`endTime = AddMilliseconds(ZT_local_time, -tickTimeLag)`
2. 找到有效tick范围：
   - `endIndex`：最后一个在范围内的tick索引
   - `startIndex`：第一个在范围内的tick索引（>=93000000）
3. 若`startIndex <= endIndex`：
   - `totalSize = endIndex - startIndex + 1`
   - `halfSize = totalSize / 2`（至少为1）
   - `halfIndex = endIndex - halfSize`
4. 遍历tick：
   - 计算偏离：`res = (ask_avg_px - last_px) / pre_close`
   - 若为注册制：`res = res / 2`
   - 累加到`mean`
   - 若`startIndex > halfIndex`：累加到`halfMean`
5. 计算均值：`mean = mean / totalSize`，`halfMean = halfMean / halfSize`
6. 若`halfMean != 0`：`factorVal = log(halfMean / mean)`
7. 否则：`factorVal = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->ask_avg_px | double | Tick结构 | 平均委卖价格 |
| tick->last_px | double | Tick结构 | 最新成交价 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |

### 代码实现位置
`因子/Sss_tk_spctdiff_all_half.h`

---

## 29. stdBvol - 近1分钟买单量标准差

### 因子说明
若last_1min_trade_buy_map非空，对其中每个编号的qty求样本标准差，空集返回0。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 若`last_1min_trade_buy_map`为空：返回0
2. 初始化标准差计算器：`qtyStd = StandardDeviation(true)`
3. 遍历`last_1min_trade_buy_map`：
   - 对每个订单：`qtyStd.increment(order.qty)`
4. 计算标准差：`val = qtyStd.getResult()`
5. 若为NaN：`val = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| last_1min_trade_buy_map | unordered_map<uint64_t, MarketOrder> | MarketDataManager | 近1分钟买单映射 |
| MarketOrder.qty | uint64_t | MarketOrder结构 | 订单总成交量 |

### 代码实现位置
`因子/Stdbvol.h`

---

## 30. Straight_diff_mean - 实际路径与线性拉升差额

### 因子说明
维护成交时间戳→成交价映射与总价和，假设价格从jhjj_price线性拉升到high_price，计算实际路径与线性路径的差额。

### 计算逻辑

#### 初始化阶段
1. `lxjj_time_to_price`：时间戳到成交价的映射（维护每个时间戳的最新成交价）
2. `price_sum = 0`：所有时间戳对应价格的总和

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 查找该时间戳是否已有价格：
   - 若存在：从`price_sum`中减去旧价格
2. 更新映射：`lxjj_time_to_price[fill->timestamp] = fill->price`
3. 累加新价格：`price_sum += fill->price`

#### 计算阶段（Calculate方法）
1. 获取参数：
   - `size = lxjj_time_to_price.size()`：时间戳数量
   - `jhjj_price`：集合竞价价格
   - `zt_price = high_price`：当日最高价
2. 计算线性增量：`ratio = (zt_price - jhjj_price) / size`
3. 计算线性路径总价：`linear_sum = jhjj_price × size + cnt × ratio × size / 2`（其中`cnt = size + 1`）
4. 计算差额：`sum = jhjj_price × size - price_sum + cnt × ratio × size / 2`
5. 计算因子值：`factor_value = sum / cnt`
6. 若为NaN：`factor_value = 0.2`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| fill->price | double | Fill结构 | 成交价格 |
| jhjj_price | double | MarketDataManager | 集合竞价价格 |
| high_price | double | MarketDataManager | 当日最高价 |
| pre_close | double | MarketDataManager | 昨收盘价 |

### 代码实现位置
`因子/Straight_diff_mean.h`

---

## 字段说明

### Fill结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| price | double | 成交价格（元） |
| qty | uint64_t | 成交数量（股） |
| amt | double | 成交金额（元） |
| buy_no | uint64_t | 买方订单号 |
| sell_no | uint64_t | 卖方订单号 |
| timestamp | uint64_t | 成交时间戳（毫秒） |
| side | int | 买卖方向（1=买方主动，2=卖方主动） |

### MarketDataManager主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| pre_close | double | 昨收盘价（元） |
| zcz | bool | 注册制标记 |
| starts_with3 | bool | 创业板标记 |
| starts_with68 | bool | 科创板标记 |
| high_price | double | 当日最高价（元） |
| jhjj_price | double | 集合竞价价格（元） |
| ZT_local_time | uint64_t | 涨停触发本地时间（毫秒） |
| lxjj_time_to_price | map<uint64_t, double> | 时间戳到成交价映射 |
| last_1min_trade_buy_map | unordered_map<uint64_t, MarketOrder> | 近1分钟买单映射 |
| last_5sec_fill_list | vector<Fill*> | 近5秒成交列表 |
| lxjj_trade_buy_map | unordered_map<uint64_t, MarketOrder> | 连续竞价买单映射 |
| lxjj_trade_sell_map | unordered_map<uint64_t, MarketOrder> | 连续竞价卖单映射 |
| lxjj_order_list | vector<TOrder*> | 连续竞价委托列表 |
| quote_list | vector<Tick*> | 行情快照列表 |
| params | EventDrivenParams* | 运行参数 |
| last_fill | Fill* | 最新一笔成交 |

### MarketOrder结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| first_fill_time | uint64_t | 订单首笔成交时间 |
| qty | uint64_t | 订单总成交量 |

### TOrder结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 委托时间戳（毫秒） |
| order_price | double | 委托价格（元） |
| order_qty | uint64_t | 委托数量（股） |
| order_type | char | 委托类型 |
| side | char | 买卖方向（'1'=买，'2'=卖） |

### Tick结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 行情时间戳（毫秒） |
| ask_order_qty | uint64_t | 卖盘挂单总量（股） |
| bid_order_qty | uint64_t | 买盘挂单总量（股） |
| ask_avg_px | double | 平均委卖价格（元） |
| ask_qty[0..9] | uint64_t | 卖盘第1-10档挂单数量（股） |
| ask_price[0..9] | double | 卖盘第1-10档价格（元） |
| last_px | double | 最新成交价（元） |

---

## 注意事项

1. **时间窗口**：部分因子使用时间窗口（如5秒、30秒、1分钟），需要正确计算时间范围。

2. **集合去重**：使用集合去重时需要注意集合的大小和遍历顺序。

3. **异常值处理**：当计算结果为NaN或Inf时，通常返回默认值。

4. **注册制处理**：创业板和科创板的涨跌幅限制为20%，非注册制为10%。

5. **秒级聚合**：部分因子按秒级聚合数据，需要注意时间戳的转换（除以1000）。
