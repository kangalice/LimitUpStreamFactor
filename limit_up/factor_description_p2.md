# 打板因子算法文档 - 第二部分（因子11-20）

## 文档说明

本文档详细描述打板因子的计算逻辑，包括每个因子的具体计算步骤和依赖的逐笔成交数据字段。本部分涵盖因子11-20。

---

## 11. A_P_opponents_ratio_sum_fix0 - 主动/被动小单占比之和

### 因子说明
逐笔成交区分买卖方向，若成交额≤5万计入"小单"，计算主动小单占比和被动小单占比之和。

### 计算逻辑

#### 初始化阶段
1. 初始化累加变量：
   - `activeSum1 = 0`：主动买小单成交量
   - `activeSum2 = 0`：主动买总成交量
   - `passiveSum1 = 0`：被动卖小单成交量
   - `passiveSum2 = 0`：被动卖总成交量

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若为主动买（`fill->side == 1`）：
   - 若成交额≤5万（`fill->amt <= 50000`）：
     - `activeSum1 += fill->qty`
   - `activeSum2 += fill->qty`
2. 若为被动卖（`fill->side == 2`）：
   - 若成交额≤5万（`fill->amt <= 50000`）：
     - `passiveSum1 += fill->qty`
   - `passiveSum2 += fill->qty`

#### 计算阶段（Calculate方法）
1. 默认值：`sum = 1.15`
2. 若`activeSum2 != 0 && passiveSum2 != 0`：
   - `factor_value = activeSum1 / activeSum2 + passiveSum1 / passiveSum2`
3. 否则：`factor_value = 1.15`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->side | int | Fill结构 | 买卖方向（1=买，2=卖） |
| fill->amt | double | Fill结构 | 成交金额（元） |
| fill->qty | uint64_t | Fill结构 | 成交数量（股） |

### 代码实现位置
`因子/A_P_opponents_ratio_sum_fix0.h`

---

## 12. BA_emotion_diff_sqrt - 买盘在涨停半程上下的强弱差

### 因子说明
计算买盘在涨停半程价格上下方的成交量差异，取平方根并保留符号。

### 计算逻辑

#### 初始化阶段
1. `halfPrice = 0`：半程价格（首次计算）
2. `aboveSum = 0`：半程上方的买单成交量
3. `belowSum = 0`：半程下方的买单成交量

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若`halfPrice == 0`（首次）：
   - 计算半程价格：`halfPrice = (high_limit_close - jhjj_price) × 0.5 + jhjj_price`
2. 若为买单（`fill->side == 1`）：
   - 若`fill->price >= halfPrice`：
     - `aboveSum += fill->qty`
   - 否则：
     - `belowSum += fill->qty`

#### 计算阶段（Calculate方法）
1. 计算原始差值：`diffRaw = (aboveSum - belowSum) / free_float_capital`
2. 确定符号：
   - 若`diffRaw > 0`：`tmp = 1`
   - 若`diffRaw < 0`：`tmp = -1`
   - 若`diffRaw == 0`：`tmp = 0`
   - 若为NaN：`tmp = NAN`
3. 计算平方根并保留符号：`diffSqrt = tmp × sqrt(abs(diffRaw))`
4. 若为NaN：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| high_limit_close | double | MarketDataManager | 涨停价 |
| jhjj_price | double | MarketDataManager | 集合竞价价格 |
| free_float_capital | double | MarketDataManager | 自由流通股本（万股） |
| fill->side | int | Fill结构 | 买卖方向（1=买，2=卖） |
| fill->price | double | Fill结构 | 成交价格 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |

### 代码实现位置
`因子/BA_emotion_diff_sqrt_jup.h`

---

## 13. Boosting_average_zb_ratio_before_last_20 - 剔除最新20个大幅波动买单后的平均占比

### 因子说明
若买单编号在集合竞价阶段价格波动≥0.01，则纳入集合并累计其整单成交量。样本≥50时剔除最新20个编号，计算剩余编号的平均占比。

### 计算逻辑

#### 初始化阶段
1. `filteredNo`：符合条件的买单编号集合
2. `totalUpdateQty = 0`：符合条件的买单总成交量

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 获取买单编号：`buyNo = fill->buy_no`
2. 若编号已在集合中（`filteredNo.count(buyNo) != 0`）：
   - `totalUpdateQty += fill->qty`
3. 否则：
   - 从`lxjj_trade_buy_map`中查找该编号的订单信息
   - 若价格波动≥0.01（`order.max_price - order.min_price >= 0.01`）：
     - `totalUpdateQty += order.qty`
     - `filteredNo.emplace(buyNo)`

