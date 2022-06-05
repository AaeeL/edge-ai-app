# A function to calculate the dew point

# A dew point is defined as the temperature at which a given volume of air
# at a certain atmospheric pressure is satureated with water vapor, causing
# condensation and the formation of dew.

# A simple approximation of dew point can be calculated using the formula
# Td = T - ((100-RH) / 5), where Td is dew point, T is observed temperature and 
# RH is relative humidity in percent. For relative humidity values above 50% this
# formula is accurate and below those values it is accurate enough, that it can be
# used in this research.

def calculate(humidity, temperature):
    '''
    print("Humidity was > ")
    print(humidity)
    print("Temperature was > ")
    print(temperature)
    '''
    return temperature - ((100 - humidity) / 5)
