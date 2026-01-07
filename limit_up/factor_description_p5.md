# 打板因子算法文档 - 第五部分（因子41-50）

## 文档说明

本文档详细描述打板因子的计算逻辑，包括每个因子的具体计算步骤和依赖的逐笔成交数据字段。本部分涵盖因子41-50。

---

## 41. wd_t_big_pct_max - 前一档最低价到当前最高价的最大涨幅

### 因子说明
逐毫秒跟踪同一时间点的高低价，计算相邻时间段最高价相对前一个时间段最小价的最大涨幅；注册制按收益减半；若仍在同一段内则与最新价比较。

### 计算逻辑

#### 初始化阶段
1. `timePoint = 0`：当前时间点
2. `maxPrice = 0`：当前时间点的最高价
3. `minPrice = DBL_MAX`：当前时间点的最低价
4. `preMinPrice = -1`：前一个时间点的最低价
5. `times`：时间点列表
6. `ratios`：涨幅列表

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 获取时间戳：`md = fill->timestamp`
2. 若`timePoint == 0`（首次）：
   - 初始化：`maxPrice = fill->price`，`minPrice = fill->price`，`timePoint = md`
3. 若时间戳相同（`md == timePoint`）：
   - 更新：`maxPrice = max(maxPrice, fill->price)`，`minPrice = min(minPrice, fill->price)`
4. 若时间戳不同：
   - 若`preMinPrice != -1`：
     - 计算涨幅：
       - 注册制：`ratio = ((maxPrice/pre_close-1)/2+1) / ((preMinPrice/pre_close-1)/2+1) - 1`
       - 非注册制：`ratio = maxPrice / preMinPrice - 1`
     - 记录到列表：`times.push_back(timePoint)`，`ratios.push_back(ratio)`
   - 更新：`timePoint = md`，`preMinPrice = minPrice`，`maxPrice = fill->price`，`minPrice = fill->price`

#### 计算阶段（Calculate方法）
1. 若`preMinPrice == -1`：返回`0.0065`
2. 获取涨停前5秒的窗口：`startTime = FunGetTime(ztTime, -5)`
3. 遍历窗口内的涨幅，取最大值
4. 计算当前时间段的涨幅并比较
5. 返回最大涨幅

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->price | double | Fill结构 | 成交价格 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |

### 代码实现位置
`因子/Wd_t_big_pct_max.h`

---

## 42. wj_TTick_20_bsv4 - 动态窗口买挂差距信噪比

### 因子说明
根据涨停时间和延迟确定窗口，按秒级自适应长度收集`(ask_order_qty - total_volume +1)/(total_volume+1)`，计算均值与标准差，输出`(mean+1e-4)/(std+1e-4)`。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 找到有效tick范围：
   - `start_index`：第一个时间>=93000000的tick索引
   - `trunc_time = AddMilliseconds(ZT_local_time, -tickTimeLag)`
   - `trunc_size`：最后一个时间<=trunc_time的tick索引
2. 若`trunc_size <= start_index`：返回0
3. 获取窗口起点时间：`start_time = quote_list[trunc_size-1]->md_time`
4. 若`start_time <= 93100000`：返回0
5. 根据时间确定窗口长度：
   - 若`start_time <= 93500000`：`sec_delta = 40`
   - 否则若`start_time <= 100000000`：`sec_delta = 240`
   - 否则：`sec_delta = 1200`
6. 计算窗口起点：`start_time = max(FunGetTime(end_time, -sec_delta), 93000000)`
7. 收集数据：
   - 遍历tick（从`start_index`到`trunc_size`）
   - 若`tick->md_time > start_time`：
     - 计算比值：`ratio = (ask_order_qty - total_volume + 1) / (total_volume + 1)`
     - 加入列表：`ret_pct_o.push_back(ratio)`