#### 计算阶段（Calculate方法）
1. 若`lxjj_trade_buy_map.size() < 50`：返回0
2. 初始化：`totalQty = totalUpdateQty`，`count = filteredNo.size()`
3. 从`lxjj_buy_no_set`的末尾开始，剔除最新20个符合条件的编号：
   - 遍历编号集合（倒序）
   - 若编号在`filteredNo`中：
     - `totalQty -= order.qty`
     - `count--`
   - 最多处理20个
4. 若`count == 0`：返回0
5. 计算因子值：`factor_value = (totalQty / free_float_capital) / count`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| lxjj_trade_buy_map | unordered_map<uint64_t, MarketOrder> | MarketDataManager | 连续竞价买单映射 |
| MarketOrder.max_price | double | MarketOrder结构 | 订单最高成交价 |
| MarketOrder.min_price | double | MarketOrder结构 | 订单最低成交价 |
| MarketOrder.qty | uint64_t | MarketOrder结构 | 订单总成交量 |
| lxjj_buy_no_set | set<uint64_t> | MarketDataManager | 连续竞价买方编号集合 |
| free_float_capital | double | MarketDataManager | 自由流通股本（万股） |

### 代码实现位置
`因子/Boosting_average_zb_ratio_before_last_20.h`

---

## 14. Decaying_strength - 时间加权涨幅强度

### 因子说明
每笔计算距9:30的毫秒差与收益率，累加时间×收益率×成交量，最终除以总成交量得到时间加权涨幅强度。

### 计算逻辑

#### 初始化阶段
1. `delta = 0`：距9:30的毫秒差
2. `sum1 = 0`：时间×收益率×成交量的累加
3. `isCyb = zcz`：注册制标记
4. `timestamp930 = 93000000`：9:30的时间戳

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 计算时间差：`delta = CalTimeDelta(timestamp930, fill->timestamp)`
2. 计算收益率：
   - `ret = fill->price / pre_close - 1.0`
   - 若为注册制：`ret = ret / 2.0`
3. 累加：`sum1 += delta × ret × fill->qty`

#### 计算阶段（Calculate方法）
1. 计算强度：`strength = sum1 / delta / lxjj_total_qty`
2. 若为NaN：`factor_value = 0.3`
3. 否则：`factor_value = strength`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| fill->price | double | Fill结构 | 成交价格 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| lxjj_total_qty | uint64_t | MarketDataManager | 连续竞价成交总量 |

### 代码实现位置
`因子/Decaying_strength.h`

---

## 15. EF_profit_coss_ratio_real_3 - 形态分组收益交叉比

### 因子说明
从参数读取shape4和shape2股票列表，合并其近两日open-to-pre-high比值与计数，计算交叉比。

### 计算逻辑

#### 初始化阶段
1. 从`params`中读取：
   - `filterShape4StockList`：shape4股票列表
   - `filterShape2StockList`：shape2股票列表

#### 计算阶段（Calculate方法）
1. 获取初始值：
   - `shape4_last2_days_sum = params->shape4Last2DaysSum`
   - `shape4_last_3days_count = params->shape4Last3DaysCount`
2. 遍历shape4股票列表：
   - 对每个股票调用`GetOpenToPreHighRatio(symbol)`
   - 若不为NaN：累加到`shape4_last2_days_sum`，否则`shape4_last_3days_count--`
3. 获取初始值：
   - `shape2_last_2days_sum = params->shape2Last2DaysSum`
   - `shape2_last_3days_count = params->shape2Last3DaysCount`
4. 遍历shape2股票列表：
   - 对每个股票调用`GetOpenToPreHighRatio(symbol)`
   - 若不为NaN：累加到`shape2_last_2days_sum`，否则`shape2_last_3days_count--`
