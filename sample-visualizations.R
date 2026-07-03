# ============================================================
# サンプル可視化スクリプト（RStudio 用）
# データ: airquality（1973年ニューヨークの大気観測・base R 同梱）
# 必要: install.packages("tidyverse")  # 初回のみ
# ============================================================

library(ggplot2)
library(dplyr)

# ── データ準備：折れ線グラフ用に日付列を追加
data(airquality)
aq <- airquality %>%
  mutate(Date = as.Date(paste(1973, Month, Day, sep = "-")))

# 1. ヒストグラム：気温の分布 --------------------------------
ggplot(aq, aes(x = Temp)) +
  geom_histogram(binwidth = 3, fill = "#1E6FD0", color = "white") +
  labs(title = "気温の分布", x = "気温 (°F)", y = "日数")

# 2. 棒グラフ：月別の平均オゾン濃度 --------------------------
aq %>%
  group_by(Month) %>%
  summarise(Ozone_mean = mean(Ozone, na.rm = TRUE)) %>%
  ggplot(aes(x = factor(Month), y = Ozone_mean)) +
  geom_col(fill = "#12857A") +
  labs(title = "月別の平均オゾン濃度", x = "月", y = "平均オゾン (ppb)")

# 3. 折れ線グラフ：気温の時系列 ------------------------------
ggplot(aq, aes(x = Date, y = Temp)) +
  geom_line(color = "#E07B34") +
  labs(title = "気温の推移", x = "日付", y = "気温 (°F)")

# 4. 散布図：気温 vs オゾン ----------------------------------
ggplot(aq, aes(x = Temp, y = Ozone)) +
  geom_point(color = "#3B3E9E", alpha = 0.7) +
  labs(title = "気温とオゾンの関係", x = "気温 (°F)", y = "オゾン (ppb)")

# 5. 散布図 + 回帰直線 ---------------------------------------
ggplot(aq, aes(x = Temp, y = Ozone)) +
  geom_point(alpha = 0.6) +
  geom_smooth(method = "lm", se = TRUE, color = "#8E3B5E") +
  labs(title = "回帰：オゾン ~ 気温", x = "気温 (°F)", y = "オゾン (ppb)")

# 回帰係数・決定係数の確認
summary(lm(Ozone ~ Temp, data = aq))
