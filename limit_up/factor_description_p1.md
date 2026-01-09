# 打板因子算法文档 - 第一部分（因子1-10）

## 文档说明

本文档详细描述打板因子的计算逻辑，包括每个因子的具体计算步骤和依赖的逐笔成交数据字段。本部分涵盖因子1-10。

---

## 1. xly_t_prup_qty_ratio - 涨停价及以上成交量占比  已成

### 因子说明
计算涨停价及以上成交量的占比，反映价格在高位的成交强度。

### 计算逻辑

#### 初始化阶段
1. 根据注册制标记`zcz`确定涨停阈值：
   - 若`zcz == true`（注册制）：`threshold = RoundHalfUp(pre_close × 1.12, 4)`
   - 若`zcz == false`（非注册制）：`threshold = RoundHalfUp(pre_close × 1.06, 4)`
2. 初始化累加变量：`sum = 0.0`，`high_sum = 0.0`

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 累加总成交量：`sum += fill->qty`
2. 若成交价达到或超过阈值：`if (fill->price >= threshold) { high_sum += fill->qty }`

#### 计算阶段（Calculate方法）
1. 若`sum != 0`：`factor_value = high_sum / sum`
2. 若`sum == 0`：`factor_value = 0.0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记（创业板/科创板） |
| fill->price | double | Fill结构 | 成交价格 |
| fill->qty | uint64_t | Fill结构 | 成交数量（股） |

### 代码实现位置
`因子/factor_xly_t_prup_qty_ratio.h`

---

## 2. xly_t_prupt_uni_ratio - 涨停触发价位上独立价档占比  已成

### 因子说明
计算首次触及涨停价时及之后，所有达到涨停价的独立价档数量占全部成交价档的比例。

### 计算逻辑

#### 初始化阶段
1. 根据注册制标记确定涨停价（同xly_t_prup_qty_ratio）
2. 初始化集合：
   - `trade_prices`：所有成交价的去重集合
   - `high_trade_prices`：达到涨停价的成交价集合
   - `tmp_trade_prices`：当前时间戳内的成交价临时集合
3. `first_reach = false`：标记是否首次触及涨停价
4. `pre_md_time = -1`：上一笔成交的时间戳

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 将成交价四舍五入到4位小数：`price = RoundHalfUp(fill->price, 4)`
2. 将价格加入总集合：`trade_prices.emplace(price)`
3. 若尚未首次触及涨停价（`!first_reach`）：
   - 若时间戳变化：清空`tmp_trade_prices`并加入当前价格
   - 若时间戳相同：将当前价格加入`tmp_trade_prices`
   - 若`price >= threshold`：
     - 设置`first_reach = true`
     - 将`tmp_trade_prices`中的所有价格加入`high_trade_prices`
4. 若已首次触及涨停价（`first_reach == true`）：
   - 若`price >= threshold`：将价格加入`high_trade_prices`
5. 更新`pre_md_time = fill->timestamp`

#### 计算阶段（Calculate方法）
1. 若`trade_prices`非空：`factor_value = high_trade_prices.size() / trade_prices.size()`
2. 若`trade_prices`为空：`factor_value = 0.0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| fill->price | double | Fill结构 | 成交价格 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳（毫秒） |

### 代码实现位置
`因子/factor_xly_t_prupt_uni_ratio.h`

---

## 3. sss_turnsum_b10_p7 - 突破阈值后的买单"冲击量"近10段合计/流通股本

### 因子说明
计算突破阈值后，连续同编号买单的成交量，维护最近10段，计算段量之和占自由流通股本的比例。

### 计算逻辑

#### 初始化阶段
1. 根据注册制标记计算阈值：
   - `zcz = marketDataManager->zcz ? 1 : 0`
   - `price = floor(pre_close × 100 × (1.07 + 0.07 × zcz) + 0.5) / 100`
