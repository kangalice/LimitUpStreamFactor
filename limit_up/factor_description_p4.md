# 打板因子算法文档 - 第四部分（因子31-40）

## 文档说明

本文档详细描述打板因子的计算逻辑，包括每个因子的具体计算步骤和依赖的逐笔成交数据字段。本部分涵盖因子31-40。

---

## 31. Straight_diff_mean_s - 线性差额相对涨幅比例

### 因子说明
与`Straight_diff_mean`相同，但结果除以涨幅，得到相对比例。

### 计算逻辑

#### 计算步骤
1. 执行`Straight_diff_mean`的所有计算步骤
2. 计算涨幅：`price_change = zt_price - pre_close`
3. 若`price_change != 0 && size != 0`：
   - `factor_value = sum / cnt / price_change`
4. 否则：`factor_value = 0.11`

### 依赖字段
与`Straight_diff_mean`相同。

### 代码实现位置
`因子/Straight_diff_mean_s.h`

---

## 32. sundc_t_tran_6 / sundc_t_tran_12 / sundc_t_tran_13 - 最近触发窗口的买卖量&集中度特征

### 因子说明
取最近5个成交/撤单时间构成窗口，计算三个因子：
- `sundc_t_tran_6`：窗口内买卖金额之和除以(high_price×free_float_capital)
- `sundc_t_tran_12`：用成交量中位数阈值区分大单，计算大买量与大卖量差占比
- `sundc_t_tran_13`：买侧成交次数标准差与卖侧成交次数标准差之比

### 计算逻辑

#### 初始化阶段
1. `timeSet`：最近5个成交/撤单时间点的有序集合

#### 更新阶段（Update方法）
对每个`Trade`或`Cancel`：
1. 获取时间：`millTime = trade->md_time`或`cancelOrder->md_time`
2. `timeSet.insert(millTime)`
3. 若`timeSet.size() > 5`：删除最早的时间点

#### 计算阶段（Calculate方法）
1. 若`timeSet`为空：所有因子返回0
2. 获取窗口起点：`startDate = *timeSet.begin()`
3. 从`fill_list`中提取窗口内的成交数据：
   - 初始化映射：
     - `tradeBuyMap`：买单编号到成交量的映射
     - `tradeSellMap`：卖单编号到成交量的映射
     - `tradeBuyCountMap`：买单编号到成交次数的映射
     - `tradeSellCountMap`：卖单编号到成交次数的映射
   - 从后向前遍历`fill_list`：
     - 若`fill->timestamp < startDate`：跳出
     - 累加买卖金额：`buyAmt += fill->amt`（若`fill->side == 1`）
     - 累加买卖金额：`sellAmt += fill->amt`（若`fill->side == 2`）
     - 更新映射：`merge(tradeBuyMap, buyNo, qty)`等
4. 计算`sundc_t_tran_6`：
   - `res2 = (buyAmt + sellAmt) / (high_price × free_float_capital)`
5. 计算`sundc_t_tran_12`：
   - 收集所有编号的成交量到`lVol`
   - 计算中位数：`mid = 2 × calcSortedMedian(lVol)`
   - 统计大单：
     - `lBigBuyVolSum`：大买单成交量总和
     - `lBigSellVolSum`：大卖单成交量总和
   - `res3 = (lBigBuyVolSum - lBigSellVolSum) / lvolSum`
