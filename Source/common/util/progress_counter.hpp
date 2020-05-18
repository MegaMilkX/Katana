#ifndef PROGRESS_COUNTER_HPP
#define PROGRESS_COUNTER_HPP

#include <string>


void progressBegin(const char* desc, float count);
void progressStep(float increment = 1.0f, const char* desc = 0);
void progressEnd();

float getProgressCount();
float getMaxProgressCount();
std::string getProgressDesc();


#endif