2. 初始化状态变量：
   - `lastFlag = 0`：上一笔成交的方向
   - `qty = 0`：当前段的累计成交量
   - `isAbove = false`：是否已突破阈值
   - `lastTradeNo = 0`：上一笔的订单编号
   - `buyIndex = 0`：循环数组索引
   - `buyTurn[10]`：存储最近10段的成交量（初始为NAN）

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若已突破阈值或当前价格达到阈值（`isAbove || fill->price >= price`）：
   - 设置`isAbove = true`
   - 若上一笔是买单（`lastFlag == 1`）：
     - 获取当前订单编号：`tradeNo = fill->side == 1 ? fill->buy_no : fill->sell_no`
     - 若编号相同（`tradeNo == lastTradeNo`）：
       - 累加成交量：`qty += fill->qty`
     - 若编号不同：
       - 将当前段量存入数组：`buyTurn[buyIndex] = qty`
       - 更新索引（循环）：`buyIndex = (buyIndex != 9) ? buyIndex + 1 : 0`
       - 更新编号和段量：`lastTradeNo = tradeNo`，`qty = fill->qty`
   - 若上一笔不是买单：
     - 初始化新段：`qty = fill->qty`，`lastTradeNo = fill->side == 1 ? fill->buy_no : fill->sell_no`
   - 更新方向：`lastFlag = fill->side`

#### 计算阶段（Calculate方法）
1. 复制数组：`buyTurnCalc = buyTurn`
2. 若最新一笔是买单（`last_fill->side == 1`）：
   - 将当前段量存入：`buyTurnCalc[buyIndex] = qty`
3. 计算总和（忽略NAN）：`sum = calcSum(buyTurnCalc, false, true)`
4. 计算因子值：`factor_value = sum × 100 / free_float_capital`
5. 若为NaN：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| fill->price | double | Fill结构 | 成交价格 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| fill->side | int | Fill结构 | 买卖方向（1=买，2=卖） |
| free_float_capital | double | MarketDataManager | 自由流通股本（万股） |
| last_fill | Fill* | MarketDataManager | 最新一笔成交 |

### 代码实现位置
`因子/sss_turnsum_b10_p7.h`

---

## 4. zwh_20230831_005 - 成交额与(买挂量+成交量)之比相对昨收的偏离

### 因子说明
计算最新tick的成交额与(买挂量+成交量)的比值，相对昨收盘价的偏离。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 获取tick列表：`tick_list = marketDataManager->quote_list`
2. 计算截断时间：`endTime = AddMilliseconds(ZT_local_time, -tickTimeLag)`
3. 从后向前遍历tick列表：
   - 若`tick->md_time >= 93000000 && tick->md_time <= endTime`：
     - 计算比值：`res = tick->total_amount / (tick->bid_order_qty + tick->total_volume)`
     - 计算相对昨收偏离：`factor_value = res / pre_close - 1`
     - 若为注册制：`factor_value = factor_value / 2`
     - 跳出循环

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->total_amount | double | Tick结构 | 累计成交额 |
| tick->total_volume | uint64_t | Tick结构 | 累计成交量 |
| tick->bid_order_qty | uint64_t | Tick结构 | 买盘挂单总量 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |

### 代码实现位置
`因子/Zwh_20230831_005.h`

---

## 5. sss_to_pricediffstd_all_s60 - 近60秒委托价相对成交价的加权标准差

### 因子说明
计算近60秒内未成交委托价格相对最近成交价格的价差，按委托量加权计算标准差。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 计算时间阈值：`time_threshold = max(FunGetTime(ZT_local_time, -60), 93000000)`
2. 初始化变量：
   - `curr_timestamp = MAX_UINT64`：当前处理的时间戳
   - `trade_price = 0`：最近成交价
   - `order_qty_sum = 0`：委托量总和
   - `pricediff_times_order_qty_sum = 0`：价差×委托量的累加
   - `pricediff_list`：价差列表
3. 从后向前遍历委托列表`lxjj_order_list`：
   - 若`order->md_time < time_threshold`：跳出
   - 若时间戳变化（`order->md_time <= curr_timestamp`）：
     - 从成交列表中找到该时间戳对应的成交，计算VWAP作为`trade_price`
     - 若找不到：使用`pre_close`
   - 计算价差（百分比）：
     - `pricediff = (order->order_price - trade_price) / pre_close × 100`
     - 若为注册制：`pricediff = pricediff / 2`
   - 累加：`pricediff_list.push_back(pricediff)`
   - `pricediff_times_order_qty_sum += pricediff × order->order_qty`
   - `order_qty_sum += order->order_qty`