5. 计算因子值：
   - 若`shape2_last_2days_sum != 0 && shape4_last_3days_count != 0`：
     - `factor = -shape4_last2_days_sum × shape2_last_3days_count / shape4_last_3days_count / shape2_last_2days_sum`
   - 否则：`factor = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| params->filterShape4StockList | vector | EventDrivenParams | shape4股票列表 |
| params->shape4Last2DaysSum | double | EventDrivenParams | shape4近两日总和 |
| params->shape4Last3DaysCount | int | EventDrivenParams | shape4近三日计数 |
| params->filterShape2StockList | vector | EventDrivenParams | shape2股票列表 |
| params->shape2Last2DaysSum | double | EventDrivenParams | shape2近两日总和 |
| params->shape2Last3DaysCount | int | EventDrivenParams | shape2近三日计数 |
| GetOpenToPreHighRatio | function | MarketDataManager | 获取开盘相对前高的比值 |

### 代码实现位置
`因子/EF_profit_coss_ratio_real_3.h`

---

## 16. FQS_2_ZT_compared_volume - 首次触及涨停后的成交量占比

### 因子说明
根据注册制与否设定阈值（7%或14%），当成交价首次≥阈值时记录时间并累计该时刻及之后的成交量，总量/自由流通股本为因子值。

### 计算逻辑

#### 初始化阶段
1. `FQS_Time = 0`：首次触及阈值的时间
2. `FQSSum = 0`：累计成交量
3. `index = 0`：在连续竞价成交列表中的索引

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若已记录首次触及时间（`FQS_Time != 0`）：
   - 累加成交量：`FQSSum += fill->qty`
2. 若尚未记录（`FQS_Time == 0`）：
   - 根据注册制标记计算阈值：
     - 注册制：`FQS_price = floor(pre_close × 100 × 1.14 + 0.5) / 100`
     - 非注册制：`FQS_price = floor(pre_close × 100 × 1.07 + 0.5) / 100`
   - 若`fill->price >= FQS_price`：
     - 记录时间：`FQS_Time = fill->timestamp`
     - 开始累加：`FQSSum += fill->qty`
     - 记录索引：`index = lxjj_fill_list.size() - 2`

#### 计算阶段（Calculate方法）
1. 从`lxjj_fill_list[index]`开始向前回溯，找到所有时间戳等于`FQS_Time`的成交
2. 累加这些成交的成交量到`FQSSum`
3. 计算因子值：`factor_value = FQSSum / free_float_capital`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| fill->price | double | Fill结构 | 成交价格 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| lxjj_fill_list | vector<Fill*> | MarketDataManager | 连续竞价成交列表 |
| free_float_capital | double | MarketDataManager | 自由流通股本（万股） |

### 代码实现位置
`因子/FQS_2_ZT_compared_volume.h`

---

## 17. fc_ttickab_bs_qty_ratio - 涨停前盘口买卖挂量比中位数

### 因子说明
在9:30到(ZT_time - tickTimeLag)区间倒序读取最多10条tick，计算(ask_order_qty+0.01)/(bid_order_qty+0.01)，返回中位数。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 计算截断时间：`endTime = AddMilliseconds(ZT_local_time, -tickTimeLag)`
2. 获取tick列表：`tickList = quote_list`
3. 初始化结果列表：`ret`（预留10个元素）
4. 从后向前遍历tick列表：
   - 若`tick->md_time <= endTime && tick->md_time >= 93000000`：
     - 计算比值：`ratio = (tick->ask_order_qty + 0.01) / (tick->bid_order_qty + 0.01)`
     - 加入列表：`ret.emplace_back(ratio)`
   - 若`ret.size() == 10`：跳出
5. 计算中位数：`factorVal = calcMedian(ret)`
6. 若为NaN：`factorVal = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->ask_order_qty | uint64_t | Tick结构 | 卖盘挂单总量 |
| tick->bid_order_qty | uint64_t | Tick结构 | 买盘挂单总量 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |

### 代码实现位置
`因子/Fc_ttickab_bs_qty_ratio.h`

---

## 18. fc_ttickab_lp_op_diff_max_turn - 涨停前盘口均价-最新价差最大值

### 因子说明
搜集有效tick的ask_avg_px - last_px；若样本>50取最近50条的最大值/pre_close，否则固定输出0.025。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 计算截断时间：`endTime = AddMilliseconds(ZT_local_time, -tickTimeLag)`
2. 初始化差值列表：`lastPx_avgOfferPx_diff`
3. 遍历tick列表：
   - 若`tick->md_time <= endTime && tick->md_time > 93000000`：
     - 计算差值：`diff = (tick->ask_avg_px == 0) ? 0 : tick->ask_avg_px - tick->last_px`
     - 加入列表：`lastPx_avgOfferPx_diff.emplace_back(diff)`
4. 若`lastPx_avgOfferPx_diff.size() > 50`：
   - 取最近50条的最大值：`max = max(lastPx_avgOfferPx_diff[size-50..size-1])`
   - `factorVal = max / pre_close`
5. 否则：`factorVal = 0.025`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->ask_avg_px | double | Tick结构 | 平均委卖价格 |
| tick->last_px | double | Tick结构 | 最新成交价 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |
| pre_close | double | MarketDataManager | 昨收盘价 |

### 代码实现位置
`因子/Fc_ttickab_lp_op_diff_max_turn.h`

---

## 19. fc_ttickab_20231026_20 - 涨停前ask5溢价的均值/波动比

### 因子说明
以ZT_time减tickTimeLag确定终点，倒序找到首条不晚于该时刻的tick并向前回溯最多600毫秒形成窗口；对窗口内tick按md_time/10000分组，计算ask5/pre_close-1（注册制股票再除以2）的组均值；最终取所有组均值的平均值除以(标准差+0.001)。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 计算截断时间：`tickEndTime = max(AddMilliseconds(ZT_local_time, -tickTimeLag), 93000000)`
2. 从后向前找到第一个符合条件的tick：
   - 若`tick->md_time <= tickEndTime`：
     - `startTime = max(FunGetTime(tick->md_time, -600), 93000000)`
     - `tickEndTime = tick->md_time`