6. 计算`sundc_t_tran_13`：
   - `sellStd = calcStd(tradeSellCountMap)`
   - 若`sellStd != 0 && !isnan(sellStd)`：
     - `buyStd = calcStd(tradeBuyCountMap)`
     - `res4 = buyStd / sellStd`
   - 否则：`res4 = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill_list | vector<Fill*> | MarketDataManager | 全量成交列表 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->amt | double | Fill结构 | 成交金额 |
| fill->side | int | Fill结构 | 买卖方向 |
| trade->md_time | uint64_t | Trade结构 | 成交时间戳 |
| cancel->md_time | uint64_t | Cancel结构 | 撤单时间戳 |
| high_price | double | MarketDataManager | 当日最高价 |
| free_float_capital | double | MarketDataManager | 自由流通股本（万股） |

### 代码实现位置
`因子/Sundc_T_Tran_6_13.h`

---

## 33. sundc_t_tran_14 - 窗口内委托编号成交次数偏度

### 因子说明
取最近5个成交/撤单时间点，统计窗口内各买/卖编号的成交次数并计算偏度，买卖两侧偏度之和为因子。

### 计算逻辑

#### 初始化阶段
1. `last5TimeList`：最近5个时间点的列表

#### 更新阶段（Update方法）
对每个`Trade`或`Cancel`：
1. 获取时间：`thisDate = trade->md_time`或`cancelOrder->md_time`
2. 若列表为空或时间大于最后一个时间：
   - `last5TimeList.emplace_back(thisDate)`
   - 若`last5TimeList.size() > 5`：删除最早的时间点

#### 计算阶段（Calculate方法）
1. 若`last5TimeList`为空：返回0
2. 获取窗口起点：`startTime = last5TimeList.front()`
3. 初始化映射：
   - `buyOrderCount`：买单编号到成交次数的映射
   - `sellOrderCount`：卖单编号到成交次数的映射
4. 从后向前遍历`fill_list`：
   - 若`fill->timestamp < startTime`：跳出
   - 累加次数：`buyOrderCount[buy_no]++`，`sellOrderCount[sell_no]++`
5. 计算偏度：
   - `buySkew = calcSkew(buyOrderCount, false)`
   - `sellSkew = calcSkew(sellOrderCount, false)`
   - `factor_value = buySkew + sellSkew`
6. 若为Inf或NaN：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill_list | vector<Fill*> | MarketDataManager | 全量成交列表 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| trade->md_time | uint64_t | Trade结构 | 成交时间戳 |
| cancel->md_time | uint64_t | Cancel结构 | 撤单时间戳 |

### 代码实现位置
`因子/Sundc_t_tran_14.h`

---

## 34. sundc_t_tran_18 - 涨停/非涨停成交编号对比

### 因子说明
统计涨停价成交与非涨停价成交的买卖编号集合，输出 `|ZT买|/(|ZT卖|+1) + |非ZT买|/(|非ZT卖|+1)`。

### 计算逻辑

#### 初始化阶段
1. `ztBuyNo`：涨停价成交的买方编号集合
2. `ztSellNo`：涨停价成交的卖方编号集合
3. `unztBuyNo`：非涨停价成交的买方编号集合
4. `unztSellNo`：非涨停价成交的卖方编号集合

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若`fill->price == high_limit_close`（涨停价）：
   - `ztBuyNo.emplace(fill->buy_no)`
   - `ztSellNo.emplace(fill->sell_no)`
2. 否则（非涨停价）：
   - `unztBuyNo.emplace(fill->buy_no)`
   - `unztSellNo.emplace(fill->sell_no)`

#### 计算阶段（Calculate方法）
1. `factor_value = ztBuyNo.size() / (ztSellNo.size() + 1.0) + unztBuyNo.size() / (unztSellNo.size() + 1.0)`
2. 若为NaN或Inf：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->price | double | Fill结构 | 成交价格 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| high_limit_close | double | MarketDataManager | 涨停价 |

### 代码实现位置
`因子/Sundc_t_tran_18_jup.h`

---

## 35. V_BLtNzR23 - 突破前1分钟买单成交时长非零占比

### 因子说明
统计`last_1min_trade_buy_map`中首尾成交时间不相同的买单数量，占所有买单数量的比例。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 若`last_1min_trade_buy_map`为空：返回0
2. 初始化：`tradeTimeNotZeroSize = 0`
3. 遍历`last_1min_trade_buy_map`：
   - 若`order.last_fill_time > order.first_fill_time`：
     - `tradeTimeNotZeroSize++`
4. 计算占比：`factor_value = tradeTimeNotZeroSize / tradeBuyMap.size()`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| last_1min_trade_buy_map | unordered_map<uint64_t, MarketOrder> | MarketDataManager | 近1分钟买单映射 |
| MarketOrder.first_fill_time | uint64_t | MarketOrder结构 | 订单首笔成交时间 |
| MarketOrder.last_fill_time | uint64_t | MarketOrder结构 | 订单末笔成交时间 |

### 代码实现位置
`因子/V_BLtNzR23.h`

---

## 36. V_BSVolRatio2 - 买卖累计量之比

### 因子说明
累加成交量并结合撤单量，分别形成买/卖侧总量；返回`totalSellQty/totalBuyQty`，空集返回0。

### 计算逻辑

#### 初始化阶段
1. `totalBuyQty = 0`：买方总成交量
2. `totalSellQty = 0`：卖方总成交量

#### 更新阶段（Update方法）
对每个`Trade`：
1. `totalBuyQty += trade->trade_qty`
2. `totalSellQty += trade->trade_qty`

对每个`Cancel`：
1. 若`cancel->side == '1'`（买方撤单）：
   - `totalBuyQty += cancel->order_qty`
2. 否则（卖方撤单）：
   - `totalSellQty += cancel->order_qty`

#### 计算阶段（Calculate方法）
1. 若`totalBuyQty == 0 || totalSellQty == 0`：返回0
2. `factor_value = totalSellQty / totalBuyQty`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| trade->trade_qty | uint64_t | Trade结构 | 成交数量 |
| cancel->order_qty | uint64_t | Cancel结构 | 撤单数量 |
| cancel->side | char | Cancel结构 | 买卖方向（'1'=买，'2'=卖） |

### 代码实现位置
`因子/V_BSVolRatio2.h`

---

## 37. V_WAmtSLt23 - 突破前1分钟卖单成交时长加权均值

### 因子说明
对`last_1min_trade_sell_map`中的卖单，以成交金额为权重计算`fill_time_delta`的加权平均；无数据返回0。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 若`last_1min_trade_sell_map`为空：返回0
2. 初始化累加变量：
   - `totalAmt = 0`：总成交金额
   - `totalTimeMultiAmt = 0`：时间×金额的累加
3. 遍历`last_1min_trade_sell_map`：
   - `totalAmt += order.amt`
   - `totalTimeMultiAmt += order.fill_time_delta × order.amt`
4. 计算加权平均：`factor_value = totalAmt == 0 ? 0 : totalTimeMultiAmt / totalAmt`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| last_1min_trade_sell_map | unordered_map<uint64_t, MarketOrder> | MarketDataManager | 近1分钟卖单映射 |
| MarketOrder.fill_time_delta | uint64_t | MarketOrder结构 | 订单成交持续时间（毫秒） |
| MarketOrder.amt | double | MarketOrder结构 | 订单总成交金额 |

### 代码实现位置
`因子/V_WAmtSLt23.h`

---

## 38. V_vtpcBact23_4 - 主动买成交VWAP相对昨收偏离

### 因子说明
在`last_1min_trade_list`中挑选对手编号买方大于卖方的成交，计算这些成交的VWAP与昨收之差（%），注册制标的再乘0.5。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 初始化累加变量：`totalAmt = 0`，`totalQty = 0`
2. 遍历`last_1min_trade_list`：
   - 若`trade_price > 0 && trade_buy_no > trade_sell_no`（主动买入）：
     - `totalAmt += trade_qty × trade_price`
     - `totalQty += trade_qty`
3. 若`totalQty != 0`：
   - 计算VWAP：`vwap = totalAmt / totalQty`
   - 计算偏离百分比：`factorValue = (vwap / pre_close - 1) × 100`
4. 若为注册制：`factorValue = factorValue × 0.5`
5. 否则：`factorValue = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| last_1min_trade_list | vector<Trade*> | MarketDataManager | 近1分钟成交列表 |
| trade->trade_price | double | Trade结构 | 成交价格 |
| trade->trade_qty | uint64_t | Trade结构 | 成交数量 |
| trade->trade_buy_no | uint64_t | Trade结构 | 买方编号 |
| trade->trade_sell_no | uint64_t | Trade结构 | 卖方编号 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |

### 代码实现位置
`因子/V_vtpcBact23_4.h`

---

## 39. Volume_pctchg_corr_linear / Relative_total_strength / Relative_total_strength_linear_0085 - 成交量-收益相关与相对强度

### 因子说明
使用`lxjj_time_info_list`构建成交量与收益序列：更新时递推线性回归，得到收益与成交量的相关性偏离、累计成交量×收益占比，以及与0.085阈值的偏离。输出三个因子：
- `Volume_pctchg_corr_linear`：相关性差值
- `Relative_total_strength`：相对强度
- `Relative_total_strength_linear_0085`：相对强度与0.085差值

### 计算逻辑

#### 初始化阶段
1. `regression`：线性回归计算器
2. `index = 0`：当前处理的TimeInfo索引
3. `totalQtyMultiRetSum = 0`：累计成交量×收益的总和

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若`lxjj_time_info_list.size() > index + 1`：
   - 调用`updateHelp()`更新回归
   - `index++`

#### updateHelp方法
1. 获取当前TimeInfo：`timeInfo = lxjj_time_info_list[index]`
2. 计算收益率：
   - 若`index == 0`：`lastPrice = jhjj_price`
   - 否则：`lastPrice = timeInfo[index-1].price`
   - `ret = (timeInfo[index].price - lastPrice) / preClose`
3. 获取成交量：`qty = timeInfo[index].qty`
4. 更新回归：`regression.addData(qty, ret)`
5. 累加：`totalQtyMultiRetSum += qty × 100 × ret`

#### 计算阶段（Calculate方法）
1. 默认值：
   - `volumePctchgCorrLinear = 1`
   - `relativeTotalStrength = 0`
   - `relativeTotalStrengthLinear = 5`