4. 若`order_qty_sum == 0`：返回0
5. 计算加权平均价差：`vwap = pricediff_times_order_qty_sum / order_qty_sum`
6. 计算加权方差：
   - `numerator = Σ((pricediff[i] - vwap)² × order_qty[i])`
7. 计算标准差：`factor_value = sqrt(numerator / order_qty_sum)`
8. 若为NaN或Inf：返回0

### 依赖字段
| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_order_list | vector<TOrder*> | MarketDataManager | 连续竞价委托列表 |
| order->order_price | double | TOrder结构 | 委托价格 |
| order->order_qty | uint64_t | TOrder结构 | 委托数量 |
| order->md_time | uint64_t | TOrder结构 | 委托时间戳 |
| fill_list | vector<Fill*> | MarketDataManager | 全量成交列表 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->amt | double | Fill结构 | 成交金额 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |

### 代码实现位置
`因子/Sss_to_pricediffstd_all_s60.h`

---

## 6. yzhan_hf_f2_53 - 上涨成交量波动 - 全体成交量波动

### 因子说明
维护全部成交量和涨价成交量的对数成交量序列，分别计算50笔滚动标准差，输出涨价序列标准差减去总体标准差。

### 计算逻辑

#### 初始化阶段
1. 初始化变量：
   - `vol_count = 0`：全部成交量计数
   - `vol_filter_count = 0`：涨价成交量计数
   - `vol_sum = 0`：全部对数成交量总和
   - `vol_filter_sum = 0`：涨价对数成交量总和
   - `vol_list`：全部对数成交量列表
   - `vol_filter_list`：涨价对数成交量列表
   - `vol_std`：全部成交量标准差计算器
   - `vol_filter_std`：涨价成交量标准差计算器

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 计算对数成交量：`vol = log(fill->qty)`
2. 全部成交量处理：
   - 加入列表：`vol_list.emplace_back(vol)`
   - 累加：`vol_sum += vol`，`vol_count++`
   - 若`vol_count == 50`：计算均值并更新标准差
   - 若`vol_count > 50`：移除最旧的值，更新标准差
3. 若价格上涨（`fill->ret > 0`）：
   - 加入列表：`vol_filter_list.emplace_back(vol)`
   - 累加：`vol_filter_sum += vol`，`vol_filter_count++`
   - 若`vol_filter_count == 50`：计算均值并更新标准差
   - 若`vol_filter_count > 50`：移除最旧的值，更新标准差

#### 计算阶段（Calculate方法）
1. 若`vol_filter_std.getN() > 1`：
   - `factor_value = vol_filter_std.getResult() - vol_std.getResult()`
2. 否则：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->ret | double | Fill结构 | 相对上一笔的收益率 |

### 代码实现位置
`因子/Yzhan_hf_f2_53.h`

---

## 7. Vwap - 当日VWAP相对昨收的偏离（%） 已成

### 因子说明
计算当日成交量加权平均价格（VWAP）相对昨收盘价的偏离百分比。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 获取连续竞价累计数据：
   - `lxjjTotalAmt`：连续竞价成交总金额
   - `lxjjTotalQty`：连续竞价成交总量
2. 若`pre_close != 0 && lxjjTotalQty != 0`：
   - 计算VWAP：`vwap = lxjjTotalAmt / lxjjTotalQty`
   - 计算偏离百分比：`factorValue = (vwap / pre_close - 1) × 100`
   - 若为NaN：`factorValue = 0`
