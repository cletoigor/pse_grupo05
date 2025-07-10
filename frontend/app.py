import streamlit as st
import paho.mqtt.client as mqtt
import pandas as pd
import json
from datetime import datetime
import os
from streamlit_autorefresh import st_autorefresh

# --- Configuração MQTT ---
MQTT_BROKER = "f98b1789.ala.dedicated.azure.emqxcloud.com"
MQTT_PORT = 8883
MQTT_USERNAME = "admin2"
MQTT_PASSWORD = "Admin@@@"
MQTT_CERT_PATH = "emqx.pem"
MQTT_TOPIC_ANGULO = "esp32/angulo"
MQTT_TOPIC_COMANDO = "esp32/comando"

# --- Armazenamento de Dados ---
DATA_FILE = "dados_historicos.csv"

# --- Estado da Aplicação Streamlit ---
if 'dados_atuais' not in st.session_state:
    st.session_state.dados_atuais = {'angle': 0.0, 'azimute': 0.0, 'manual': False, 'rele': 0}
if not os.path.exists(DATA_FILE):
    # Cria o arquivo com cabeçalho se não existir
    pd.DataFrame(columns=["timestamp", "angulo", "azimute", "manual", "rele"]).to_csv(DATA_FILE, index=False)
if 'historico' not in st.session_state:
    st.session_state.historico = pd.read_csv(DATA_FILE)

if 'mqtt_client' not in st.session_state:
    st.session_state.mqtt_client = None
if 'operating_mode' not in st.session_state:
    st.session_state.operating_mode = "Manual" # Default to Manual mode

def on_connect(client, userdata, flags, rc, properties=None):
    """Callback para quando o cliente se conecta ao broker."""
    if rc == 0:
        print("Conectado ao Broker MQTT!")
        client.subscribe(MQTT_TOPIC_ANGULO)
    else:
        print(f"Falha ao conectar, código de retorno {rc}\n")

def on_message(client, userdata, msg, properties=None):
    """Callback para quando uma mensagem é recebida do broker."""
    print("----------")  # Barra de separação para cada mensagem recebida
    print(f"Mensagem recebida no tópico {msg.topic}: {msg.payload.decode()}")
    if msg.topic == MQTT_TOPIC_ANGULO:
        try:
            # Espera payload no formato: float;float;bool;1/0
            partes = msg.payload.decode().split(";")
            if len(partes) == 4:
                angulo = float(partes[0])
                azimute = float(partes[1])
                manual = partes[2].strip().lower() in ['true', '1']
                rele = int(partes[3])

                nova_linha = pd.DataFrame([{
                    "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                    "angulo": angulo,
                    "azimute": azimute,
                    "manual": manual,
                    "rele": rele
                }])

                # Salva no arquivo CSV (append)
                nova_linha.to_csv(DATA_FILE, mode='a', header=False, index=False)
                # No direct Streamlit session_state update here due to threading
            else:
                print("Payload recebido não possui 4 campos esperados.")
        except ValueError:
            print("Erro ao converter o payload para os tipos esperados.")
        except Exception as e:
            print(f"Ocorreu um erro em on_message: {e}")

def setup_mqtt_client():
    """Configura e retorna um cliente MQTT."""
    client = mqtt.Client(protocol=mqtt.MQTTv311)
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.tls_set(ca_certs=MQTT_CERT_PATH)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
    return client

# --- UI Streamlit ---
st.set_page_config(page_title="Dashboard Painel Solar", layout="wide", initial_sidebar_state="expanded")

# Auto-refresh every 0.5 seconds (500 milliseconds)
st_autorefresh(interval=500, key="data_refresh")

# --- Lógica Principal da Aplicação ---
if st.session_state.mqtt_client is None:
    st.session_state.mqtt_client = setup_mqtt_client()

client = st.session_state.mqtt_client

# Function to load/reload data from CSV
def load_data_from_csv():
    try:
        historico_df = pd.read_csv(DATA_FILE)
        if not historico_df.empty:
            ultimo = historico_df.iloc[-1]
            st.session_state.dados_atuais = {
                'angle': ultimo['angulo'],
                'azimute': ultimo['azimute'],
                'manual': ultimo['manual'],
                'rele': ultimo['rele']
            }
            st.session_state.historico = historico_df
        else:
            st.session_state.dados_atuais = {'angle': 0.0, 'azimute': 0.0, 'manual': False, 'rele': 0}
            st.session_state.historico = historico_df
    except Exception as e:
        st.session_state.dados_atuais = {'angle': 0.0, 'azimute': 0.0, 'manual': False, 'rele': 0}
        st.session_state.historico = pd.DataFrame(columns=["timestamp", "angulo", "azimute", "manual", "rele"])