8. 计算均值和标准差：`mean = calcMean(ret_pct_o)`，`std = calcStd(ret_pct_o, true, false, true, mean)`
9. 计算因子值：`factor_value = (mean + 1e-4) / (std + 1e-4)`
10. 若为NaN或Inf：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| tick->ask_order_qty | uint64_t | Tick结构 | 卖盘挂单总量 |
| tick->total_volume | uint64_t | Tick结构 | 累计成交量 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |

### 代码实现位置
`因子/Wj_TTick_20_bsv4.h`

---

## 43. wwd_t1_corrl_pv - 最近1分钟买单均价与数量相关

### 因子说明
聚合最近1分钟每个买编号的总金额/总量得到均价，再与对数数量做回归，返回皮尔逊相关系数。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 若`last_1min_fill_list`为空：返回0
2. 初始化映射：`buyNoAmtQty`（买编号到DataPair的映射）
   - `DataPair.left`：总金额
   - `DataPair.right`：总数量
3. 遍历`last_1min_fill_list`：
   - 对每个`fill`：`merge(buyNoAmtQty, fill->buy_no, fill->amt, fill->qty)`
4. 初始化回归：`regression = SimpleRegression`
5. 遍历`buyNoAmtQty`：
   - 计算均价：`avgPrice = itr.second.left / itr.second.right`
   - 计算对数数量：`logQty = log(itr.second.right)`
   - 添加数据：`regression.addData(avgPrice, logQty)`
6. 计算相关系数：`factorValue = regression.getR()`
7. 若为NaN：`factorValue = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| last_1min_fill_list | vector<Fill*> | MarketDataManager | 近1分钟成交列表 |
| fill->buy_no | uint64_t | Fill结构 | 买方订单号 |
| fill->amt | double | Fill结构 | 成交金额 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |

### 代码实现位置
`因子/Wwd_t1_corrl_pv.h`

---

## 44. xly_t_order_xa13 - 买单时间片上的卖单VWAP-数量相关性

### 因子说明
9:30后按`md_time/100000`聚合订单：卖单累加金额和数量，买单记录时间片；`Calculate`阶段从最近的30个买单时间片中，取同步存在卖单的片段构造卖单数量序列与对应VWAP，并计算二者的皮尔逊相关系数，缺样本返回0。

### 计算逻辑

#### 初始化阶段
1. `sellMap`：时间片（md_time/100000）到OrderInfo的映射
   - `OrderInfo.amt`：累计金额
   - `OrderInfo.qty`：累计数量
2. `buySet`：买单时间片集合

#### 更新阶段（Update方法）
对每个委托`order`：
1. 若`order->md_time >= 93000000`：
   - 计算时间片：`min = order->md_time / 100000`
   - 若为卖单（`order->side == '2'`）：
     - 查找或创建该时间片的OrderInfo
     - `amt += order->order_qty × order->order_price`
     - `qty += order->order_qty`
   - 若为买单（`order->side == '1'`）：
     - `buySet.insert(min)`

#### 计算阶段（Calculate方法）
1. 初始化：`counter = 0`，`orderQty`，`vwap`（预留30个元素）
2. 从`buySet`的末尾开始，取最近30个时间片：
   - 若该时间片在`sellMap`中存在：
     - `orderQty.push_back(sellMap[min].qty)`
     - `vwap.push_back(sellMap[min].amt / sellMap[min].qty)`
     - `counter++`
   - 若`counter == 30`：跳出
3. 计算皮尔逊相关系数：`value = PearsonCorrelation(orderQty, vwap)`
4. 若为NaN：`value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_order_list | vector<TOrder*> | MarketDataManager | 连续竞价委托列表 |
| order->md_time | uint64_t | TOrder结构 | 委托时间戳 |
| order->side | char | TOrder结构 | 买卖方向（'1'=买，'2'=卖） |
| order->order_qty | uint64_t | TOrder结构 | 委托数量 |
| order->order_price | double | TOrder结构 | 委托价格 |

### 代码实现位置
`因子/xly_t_order_xa13.h`

---

## 45. xbc_20230525_2 - 成交价与近邻买均价偏离对比

