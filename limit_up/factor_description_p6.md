# 打板因子算法文档 - 第六部分（因子51-52）

## 文档说明

本文档详细描述打板因子的计算逻辑，包括每个因子的具体计算步骤和依赖的逐笔成交数据字段。本部分涵盖因子51-52。

---

## 51. ZT_Time / sss_ZT_Time_ms / T_o2pre / sss_t_o2pre - 涨停触发时间与开盘偏离

### 因子说明
输出四个相关因子：
- `ZT_Time`：涨停触发时刻的原始时间戳
- `sss_ZT_Time_ms`：涨停触发时刻距9:30的毫秒差
- `T_o2pre`：开盘价相对昨收的收益率
- `sss_t_o2pre`：若为注册制则收益率除以2

### 计算逻辑

#### 计算阶段（Calculate方法）
1. `ZT_Time = reached_ZT_time`（原始时间戳，直接输出）
2. `sss_ZT_Time_ms = CalTimeDelta(Lxjj_AM_Start, ZT_local_time)`（距9:30的毫秒差）
3. `T_o2pre = open_price / pre_close - 1`（开盘收益率）
4. `sss_t_o2pre = zcz ? T_o2pre / 2 : T_o2pre`（注册制折半）

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| reached_ZT_time | uint64_t | MarketDataManager | 达到涨停的市场时间（毫秒） |
| ZT_local_time | uint64_t | MarketDataManager | 涨停触发本地时间（毫秒） |
| Lxjj_AM_Start | uint64_t | 常量 | 连续竞价开始时间（93000000） |
| open_price | double | MarketDataManager | 当日开盘价（元） |
| pre_close | double | MarketDataManager | 昨收盘价（元） |
| zcz | bool | MarketDataManager | 注册制标记 |

### 代码实现位置
`因子/ZT_Time_T_o2pre.h`

---

## 52. zwh_20230824_004 - 价格区间内高低位成交额对比

### 因子说明
设两个阈值：高位为昨收×(1.08或1.16)，低位为昨收×(0.94或0.88)；累计对应价区间的成交额，计算`(高位成交额 + 100×高位成交额标准差)/(0.1+总成交额)`并四舍五入。

### 计算逻辑

#### 初始化阶段
1. 根据注册制标记设置两个阈值：
   - 注册制：
     - `thres_price1 = floor(pre_close × 100 × 1.16 + 0.5) / 100`（高位）
     - `thres_price2 = floor(pre_close × 100 × 0.88 + 0.5) / 100`（低位）
   - 非注册制：
     - `thres_price1 = floor(pre_close × 100 × 1.08 + 0.5) / 100`
     - `thres_price2 = floor(pre_close × 100 × 0.94 + 0.5) / 100`
2. 初始化累加变量：
   - `total_sum = 0.0`：总成交额
   - `tp1_sum = 0.0`：高位成交额
   - `tp2_sum = 0.0`：低位成交额
   - `tp1_list`：高位成交额列表（用于计算标准差）

#### 逐笔更新阶段（Update方法）
对每笔成交`fill`：
1. 累加总成交额：`total_sum += fill->amt`
2. 若`fill->price >= thres_price1`（高位）：
   - `tp1_sum += fill->amt`
   - `tp1_list.push_back(fill->amt)`
3. 若`fill->price <= thres_price2`（低位）：
   - `tp2_sum += fill->amt`

#### 计算阶段（Calculate方法）
1. 若`tp1_list`非空：
   - 计算高位成交额标准差：`std = calcStd(tp1_list, true)`
   - 计算因子值：`factor_value = (tp1_sum + 100 × std) / (0.1 + total_sum)`
   - 四舍五入到5位小数：`factor_value = RoundHalfUp(factor_value, 5)`
2. 否则：`factor_value = 0.0`

### 依赖字段

| 字段名称 | 类型 | 来源 | 说明 |
|---------|------|------|------|
| pre_close | double | MarketDataManager | 昨收盘价 |
| zcz | bool | MarketDataManager | 注册制标记 |
| fill->price | double | Fill结构 | 成交价格 |
| fill->amt | double | Fill结构 | 成交金额（元） |

### 代码实现位置
`因子/Zwh_20230824_004.h`

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
| open_price | double | 当日开盘价（元） |
| ZT_local_time | uint64_t | 涨停触发本地时间（毫秒） |
| reached_ZT_time | uint64_t | 达到涨停的市场时间（毫秒） |

---

## 注意事项

1. **注册制处理**：创业板（代码以3开头）和科创板（代码以68开头）的涨跌幅限制为20%，非注册制为10%。部分因子需要对注册制股票进行特殊处理（如阈值调整、收益折半等）。

2. **时间戳格式**：时间戳为毫秒级整数，例如`93000000`表示09:30:00.000。

3. **精度处理**：价格和金额计算通常保留2-5位小数，使用`RoundHalfUp`进行四舍五入。

4. **异常值处理**：当计算结果为NaN或Inf时，通常返回默认值（如0等）。

5. **时间差计算**：使用`CalTimeDelta`计算时间差，返回毫秒数。

---

## 文档索引

- **第一部分（因子1-10）**：`factor_description_p1.md`
- **第二部分（因子11-20）**：`factor_description_p2.md`
- **第三部分（因子21-30）**：`factor_description_p3.md`
- **第四部分（因子31-40）**：`factor_description_p4.md`
- **第五部分（因子41-50）**：`factor_description_p5.md`
- **第六部分（因子51-52）**：`factor_description_p6.md`

---

## 总结

本文档完整描述了所有52个打板因子的计算逻辑，每个因子都包含：
- 因子说明
- 详细的计算步骤（初始化、更新、计算三个阶段）
- 依赖的逐笔成交数据字段
- 代码实现位置

所有因子均基于逐笔成交数据（Fill结构）和行情快照数据（Tick结构）进行计算，通过MarketDataManager统一管理数据流和状态信息。