2. 若`lxjj_fill_list`非空：
   - 获取当前TimeInfo：`timeInfo = lxjj_time_info_list[index]`
   - 计算当前收益率和成交量（同updateHelp）
   - 创建临时回归：`regressionNow.append(regression)`，`regressionNow.addData(qty, ret)`
   - 计算相关性差值：`volumePctchgCorrLinear = abs(0.2 - regressionNow.getR())`
   - 计算相对强度：`relativeTotalStrength = (totalQtyMultiRetSum + qty × 100 × ret) / total_qty`
   - 计算与阈值差值：`relativeTotalStrengthLinear = abs(0.085 - relativeTotalStrength)`
3. 若为NaN：使用默认值

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_fill_list | vector<Fill*> | MarketDataManager | 连续竞价成交列表 |
| lxjj_time_info_list | vector<TimeInfo> | MarketDataManager | 时间信息列表 |
| TimeInfo.price | double | TimeInfo结构 | 价格 |
| TimeInfo.qty | uint64_t | TimeInfo结构 | 成交量 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| jhjj_price | double | MarketDataManager | 集合竞价价格 |
| total_qty | uint64_t | MarketDataManager | 当日累计成交量 |

### 代码实现位置
`因子/Volume_pctchg_corr_linear.h`

---

## 40. wd_t_10_itvl_max - 涨停前10秒最大成交间隔

### 因子说明
集合所有成交时间，回溯涨停触发前10秒区间，输出该区间内相邻成交的最大间隔，若无合规间隔则给默认1200毫秒。

### 计算逻辑

#### 初始化阶段
1. `timeSet`：所有成交时间戳的有序集合
2. 初始化：`timeSet.insert(93000000)`

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. `timeSet.emplace(fill->timestamp)`

#### 计算阶段（Calculate方法）
1. 获取涨停时间：`ztTime = GetZTLocalTime()`
2. 计算窗口起点：`startTime = FunGetTime(ztTime, -10)`
3. 初始化：`timeDeltaMax = 0`，`nextMd = optional`
4. 从后向前遍历`timeSet`：
   - 若`nextMd`有值：
     - 计算时间差：`delta = CalTimeDelta(*itr, *nextMd)`
     - 更新最大值：`timeDeltaMax = max(timeDeltaMax, delta)`
   - 若`*itr < startTime`：跳出
   - `nextMd = *itr`
5. 若`timeDeltaMax != 0 && timeDeltaMax <= 100000`：
   - `factor_value = timeDeltaMax`
6. 否则：`factor_value = 1200`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| GetZTLocalTime | function | MarketDataManager | 获取涨停触发本地时间 |

### 代码实现位置
`因子/Wd_t_10_itvl_max.h`

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
| high_price | double | 当日最高价（元） |
| high_limit_close | double | 涨停价（元） |
| jhjj_price | double | 集合竞价价格（元） |
| free_float_capital | double | 自由流通股本（万股） |
| fill_list | vector<Fill*> | 全量成交列表 |
| lxjj_fill_list | vector<Fill*> | 连续竞价成交列表 |
| lxjj_time_info_list | vector<TimeInfo> | 时间信息列表 |
| last_1min_trade_buy_map | unordered_map<uint64_t, MarketOrder> | 近1分钟买单映射 |
| last_1min_trade_sell_map | unordered_map<uint64_t, MarketOrder> | 近1分钟卖单映射 |
| last_1min_trade_list | vector<Trade*> | 近1分钟成交列表 |
| total_qty | uint64_t | 当日累计成交量 |

### MarketOrder结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| first_fill_time | uint64_t | 订单首笔成交时间 |
| last_fill_time | uint64_t | 订单末笔成交时间 |
| fill_time_delta | uint64_t | 订单成交持续时间（毫秒） |
| qty | uint64_t | 订单总成交量 |
| amt | double | 订单总成交金额 |

### Trade结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 成交时间戳 |
| trade_price | double | 成交价格 |
| trade_qty | uint64_t | 成交数量 |
| trade_buy_no | uint64_t | 买方编号 |
| trade_sell_no | uint64_t | 卖方编号 |

### Cancel结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 撤单时间戳 |
| order_qty | uint64_t | 撤单数量 |
| side | char | 买卖方向（'1'=买，'2'=卖） |

### TimeInfo结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| price | double | 价格 |
| qty | uint64_t | 成交量 |
| time | uint64_t | 时间戳 |

---

## 注意事项

1. **时间窗口**：部分因子使用时间窗口（如最近5个时间点、10秒窗口），需要正确维护窗口数据。

2. **集合操作**：使用集合去重时需要注意集合的大小和遍历顺序。

3. **异常值处理**：当计算结果为NaN或Inf时，通常返回默认值。

4. **加权计算**：部分因子使用加权平均（如金额加权），需要注意权重和总和的正确性。

5. **回归分析**：部分因子使用线性回归，需要正确维护回归状态。