### 因子说明
以成交时间匹配同一3秒桶内的行情，累加`(成交价 - tick.bid_avg_px)×成交量/(free_float_capital×pre_close)`，最多统计100次，少于5次输出0，最后取平均。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 计算截断时间：`end_time = AddMilliseconds(ZT_local_time, -tickTimeLag)`
2. 构建tick时间映射：`tick_time_map`（按3秒桶分组：`md_time / 3000`）
3. 遍历`lxjj_fill_list`（从后向前）：
   - 计算时间桶：`md = fill->timestamp / 3000`
   - 查找该时间桶的tick列表
   - 对每个tick：
     - 计算偏离：`diff = (fill->price - tick->bid_avg_px) × fill->qty / (free_float_capital × pre_close)`
     - 累加：`sum += diff`，`cnt++`
     - 若`cnt >= 100`：跳出
4. 若`cnt < 5`：`factorVal = 0`
5. 否则：`factorVal = sum / cnt`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_fill_list | vector<Fill*> | MarketDataManager | 连续竞价成交列表 |
| fill->price | double | Fill结构 | 成交价格 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->timestamp | uint64_t | Fill结构 | 成交时间戳 |
| quote_list | vector<Tick*> | MarketDataManager | 行情快照列表 |
| tick->bid_avg_px | double | Tick结构 | 平均委买价格 |
| tick->md_time | uint64_t | Tick结构 | 行情时间戳 |
| free_float_capital | double | MarketDataManager | 自由流通股本（万股） |
| pre_close | double | MarketDataManager | 昨收盘价 |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发时间 |
| params->tickTimeLag | int | EventDrivenParams | 时间延迟参数 |

### 代码实现位置
`因子/Xbc_20230525_2.h`

---

## 46. yzhan_hf_ak2_40 - 高累计持仓相关性差值（量基准）

### 因子说明
在最后成交序列中取cum_trade_qty1占比>0.98与>0.96部分，分别计算成交量占比与成交额占比的相关系数，输出两者差值。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 若`lxjj_fill_list`为空：返回0
2. 获取总量：
   - `trade_qt1_sum = last_fill->cum_trade_qty1`
   - `trade_qty_sum = lxjj_total_qty`
   - `tradeMoneySum = lxjj_total_amt`
3. 初始化回归：`regression98`，`regression96`
4. 从后向前遍历`lxjj_fill_list`：
   - 若`fill->cum_trade_qty1 / trade_qt1_sum > 0.98`：
     - `regression98.addData(fill->qty / trade_qty_sum, fill->amt / tradeMoneySum)`
     - `regression96.addData(fill->qty / trade_qty_sum, fill->amt / tradeMoneySum)`
   - 否则若`fill->cum_trade_qty1 / trade_qt1_sum > 0.96`：
     - `regression96.addData(fill->qty / trade_qty_sum, fill->amt / tradeMoneySum)`
   - 否则：跳出
5. 计算相关系数：
   - `correlation98 = regression98.getR()`
   - `correlation96 = regression96.getR()`
6. `factor_value = (isnan(correlation98) ? 0 : correlation98) - (isnan(correlation96) ? 0 : correlation96)`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_fill_list | vector<Fill*> | MarketDataManager | 连续竞价成交列表 |
| fill->cum_trade_qty1 | uint64_t | Fill结构 | 累计买方成交量 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->amt | double | Fill结构 | 成交金额 |
| lxjj_total_qty | uint64_t | MarketDataManager | 连续竞价成交总量 |
| lxjj_total_amt | double | MarketDataManager | 连续竞价成交总金额 |
| last_fill | Fill* | MarketDataManager | 最新一笔成交 |

### 代码实现位置
`因子/Yzhan_hf_ak2_40.h`

---

## 47. yzhan_hf_ak4_40 - 高累计持仓相关性差值（金额基准）

### 因子说明
与`yzhan_hf_ak2_40`类似，但使用`cum_trade_money`占比>0.98与>0.96的成交，比较相关系数差值。

### 计算逻辑

