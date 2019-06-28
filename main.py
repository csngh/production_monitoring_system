#Author: Chetan Singh

import RPi.GPIO as GPIO
import os
import time
from RPLCD import CharLCD
from datetime import datetime
import MySQLdb

tot_start = 0
tot_stop = 0
t_sec = 0
t_min = 0
t_temp = 0

lcd = CharLCD(cols=20, rows=4, pin_rs=37, pin_e=35, pins_data=[33, 31, 29, 23])

db = MySQLdb.connect(host="192.168.43.36", user="csingh", passwd="password", db="test")
cur = db.cursor()

file = open("/home/pi/data_log_state.csv", "a")
if os.stat("/home/pi/data_log_state.csv").st_size == 0:
    file.write("Status, Date, Time\n")

GPIO.setmode(GPIO.BOARD)
GPIO.setup(12, GPIO.IN)

previous_state = 0

try:
    while True:

        now = datetime.now()
        cr_date = str(now.day) + "-" + str(now.strftime("%B")) + "-" + str(now.year)
        cr_time = str(now.hour) + ":" + str(now.minute) + ":" + str(now.second)
        
        inputVal = GPIO.input(12)
        if(inputVal == previous_state):
                mess = ""
                if(inputVal == 1):
                    mess = "Started Since "
                    tot_start+=1
                elif(inputVal == 0):
                    mess = "Stopped Since "
                    tot_stop+=1
                if(t_sec>59):
                    if(t_temp>59):
                        t_temp = 0
                    t_min = t_sec // 60
                    print(mess + str(t_min) + " Mins " + str(t_temp) + " Secs")
                    lcd.cursor_pos = (2, 0)
                    lcd.write_string(mess + str(t_min) + " Mins " + str(t_temp) + " Secs")
                    t_temp += 1
                else:
                    print(mess + str(t_sec) + " Secs")
                    lcd.cursor_pos = (2, 0)
                    lcd.write_string(mess + str(t_sec) + " Secs")
        else:
            m_state = "State Changed,"
            print m_state
            previous_state = inputVal
            t_sec = 0
            if(inputVal == 0):
                msg = "Stopped at " + cr_time
                print (str(msg))
                lcd.clear()
                lcd.cursor_pos = (0, 0)
                lcd.write_string(str(msg))
                stat = "Stopped"
                file.write(str(stat)+","+str(cr_date)+","+str(cr_time)+"\n")
                cur.execute("""insert into data_log_mc_stat (status, mc_id, date, time)
                            values (%s, %s, %s, %s)""", (str(stat), str(34), str(cr_date), str(cr_time)))
                db.commit()
            elif(inputVal == 1):
                msg = "Started at " + cr_time
                print (str(msg))
                lcd.clear()
                lcd.cursor_pos = (0, 0)
                lcd.write_string(str(msg))
                stat = "Started"
                file.write(str(stat)+","+str(cr_date)+","+str(cr_time)+"\n")
                cur.execute("""insert into data_log_mc_stat (status, mc_id, date, time)
                            values (%s, %s, %s, %s)""", (str(stat), str(34), str(cr_date), str(cr_time)))
                db.commit()
        time.sleep(1)
        t_sec+=1

except KeyboardInterrupt:
    print "Shutdown"
    GPIO.cleanup()
    
