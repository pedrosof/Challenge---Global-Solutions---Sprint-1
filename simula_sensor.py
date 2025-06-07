import requests
import random
import time

# URL do backend
BACKEND_URL = "http://localhost:6000/prever"
API_KEY = "xxxxxxxx"  # Substitua pela sua chave

# Gera dados simulados como se fossem sensores reais
def gerar_dado_simulado():
    return {
        "umidade": round(random.uniform(10, 100), 2),        # em porcentagem
        "inclinacao": round(random.uniform(0, 45), 2),       # graus
        "chuva": round(random.uniform(0, 100), 2)            # em porcentagem
    }

# Envia os dados para o backend como o sensor faria
def enviar_simulacao(qtd=10, intervalo=2):
    for i in range(qtd):
        dado = gerar_dado_simulado()

        print(f"\n[{i+1}] LED ON → Coletando dados...")
        print(f"→ Enviando: {dado}")

        headers = {
            "x-api-key": API_KEY,
            "Content-Type": "application/json"
        }

        try:
            response = requests.post(BACKEND_URL, json=dado, headers=headers)
            if response.status_code == 200:
                print(f"✔️ Enviado com sucesso → Resposta: {response.json()}")
            else:
                print(f"❌ Erro HTTP {response.status_code} → {response.text}")
        except Exception as e:
            print(f"🚫 Erro de conexão: {e}")

        print(f"[{i+1}] LED OFF → Aguardando próxima leitura...\n")
        time.sleep(intervalo)

if __name__ == "__main__":
    enviar_simulacao(qtd=10, intervalo=1.5)