# Initial load of data
load_data_from_csv()

# --- Barra Lateral (Sidebar) ---
with st.sidebar:
    # Imagem do painel solar pequena no topo da barra lateral
    st.image("painel.png", width=120)
    st.header("🕹️ Controles")
    
    st.subheader("Modo de Operação")
    
    # Buttons for mode selection
    col_mode1, col_mode2 = st.columns(2)
    with col_mode1:
        if st.button("Ativar Modo Automático", disabled=(st.session_state.operating_mode == "Automático")):
            st.session_state.operating_mode = "Automático"
            client.publish(MQTT_TOPIC_COMANDO, "A")
            st.success("Modo alterado para Automático.")
            st.rerun() # Rerun to update button state and UI
    with col_mode2:
        if st.button("Ativar Modo Manual", disabled=(st.session_state.operating_mode == "Manual")):
            st.session_state.operating_mode = "Manual"
            client.publish(MQTT_TOPIC_COMANDO, "M")
            st.success("Modo alterado para Manual.")
            st.rerun() # Rerun to update button state and UI

    st.info(f"Modo atual: **{st.session_state.operating_mode}**")
    
    st.divider()

    if st.session_state.operating_mode == "Manual":
        with st.container():
            st.subheader("Definir Ângulo Manualmente")
            angulo_manual = st.slider("Ângulo (°)", 0, 180, 90, help="Arraste para selecionar o ângulo desejado.")
            if st.button("Enviar Ângulo"):
                client.publish(MQTT_TOPIC_COMANDO, str(angulo_manual))
                st.success(f"Comando de ângulo enviado: {angulo_manual}")

    # Removed the manual refresh button
    # st.divider()
    # if st.button("Atualizar Dados"):
    #     load_data_from_csv()
    #     st.rerun() # Force a rerun to update the display

# --- Página Principal ---
st.title("☀️ Dashboard de Controle do Painel Solar")

# --- Seções de Exibição ---
st.subheader("Status Atual")
col1, col2, col3, col4 = st.columns(4)

with col1:
    st.metric("Ângulo Atual (°)", f"{st.session_state.dados_atuais.get('angle', 0.0):.2f}")
with col2:
    st.metric("Azimute (°)", f"{st.session_state.dados_atuais.get('azimute', 0.0):.2f}")
with col3:
    modo_automatico = not st.session_state.dados_atuais.get('manual', False)
    cor_led = "#21c521" if modo_automatico else "#d11a1a"
    texto_modo = "Automático" if modo_automatico else "Manual"
    st.markdown(
        f"""
        <div style="display: flex; align-items: center; gap: 0.5em;">
            <span style="font-weight: 600;">Modo Automático</span>
            <span style="width: 18px; height: 18px; border-radius: 50%; background: {cor_led}; display: inline-block; border: 2px solid #888;"></span>
        </div>
        <div style="font-size: 0.9em; color: #666;">{texto_modo}</div>
        """,
        unsafe_allow_html=True
    )
with col4:
    rele_val = st.session_state.dados_atuais.get('rele', 0)
    cor_led_rele = "#21c521" if rele_val else "#d11a1a"
    texto_rele = "Ligado" if rele_val else "Desligado"
    st.markdown(
        f"""
        <div style="display: flex; align-items: center; gap: 0.5em;">
            <span style="font-weight: 600;">Rele</span>
            <span style="width: 18px; height: 18px; border-radius: 50%; background: {cor_led_rele}; display: inline-block; border: 2px solid #888;"></span>
        </div>
        <div style="font-size: 0.9em; color: #666;">{texto_rele}</div>
        """,
        unsafe_allow_html=True
    )

st.divider()

st.subheader("Histórico de Ângulos")
historico_df = st.session_state.historico

if not historico_df.empty:
    chart_container = st.container()
    with chart_container:
        st.line_chart(
            historico_df.set_index('timestamp')[['angulo', 'azimute']]
        )
    with st.expander("Ver Tabela de Dados Históricos"):
        st.dataframe(
            historico_df.sort_values(by="timestamp", ascending=False),
            use_container_width=True
        )
else:
    st.info("Nenhum dado histórico disponível ainda. Aguardando dados do painel...")

# Mantém a aplicação rodando
# O loop do MQTT está rodando em segundo plano.

# Rodapé
st.markdown(
    """
    <hr style="margin-top:2em; margin-bottom:0.5em;">
    <div style="text-align: center; color: #888; font-size: 0.95em;">
        Desenvolvido por Igor Cleto e Matheus Galbiatti
    </div>
    """,
    unsafe_allow_html=True
)
