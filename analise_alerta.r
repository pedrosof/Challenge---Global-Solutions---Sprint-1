# options(repos = c(CRAN = "https://cran.r-project.org"))
# install.packages("DBI")
# install.packages("RSQLite")
# install.packages("ggplot2")

library(DBI)
library(RSQLite)
library(ggplot2)

db_path <- "dados_alertai.db"
con <- dbConnect(RSQLite::SQLite(), db_path)

# Lê dados
dados <- dbGetQuery(con, "SELECT * FROM leituras")

# Garante tabela de gráficos
dbExecute(con, "
  CREATE TABLE IF NOT EXISTS graficos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nome TEXT UNIQUE,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    imagem BLOB
  )
")

# === Função para salvar gráfico como BLOB ===
salvar_grafico <- function(nome, grafico) {
  path <- tempfile(fileext = ".png")
  png(path, width = 800, height = 600)
  print(grafico)
  dev.off()

  bin <- readBin(path, what = "raw", n = file.info(path)$size)
  dbExecute(con, "DELETE FROM graficos WHERE nome = ?", params = list(nome))
  dbExecute(con, "INSERT INTO graficos (nome, imagem) VALUES (?, ?)", 
            params = list(nome, list(bin)))
}

# === Gráfico 1: Dispersão chuva x inclinação ===
g1 <- ggplot(dados, aes(x = chuva, y = inclinacao, color = risco)) +
  geom_point(size = 3) +
  labs(title = "Chuva vs Inclinação por Risco",
       x = "Chuva (%)", y = "Inclinação (°)", color = "Risco") +
  theme_minimal()
salvar_grafico("grafico_risco", g1)

# === Gráfico 2: Boxplot de umidade por risco ===
g2 <- ggplot(dados, aes(x = risco, y = umidade, fill = risco)) +
  geom_boxplot() +
  labs(title = "Umidade do Solo por Classificação de Risco",
       x = "Risco", y = "Umidade (%)") +
  theme_minimal()
salvar_grafico("grafico_umidade_boxplot", g2)

# === Gráfico 3: Histograma de umidade geral ===
g3 <- ggplot(dados, aes(x = umidade)) +
  geom_histogram(binwidth = 10, fill = "#4682B4", color = "white") +
  labs(title = "Distribuição da Umidade do Solo",
       x = "Umidade (%)", y = "Frequência") +
  theme_minimal()
salvar_grafico("grafico_umidade_hist", g3)

g4 <- ggplot(dados, aes(x = chuva, y = inclinacao)) +
  stat_density_2d(aes(fill = after_stat(level)), geom = "polygon") +
  geom_point(aes(color = risco), size = 2) +
  scale_fill_viridis_c() +
  labs(title = "Densidade de Ocorrência por Classe de Risco",
       x = "Chuva (%)", y = "Inclinação (°)", fill = "Densidade") +
  theme_minimal()
salvar_grafico("grafico_densidade_decisao", g4)

importancias <- dbGetQuery(con, "SELECT * FROM importancias")

g_importancias <- ggplot(importancias, aes(x = reorder(variavel, importancia), y = importancia)) +
  geom_col(fill = "#2E8B57") +
  labs(title = "Importância das Variáveis no Modelo",
       x = "Variável", y = "Importância") +
  coord_flip() +
  theme_minimal()

salvar_grafico("grafico_importancia_variaveis", g_importancias)

# Finaliza
dbDisconnect(con)
cat("Gráficos salvos no banco de dados com sucesso.\n")
