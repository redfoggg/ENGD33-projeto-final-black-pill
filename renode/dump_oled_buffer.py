import os

BUFFER_ADDR = 0x20001114
BUFFER_SIZE = 1024

PROJECT_DIR = r"""/home/joao-cerqueira/Documents/estudos/aaSemestres/9 sem/Tempo real - embarcados/Trabalho/ENGD33-projeto-final-black-pill"""
OUT_DIR = os.path.join(PROJECT_DIR, "simulation_logs")
OUT_FILE = os.path.join(OUT_DIR, "oled_framebuffer.bin")

if not os.path.exists(OUT_DIR):
    os.makedirs(OUT_DIR)

sysbus = monitor.Machine["sysbus"]

f = open(OUT_FILE, "wb")

for i in range(BUFFER_SIZE):
    value = int(sysbus.ReadByte(BUFFER_ADDR + i)) & 0xFF
    f.write(chr(value))

f.close()

print "OLED framebuffer salvo em %s" % OUT_FILE
