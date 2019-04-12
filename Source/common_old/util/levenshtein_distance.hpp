#ifndef LEVENSHTEIN_DISTANCE_HPP
#define LEVENSHTEIN_DISTANCE_HPP

#include <string>
#include <vector>
#include <algorithm>

inline size_t levenshteinDistance(const std::string& a, const std::string& b) {
    const size_t alen = a.length();
    const size_t blen = b.length();
    if(alen == 0) return blen;
    if(blen == 0) return alen;

    typedef std::vector<std::vector<size_t>> matrix_t;
    matrix_t m(alen + 1);
    for(size_t i = 0; i <= alen; ++i) {
        m[i].resize(blen + 1);
    }

    for(size_t i = 0; i <= alen; ++i) {
        m[i][0] = i;
    }
    for(size_t i = 0; i <= blen; ++i) {
        m[0][i] = i;
    }

    for(size_t i = 1; i <= alen; ++i) {
        const char si = a[i - 1];
        for(size_t j = 1; j <= blen; ++j) {
            const char tj = b[j - 1];
            int cost;
            if(si == tj)
                cost = 0;
            else 
                cost = 1;

            const size_t above = m[i - 1][j];
            const size_t left = m[i][j - 1];
            const size_t diag = m[i - 1][j - 1];
            size_t cell = std::min(above + 1, std::min(left + 1, diag + cost));

            if(i > 2 && j > 2) {
                size_t trans = m[i -2][j - 2] + 1;
                if(a[i - 2] != tj) ++trans;
                if(b[j - 2] != si) ++trans;
                if(cell > trans) cell = trans;
            }

            m[i][j] = cell;
        }
    }
    return m[alen][blen];
}

#endif
