#include "progress_counter.hpp"

static std::string progress_desc;
static float progress_max;
static float progress_count;


void progressBegin(const char* desc, float count) {
    progress_max = count;
    progress_desc = desc;
    progress_count = 0;
}
void progressStep(float increment, const char* desc) {
    progress_count += increment;
    if(desc) {
        progress_desc = desc;
    }
}
void progressEnd() {
    progress_count = progress_max;
}

float getProgressCount() {
    return progress_count;
}
float getMaxProgressCount() {
    return progress_max;
}
std::string getProgressDesc() {
    return std::string(progress_desc);
}