#### 计算阶段（Calculate方法）
1. 若`lxjj_fill_list`为空：返回0
2. 获取总量：
   - `trade_qty_sum = lxjj_total_qty`
   - `trade_money_sum = lxjj_total_amt`
3. 初始化回归：`regression98`，`regression96`
4. 从后向前遍历`lxjj_fill_list`：
   - 若`fill->cum_trade_money / trade_money_sum > 0.98`：
     - `regression98.addData(fill->qty / trade_qty_sum, fill->amt / trade_money_sum)`
     - `regression96.addData(fill->qty / trade_qty_sum, fill->amt / trade_money_sum)`
   - 否则若`fill->cum_trade_money / trade_money_sum > 0.96`：
     - `regression96.addData(fill->qty / trade_qty_sum, fill->amt / trade_money_sum)`
   - 否则：跳出
5. 计算相关系数差值（同yzhan_hf_ak2_40）

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| lxjj_fill_list | vector<Fill*> | MarketDataManager | 连续竞价成交列表 |
| fill->cum_trade_money | double | Fill结构 | 累计成交金额 |
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->amt | double | Fill结构 | 成交金额 |
| lxjj_total_qty | uint64_t | MarketDataManager | 连续竞价成交总量 |
| lxjj_total_amt | double | MarketDataManager | 连续竞价成交总金额 |

### 代码实现位置
`因子/Yzhan_hf_ak4_40.h`

---

## 48. yzhan_hf_b2_38 - 涨价成交方向波动

### 因子说明
收集`ret>0`成交的买卖方向序列，滚动窗口保留最多32个，计算方向去均值后的标准差，序列不足10个时输出0。

### 计算逻辑

#### 初始化阶段
1. `count = 0`：计数
2. `sum = 0`：方向值总和
3. `filter_bs_flags`：方向序列列表
4. `diff_std`：标准差计算器

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若价格上涨（`fill->ret > 0`）：
   - 获取方向：`bs = fill->side`
   - 加入列表：`filter_bs_flags.emplace_back(bs)`
   - 累加：`sum += bs`，`count++`
   - 若`count < 10`：跳过
   - 否则若`count <= 32`：
     - `diff_std.increment(bs - sum / count)`
   - 否则（`count > 32`）：
     - 移除最旧的值：`sum -= filter_bs_flags[count - 33]`
     - `diff_std.increment(bs - sum / 32)`

#### 计算阶段（Calculate方法）
1. 若`filter_bs_flags.size() >= 10`：
   - `factor_value = diff_std.getResult()`
2. 否则：`factor_value = 0`
3. 若为NaN：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->ret | double | Fill结构 | 相对上一笔的收益率 |
| fill->side | int | Fill结构 | 买卖方向（1=买，2=卖） |

### 代码实现位置
`因子/Yzhan_hf_b2_38.h`

---

## 49. yzhan_hf_c0_1 - 成交收益序列的标准化偏度

### 因子说明
累积非NaN收益，计算样本均值，并用三阶中心矩与二阶中心矩组合得到带样本修正的偏度；若收益完全相同则输出0。

### 计算逻辑

#### 初始化阶段
1. `isSame = true`：是否所有收益相同
2. `lastRet = NAN`：上一笔收益
3. `sum = 0`：收益总和
4. `rtnList`：收益列表

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 若`fill->ret`不为NaN：
   - `rtnList.push_back(fill->ret)`
   - `sum += fill->ret`
   - 若`isSame == true`：
     - 若`lastRet`不为NaN且`ret != lastRet`：
       - `isSame = false`
     - `lastRet = ret`

#### 计算阶段（Calculate方法）
1. 若`rtnList.size() <= 2 || isSame`：返回0
2. 计算均值：`mean = sum / size`
3. 计算中心矩：
   - `m3 = 0`：三阶中心矩
   - `m2 = 0`：二阶中心矩
   - 遍历收益：`diff = i - mean`，`m2 += diff²`，`m3 += diff³`
   - `m3 /= size`，`m2 /= size`