3. 初始化分组映射：`group_res`（按秒级分组）
4. 从后向前遍历tick：
   - 若`tick->md_time > startTime && tick->md_time <= tickEndTime`：
     - 计算溢价：`pct = tick->ask_price[4] / preClose - 1`
     - 若为注册制：`pct = pct / 2`
     - 按秒分组：`sec10 = tick->md_time / 10000`
     - 累加到对应分组
5. 计算各组均值：`lxjj_ret_list = [group_mean for each group]`
6. 计算所有组均值的平均值：`mean = calcMean(lxjj_ret_list)`
7. 计算标准差：`std = calcStd(lxjj_ret_list, true, false, false, mean)`
8. 计算因子值：`factorValue = mean / (std + 0.001)`
9. 若为NaN：`factorValue = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->ask_price[4] | double | Tick结构 | 卖盘第5档价格 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| starts_with3 | bool | MarketDataManager | 创业板标记 |
| starts_with68 | bool | MarketDataManager | 科创板标记 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |

### 代码实现位置
`因子/fc_ttickab_20231026_20.h`

---

## 20. Sheep_or_goat - 主动卖单分档及买卖均单量比

### 因子说明
统计主动卖单次数，根据次数分档返回不同值；超出时按(主动买单总量/买方编号数)/(主动卖单总量/卖方编号数)计算。

### 计算逻辑

#### 初始化阶段
1. `activeSell = 0`：主动卖单次数
2. `activeBuy = 0`：主动买单次数

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若为主动卖（`fill->side == 2`）：
   - `activeSell += 1`
2. 若为主动买（`fill->side == 1`）：
   - `activeBuy += 1`

#### 计算阶段（Calculate方法）
1. 若`activeSell <= 5`：`factorValue = 0.5`
2. 否则若`activeSell <= 25`：`factorValue = 1.5`
3. 否则若`activeSell <= 40`：`factorValue = 2.0`
4. 否则：
   - `lxjjBuyCount = lxjj_buy_no_set.size()`
   - `lxjjSellCount = lxjj_sell_no_set.size()`
   - 若`lxjjBuyCount > 0 && lxjjSellCount > 0`：
     - `factorValue = (activeBuy / lxjjBuyCount) / (activeSell / lxjjSellCount)`
   - 否则：`factorValue = 0`
5. 若为NaN：`factorValue = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->side | int | Fill结构 | 买卖方向（1=买，2=卖） |
| lxjj_buy_no_set | set<uint64_t> | MarketDataManager | 连续竞价买方编号集合 |
| lxjj_sell_no_set | set<uint64_t> | MarketDataManager | 连续竞价卖方编号集合 |

### 代码实现位置
`因子/Sheep_or_goat.h`

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
| free_float_capital | double | 自由流通股本（万股） |
| high_limit_close | double | 涨停价（元） |
| jhjj_price | double | 集合竞价价格（元） |
| ZT_local_time | uint64_t | 涨停触发本地时间（毫秒） |
| lxjj_total_qty | uint64_t | 连续竞价成交总量（股） |
| lxjj_fill_list | vector<Fill*> | 连续竞价成交列表 |
| lxjj_trade_buy_map | unordered_map<uint64_t, MarketOrder> | 连续竞价买单映射 |
| lxjj_buy_no_set | set<uint64_t> | 连续竞价买方编号集合 |
| lxjj_sell_no_set | set<uint64_t> | 连续竞价卖方编号集合 |
| quote_list | vector<Tick*> | 行情快照列表 |
| params | EventDrivenParams* | 运行参数 |

### MarketOrder结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| max_price | double | 订单最高成交价 |
| min_price | double | 订单最低成交价 |
| qty | uint64_t | 订单总成交量 |

### Tick结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 行情时间戳（毫秒） |
| ask_order_qty | uint64_t | 卖盘挂单总量（股） |
| bid_order_qty | uint64_t | 买盘挂单总量（股） |
| ask_avg_px | double | 平均委卖价格（元） |
| ask_price[0..9] | double | 卖盘第1-10档价格（元） |
| last_px | double | 最新成交价（元） |

---

## 注意事项

1. **注册制处理**：创业板和科创板的涨跌幅限制为20%，非注册制为10%。

2. **时间窗口**：部分因子使用时间窗口（如600毫秒、60秒），需要正确计算时间范围。

3. **分组聚合**：部分因子按时间分组（如按秒级分组），需要注意分组逻辑。

4. **异常值处理**：当计算结果为NaN或Inf时，通常返回默认值。

5. **集合操作**：使用集合去重时需要注意集合的大小和遍历顺序。
