#ifndef DRAND_H
#define DRAND_H


#include <vector>


inline int mod10(int value);
inline int mod100(int value);
inline int mod1k(int value);
inline int mod10k(int value);
inline int mod100k(int value);
inline int mod1kk(int value);
inline int mod10kk(int value);
inline int mod100kk(int value);
inline int qmod(int value);


std::vector<int> nmod(int value);
int middle(int value);

//--------------------------------------

int sgen(int r,int q);
float xrand();
float xrand(float range);
float xrand(float bot, float top);
float getRandSign();

float xrands();
float xrands(float range);

void xrand2(float &v);
void xrands2(float &v);


float partOf(float PART, float TOTAL);
float sym_partOf(float PART, float TOTAL);

#endif // DRAND_H