4. 计算偏度：`factorVal = m3 / pow(m2, 1.5) × sqrt(size) × sqrt(size-1) / (size-2)`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->ret | double | Fill结构 | 相对上一笔的收益率 |

### 代码实现位置
`因子/Yzhan_hf_c0_1.h`

---

## 50. yzhan_hf_d6_46_fix - 长短期对比的对数成交量差

### 因子说明
对每笔成交取log(qty)，累计所有成交；若距当前≥128笔且过去价格更高，则同时累计子序列并记录数量，最终输出子序列均值与总体均值之差。

### 计算逻辑

#### 初始化阶段
1. `vol0_sum = 0`：全部对数成交量总和
2. `vol1_sum = 0`：符合条件的子序列对数成交量总和
3. `cnt1 = 0`：子序列数量
4. `length = 0`：总成交数量

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 计算对数成交量：`vol = log(fill->qty)`
2. 累加全部：`vol0_sum += vol`，`length++`
3. 若`length >= 128`：
   - 获取128笔前的成交：`oldFill = fill_list[length - 128]`
   - 若`oldFill->price / fill->price - 1 > 0`（过去价格更高）：
     - `vol1_sum += vol`
     - `cnt1++`

#### 计算阶段（Calculate方法）
1. 若`cnt1 > 0`：
   - `factor_value = vol1_sum / cnt1 - vol0_sum / length`
2. 否则：`factor_value = NAN`
3. 若为NaN：`factor_value = 0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| fill->qty | uint64_t | Fill结构 | 成交数量 |
| fill->price | double | Fill结构 | 成交价格 |
| fill_list | vector<Fill*> | MarketDataManager | 全量成交列表 |

### 代码实现位置
`因子/Yzhan_hf_d6_46_fix.h`

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
| ret | double | 相对上一笔的收益率 |
| cum_trade_qty1 | uint64_t | 累计买方成交量 |
| cum_trade_money | double | 累计成交金额 |

### MarketDataManager主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| pre_close | double | 昨收盘价（元） |
| zcz | bool | 注册制标记 |
| free_float_capital | double | 自由流通股本（万股） |
| ZT_local_time | uint64_t | 涨停触发本地时间（毫秒） |
| lxjj_fill_list | vector<Fill*> | 连续竞价成交列表 |
| fill_list | vector<Fill*> | 全量成交列表 |
| lxjj_total_qty | uint64_t | 连续竞价成交总量（股） |
| lxjj_total_amt | double | 连续竞价成交总金额（元） |
| last_1min_fill_list | vector<Fill*> | 近1分钟成交列表 |
| lxjj_order_list | vector<TOrder*> | 连续竞价委托列表 |
| quote_list | vector<Tick*> | 行情快照列表 |
| last_fill | Fill* | 最新一笔成交 |
| params | EventDrivenParams* | 运行参数 |

### TOrder结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 委托时间戳（毫秒） |
| order_price | double | 委托价格（元） |
| order_qty | uint64_t | 委托数量（股） |
| side | char | 买卖方向（'1'=买，'2'=卖） |

### Tick结构主要字段

| 字段名称 | 类型 | 说明 |
|---------|------|------|
| md_time | uint64_t | 行情时间戳（毫秒） |
| ask_order_qty | uint64_t | 卖盘挂单总量（股） |
| total_volume | uint64_t | 累计成交量（股） |
| bid_avg_px | double | 平均委买价格（元） |

---

## 注意事项

1. **时间桶聚合**：部分因子按时间桶聚合（如3秒桶、10秒桶），需要注意时间戳的转换。

2. **滚动窗口**：部分因子使用滚动窗口（如32笔、128笔），需要正确维护窗口数据。

3. **相关性计算**：部分因子计算相关系数，需要注意样本数量和数据质量。

4. **偏度计算**：偏度计算需要足够的样本量，且需要处理边界情况。

5. **异常值处理**：当计算结果为NaN或Inf时，通常返回默认值。
