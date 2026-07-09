oled = monitor.Machine["sysbus.i2c1.oled"]

def log_i2c(data):
    values = [int(x) for x in data]
    head = " ".join(["%02X" % x for x in values[:24]])

    if len(values) > 24:
        print "OLED I2C WRITE len=%d data=%s ..." % (len(values), head)
    else:
        print "OLED I2C WRITE len=%d data=%s" % (len(values), head)

oled.DataReceived += log_i2c

print "OLED I2C logger instalado"
