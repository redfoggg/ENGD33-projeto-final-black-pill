import socket
import time
from datetime import datetime
from pathlib import Path

HOST = "localhost"
PORT = 12345

LOG_DIR = Path("simulation_logs")


def read_available(sock, duration=2.0):
    end_time = time.time() + duration
    data = b""

    while time.time() < end_time:
        try:
            chunk = sock.recv(4096)
            if chunk:
                data += chunk
        except socket.timeout:
            pass

    text = data.decode(errors="ignore")

    if text:
        print(text, end="")

    return text


def send_command(sock, command):
    msg = f"\n>>> Enviando comando: {command!r}\n"
    print(msg, end="")
    sock.sendall(command.encode())
    return msg


def main():
    LOG_DIR.mkdir(exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = LOG_DIR / f"renode_uart_test_{timestamp}.txt"

    full_log = ""

    header = (
        "TESTE AUTOMATICO RENODE + STM32 + FREERTOS\n"
        f"Data/hora: {datetime.now()}\n"
        "Objetivo: validar troca de telas via UART de simulacao\n"
        "Sequencia esperada: SCREEN1 -> SCREEN2 -> SCREEN3 -> SCREEN1\n"
        "\n"
    )

    print(header)
    full_log += header

    print("Conectando na UART2 simulada do Renode...\n")

    with socket.create_connection((HOST, PORT), timeout=5) as sock:
        sock.settimeout(0.2)

        print("Conectado.\n")
        full_log += "Conectado na UART2 simulada.\n\n"

        # Saída inicial
        text = read_available(sock, duration=3.0)
        full_log += text

        # SCREEN1 -> SCREEN2
        full_log += send_command(sock, "n\n")
        text = read_available(sock, duration=3.0)
        full_log += text

        # SCREEN2 -> SCREEN3
        full_log += send_command(sock, "n\n")
        text = read_available(sock, duration=3.0)
        full_log += text

        # SCREEN3 -> SCREEN1
        full_log += send_command(sock, "n\n")
        text = read_available(sock, duration=3.0)
        full_log += text

    checks = {
        "SCREEN1": "SCREEN1" in full_log,
        "SCREEN2": "SCREEN2" in full_log,
        "SCREEN3": "SCREEN3" in full_log,
        "SIM CMD": "SIM CMD | next screen" in full_log,
    }

    summary = "\n\n===== RESUMO DO TESTE =====\n"

    for name, passed in checks.items():
        status = "OK" if passed else "FALHOU"
        summary += f"{name}: {status}\n"

    if all(checks.values()):
        summary += "\nRESULTADO FINAL: TESTE APROVADO\n"
    else:
        summary += "\nRESULTADO FINAL: TESTE COM FALHA\n"

    print(summary)
    full_log += summary

    log_path.write_text(full_log, encoding="utf-8")

    print(f"Log salvo em: {log_path}")


if __name__ == "__main__":
    main()
