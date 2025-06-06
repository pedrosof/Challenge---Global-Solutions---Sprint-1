from flask import Flask, request, jsonify
import joblib
import sqlite3
import pandas as pd
import boto3
import configparser
from datetime import datetime, timedelta
from sklearn.ensemble import RandomForestClassifier

app = Flask(__name__)
model = joblib.load('modelo_risco_deslizamento.joblib')

# === Lê configurações AWS do config.ini ===
config = configparser.ConfigParser()
config.read('config.ini')

aws_access_key = config.get('aws', 'access_key')
aws_secret_key = config.get('aws', 'secret_key')
aws_region = config.get('aws', 'region')
sns_topic_arn = config.get('aws', 'sns_topic_arn')

# === Cliente SNS ===
sns_client = boto3.client(
    'sns',
    aws_access_key_id=aws_access_key,
    aws_secret_access_key=aws_secret_key,
    region_name=aws_region
)

DB_PATH = 'dados_alertai.db'
ALERTA_INTERVALO_MINUTOS = 30

def inicializar_tabelas():
    conn = sqlite3.connect(DB_PATH)
    cur = conn.cursor()
    cur.execute('''
        CREATE TABLE IF NOT EXISTS leituras (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            umidade REAL,
            inclinacao REAL,
            chuva REAL,
            risco TEXT
        )
    ''')
    cur.execute('''
        CREATE TABLE IF NOT EXISTS alertas (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            enviado_em DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    conn.commit()
    conn.close()

def pode_enviar_alerta():
    conn = sqlite3.connect(DB_PATH)
    cur = conn.cursor()
    cur.execute("SELECT enviado_em FROM alertas ORDER BY enviado_em DESC LIMIT 1")
    row = cur.fetchone()
    conn.close()
    if not row:
        return True
    ultimo_alerta = datetime.fromisoformat(row[0])
    return datetime.now() - ultimo_alerta >= timedelta(minutes=ALERTA_INTERVALO_MINUTOS)

def registrar_alerta():
    conn = sqlite3.connect(DB_PATH)
    cur = conn.cursor()
    cur.execute("INSERT INTO alertas (enviado_em) VALUES (CURRENT_TIMESTAMP)")
    conn.commit()
    conn.close()

def enviar_alerta_sns(data):
    mensagem = (
        f"⚠️ ALERTA DE RISCO DE DESLIZAMENTO ⚠️\n"
        f"Umidade: {data['umidade']}%\n"
        f"Inclinação: {data['inclinacao']}°\n"
        f"Chuva: {data['chuva']}%\n"
        f"Classificação: RISCO ALTO"
    )
    try:
        sns_client.publish(
            TopicArn=sns_topic_arn,
            Message=mensagem,
            Subject="ALERT-AI - Risco de Deslizamento"
        )
        print("[SNS] Alerta enviado com sucesso.")
        registrar_alerta()
    except Exception as e:
        print(f"[SNS] Falha ao enviar alerta: {e}")

def verificar_e_treinar_modelo():
    conn = sqlite3.connect(DB_PATH)
    df = pd.read_sql_query("SELECT umidade, inclinacao, chuva, risco FROM leituras", conn)
    conn.close()

    if len(df) > 0 and len(df) % 10 == 0:
        print(f"[MODELO] Re-treinando com {len(df)} registros...")
        df['risco'] = df['risco'].map({'baixo': 0, 'medio': 1, 'alto': 2})
        X = df[['umidade', 'inclinacao', 'chuva']]
        y = df['risco']

        novo_modelo = RandomForestClassifier(n_estimators=100, random_state=42)
        novo_modelo.fit(X, y)
        joblib.dump(novo_modelo, 'modelo_risco_deslizamento.joblib')

        global model
        model = novo_modelo
        print("[MODELO] Novo modelo salvo e carregado com sucesso.")

@app.route('/prever', methods=['POST'])
def prever():
    data = request.json

    entrada = pd.DataFrame([{
        'umidade': data['umidade'],
        'inclinacao': data['inclinacao'],
        'chuva': data['chuva']
    }])

    pred = model.predict(entrada)[0]
    risco_map = {0: "baixo", 1: "medio", 2: "alto"}
    resultado = risco_map[pred]

    # Grava leitura no banco
    conn = sqlite3.connect(DB_PATH)
    cur = conn.cursor()
    cur.execute('''
        INSERT INTO leituras (umidade, inclinacao, chuva, risco)
        VALUES (?, ?, ?, ?)
    ''', (data['umidade'], data['inclinacao'], data['chuva'], resultado))
    conn.commit()
    conn.close()

    # Treina modelo automaticamente a cada 10 leituras
    verificar_e_treinar_modelo()

    # Envia alerta via SNS se risco for alto E já passou intervalo
    if resultado == "alto" and pode_enviar_alerta():
        enviar_alerta_sns(data)

    return jsonify({
        'status': 'ok',
        'mensagem': f'Leitura registrada com risco {resultado}.'
    })

if __name__ == '__main__':
    inicializar_tabelas()
    app.run(host='0.0.0.0', port=6000)
