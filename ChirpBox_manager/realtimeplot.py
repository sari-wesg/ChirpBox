import matplotlib.pyplot as plt
from drawnow import drawnow
import numpy as np
import transfer_to_initiator.myserial.serial_send


def make_fig():
    ax = plt.gca()

    line1, = plt.plot(x_audio, y_audio, 'b-', label='Audio')
    line2, = plt.plot(x_envelope, y_envelope, 'r--', label='Envelope')
    # line3, = plt.plot(x_gate, y_gate, 'k.-', label='Gate')

    plt.xlabel('time',fontsize=12)
    plt.ylabel('Analog voltage',fontsize=12)

    legend = plt.legend(loc='upper right', edgecolor='k',fontsize = 8, fancybox=True, ncol=3)
    plt.title('Loudness sensor value', fontsize=12)
    figManager = plt.get_current_fig_manager()
    figManager.window.showMaximized()

# plt.legend(loc='upper right')
# plt.ion()  # enable interactivity
# fig = plt.figure()  # make a figure


x_audio = list()
y_audio = list()
x_envelope = list()
y_envelope = list()
x_gate = list()
y_gate = list()

# config and open the serial port
com_port = 'com7'
ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
i = 0
while True:
    line = ser.readline().decode('ascii').strip()  # skip the empty data
    audio = 0
    envelope = 0
    gate = 0
    if line:
        if line.startswith("audio"):
            try:
                strings = line.split(',')
                audio = float(strings[0].split("audio:",1)[1])
                envelope = float(strings[1].split("envelope:",1)[1])
                # gate = float(strings[2].split("gate:",1)[1])

                x_audio.append(i)
                y_audio.append(audio)
                x_envelope.append(i)
                y_envelope.append(envelope)
                # x_gate.append(i)
                # y_gate.append(gate)
                i += 1
                if (i % 20 == 0):
                    drawnow(make_fig)
            except:
                pass