3. 若为注册制：`factorValue = factorValue × 0.53`
4. 否则：`factorValue = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_total_amt | double | MarketDataManager | 连续竞价成交总金额 |
| lxjj_total_qty | uint64_t | MarketDataManager | 连续竞价成交总量 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |

### 代码实现位置
`因子/Vwap.h`

---

## 8. sss_after_pbtimemax_7 - 首次上穿7%线后回落至阈值下方的最长停留时长

### 因子说明
计算首次上穿7%阈值后，价格回落至阈值下方的最长连续停留时长（毫秒）。

### 计算逻辑

#### 初始化阶段
1. 根据注册制标记计算阈值：
   - `zcz = starts_with3 || starts_with68 ? 1 : 0`
   - `linePrice = floor(pre_close × 100 × (1 + 0.01 × 7 × (1 + zcz)) + 0.5) / 100.0`
2. 初始化状态变量：
   - `isAbove = false`：是否已上穿阈值
   - `lastOpenTime = 0`：开始停留在阈值下方的时间
   - `lastCloseTime = 0`：结束停留在阈值下方的时间
   - `lastFillPrice = NAN`：上一笔成交价
   - `maxMs = 0`：最大停留时长

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若已上穿阈值（`isAbove == true`）：
   - 若从阈值上方跌破（`lastFillPrice >= linePrice && fill->price < linePrice`）：
     - 若`lastOpenTime != 0`：更新最大时长
     - 记录开始时间：`lastOpenTime = fill->timestamp`
   - 若从阈值下方涨回（`lastFillPrice < linePrice && fill->price >= linePrice`）：
     - 记录结束时间：`lastCloseTime = fill->timestamp`
   - 更新：`lastFillPrice = fill->price`
2. 若尚未上穿且当前价格达到阈值（`!isAbove && fill->price >= linePrice`）：
   - 设置`isAbove = true`
   - `lastFillPrice = fill->price`

#### 计算阶段（Calculate方法）
1. 若最后状态仍在阈值下方且最新价格在阈值上方：
   - 更新`lastCloseTime = last_fill->timestamp`
2. 若`lastOpenTime != 0`：
   - 计算当前停留时长：`currentMs = CalTimeDelta(lastOpenTime, lastCloseTime)`
   - 更新最大值：`maxMs = max(maxMs, currentMs)`
3. 若为NaN：`maxMs = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| pre_close | double | MarketDataManager | 昨收盘价 |
| starts_with3 | bool | MarketDataManager | 创业板标记 |
| starts_with68 | bool | MarketDataManager | 科创板标记 |
| fill->price | double | Fill结构 | 成交价格 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| last_fill | Fill* | MarketDataManager | 最新一笔成交 |

### 代码实现位置
`因子/Sss_after_pbtimemax_7.h`

---

## 9. yzhan_hf_f1_27 - 卖出成交额的50笔滚动波动

### 因子说明
仅对卖出成交累加对数成交额，维护50笔滚动窗口计算标准差。

### 计算逻辑

#### 初始化阶段
1. 初始化变量：
   - `count = 0`：计数
   - `sum = 0`：对数成交额总和
   - `amt_filter_list`：对数成交额列表
   - `amt_std`：标准差计算器

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若为卖出成交（`fill->side == 2`）：
   - 计算对数成交额：`value = log(fill->amt)`
   - 加入列表：`amt_filter_list.emplace_back(value)`
   - 累加：`sum += value`，`count++`
   - 若`count == 50`：计算均值并更新标准差
   - 若`count > 50`：移除最旧的值，更新标准差

#### 计算阶段（Calculate方法）
1. 若`amt_filter_list.size() >= 50`：
   - `factor_value = amt_std.getResult()`
   - 若为NaN：`factor_value = 0`
2. 否则：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->amt | double | Fill结构 | 成交金额 |
| fill->side | int | Fill结构 | 买卖方向（1=买，2=卖） |

### 代码实现位置
`因子/Yzhan_hf_f1_27.h`

---

## 10. sundc_t_tran_16_fix - 主动买卖平均量差/总量  完成

### 因子说明
对主动买/卖分别累计成交量与订单数，计算平均量差并除以总成交量。

### 计算逻辑

#### 初始化阶段
1. 初始化累加变量：
   - `activeBuyVol = 0`：主动买成交量
   - `activeSellVol = 0`：主动卖成交量
   - `activeBuySet`：主动买订单编号集合
   - `activeSellSet`：主动卖订单编号集合

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若为主动买（`fill->side == 1`）：
   - `activeBuyVol += fill->qty`
   - `activeBuySet.emplace(fill->buy_no)`
