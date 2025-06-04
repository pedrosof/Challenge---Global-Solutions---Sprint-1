import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
import joblib

# Simula dados
data = pd.DataFrame({
    'umidade': [60, 75, 90, 85, 50, 30, 70, 95, 40, 80],
    'inclinacao': [10, 12, 20, 25, 8, 6, 18, 30, 5, 22],
    'chuva': [5, 10, 30, 40, 0, 0, 25, 50, 2, 35],
    'risco': ['baixo', 'medio', 'alto', 'alto', 'baixo', 'baixo', 'medio', 'alto', 'baixo', 'alto']
})

data['risco'] = data['risco'].map({'baixo': 0, 'medio': 1, 'alto': 2})

X = data[['umidade', 'inclinacao', 'chuva']]
y = data['risco']

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

model = RandomForestClassifier(n_estimators=100, random_state=42)
model.fit(X_train, y_train)

# Salva o modelo no arquivo
joblib.dump(model, 'modelo_risco_deslizamento.joblib')
print("Modelo salvo como modelo_risco_deslizamento.joblib")
