#ifndef _WEATHER_H
#define _WEATHER_H


struct daily
{
    char date[32];
    char text_day[32];
};

extern struct daily life_dailey[3];
void get_weather();
#endif
