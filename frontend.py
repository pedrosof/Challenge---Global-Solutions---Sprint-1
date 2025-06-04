from flask import Flask, render_template_string
import sqlite3
import os

app = Flask(__name__)

DB_PATH = "dados_alertai.db"
STATIC_DIR = "static"

GRAFICOS = {
    "grafico_risco": "grafico_risco.png",
    "grafico_umidade_boxplot": "grafico_umidade_boxplot.png",
    "grafico_umidade_hist": "grafico_umidade_hist.png"
}

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Dashboard ALERT-AI</title>
</head>
<body style="text-align:center; font-family:Arial;">
    <h1>Dashboard ALERT-AI</h1>
    {% for nome, arquivo in graficos.items() %}
        <h2>{{ nome.replace('_', ' ').title() }}</h2>
        {% if arquivos_existentes[nome] %}
            <img src="/static/{{ arquivo }}" width="800"><br><br>
        {% else %}
            <p style="color:red;">{{ nome }} não encontrado no banco de dados.</p>
        {% endif %}
    {% endfor %}
</body>
</html>
"""

def recuperar_graficos():
    arquivos_existentes = {nome: False for nome in GRAFICOS}

    if not os.path.exists(DB_PATH):
        print("[ERRO] Banco não encontrado.")
        return arquivos_existentes

    os.makedirs(STATIC_DIR, exist_ok=True)

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()

        for nome, filename in GRAFICOS.items():
            cursor.execute("SELECT imagem FROM graficos WHERE nome = ?", (nome,))
            row = cursor.fetchone()
            if row and row[0]:
                with open(os.path.join(STATIC_DIR, filename), "wb") as f:
                    f.write(row[0])
                arquivos_existentes[nome] = True

        conn.close()
    except Exception as e:
        print(f"[ERRO] Falha ao recuperar gráficos: {e}")

    return arquivos_existentes

@app.route("/")
def index():
    arquivos_ok = recuperar_graficos()
    return render_template_string(HTML_TEMPLATE, graficos=GRAFICOS, arquivos_existentes=arquivos_ok)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)
