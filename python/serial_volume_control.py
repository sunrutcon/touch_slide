import serial, os

#initialize serial port
ser = serial.Serial('/dev/ttyUSB0', 9600)

#read line from serial
def rs():
  return ser.readline().split('\r\n')[0]

#set volume with amixer
def set_volume(val):
    vol = val
    vol_command = 'amixer sset Master ' + vol + '% -M -q'
    #print vol_command
    print vol_command
    os.system(vol_command);
    

def main():
  while True:
    val = rs();
    print val
    #os.system('amixer sset Master ' + val);
    set_volume(val)

print 'ready...'
main()
