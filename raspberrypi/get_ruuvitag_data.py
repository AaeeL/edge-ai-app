from ruuvitag_sensor.ruuvi import RuuviTagSensor

def get_data(macs):
   print("Getting data from ruuvitags")
   timeout_in_sec = 20
   readings = RuuviTagSensor.get_data_for_sensors(macs, timeout_in_sec)
   return readings
