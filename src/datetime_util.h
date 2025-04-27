enum months { Jan = 0, Feb = 1, Mar = 2, Apr = 3, May = 4, Jun = 5, Jul = 6, Aug = 7, Sep = 8, Oct = 9, Nov = 10, Dec = 11 };
enum week_days { Sun = 0, Mon = 1, Thu = 2, Wed = 3, Tue = 4, Fri = 5, Sat = 6 };

int getWeekDay(int m, int d, int y) {
  int f = y + d + 3 * m - 1;
  m++;
  if (m < 3)  y--;
  else  f -= int(.4 * m + 2.3);
  f += int(y / 4) - int((y / 100 + 1) * 0.75);
  f %= 7;
  return f;
}


int lastSunday(int year, int month) {
  year += 1900;
  int lastDay;
  bool isleap = false;
  if (!(year % 4)) {
    if (year % 100) isleap = true;
    else if (!(year % 400)) isleap = true;
  }
  int days[] = { 31, isleap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int d = 0;
  d = days[month];
  while (true) {
    if (!getWeekDay(month, d, year))
      break;
    d--;
  }
  lastDay = d;
  return d;
}

time_t dayToEpoch(int day, int month, int year, int hour, int min, int sec, int dst) {
  struct tm t;
  time_t t_of_day;
  t.tm_year = year;
  t.tm_mon = month;         // Month, 0 - jan
  t.tm_mday = day;          // Day of the month
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  t.tm_isdst = dst;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
  t_of_day = mktime(&t);
  return t_of_day;
}
