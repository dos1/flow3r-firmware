from st3m.run import run_main

import network
station = network.WLAN(network.STA_IF)
station.active(True)
station.connect("Camp2023-open")

run_main()
