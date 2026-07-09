import socket
import time
import threading
import math

HOST = "localhost"
PORT = 12345


def reader(sock):
    while True:
        try:
            data = sock.recv(4096)
            if not data:
                break
            print(data.decode(errors="ignore"), end="")
        except OSError:
            break


def send_line(sock, line):
    print(f">>> {line}")
    sock.sendall((line + "\n").encode())


def main():
    with socket.create_connection((HOST, PORT), timeout=5) as sock:
        print("Conectado à UART2 simulada.\n")

        t = threading.Thread(target=reader, args=(sock,), daemon=True)
        t.start()

        time.sleep(2)

        for i in range(20):
            px = 100.0 + i * 5.0
            py = 200.0 + i * 3.0

            m1 = 12.0 + 2.0 * math.sin(i * 0.3)
            m2 = 15.0 + 2.0 * math.sin(i * 0.3 + 1.0)
            m3 = 18.0 + 2.0 * math.sin(i * 0.3 + 2.0)

            ix = 2.1 + 0.1 * math.sin(i * 0.4)
            iy = 2.2 + 0.1 * math.sin(i * 0.4 + 1.0)
            iz = 2.3 + 0.1 * math.sin(i * 0.4 + 2.0)

            send_line(sock, f"P,{px:.2f},{py:.2f},0.0")
            send_line(sock, f"A,{m1:.2f},{m2:.2f},{m3:.2f}")
            send_line(sock, f"C,{ix:.2f},{iy:.2f},{iz:.2f}")

            if i == 5:
                send_line(sock, "n")
            if i == 10:
                send_line(sock, "n")
            if i == 15:
                send_line(sock, "n")

            time.sleep(1)

        print("\nFim da simulação de dados externos.")


if __name__ == "__main__":
    main()