2. 若为主动卖（`fill->side == 2`）：
   - `activeSellVol += fill->qty`
   - `activeSellSet.emplace(fill->sell_no)`

#### 计算阶段（Calculate方法）
1. 获取总成交量：`lxjjTotalQty = marketDataManager->lxjj_total_qty`
2. 若`lxjjTotalQty > 0`：
   - 计算平均量差：
     - `avgBuyVol = activeBuyVol / (activeBuySet.size() + 1)`
     - `avgSellVol = activeSellVol / (activeSellSet.size() + 1)`
     - `factor_value = (avgBuyVol - avgSellVol) / lxjjTotalQty`
3. 若为Inf或NaN：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->sell_no | uint64_t | Fill结构 | 卖方订单号 |
| fill->side | int | Fill结构 | 买卖方向（1=买，2=卖） |
| lxjj_total_qty | uint64_t | MarketDataManager | 连续竞价成交总量 |

### 代码实现位置
`因子/Sundc_t_tran_16_fix.h`

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
| timestamp | uint64_t | 成交时间戳（毫秒，如93000000表示09:30:00） |
| side | int | 买卖方向（1=买方主动，2=卖方主动） |
| ret | double | 相对上一笔价格的收益率 |

### MarketDataManager主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| pre_close | double | 昨收盘价（元） |
| zcz | bool | 注册制标记（创业板/科创板） |
| starts_with3 | bool | 创业板标记 |
| starts_with68 | bool | 科创板标记 |
| free_float_capital | double | 自由流通股本（万股） |
| high_limit_close | double | 涨停价（元） |
| high_price | double | 当日最高价（元） |
| open_price | double | 当日开盘价（元） |
| jhjj_price | double | 集合竞价价格（元） |
| ZT_local_time | uint64_t | 涨停触发本地时间（毫秒） |
| reached_ZT_time | uint64_t | 达到涨停的市场时间（毫秒） |
| lxjj_total_amt | double | 连续竞价成交总金额（元） |
| lxjj_total_qty | uint64_t | 连续竞价成交总量（股） |
| lxjj_fill_list | vector<Fill*> | 连续竞价成交列表 |
| fill_list | vector<Fill*> | 全量成交列表 |
| lxjj_order_list | vector<TOrder*> | 连续竞价委托列表 |
| quote_list | vector<Tick*> | 行情快照列表 |
| last_fill | Fill* | 最新一笔成交 |

### Tick结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 行情时间戳（毫秒） |
| total_amount | double | 累计成交额（元） |
| total_volume | uint64_t | 累计成交量（股） |
| bid_order_qty | uint64_t | 买盘挂单总量（股） |
| ask_order_qty | uint64_t | 卖盘挂单总量（股） |
| last_px | double | 最新成交价（元） |

### TOrder结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 委托时间戳（毫秒） |
| order_price | double | 委托价格（元） |
| order_qty | uint64_t | 委托数量（股） |
| order_type | char | 委托类型 |
| side | char | 买卖方向（'1'=买，'2'=卖） |

---

## 注意事项

1. **注册制处理**：创业板（代码以3开头）和科创板（代码以68开头）的涨跌幅限制为20%，非注册制为10%。部分因子需要对注册制股票进行特殊处理（如阈值调整、收益折半等）。

2. **时间戳格式**：时间戳为毫秒级整数，例如`93000000`表示09:30:00.000。

3. **精度处理**：价格和金额计算通常保留2-4位小数，使用`RoundHalfUp`进行四舍五入。

4. **异常值处理**：当计算结果为NaN或Inf时，通常返回默认值（如0、0.2、0.3等）。

5. **数据窗口**：部分因子使用滚动窗口（如近1分钟、近5秒），需要维护窗口内的数据。

6. **主动买卖判断**：通常通过`buy_no`和`sell_no`的大小关系判断，`buy_no > sell_no`表示主动买，`buy_no < sell_no`表示主动卖。
