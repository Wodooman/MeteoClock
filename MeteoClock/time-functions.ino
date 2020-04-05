void readTime(){
  now = rtc.now();
  secs = now.second();
  mins = now.minute();
  hrs = now.hour();
  mnth = now.month();
  day_val = now.day();
  dayOfWeek = now.dayOfTheWeek(); 
  year_val = now.year();
}

boolean dotFlag;
void clockTick() {
  dotFlag = !dotFlag;
  if (dotFlag) {          // каждую секунду пересчёт времени
    secs++;
    if (secs > 59) {      // каждую минуту
      secs = 0;
      mins++;
      if (mins <= 59 && mode == 0) drawClock(hrs, mins, 0, 0, 1);
    }
    if (mins > 59) {      // каждый час
      readTime();
      if (mode == 0) drawClock(hrs, mins, 0, 0, 1);
      if (hrs > 23) {
        hrs = 0;
      }
      if (mode == 0 && DISPLAY_TYPE) drawData();
    }
    if (DISP_MODE == 2 && mode == 0) {
      lcd.setCursor(16, 1);
      if (secs < 10) lcd.print(" ");
      lcd.print(secs);
    }
  }
  if (mode == 0) drawdots(7, 0, dotFlag);
}
