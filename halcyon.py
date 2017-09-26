from RPi import GPIO
TX = 24
RX = 26

GPIO.setmode(GPIO.BOARD)
GPIO.setup(RX, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(TX, GPIO.OUT)

def handle(pin):
    GPIO.output(TX, GPIO.input(pin))

GPIO.add_event_detect(RX, GPIO.BOTH, callback=handle)

import time

time.sleep(100)
